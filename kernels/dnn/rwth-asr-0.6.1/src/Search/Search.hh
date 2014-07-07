// Copyright 2011 RWTH Aachen University. All rights reserved.
//
// Licensed under the RWTH ASR License (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.hltpr.rwth-aachen.de/rwth-asr/rwth-asr-license.html
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
#ifndef _SEARCH_SEARCHALGORITHM_HH
#define _SEARCH_SEARCHALGORITHM_HH

#include <Am/AcousticModel.hh>
#include <Bliss/Lexicon.hh>
#include <Core/Component.hh>
#include <Lm/LanguageModel.hh>
#include <Lattice/Lattice.hh>
#include <Search/LatticeAdaptor.hh>
#include <Speech/ModelCombination.hh>
#include "Types.hh"

namespace Search {

    /*
     * Search
     *
     * Abstract interface of search implementations
     */

    class SearchAlgorithm : public virtual Core::Component {
    public:
	typedef Speech::Score Score;
	typedef Speech::TimeframeIndex TimeframeIndex;

	struct ScoreVector {
	    Score acoustic, lm;
	    ScoreVector(Score a, Score l) : acoustic(a), lm(l) {}
	    operator Score() const {
		return acoustic + lm;
	    };
	};
	struct TracebackItem {
	public:
	    typedef Lattice::WordBoundary::Transit Transit;
	public:
	    const Bliss::LemmaPronunciation *pronunciation;
	    TimeframeIndex time; // Ending time
	    ScoreVector score; // Absolute score
	    Transit transit; // Final transition description
	    TracebackItem(const Bliss::LemmaPronunciation *p, TimeframeIndex t, ScoreVector s, Transit te):
		pronunciation(p), time(t), score(s), transit(te) {}
	};
	class Traceback : public std::vector<TracebackItem> {
	public:
	    void write(std::ostream &os, Core::Ref<const Bliss::PhonemeInventory>) const;
	    Fsa::ConstAutomatonRef lemmaAcceptor(Core::Ref<const Bliss::Lexicon>) const;
	    Fsa::ConstAutomatonRef lemmaPronunciationAcceptor(Core::Ref<const Bliss::Lexicon>) const;
	    Lattice::WordLatticeRef wordLattice(Core::Ref<const Bliss::Lexicon>) const;
	};
    public:
	SearchAlgorithm(const Core::Configuration&);
	virtual ~SearchAlgorithm() {}

	virtual Speech::ModelCombination::Mode modelCombinationNeeded() const;
	virtual bool setModelCombination(const Speech::ModelCombination &modelCombination) = 0;

	virtual void setGrammar(Fsa::ConstAutomatonRef) = 0;

	virtual void init() {}
	virtual void restart() = 0;
	virtual void setSegment(const std::string &name) {}
	virtual void feed(const Mm::FeatureScorer::Scorer&) = 0;
	/// Should return the longest fixed prefix of the final best sentence
	/// which has been been recognized since the last call to getPartialSentence.
	/// Optional: Only required for online recognition
	virtual void getPartialSentence(Traceback &result);
	/// Should return the part of the current best sentence which has not yet
	/// been returned by getPartialSentence().
	/// Optional: Only required for online recognition
	virtual void getCurrentBestSentencePartial(Traceback &result) const;
	/// Should return the whole currently best sentence
	virtual void getCurrentBestSentence(Traceback &result) const = 0;
	virtual Core::Ref<const LatticeAdaptor> getCurrentWordLattice() const = 0;
	virtual Core::Ref<const LatticeAdaptor> getPartialWordLattice();
	virtual void resetStatistics() = 0;
	virtual void logStatistics() const = 0;

	////// Optional methods:

	/// Allow HMM skips during decoding ? By default, HMM skips are allowed.
	virtual void setAllowHmmSkips(bool allow=true) {}

	/**
	 * Length of acoustic look-ahead.
	 * 0 means no look-ahead is used.
	 */
	virtual u32 lookAheadLength() {
	    return 0;
	}
	/**
	 * Called before feed() with as many acoustic feature vectors from the future
	 * as returned by lookaheadLength().  At the end of a segment, less than requested,
	 * or even zero feature vectors may be given.
	 */
	virtual void setLookAhead(const std::vector<Mm::FeatureVector>&) {}

	class Pruning : public Core::ReferenceCounted {
	public:
	    virtual ~Pruning() {}
	    /// Merge in timeframe-specific pruning specifiers, creating a pruning-map.
	    /// Returns false if the operation is not supported.
	    virtual Core::Ref<Pruning> clone() const = 0;
	    // endTime is _inclusive_
	    virtual bool merge(const Core::Ref<Pruning>& /*rhs*/, TimeframeIndex /*ownLength*/, TimeframeIndex /*startTime*/, TimeframeIndex endTime) { return false; }
	    /// Extend the pruning-thresholds by a score offset, a score factor, and by a specific number of timeframes
	    virtual bool extend(Score /*scoreFactor*/, Score /*scoreOffset*/, TimeframeIndex /*timeOffset*/) { return false; }
	    virtual std::string format() const { return std::string(); }
	    /// Should return false if the search space was too tight and must be relaxed.
	    virtual bool checkSearchSpace() const { return true; }
	    /// Currently configured master beam size (relative to LM scale, eg. typically around 13)
	    virtual Score masterBeam() const = 0;
	    /// Maximum recorded size of the master beam
	    virtual Score maxMasterBeam() const = 0;
	};
	typedef Core::Ref<Pruning> PruningRef;

	/// Returns a descriptor of the current pruning thresholds, which can be used to
	/// re-activate the current pruning thresholds using resetPruning.
	virtual PruningRef describePruning();

	/// Relaxes the pruning. Returns false if relaxing failed, otherwise true.
	virtual bool relaxPruning(f32 factor, f32 offset);

	/// Resets pruning according to the given descriptor
	virtual void resetPruning(PruningRef pruning);

	struct RecognitionContext {
	    enum LatticeMode {
		Auto,
		Yes,
		No
	    };
	    RecognitionContext() :
		coarticulation(std::make_pair(Bliss::Phoneme::term, Bliss::Phoneme::term)),
		finalCoarticulation(std::make_pair(Bliss::Phoneme::term, Bliss::Phoneme::term)),
		latticeMode(Auto),
		prePhon(Bliss::Phoneme::term),
		sufPhon(Bliss::Phoneme::term)
	    {}
	    std::pair<Bliss::Phoneme::Id, Bliss::Phoneme::Id> coarticulation, finalCoarticulation;
	    std::vector<const Bliss::Lemma*> prefix, suffix;
	    LatticeMode latticeMode;
	    Bliss::Phoneme::Id prePhon, sufPhon;
	};

	/**
	 * Set LM context to be prepended (prefix) or appended(suffix) to the search hypotheses, and the acoustic phoneme
	 * coarticulation to be used for startup.
	 *
	 * The context should never be forgotten, but can be reset by calling this function with default arguments.
	 * Should return the context which was used previously.
	 * */
	virtual RecognitionContext setContext( RecognitionContext ) {
	    return RecognitionContext();
	}
    };

} // namespace Search

#endif // _SEARCH_SEARCHALGORITHM_HH

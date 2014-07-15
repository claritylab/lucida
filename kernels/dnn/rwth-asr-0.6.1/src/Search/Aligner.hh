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
#ifndef _SEARCH_ALIGNER_HH
#define _SEARCH_ALIGNER_HH

#include <Core/Component.hh>
#include <Core/Parameter.hh>
#include <Core/Statistics.hh>
#include <Bliss/Lexicon.hh>
#include <Fsa/Automaton.hh>
#include <Speech/Alignment.hh>
#include <Speech/AllophoneStateGraphBuilder.hh>
#include <Mm/FeatureScorer.hh>
#include <Lattice/Lattice.hh>
#include <Am/AcousticModel.hh>
// #include <Core/Archive.hh>
// #include <Speech/AlignerModelAcceptor.hh>


namespace Am {
    class AllophoneStateAlphabet;
}

namespace Search {

    /** Forced state-level alignment of network with observation seqeuence. */
    class Aligner : public Core::Component {
	typedef Speech::Score Score;
    public:
	enum Mode {
	    modeViterbi,
	    modeBaumWelch
	};
	static const Core::Choice	   choiceMode;
	static const Core::ParameterChoice paramMode;
	static const Core::ParameterFloat  paramMinAcousticPruningThreshold;
	static const Core::ParameterFloat  paramMaxAcousticPruningThreshold;
	static const Core::ParameterFloat  paramAcousticPruningThresholdIncrementFactor;
	static const Core::ParameterFloat  paramMinAverageNumberOfStateHypotheses;
	static const Core::ParameterBool   paramIncreasePruningUntilNoScoreDifference;
	static const Core::ParameterBool   paramLogIterations;
	static const Core::ParameterFloat  paramMinWeight;
	static const Core::ParameterBool   paramUsePartialSums;

	class WordLatticeBuilder : public Core::Component {
	private:
	    Bliss::LexiconRef lexicon_;
	    Core::Ref<const Am::AcousticModel> acousticModel_;
	    Core::Ref<const Bliss::LemmaPronunciationAlphabet> lemmaPronunciationAlphabet_;
	    Core::Ref<const Am::AllophoneStateAlphabet> allophoneStateAlphabet_;
	    Fsa::ConstAutomatonRef allophoneToLemmaPronunciationTransducer_;
	    mutable Core::XmlChannel dumpAutomaton_;
	    class AddWordBoundaryDisambiguatorsAutomaton;
	    class Converter;
	private:
	    Fsa::ConstAutomatonRef addWordBoundaryDisambiguators(Fsa::ConstAutomatonRef alignmentFsa) const;
	    Fsa::ConstAutomatonRef buildAlignmentToLemmaPronunciationTransducer(
	    Fsa::ConstAutomatonRef alignmentFsaWithDisambiguators) const;
	    Lattice::ConstWordLatticeRef convertToWordLattice(
	    Fsa::ConstAutomatonRef alignmentToLemmaPronunciationTransducer) const;
	public:
	    WordLatticeBuilder(const Core::Configuration &, Bliss::LexiconRef, Core::Ref<const Am::AcousticModel>);
	    /**
	     *  Sets transducer created from the aligned training sentence.
	     *  Input labels: allophone states.
	     *  Output labels: lemma-pronunciations.
	     *  For more details @see build.
	     */
	    void setModelTransducer(Fsa::ConstAutomatonRef modelTransducer);
	    /**
	     *  Builds the word lattice for a given traceback and model-transducer.
	     *
	     *  ModelTransducer (@see setModelTransducer) is used to find the word
	     *  (e.i. lemma-pronunciations) boundaries. It is composed by a allophone
	     *  state acceptor created from the aligment. The result of the composition
	     *  contains word boundary (i.e. disambiguator) arcs and lemma-pronunciation
	     *  output labels.
	     *  The shortest path of the composition gives finally to necesary informations
	     *  (e.i. allophone states, lemma-pronunciations, and acoustic scores) to create
	     *  a linear word lattice from an alignment traceback.
	     */
	    Lattice::ConstWordLatticeRef build(Fsa::ConstAutomatonRef alignmentFsa);
	};
    private:
	Fsa::ConstAutomatonRef model_;
	class SearchSpace;
	SearchSpace *ss_;

	Core::Ref<const Am::AcousticModel> acousticModel_;
	Score acousticPruningThreshold_;
	Score minAcousticPruningThreshold_;
	Score maxAcousticPruningThreshold_;
	Score acousticPruningThresholdIncrementFactor_;
	f64 minAverageNumberOfStateHypotheses_;
	bool increasePruningUntilNoScoreDifference_;
	bool logIterations_;
	u32 nIterations_;
	bool viterbi_;

	Mm::Weight minWeight_;
	bool usePartialSums_;

	Core::Statistics<u32> nStateHypotheses_;
	mutable Mm::ProbabilityStatistics statePosteriorStats_;
	mutable Core::XmlChannel statisticsChannel_;
	mutable Core::XmlChannel alignmentChannel_;
    private:
	Mm::Weight getLogWeightThreshold() const;
    public:
	Aligner(const Core::Configuration&);
	~Aligner();

	void setModel(Fsa::ConstAutomatonRef model,
		      Core::Ref<const Am::AcousticModel>);
	Fsa::ConstAutomatonRef getModel() const { return model_; }
	void restart();
	/**
	 *  Feeds a single feature scorers into the aligner.
	 *  Acoustic pruninig treshold will be set to maxAcousticPruningThreshold_.
	 */
	void feed(Mm::FeatureScorer::Scorer scorer);
	/**
	 *  Feeds vector of feature scorers into the aligner.
	 *  Acoustic pruninig treshold will be optimized within the interval
	 *  [0..maxAcousticPruningThreshold_].
	 */
	void feed(const std::vector<Mm::FeatureScorer::Scorer>&);

	/**
	 *  Returns an automaton representing the alignment of HMM model states and
	 *  observations (features).
	 *  In the case of Viterbi training, the resulting acceptor has a simple
	 *  linear structure representing the best path.
	 *  With Baum-Welch Training, we have an acyclic graph representing the search
	 *  space of the aligner limited to those paths which reached a final state.
	 *  In either case, an allophone-state acceptor with arc weights being acoustic
	 *  scores is returned.
	 *  If the aligner does not reach a final state due pruning settings which are
	 *  too restrictive, an empty automaton is returned.
	 */
	Fsa::ConstAutomatonRef getAlignmentFsa() const;
	/**
	 *  Returns an automaton which is topologically equivalent to the automaton
	 *  returned by getAlignmentFsa(), but where arc weights represent (negative
	 *  logarithmic) posterior probabilities.
	 */
	std::pair<Fsa::ConstAutomatonRef, Fsa::Weight> getAlignmentPosteriorFsa(
	    Fsa::ConstAutomatonRef alignmentFsa) const;
	void getAlignment(Speech::Alignment&, std::pair<Fsa::ConstAutomatonRef, Fsa::Weight> alignmentPosteriorFsa) const;
	void getAlignment(Speech::Alignment&) const;
	Score alignmentScore() const;
	bool reachedFinalState() const {
	    return alignmentScore() != Core::Type<Score>::max;
	}
	void selectMode(Mode);
	void selectMode();
    };


    // ================================================================================


    class WordSequenceAligner : public Core::Component {
    private:
	Core::Ref<const Am::AcousticModel> acousticModel_;
	Core::Ref<const Bliss::Lexicon> lexicon_;
	Speech::AllophoneStateGraphBuilder *modelBuilder_;
	Aligner aligner_;
	Speech::Alignment alignment_;
	Core::XmlChannel alignmentChannel_;
    public:
	WordSequenceAligner(const Core::Configuration&,
			    Core::Ref<const Bliss::Lexicon>,
			    Core::Ref<const Am::AcousticModel>);
	~WordSequenceAligner();

	void setLemmaAcceptor(Fsa::ConstAutomatonRef lemmaAcceptor) {
	    require(lemmaAcceptor->type() == Fsa::TypeAcceptor);
	    require(lemmaAcceptor->getInputAlphabet() == lexicon_->lemmaAlphabet());
	    aligner_.setModel(modelBuilder_->build(lemmaAcceptor), acousticModel_);
	}

	Speech::Alignment align(
	    const Mm::FeatureScorer&,
	    const std::vector<Core::Ref<const Mm::Feature> >&);
    };

}

#endif // _SEARCH_ALIGNER_HH

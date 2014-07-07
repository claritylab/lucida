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
#ifndef _SPEECH_PHONEME_SEQUENCE_ALIGNMENT_GENERATOR_HH
#define _SPEECH_PHONEME_SEQUENCE_ALIGNMENT_GENERATOR_HH

#include "SegmentwiseAlignmentGenerator.hh"
#include <Flf/FlfCore/Lattice.hh>

namespace Speech
{
    /**
     * PhonemeSequenceAlignmentGenerator
     * Used in combination with lattice rescoring.
     * Calculates the alignment for each arc,
     * assuming that the word boundaries are known.
     */
    class PhonemeSequenceAlignmentGenerator : public SegmentwiseAlignmentGenerator
    {
	typedef SegmentwiseAlignmentGenerator Precursor;
    private:
	static const Core::ParameterBool paramAddEmissionScores;
    public:
	struct Key {
	    Fsa::LabelId _id;
	    TimeframeIndex _tbeg;
	    TimeframeIndex _tend;
	    Bliss::Phoneme::Id _leftContext;
	    Bliss::Phoneme::Id _rightContext;

	    Key() {}
	    Key(const Bliss::Coarticulated<Bliss::LemmaPronunciation> &p,
		TimeframeIndex tbeg, TimeframeIndex tend) :
		_id(p.object().id()), _tbeg(tbeg), _tend(tend),
		_leftContext(p.leftContext()), _rightContext(p.rightContext()) {}
	    std::string string() const {
		return Core::form("%d|%d|%d|%d|%d", _id, _tbeg, _tend, _leftContext, _rightContext);
	    }
	    bool operator==(const Key &rhs) const {
		return ((_id == rhs._id) && (_tbeg == rhs._tbeg)
			&& (_tend == rhs._tend)
			&& (_leftContext == rhs._leftContext)
			&& (_rightContext == rhs._rightContext));
	    }

	    friend Core::BinaryInputStream& operator>>(Core::BinaryInputStream &i, Key &key) {
		return (i >> key._id >> key._tbeg >> key._tend >> key._leftContext >> key._rightContext);
	    }
	    friend Core::BinaryOutputStream& operator<<(Core::BinaryOutputStream &o, const Key &key) {
		return (o << key._id << key._tbeg << key._tend << key._leftContext << key._rightContext);
	    }
	};

	struct KeyHash {
	    size_t operator() (const Key &key) const {
		return (key._id & 0x0FFF)
		    | ((key._tbeg & 0x03FF) << 12)
		    | ((key._tend & 0x03FF) << 22);
	    }
	};

	struct KeyEquality : std::binary_function<const char*, const char*, bool> {
	    bool operator() (const Key &k1, const Key &k2) const {
		return (k1 == k2);
	    }
	};

	class Cache : public Core::Component
	{
	    typedef Core::hash_map<Key, Alignment*, KeyHash, KeyEquality> AlignmentMap;
	    typedef Core::hash_map<Key, Score, KeyHash, KeyEquality> ScoreMap;
	    typedef const Alignment* ConstAlignmentPtr;
	private:
	    static const Core::ParameterString paramCache;
	    static const Core::ParameterBool paramReadOnly;
	private:
	    Core::Archive *archive_;
	    bool dirty_;
	    AlignmentMap alignments_;
	    std::string segmentId_;
	private:
	    void initializeArchive(const ModelCombination &);
	    void clear();
	    void read(const std::string &);
	    void write(const std::string &);
	    bool dirty() const { return (archive_ != 0) && dirty_; }
	    void setDirty() { if (archive_ != 0) dirty_ = true; }
	    void resetDirty() { if (archive_ != 0) dirty_ = false; }
	public:
	    Cache(const Core::Configuration &, const ModelCombination &);
	    ~Cache();
	    void setSpeechSegmentId(const std::string &segmentId);
	    bool findForReadAccess(const Key &key, ConstAlignmentPtr *result);
	    bool insert(const Key &, Alignment *);
	};
    protected:
	FsaCache *modelAcceptorCache_;
	Cache *alignmentCache_;
	bool useAlignmentCache_;
	bool addEmissionScores_;
    protected:
	void update(const std::string &segmentId);
	void addEmissionScores(Alignment &, std::vector<Mm::FeatureScorer::Scorer> &);
    public:
	PhonemeSequenceAlignmentGenerator(const Core::Configuration &, Core::Ref<const ModelCombination> = Core::Ref<const ModelCombination>());
	virtual ~PhonemeSequenceAlignmentGenerator();

	// call either or
	virtual void setSpeechSegmentId(const std::string &);
	virtual void setSpeechSegment(Bliss::SpeechSegment *);

	/**
	 * Alignment of time frame indices between acoustic features and HMM states
	 * features:    tbeg               t-1      t      t+1         tend-1
	 *   INITIAL O---------O-->   --O-------O-------O----->    --O---------O FINAL
	 * states: tbeg     tbeg+1     t-1      t      t+1         tend-1     tend
	 *
	 * Note: the alignment times are corrected so that they start with tbeg.
	 */
	const Alignment* getAlignment(const Bliss::LemmaPronunciation &,
				      TimeframeIndex tbeg, TimeframeIndex tend);
	const Alignment* getAlignment(const Bliss::Coarticulated<Bliss::LemmaPronunciation> &,
				      TimeframeIndex tbeg, TimeframeIndex tend);
	void useAlignmentCache(bool use) {
	    useAlignmentCache_ = use;
	}

	/**
	 * @param lattice is assumed to contain the acoustic scores
	 * according to @param this.
	 * @param alignment is a weighted alignment.
	 */
	void getAlignment(Alignment &alignment, Lattice::ConstWordLatticeRef lattice);
	void getAlignment(Alignment &alignment, Flf::ConstLatticeRef lattice);

	/**
	 * Score from alignment using the given arc (aka acoustic score).
	 * The scores/alignments are not cached.
	 */
	Score alignmentScore(const Bliss::Coarticulated<Bliss::LemmaPronunciation> &,
			     TimeframeIndex tbeg, TimeframeIndex tend);
    };

    typedef Core::Ref<PhonemeSequenceAlignmentGenerator> AlignmentGeneratorRef;

}

namespace Core {
    template <>
    class NameHelper<Speech::AlignmentGeneratorRef> : public std::string {
    public:
	NameHelper() : std::string("flow-alignment-generator-ref") {}
    };

} // namespace Core


#endif // _SPEECH_PHONEME_SEQUENCE_ALIGNMENT_GENERATOR_HH

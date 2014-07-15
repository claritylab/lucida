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
// $Id: StateTree.hh 9621 2014-05-13 17:35:55Z golik $

#ifndef _SEARCH_STATE_TREE_HH
#define _SEARCH_STATE_TREE_HH

#include <list>
#include <Am/AcousticModel.hh>
#include <Am/ClassicStateModel.hh>

namespace Search
{

    class SearchTreeIo;
    class SearchTreeReader;
    class SearchTreeWriter;

    /**
     * Acoustic prefix tree.
     *
     * Not strictly a tree anymore, but a DAG where the successors
     * sets at each level of any two nodes are either identical or
     * disjoint.
     *
     * paramCiCrossWordTransitions:
     *  instead of allowing word boundary transitions a{b+#}@f -> a{#+d}@i
     *  use only transitions a{b+#}@f -> ci{#+#}@i
     *   with ci a context independent phone (silence, noise)
     **/
    class StateTree :
	public virtual Core::Component
    {
    private:
	Core::Ref<const Am::AcousticModel> acousticModel_;

	Core::Ref<const Bliss::PhonemeInventory> phonemeInventory() const {
	    return acousticModel_->phonemeInventory();
	}

	typedef std::vector<Bliss::Phoneme::Id> PhonemeList;
	PhonemeList *initialPhonemes_;

	bool isEarlyRecombinationEnabled_;
	bool isPathRecombinationInFanInEnabled_;
	s32  earlyRecombinationLimit_;
	bool haveSuccessorBatches_;
	bool allowSkipTransitions_;
	bool allowCiCrossWordTransitions_;
    public:
	struct StateDesc;
    private:
	class PronunciationSuffix;

	bool advanceSubState(PronunciationSuffix&) const;
	bool advancePhoneState(PronunciationSuffix&) const;
	const Am::Allophone * getAllophone(const PronunciationSuffix&) const;
	s8 nPhoneStates(const PronunciationSuffix&) const;
	s8 nSubStates  (const PronunciationSuffix&) const;
	s16 boundary(const PronunciationSuffix&) const;
	u16 transitionModelIndex(const PronunciationSuffix&) const;
	std::vector<const Am::StateTransitionModel*> transitionModels_;

	void advancePronunciationSuffix(
	    const PronunciationSuffix &s,
	    std::back_insert_iterator< std::vector<PronunciationSuffix> > continuing,
	    std::back_insert_iterator< std::vector<PronunciationSuffix> > ending) const;
	void leap(PronunciationSuffix &s) const;
	StateDesc head(const PronunciationSuffix&) const;
	u32 nStatesRemaining(const PronunciationSuffix&) const;
    public:
	int compareSequences(const PronunciationSuffix&, const PronunciationSuffix&) const;
	u32 hashSequence(const PronunciationSuffix&) const;

    public:
	typedef s32 StateId;

	/**
	 * An exit point in the state tree.
	 * Stores the ending pronunciation and the transitEntry
	 */
	struct Exit {
	    const Bliss::LemmaPronunciation *pronunciation;
	    StateId transitEntry;

	    bool operator< (const Exit &rhs) const {
		return (pronunciation  < rhs.pronunciation)
		    || (pronunciation == rhs.pronunciation && transitEntry < rhs.transitEntry);
	    }

	    bool operator== (const Exit &rhs) const {
		return pronunciation == rhs.pronunciation
		    && transitEntry  == rhs.transitEntry;
	    }
	};

	typedef std::list<Exit> ExitList;
	typedef ExitList::iterator ExitIterator;
	typedef ExitList::const_iterator const_ExitIterator;

	typedef s16 Depth;
	static const Depth invalidDepth;

    private:
	typedef s32 BatchId;

    public:
	/**
	 * Description of a state by its mixture index and its transition model.
	 * Stores informations relevant for the score of a state
	 * Additionaly ordering and equality relations are defined, in order to
	 * use StateDesc as key in a map.
	 */
	struct StateDesc {
	    typedef u16 AcousticModelIndex;
#if 1
	    // This should remain the default!
	    typedef u8  TransitionModelIndex;
#endif

	    AcousticModelIndex   acousticModel;
	    TransitionModelIndex transitionModelIndex;

	    StateDesc() : acousticModel(0), transitionModelIndex(0) {
	    }

	    std::string toString() const {
		std::ostringstream target;
		target << (u32)acousticModel << "_" << (u32)transitionModelIndex;
		return target.str();
	    }

	    bool operator< (const StateDesc &rhs) const {
		return (acousticModel  < rhs.acousticModel)
		    || (acousticModel == rhs.acousticModel && transitionModelIndex < rhs.transitionModelIndex);
	    }

	    bool operator== (const StateDesc &rhs) const {
		return (acousticModel   == rhs.acousticModel)
		    && (transitionModelIndex == rhs.transitionModelIndex);
	    }

	    struct Hash {
		size_t operator() (const StateDesc& desc) const {
		    return 312831231 * desc.acousticModel + 7 * desc.transitionModelIndex;
		}
	    };
	};

	static const StateDesc::AcousticModelIndex invalidAcousticModel;

	/**
	 * A node in the state tree.
	 * Components of a state are:
	 *  - its description (@see StateDesc)
	 *  - the set of successor nodes
	 *  - a list of ending pronunciations
	 *  - the depth of the node in the tree
	 */
	struct State {
	    StateDesc desc;
	    Depth depth;
	    ExitList exits;
	    BatchId successors;
	    State();
	};
	/*! storage for all states */
	std::vector<State> states_;
	StateId root_, ciRoot_;

	/**
	 * Access to batches of states.
	 * Each element is the highest @c states_ index of a batch.
	 * States in batch b are batches_[b] .. batches_[b+1]
	 */
	std::vector<StateId> batches_;
	/**
	 * Successors of batches.
	 * Successor states of batch b are batches_[sb] .. batches_[sb+1]
	 * with sb = successorBatches_[b]
	 */
	std::vector<BatchId> successorBatches_;
	BatchId emptyBatch_;
	static const BatchId invalidBatch;
	StateId wordHeadsEnd_;
	struct LevelStatistics;

    private:
	class SuffixSet;
	friend class SuffixSet;

	StateId pronunciationSuffixTransitEntry(const PronunciationSuffix&) const;
	void buildNonCoarticulatedRootState(Bliss::LexiconRef);

	class CoarticulationStructure {
	public:
	    PhonemeList initialPhonemes, finalPhonemes;
	    std::vector<std::vector<StateId> > roots;
	    struct PhonemePair { Bliss::Phoneme::Id final, initial; };
	    std::vector<PhonemePair> boundaryPhonemes;
	    StateId root(Bliss::Phoneme::Id final, Bliss::Phoneme::Id initial) const;
	    CoarticulationStructure(const StateTree*, Bliss::LexiconRef);
	    CoarticulationStructure() {}
	};

	CoarticulationStructure *coarticulationStructure_;
	void buildCoarticulatedRootStates(Bliss::LexiconRef);

	struct BuildRequest;
	struct BatchRequest; friend struct StateTree::BatchRequest;
	struct LeapRequest;  friend struct StateTree::LeapRequest;
	class RequestQueue;
	class StraightRequestQueue;    friend class StateTree::StraightRequestQueue;
	class RecombiningRequestQueue; friend class StateTree::RecombiningRequestQueue;
	RequestQueue *requests_;
	void createExits(const std::vector<PronunciationSuffix>&, ExitList&);
	bool shouldLeap(const SuffixSet&, Depth) const;
	BatchId createBatch(BatchRequest*);
	StateId createLeapEntry(LeapRequest*);
	void createSecondOrderBatches();
	bool isConsistent() const;
	void buildTreeLexicon(Bliss::LexiconRef);
	bool hasMeshStructure() const;
	void getTransitionModels();

    public:
	static const Core::ParameterBool paramEarlyRecombination;
	static const Core::ParameterInt  paramEarlyRecombinationLimit;
	static const Core::ParameterBool paramEnforcePathRecombinationInFanIn;
	static const Core::ParameterString paramFile;
	static const Core::ParameterBool paramSkipTransitions;
	static const Core::ParameterBool paramCiCrossWordTransitions;

	friend class StateTreeIo;
	friend class StateTreeWriter;
	friend class StateTreeReader;

	StateTree(const Core::Configuration&,
		  Bliss::LexiconRef,
		  Core::Ref<const Am::AcousticModel>);
	~StateTree();

	StateId nStates() const { return states_.size(); }
	size_t memSizeStates() const { return states_.size() * sizeof(State); }
	StateId nWordHeadStates() const { return wordHeadsEnd_; }
	StateId root() const { return root_; }
	StateId ciRoot() const { return ciRoot_; }
	std::pair<Bliss::Phoneme::Id, Bliss::Phoneme::Id> describeRootState(StateId) const;

	Depth stateDepth(StateId s) const {
	    require_(0 <= s && s < nStates());
	    return states_[s].depth;
	}

	const StateDesc &stateDesc(StateId s) const {
	    require_(0 <= s && s < nStates());
	    return states_[s].desc;
	}

	const State& state(StateId s) const {
	    require_(0 <= s && s < nStates());
	    return states_[s];
	}

	const Am::StateTransitionModel *transitionModel(const StateDesc& desc) const {
	    return transitionModels_[size_t(desc.transitionModelIndex)];
	}

	const Am::StateTransitionModel *transitionModel(StateId s) const {
	    return transitionModels_[size_t(stateDesc(s).transitionModelIndex)];
	}

	class ReverseTopologicalStateIterator {
	private:
	    StateId current_, wordHeadsEnd_, wordTailsEnd_;
	public:
	    ReverseTopologicalStateIterator(const StateTree *st) {
		wordHeadsEnd_ = st->nWordHeadStates();
		wordTailsEnd_ = st->nStates();
		current_ = wordHeadsEnd_ - 1;
	    }

	    operator bool() const {
		return current_ >= 0;
	    }

	    StateId operator* () const {
		return current_;
	    }

	    ReverseTopologicalStateIterator &operator++ () {
		if (current_ == 0)
		    current_ = wordTailsEnd_;
		if (current_ == wordHeadsEnd_)
		    current_ = 0;
		--current_;
		return *this;
	    }
	};

	class SuccessorIterator {
	private:
	    StateId s_;
	public:
	    SuccessorIterator() {}
	    SuccessorIterator(StateId s) : s_(s) {}
	    bool operator== (const SuccessorIterator &rhs) const {
		return s_ == rhs.s_;
	    }
	    bool operator< (const SuccessorIterator &rhs) const {
		return s_ < rhs.s_;
	    }
	    SuccessorIterator &operator++ () {
		++s_;
		return *this;
	    }
	    SuccessorIterator operator++ (int) {
		SuccessorIterator result(*this);
		++s_;
		return result;
	    }
	    operator StateId() const {
		return s_;
	    }
	    StateId operator* () const {
		return s_;
	    }
	    u32 operator- (const SuccessorIterator &rhs) const {
		return s_ - rhs.s_;
	    }
	};

	std::pair<SuccessorIterator, SuccessorIterator> successors(StateId s) const {
	    require_(0 <= s && s < nStates());
	    BatchId b = states_[s].successors;
	    verify_(0 <= b && b+1 < BatchId(batches_.size()));
	    ensure_(batches_[b] <= batches_[b+1]);
	    return std::make_pair(SuccessorIterator(batches_[b  ]),
				  SuccessorIterator(batches_[b+1]));
	}

	std::pair<SuccessorIterator,SuccessorIterator> successors2(StateId s) const {
	    require_(0 <= s && s < nStates());
	    if (!haveSuccessorBatches_) {
		return std::make_pair(SuccessorIterator(0), SuccessorIterator(0));
	    }
	    BatchId b = states_[s].successors;
	    verify_(0 <= b && b < BatchId(successorBatches_.size()));
	    b = successorBatches_[b];
	    verify_(0 <= b && b+1 < BatchId(batches_.size()));
	    ensure_(batches_[b] <= batches_[b+1]);
	    return std::make_pair(SuccessorIterator(batches_[b  ]),
				  SuccessorIterator(batches_[b+1]));
	}

	std::pair<const_ExitIterator, const_ExitIterator> wordEnds(StateId s) const {
	    require_(0 <= s && s < nStates());
	    return std::make_pair(states_[s].exits.begin(),
				  states_[s].exits.end());
	}

	void logStatistics() const;
	void draw(std::ostream&, Core::Ref<const Bliss::PhonemeInventory> phi) const;
    };

} // namespace Search

#endif // _SEARCH_STATE_TREE_HH

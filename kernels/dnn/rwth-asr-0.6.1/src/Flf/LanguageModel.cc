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
#include <Core/Parameter.hh>
#include <Core/Vector.hh>
#include <Fsa/Hash.hh>
#include <Lm/Module.hh>

#include "FlfCore/LatticeInternal.hh"
#include "LanguageModel.hh"
#include "Lexicon.hh"
#include "Map.hh"
#include "RescoreInternal.hh"


namespace Flf {

    // -------------------------------------------------------------------------
    typedef Core::Ref<const Lm::LanguageModel> ConstLanguageModelRef;
    typedef Fsa::Hash<Lm::History, Lm::History::Hash> HistoryHashList;
    typedef Core::Vector<HistoryHashList> HistoriesList;


    // -------------------------------------------------------------------------
    /**
     * compose with lm
     **/
    class LmComposeLattice : public SlaveLattice {
	typedef SlaveLattice Precursor;
    private:
	ConstLanguageModelRef lm_;
	Score scale_;
	bool forceSentenceEnd_;
	Lm::History emptyHistory_;
	ConstSemiringRef semiring_;
	ScoreId id_;
	Core::Ref<const Bliss::LemmaAlphabet> lAlphabet_;
	Core::Ref<const Bliss::LemmaPronunciationAlphabet> lpAlphabet_;

	struct InternalState {
	    Fsa::StateId sid;
	    Lm::History history;

	    InternalState(Fsa::StateId _sid, Lm::History _history) :
		sid(_sid), history(_history) {}

	    struct Hash {
		size_t operator() (const InternalState &s) const {
		    return size_t(s.sid) * 2239 + s.history.hashKey();
		}
	    };

	    bool operator== (const InternalState &s) const {
		return (sid == s.sid) && (history == s.history);
	    }
	};

	mutable Fsa::Hash<InternalState, InternalState::Hash> states_;

	Fsa::StateId insertState(Fsa::StateId sid, Lm::History history) const {
	    verify_(sid <= Fsa::StateIdMask);
	    Fsa::StateId newSid = states_.insert(InternalState(sid, history));
	    hope(newSid <= Fsa::StateIdMask);
	    return newSid;
	}

	class Mapping : public Fsa::Mapping {
	private:
	    LmComposeLattice *l_;

	public:
	    Mapping(LmComposeLattice *l) : l_(l) {}
	    virtual ~Mapping() {}
	    virtual Fsa::StateId map(Fsa::StateId sid) const {
		return l_->leftStateId(sid);
	    }
	};

	mutable Core::Vector<ConstStateRef> cache_;

	ConstStateRef getCachedState(Fsa::StateId sid) const {
	    cache_.grow(sid);
	    ConstStateRef sr = cache_[sid];
	    if (!sr)
		sr = cache_[sid] = fsa_->getState(sid);
	    return sr;
	}

    public:
	LmComposeLattice(
	    ConstLatticeRef l,
	    ConstLanguageModelRef lm,
	    Score scale,
	    bool forceSentenceEnd,
	    ScoreId id) :
	    Precursor(l),
	    lm_(lm), scale_(scale),
	    forceSentenceEnd_(forceSentenceEnd) {
	    require(lm);
	    emptyHistory_ = lm->startHistory();
	    require(l);
	    semiring_ = l->semiring();
	    require(id < l->semiring()->size());
	    id_ = id;
	    switch (Lexicon::us()->alphabetId(l->getInputAlphabet())) {
	    case Lexicon::LemmaAlphabetId:
		lAlphabet_ = Lexicon::us()->lemmaAlphabet();
		break;
	    case Lexicon::LemmaPronunciationAlphabetId:
		lpAlphabet_ = Lexicon::us()->lemmaPronunciationAlphabet();
		break;
	    default:
		defect();
	    }
	    setProperties(Fsa::PropertyStorage | Fsa::PropertyCached, 0);
	    if (fsa_->getBoundaries() == InvalidBoundaries)
		setBoundaries(InvalidBoundaries);
	    else
		setBoundaries(ConstBoundariesRef(
				  new MappedBoundaries(
				      fsa_->getBoundaries(),
				      Fsa::ConstMappingRef(new Mapping(this)))));
	}

	virtual ~LmComposeLattice() { }

	virtual Fsa::StateId initialStateId() const {
	    return insertState(fsa_->initialStateId(), emptyHistory_);
	}

	virtual Fsa::StateId leftStateId(Fsa::StateId sid) const {
	    verify_(states_[sid].sid <= Fsa::StateIdMask);
	    return states_[sid].sid;
	}

	virtual Lm::History history(Fsa::StateId sid) const {
	    return states_[sid].history;
	}

	virtual void dumpState(Fsa::StateId sid, std::ostream &os) const {
	    if (sid < states_.size()) {
		os << "(";
		fsa_->dumpState(states_[sid].sid, os);
		os << ", "
		   << lm_->formatHistory(states_[sid].history)
		   << ")";
	    } else
		os << "unknown";
	}

	virtual ConstStateRef getState(Fsa::StateId sid) const {
	    require(sid < states_.size());
	    InternalState state = states_[sid];
	    ConstStateRef sr = getCachedState(state.sid);
	    State *sp = clone(*semiring_, sr);
	    sp->setId(sid);
	    for (State::iterator a = sp->begin(); a != sp->end(); ++a) {
		Fsa::LabelId labelId = a->input();
		Lm::History history = state.history;
		if ((Fsa::FirstLabelId <= labelId) && (labelId <= Fsa::LastLabelId)) {
		    const Bliss::Lemma *lemma = (lAlphabet_) ?
			lAlphabet_->lemma(labelId) :
			lpAlphabet_->lemmaPronunciation(labelId)->lemma();
		    require_(lemma);
		    Lm::Score lmScore = 0.0;
		    /*
		      Attention:
		      Max lm-rescorer uses

		      Lm::addLemmaScore(lm_, <lm-scale>, lemma, 1.0, history, lmScore),

		      but this is not feasible for the idea of Flf, because setting
		      the scale for lm-composition directly or setting the scale for
		      the lm-dimension might prodcue different results.
		      The correct procedure would be

		      Lm::addLemmaScore(lm_, scale_, lemma, 0.0, history, lmScore);

		      and add the syntaxEmissionScore to another dimension.
		      Soa is that the syntaxEmissionScore has no influence and is normally 0,
		      thus introducing a new dimension and a new parameter seems to be overhead.
		      Instead, the syntaxEmissionSale is set to lmScale.
		    */
		    Lm::addLemmaScore(lm_, scale_, lemma, scale_, history, lmScore);
		    a->weight_->add(id_, lmScore);
		}
		if (!forceSentenceEnd_ && !getCachedState(a->target())->hasArcs())
		    a->setTarget(insertState(a->target(), emptyHistory_));
		else
		    a->setTarget(insertState(a->target(), history));
	    }
	    if (forceSentenceEnd_ && sp->isFinal())
		sp->weight_->add(id_, scale_ * lm_->sentenceEndScore(state.history));
	    return ConstStateRef(sp);
	}

	virtual std::string describe() const {
	    return Core::form("composeWithLm(%s;%s,dim=%zu)",
			      fsa_->describe().c_str(),
			      lm_->fullName().c_str(),
			      id_);
	}
    };


    ConstLatticeRef composeWithLm(ConstLatticeRef l, ConstLanguageModelRef lm, ScoreId id, Score scale, bool forceSentenceEnd) {
	l = mapInputToLemmaOrLemmaPronunciation(transducer(l));
	return ConstLatticeRef(new LmComposeLattice(l, lm, scale, forceSentenceEnd, id));
    }

    class ComposeWithLmNode : public RescoreSingleDimensionNode {
    public:
	static const Core::ParameterBool paramForceSentenceEnd;
	static const Core::ParameterBool paramProjectInput;
    private:
	ConstLanguageModelRef lm_;
	bool forceSentenceEnd_;
	bool projectInput_;
    protected:
	virtual ConstLatticeRef rescore(ConstLatticeRef l, ScoreId id) {
	    l = composeWithLm(l, lm_, id, scale(), forceSentenceEnd_);
	    if (projectInput_)
		l = projectInput(l);
	    return l;
	}
	virtual void setRescoreMode(RescoreMode _rescoreMode) {
	    if (_rescoreMode != RescoreModeClone)
		warning("ComposeWithLmNode: Rescore mode must be \"%s\".",
			getRescoreModeName(RescoreModeClone).c_str());
	}
    public:
	ComposeWithLmNode(const std::string &name, const Core::Configuration &config) :
	    RescoreSingleDimensionNode(name, config) {}
	~ComposeWithLmNode() {}
	virtual void init(const std::vector<std::string> &arguments) {
	    lm_ = Lm::Module::instance().createLanguageModel(select("lm"), Lexicon::us());
	    if (!lm_)
		criticalError("ComposeWithLmNode: failed to load language model");
	    forceSentenceEnd_ = paramForceSentenceEnd(config);
	    projectInput_ = paramProjectInput(config);
	    if (forceSentenceEnd_)
		log("Treat end of segment as end of sentence; compatible with the structure of Sprint recognizer lattices.");
	}
    };
    const Core::ParameterBool ComposeWithLmNode::paramForceSentenceEnd(
	"force-sentence-end",
	"treat end of segment as end of sentence",
	true);
    const Core::ParameterBool ComposeWithLmNode::paramProjectInput(
	"project-input",
	"project input",
	false);

    NodeRef createComposeWithLmNode(const std::string &name, const Core::Configuration &config) {
	return NodeRef(new ComposeWithLmNode(name, config));
    }
    // -------------------------------------------------------------------------

} // namespace Flf

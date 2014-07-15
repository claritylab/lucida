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
#include "Compose.hh"
#include <Fsa/Hash.hh>
#include <Fsa/Basic.hh>
#include <Fsa/Cache.hh>
#include <Fsa/Properties.hh>
#include <Bliss/Fsa.hh>


using namespace Lm;

namespace LmComposeInternal {

    class ComposeAutomaton :
	public Lm::ComposeAutomaton
    {
    protected:
	Fsa::ConstAutomatonRef left_;
	Core::Ref<const Lm::LanguageModel> right_;
	Score lmScale_, syntaxEmissionScale_;

    private:
	struct State {
	    Fsa::StateId left;
	    Lm::History right;

	    State(Fsa::StateId ll, Lm::History rr) : left(ll), right(rr) {}

	    struct Hash {
		size_t operator() (const State &st) const {
		    return (size_t(st.left) * 2239) + st.right.hashKey();
		}
	    };

	    bool operator== (const State &st) const {
		return (left == st.left) && (right == st.right);
	    }
	};

	mutable Fsa::Hash<State, State::Hash> states_;

	Fsa::StateId insertState(Fsa::StateId ll, Lm::History rr) const {
	    Fsa::StateId id = states_.insert(State(ll, rr));
	    hope(id <= Fsa::StateIdMask);
	    return id;
	}

    protected:
	/**
	 *  Add the lemma score to the arc weight.
	 */
	virtual void addScore(Fsa::Arc &, Lm::History &) const = 0;

    public:
	ComposeAutomaton(
	    Fsa::ConstAutomatonRef left,
	    Core::Ref<const Lm::LanguageModel> right,
	    Score lmScale, Score syntaxEmissionScale) :
	    left_(left), right_(right), lmScale_(lmScale), syntaxEmissionScale_(syntaxEmissionScale)
	{
	    setProperties(Fsa::PropertyStorage | Fsa::PropertyCached, 0);
	    setProperties(Fsa::PropertyAcyclic, Fsa::hasProperties(left_, Fsa::PropertyAcyclic));
	}

	virtual ~ComposeAutomaton() { }

	virtual std::string describe() const {
	    return "Lm::compose(" + left_->describe() + ", " + right_->fullName() + ")";
	}

	virtual Fsa::Type type() const {
	    return left_->type();
	}

	virtual Fsa::ConstSemiringRef semiring() const {
	    return left_->semiring();
	}

	virtual Fsa::ConstAlphabetRef getInputAlphabet() const {
	    return left_->getInputAlphabet();
	}

	virtual Fsa::ConstAlphabetRef getOutputAlphabet() const {
	    return left_->getOutputAlphabet();
	}

	virtual Fsa::StateId initialStateId() const {
	    if (left_->initialStateId() == Fsa::InvalidStateId)
		return Fsa::InvalidStateId;
	    return insertState(left_->initialStateId(), right_->startHistory());
	}

	virtual Fsa::StateId leftStateId(Fsa::StateId si) const {
	    return states_[si].left;
	}

	virtual Lm::History history(Fsa::StateId si) const {
	    return states_[si].right;
	}

	virtual void dumpState(Fsa::StateId si, std::ostream &os) const {
	    if (si >= states_.size()) {
		os << "unknown"; return;
	    }
	    os << "(";
	    left_->dumpState(states_[si].left, os);
	    os << ", "
	       << right_->formatHistory(states_[si].right)
	       << ")";
	}

	virtual Fsa::ConstStateRef getState(Fsa::StateId si) const {
	    require(si < states_.size());
	    State state(states_[si]);
	    Fsa::State *result = new Fsa::State(*left_->getState(state.left));
	    result->setId(si);
	    for (Fsa::State::iterator aa = result->begin(); aa != result->end(); ++aa) {
		Lm::History history = state.right;
		addScore(*aa, history);
		aa->target_ = insertState(aa->target_, history);
	    }
	    if (result->isFinal()) {
		result->weight_ = semiring()->extend(
		    result->weight_,
		    Fsa::Weight(lmScale_ * right_->sentenceEndScore(state.right)));
	    }
	    return Fsa::ConstStateRef(result);
	}

	static ComposeAutomaton* createComposeAutomaton(Fsa::ConstAutomatonRef left,
							Core::Ref<const Lm::LanguageModel> right,
							Score lmScale, Score syntaxEmissionScale);
    };

    class LemmaComposeAutomaton :
	public ComposeAutomaton
    {
	typedef ComposeAutomaton Precursor;
    protected:
	void addScore(Fsa::Arc &a, Lm::History &history) const {
	    if (a.output() != Fsa::Epsilon) {
		require(a.output() <= Fsa::LastLabelId);
		Lm::Score score = f32(a.weight_);
		Lm::addLemmaScore(right_, lmScale_,
				  right_->lexicon()->lemmaAlphabet()->lemma(a.output()),
				  syntaxEmissionScale_,
				  history, score);
		a.weight_ = Fsa::Weight(score);
	    }
	}

    public:
	LemmaComposeAutomaton(Fsa::ConstAutomatonRef left,
			      Core::Ref<const Lm::LanguageModel> right,
			      Score lmScale, Score syntaxEmissionScale) :
	    Precursor(left, right, lmScale, syntaxEmissionScale)
	{
	    left_ = Fsa::mapOutput(left_, right_->lexicon()->lemmaAlphabet(), true);
	    left_ = Fsa::cache(left_);
	}

	virtual ~LemmaComposeAutomaton() { }
    };

    class LemmaPronunciationComposeAutomaton :
	public ComposeAutomaton
    {
	typedef ComposeAutomaton Precursor;
    private:
	Core::Ref<const Bliss::LemmaPronunciationAlphabet> alphabet_;
	Score pronunciationScale_;

    protected:
	void addScore(Fsa::Arc &a, Lm::History &history) const {
	    const Bliss::LemmaPronunciation *pronunciation =
		alphabet_->lemmaPronunciation(a.input());
	    if (pronunciation) {
		Lm::Score score = f32(a.weight_);
		Lm::addLemmaPronunciationScore(
		    right_, lmScale_,
		    pronunciation,
		    pronunciationScale_,
		    syntaxEmissionScale_,
		    history, score);
		a.weight_ = Fsa::Weight(score);
	    }
	}

    public:
	LemmaPronunciationComposeAutomaton(Fsa::ConstAutomatonRef left,
					   Core::Ref<const Lm::LanguageModel> right,
					   Score lmScale,
					   Score pronunciationScale ,
					   Score syntaxEmissionScale) :
	    Precursor(left, right, lmScale, syntaxEmissionScale),
	    alphabet_(required_cast(const Bliss::LemmaPronunciationAlphabet*,
				    left_->getInputAlphabet().get())),
	    pronunciationScale_(pronunciationScale)
	{
	    left_ = Fsa::mapInput(left_, right_->lexicon()->lemmaPronunciationAlphabet(), true);
	    left_ = Fsa::cache(left_);
	}

	virtual ~LemmaPronunciationComposeAutomaton() { }
    };

    ComposeAutomaton* ComposeAutomaton::createComposeAutomaton(
	Fsa::ConstAutomatonRef left,
	Core::Ref<const Lm::LanguageModel> right,
	Score lmScale, Score syntaxEmissionScale)
    {
	ComposeAutomaton *composeAutomaton = 0;
	if (left->getOutputAlphabet() == right->lexicon()->lemmaAlphabet()) {
	    composeAutomaton = new LemmaComposeAutomaton(left, right, lmScale, syntaxEmissionScale);
	} else if (left->getInputAlphabet() == right->lexicon()->lemmaPronunciationAlphabet()) {
	    composeAutomaton = new LemmaPronunciationComposeAutomaton(left, right, lmScale, 0.0, syntaxEmissionScale);
	} else {
	    defect();
	}
	return composeAutomaton;
    }

    class LeftComposeMapping : public Fsa::Mapping {
	typedef Fsa::Mapping Precursor;
    private:
	Core::Ref<const Lm::ComposeAutomaton> fsa_;
    public:
	LeftComposeMapping(Core::Ref<const Lm::ComposeAutomaton> f) : fsa_(f) { require(fsa_); }
	Fsa::StateId map(Fsa::StateId target) const { return fsa_->leftStateId(target); }
    };

} // namespace LmComposeInternal;

Fsa::ConstMappingRef ComposeAutomaton::mapToLeft() const
{
    return Fsa::ConstMappingRef(new LmComposeInternal::LeftComposeMapping(
				    Core::Ref<const ComposeAutomaton>(this)));
}

namespace Lm {
    Core::Ref<const ComposeAutomaton> compose(
	Fsa::ConstAutomatonRef left,
	Core::Ref<const LanguageModel> right,
	Score lmScale,
	Score syntaxEmissionScale)
    {
	return Core::ref(LmComposeInternal::ComposeAutomaton::createComposeAutomaton(
			     left, right, lmScale, syntaxEmissionScale));
    }

    Core::Ref<const ComposeAutomaton> compose(
	Fsa::ConstAutomatonRef left,
	Core::Ref<const ScaledLanguageModel> right,
	Score syntaxEmissionScale)
    {
	return Core::ref(LmComposeInternal::ComposeAutomaton::createComposeAutomaton(
			     left, right->unscaled(), right->scale(), syntaxEmissionScale));
    }



    Core::Ref<const ComposeAutomaton> composePron(
	Fsa::ConstAutomatonRef left,
	Core::Ref<const ScaledLanguageModel> right,
	Score pronunciationScale,
	Score syntaxEmissionScale)
    {
	require(left->getInputAlphabet() == right->lexicon()->lemmaPronunciationAlphabet());
	return Core::ref(new LmComposeInternal::LemmaPronunciationComposeAutomaton(
			     left, right->unscaled(), right->scale(), pronunciationScale, syntaxEmissionScale));
    }

} // namespace Lm

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
#include <Core/ProgressIndicator.hh>
#include "Hash.hh"
#include "Stack.hh"
#include <Core/Vector.hh>

#include "tAccessible.hh"
#include "tAutomaton.hh"
#include "tBasic.hh"
#include "tDfs.hh"


namespace Ftl {
    template<class _Automaton>
    class NormalizeAutomaton : public SlaveAutomaton<_Automaton> {
	typedef SlaveAutomaton<_Automaton> Precursor;
    public:
	typedef typename _Automaton::State _State;
	typedef typename _Automaton::ConstStateRef _ConstStateRef;
	typedef typename _Automaton::ConstRef _ConstAutomatonRef;
    private:
	struct StateIdKey { u32 operator() (Fsa::StateId s) { return s; } };
	mutable Fsa::Hash<Fsa::StateId, StateIdKey> states_;
    public:
	NormalizeAutomaton(_ConstAutomatonRef f) : Precursor(f) {}
	virtual Fsa::StateId initialStateId() const {
	    Fsa::StateId s = Precursor::fsa_->initialStateId();
	    if (s != Fsa::InvalidStateId) return states_.insert(s);
	    return s;
	}
	virtual _ConstStateRef getState(Fsa::StateId s) const {
	    if (s >= states_.size()) return _ConstStateRef();
	    _State *sp = new _State(*Precursor::fsa_->getState(states_[s]));
	    sp->setId(s);
	    for (typename _State::iterator a = sp->begin(); a != sp->end(); ++a)
		a->target_ = states_.insert(a->target());
	    return _ConstStateRef(sp);
	}
	virtual std::string describe() const { return "normalize(" + Precursor::fsa_->describe() + ")"; }

	/** State in the original automaton.
	 * Given a state of the normalization result, return the
	 * corresponding original state of the automaton. */
	virtual Fsa::StateId originalStateId(Fsa::StateId s) const {
	    return states_[s];
	}
    };

    template<class _Automaton>
    typename _Automaton::ConstRef normalize(typename _Automaton::ConstRef f) {
	return typename _Automaton::ConstRef(new NormalizeAutomaton<_Automaton>(f));
    }

    template<class _Automaton>
    class NormalizeMapping : public Fsa::Mapping {
    public:
	typedef typename _Automaton::ConstRef _ConstAutomatonRef;
	typedef NormalizeAutomaton<_Automaton> _NormalizeAutomaton;
    protected:
	_ConstAutomatonRef fsa_;
	const _NormalizeAutomaton *nFsa_;
    public:
	NormalizeMapping(_ConstAutomatonRef f) : fsa_(f), nFsa_(dynamic_cast<const _NormalizeAutomaton*>(f.get())) {
	    require(nFsa_);
	}
	Fsa::StateId map(Fsa::StateId target) const {
	    return nFsa_->originalStateId(target);
	}
    };

    template<class _Automaton>
    Fsa::ConstMappingRef mapNormalized(typename _Automaton::ConstRef f) {
	return Fsa::ConstMappingRef(new NormalizeMapping<_Automaton>(f));
    }

    /**
     * trim deletes all non-accessible and non-coaccessible states, optimized by
     * using SCC's
     */
    template<class _Automaton>
    class TrimAutomaton : public SlaveAutomaton<_Automaton> {
	typedef SlaveAutomaton<_Automaton> Precursor;
    public:
	typedef typename _Automaton::State _State;
	typedef typename _Automaton::ConstStateRef _ConstStateRef;
	typedef typename _Automaton::ConstRef _ConstAutomatonRef;
	typedef CoaccessibleDfsState<_Automaton> _CoaccessibleDfsState;
    private:
	std::vector<bool> accAndCoacc_;
    public:
	TrimAutomaton(_ConstAutomatonRef f, bool progress) : Precursor(f) {
	    Core::ProgressIndicator *p = 0;
	    if (progress) p = new Core::ProgressIndicator("connecting", "states");
	    _CoaccessibleDfsState v(f);
	    v.dfs(p);
	    if (p) delete p;
	    accAndCoacc_.resize(v.size());
	    for (Fsa::StateId s = 0; s < accAndCoacc_.size(); ++s)
		accAndCoacc_[s] = //(v.flags()[s] & AccessibleDfsState::FlagAccessible) &&
		    (v.flags()[s] & _CoaccessibleDfsState::FlagCoaccessible);
	}
	virtual Fsa::StateId initialStateId() const {
	    Fsa::StateId s = Precursor::fsa_->initialStateId();
	    if ((s < accAndCoacc_.size()) && (accAndCoacc_[s])) return s;
	    return Fsa::InvalidStateId;
	}
	virtual _ConstStateRef getState(Fsa::StateId s) const {
	    if (accAndCoacc_[s]) {
		_ConstStateRef _s = Precursor::fsa_->getState(s);
		_State *sp = new _State(_s->id(), _s->tags(), _s->weight_);
		for (typename _State::const_iterator a = _s->begin(); a != _s->end(); ++a)
		    if (accAndCoacc_[a->target()]) *sp->newArc() = *a;
		sp->minimize();
		return _ConstStateRef(sp);
	    }
	    return _ConstStateRef();
	}
	virtual size_t getMemoryUsed() const {
	    return Precursor::fsa_->getMemoryUsed() + accAndCoacc_.size() / 8;
	}
	virtual void dumpMemoryUsage(Core::XmlWriter &o) const {
	    o << Core::XmlOpen("trim");
	    Precursor::fsa_->dumpMemoryUsage(o);
	    o << Core::XmlFull("states", accAndCoacc_.size() / 8) << Core::XmlClose("trim");
	}
	virtual std::string describe() const {
	    return "trim(" + Precursor::fsa_->describe() + ")";
	}
    };

    template<class _Automaton>
    typename _Automaton::ConstRef trim(typename _Automaton::ConstRef f, bool progress) {
	return typename _Automaton::ConstRef(new TrimAutomaton<_Automaton>(f, progress));
    }


    template<class _Automaton>
    class PartialAutomaton : public SlaveAutomaton<_Automaton> {
	typedef SlaveAutomaton<_Automaton> Precursor;
    public:
	typedef typename _Automaton::State _State;
	typedef typename _Automaton::ConstStateRef _ConstStateRef;
	typedef typename _Automaton::ConstRef _ConstAutomatonRef;
    private:
	Fsa::StateId newInitialStateId_;
	typename _Automaton::Weight additionalFinalWeight_;
    public:
	PartialAutomaton(_ConstAutomatonRef f, Fsa::StateId newInitialStateId) :
	    Precursor(f), newInitialStateId_(newInitialStateId), additionalFinalWeight_(f->semiring()->one()) {}
	PartialAutomaton(_ConstAutomatonRef f, Fsa::StateId newInitialStateId, typename _Automaton::Weight additionalFinalWeight) :
	    Precursor(f), newInitialStateId_(newInitialStateId), additionalFinalWeight_(additionalFinalWeight) {}
	virtual Fsa::StateId initialStateId() const { return newInitialStateId_; }
	virtual std::string describe() const {
	    return Core::form("partial(%s,%d)", Precursor::fsa_->describe().c_str(), newInitialStateId_);
	}
	virtual typename _Automaton::ConstStateRef getState(Fsa::StateId s) const {
	    _ConstStateRef sp = Precursor::fsa_->getState(s);
	    if (sp->isFinal()) {
		_State *_sp = new _State(*sp);
		_sp->setWeight(Precursor::fsa_->semiring()->extend(_sp->weight(), additionalFinalWeight_));
		return _ConstStateRef(_sp);
	    } else {
		return sp;
	    }
	}
    };

    template<class _Automaton>
    typename _Automaton::ConstRef partial(typename _Automaton::ConstRef f, Fsa::StateId initialStateOfPartialAutomaton) {
	return typename _Automaton::ConstRef(new PartialAutomaton<_Automaton>(f, initialStateOfPartialAutomaton));
    }

    template<class _Automaton>
    typename _Automaton::ConstRef partial(typename _Automaton::ConstRef f, Fsa::StateId initialStateOfPartialAutomaton, typename _Automaton::Weight additionalFinalWeight) {
	return typename _Automaton::ConstRef(new PartialAutomaton<_Automaton>(f, initialStateOfPartialAutomaton, additionalFinalWeight));
    }


    template<class _Automaton>
    class HypothesisAutomaton : public ModifyAutomaton<_Automaton> {
	typedef ModifyAutomaton<_Automaton> Precursor;
    public:
	typedef typename _Automaton::ConstRef _ConstAutomatonRef;
	typedef typename _Automaton::State _State;
    private:
	Fsa::StateId n_;
    public:
	HypothesisAutomaton(_ConstAutomatonRef f, Fsa::StateId n) :
	    Precursor(f), n_(n) {}
	virtual Fsa::StateId initialStateId() const { return n_; }
	virtual std::string describe() const {
	    return Core::form("hypothesis(%s,%d)", Precursor::fsa_->describe().c_str(), n_);
	}
	virtual void modifyState(_State *sp) const {
	    if(sp->id() == initialStateId()) {
		sp->begin()->setWeight(this->fsa_->getState(n_)->begin()->weight());
	    }
	}
    };

    template<class _Automaton>
    typename _Automaton::ConstRef hypothesis(typename _Automaton::ConstRef f, Fsa::StateId initialStateOfHypothesisAutomaton) {
	return typename _Automaton::ConstRef(new HypothesisAutomaton<_Automaton>(f, initialStateOfHypothesisAutomaton));
    }


    template<class _Automaton>
    class ChangeSemiringAutomaton : public SlaveAutomaton<_Automaton> {
	typedef SlaveAutomaton<_Automaton> Precursor;
    public:
	typedef typename _Automaton::ConstStateRef _ConstStateRef;
	typedef typename _Automaton::ConstRef _ConstAutomatonRef;
	typedef typename _Automaton::ConstSemiringRef _ConstSemiringRef;
    private:
	_ConstSemiringRef semiring_;
    public:
	ChangeSemiringAutomaton(_ConstAutomatonRef f, _ConstSemiringRef semiring) :
	    Precursor(f), semiring_(semiring) {}
	virtual _ConstSemiringRef semiring() const { return semiring_; }
	virtual _ConstStateRef getState(Fsa::StateId s) const { return Precursor::fsa_->getState(s); }
	virtual std::string describe() const {
	    return Core::form("changeSemiring(%s;%s,%s)",
			      Precursor::fsa_->describe().c_str(), Precursor::fsa_->semiring()->name().c_str(), semiring_->name().c_str());
	}
    };

    template<class _Automaton>
    typename _Automaton::ConstRef changeSemiring(typename _Automaton::ConstRef f, typename _Automaton::ConstSemiringRef semiring) {
	return typename _Automaton::ConstRef(new ChangeSemiringAutomaton<_Automaton>(f, semiring));
    }

    /**
     * Functor defining a mapping between the two sets
     *
     * struct _Mapping {
     *   _AutomatonTo::Weight operator()(const _AutomatonFrom::Weight &) const;
     * };
     **/
    template<class _AutomatonFrom, class _AutomatonTo, class _Mapping>
    class ConverterAutomaton : public WrapperAutomaton<_AutomatonFrom, _AutomatonTo> {
	typedef WrapperAutomaton<_AutomatonFrom, _AutomatonTo> Precursor;
    public:
	typedef typename _AutomatonFrom::State _FromState;
	typedef Core::Ref<const _AutomatonFrom> _ConstFromAutomatonRef;
	typedef Core::Ref<const _FromState> _ConstFromStateRef;
	typedef typename _AutomatonTo::State _State;
	typedef typename _AutomatonTo::Semiring _Semiring;
	typedef Core::Ref<const _AutomatonTo> _ConstAutomatonRef;
	typedef Core::Ref<const _State> _ConstStateRef;
	typedef Core::Ref<const _Semiring> _ConstSemiringRef;
    private:
	_ConstSemiringRef semiring_;
	const _Mapping map_;
    public:
	ConverterAutomaton(_ConstFromAutomatonRef f, _ConstSemiringRef semiring, const _Mapping &map) :
	    Precursor(f), semiring_(semiring), map_(map) {}
	virtual _ConstSemiringRef semiring() const { return semiring_; }
	virtual _ConstStateRef getState(Fsa::StateId s) const {
	    _ConstFromStateRef fromSp = Precursor::fsa_->getState(s);
	    _State *sp = (fromSp->isFinal()) ?
		new _State(s, fromSp->tags(), map_(fromSp->weight())) :
		new _State(s, fromSp->tags(), semiring()->one());
	    for (typename _FromState::const_iterator a = fromSp->begin(); a != fromSp->end(); ++a)
		sp->newArc(a->target(), map_(a->weight()), a->input(), a->output());
	    return _ConstStateRef(sp);
	}
	virtual std::string describe() const {
	    return Core::form("changeSemiringAndSet(%s;%s,%s)",
			      Precursor::fsa_->describe().c_str(), Precursor::fsa_->semiring()->name().c_str(), semiring_->name().c_str());
	}
    };

    template<class _AutomatonFrom, class _AutomatonTo, class _Mapping>
    Core::Ref<const _AutomatonTo>
    convert(Core::Ref<const _AutomatonFrom> f, Core::Ref<const typename _AutomatonTo::Semiring> semiring, const _Mapping &map) {
	return Core::Ref<const _AutomatonTo>(new ConverterAutomaton<_AutomatonFrom, _AutomatonTo, _Mapping>(f, semiring, map));
    }


} //namespace Ftl

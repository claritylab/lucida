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
#include "tArithmetic.hh"
#include "Arithmetic.hh"
#include "Automaton.hh"
#include "Types.hh"
#include "Properties.hh"

namespace Ftl {

    template<class _Automaton>
    class CollectAutomaton : public ModifyAutomaton<_Automaton> {
    private:
	typedef ModifyAutomaton<_Automaton> Precursor;
    public:
	typedef typename _Automaton::Weight _Weight;
	typedef typename _Automaton::State _State;
	typedef typename _Automaton::ConstRef _ConstAutomatonRef;
    private:
	_Weight value_;
    public:
	CollectAutomaton(_ConstAutomatonRef f, _Weight value) : Precursor(f), value_(value) {}
	virtual void modifyState(_State *sp) const {
	    if (sp->hasTags(Fsa::StateTagFinal)) sp->weight_ = this->semiring()->collect(sp->weight_, value_);
	    for (typename _State::iterator a = sp->begin(); a != sp->end(); ++a)
		a->weight_ = this->semiring()->collect(a->weight_, value_);
	}
	virtual std::string describe() const {
	    return Core::form("collect(%s,%s)", Precursor::fsa_->describe().c_str(), this->semiring()->asString(value_).c_str());
	}
    };

    template<class _Automaton>
    typename _Automaton::ConstRef collect(typename _Automaton::ConstRef f, typename _Automaton::Weight value) {
	return typename _Automaton::ConstRef(new CollectAutomaton<_Automaton>(f, value));
    }


    template<class _Automaton>
    class ExtendAutomaton : public ModifyAutomaton<_Automaton> {
    private:
	typedef ModifyAutomaton<_Automaton> Precursor;
    public:
	typedef typename _Automaton::Weight _Weight;
	typedef typename _Automaton::State _State;
	typedef typename _Automaton::ConstRef _ConstAutomatonRef;
    private:
	_Weight value_;
    public:
	ExtendAutomaton(_ConstAutomatonRef f, _Weight value) : Precursor(f), value_(value) {}
	virtual void modifyState(_State *sp) const {
	    if (sp->hasTags(Fsa::StateTagFinal)) sp->weight_ = this->semiring()->extend(sp->weight_, value_);
	    for (typename _State::iterator a = sp->begin(); a != sp->end(); ++a)
		a->weight_ = this->semiring()->extend(a->weight_, value_);
	}
	virtual std::string describe() const {
	    return Core::form("extend(%s,%s)", Precursor::fsa_->describe().c_str(), this->semiring()->asString(value_).c_str());
	}
    };

    template<class _Automaton>
    typename _Automaton::ConstRef extend(typename _Automaton::ConstRef f, typename _Automaton::Weight value) {
	return typename _Automaton::ConstRef(new ExtendAutomaton<_Automaton>(f, value));
    }


    template<class _Automaton>
    class ExtendFinalAutomaton : public SlaveAutomaton<_Automaton> {
    private:
	typedef SlaveAutomaton<_Automaton> Precursor;
    public:
	typedef typename _Automaton::Weight _Weight;
	typedef typename _Automaton::State _State;
	typedef typename _Automaton::ConstStateRef _ConstStateRef;
	typedef typename _Automaton::ConstRef _ConstAutomatonRef;
    private:
	_Weight value_;
    public:
	ExtendFinalAutomaton(_ConstAutomatonRef f, _Weight value) : Precursor(f), value_(value) {
	    require(hasProperties(this->fsa_, Fsa::PropertyAcyclic));
	}
	virtual _ConstStateRef getState(Fsa::StateId s) const {
	    _ConstStateRef sp = Precursor::getState(s);
	    if (sp->hasTags(Fsa::StateTagFinal)) {
		_State *fsp = new _State(*sp);
		fsp->weight_ = this->semiring()->extend(fsp->weight_, value_);
		return _ConstStateRef(fsp);
	    }
	    return sp;
	}
	virtual std::string describe() const {
	    return Core::form("extend-final(%s,%s)", Precursor::fsa_->describe().c_str(), this->semiring()->asString(value_).c_str());
	}
    };

    template<class _Automaton>
    typename _Automaton::ConstRef extendFinal(typename _Automaton::ConstRef f, typename _Automaton::Weight value) {
	return typename _Automaton::ConstRef(new ExtendFinalAutomaton<_Automaton>(f, value));
    }


    template<class _Automaton, class Modifier>
    class WeightModifyAutomaton : public ModifyAutomaton<_Automaton> {
    private:
	typedef ModifyAutomaton<_Automaton> Precursor;
    public:
	typedef typename _Automaton::Weight _Weight;
	typedef typename _Automaton::State _State;
	typedef typename _Automaton::ConstRef _ConstAutomatonRef;
    private:
	const Modifier m_;
    public:
	WeightModifyAutomaton(_ConstAutomatonRef f, const Modifier &m) : Precursor(f), m_(m) {}
	virtual void modifyState(_State *sp) const {
	    if (sp->hasTags(Fsa::StateTagFinal)) sp->weight_ = m_(sp->weight_);
	    for (typename _State::iterator a = sp->begin(); a != sp->end(); ++a)
		a->weight_ = m_(a->weight_);
	}
	virtual std::string describe() const {
	    return Core::form("modifyWeight(%s,%s)",
			      Precursor::fsa_->describe().c_str(),
			      m_.describe().c_str());
	}
    };

    template<class _Automaton, class Modifier>
    typename _Automaton::ConstRef modifyWeight(typename _Automaton::ConstRef f, const Modifier &modifier) {
	return typename _Automaton::ConstRef(new WeightModifyAutomaton<_Automaton, Modifier>(f, modifier));
    }

} // namespace Ftl

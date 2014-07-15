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
#include "tAutomaton.hh"
#include "tProject.hh"
#include "Alphabet.hh"
#include "Types.hh"
#include "tProperties.hh"

namespace Ftl {

    template<class _Automaton>
    class ProjectInputAutomaton : public ModifyAutomaton<_Automaton> {
	typedef ModifyAutomaton<_Automaton> Precursor;
    public:
	typedef typename _Automaton::ConstRef _ConstAutomatonRef;
	typedef typename _Automaton::State _State;
    public:
	ProjectInputAutomaton(_ConstAutomatonRef f) : Precursor(f) {
	    this->unsetProperties(Fsa::PropertySortedByArc | Fsa::PropertySortedByOutput);
	    if (hasProperties<_Automaton>(Precursor::fsa_, Fsa::PropertySortedByInput))
		this->setProperties(Fsa::PropertySortedByOutput);
	}
	virtual Fsa::Type type() const { return Fsa::TypeAcceptor; }
	virtual Fsa::ConstAlphabetRef getOutputAlphabet() const { return Precursor::fsa_->getInputAlphabet(); }
	virtual void modifyState(_State *sp) const {
	    for (typename _State::iterator a = sp->begin(); a != sp->end(); ++a) a->output_ = a->input_;
	}
	virtual std::string describe() const {
	    return "projectInput(" + Precursor::fsa_->describe() + ")";
	}
    };

    template<class _Automaton>
    typename _Automaton::ConstRef projectInput(typename _Automaton::ConstRef f) {
	if (f->type() == Fsa::TypeAcceptor) return f;
	return typename _Automaton::ConstRef(new ProjectInputAutomaton<_Automaton>(f));
    }

    template<class _Automaton>
    class ProjectOutputAutomaton : public ModifyAutomaton<_Automaton> {
	typedef ModifyAutomaton<_Automaton> Precursor;
    public:
	typedef typename _Automaton::ConstRef _ConstAutomatonRef;
	typedef typename _Automaton::State _State;
    public:
	ProjectOutputAutomaton(_ConstAutomatonRef f) : Precursor(f) {
	    this->unsetProperties(Fsa::PropertySortedByArc | Fsa::PropertySortedByInput | Fsa::PropertySortedByInputAndTarget);
	    if (hasProperties<_Automaton>(Precursor::fsa_, Fsa::PropertySortedByOutput))
		this->setProperties(Fsa::PropertySortedByInput);
	}
	virtual Fsa::Type type() const { return Fsa::TypeAcceptor; }
	virtual Fsa::ConstAlphabetRef getInputAlphabet() const { return Precursor::fsa_->getOutputAlphabet(); }
	virtual void modifyState(_State *sp) const {
	    for (typename _State::iterator a = sp->begin(); a != sp->end(); ++a) a->input_ = a->output_;
	}
	virtual std::string describe() const {
	    return "projectOutput(" + Precursor::fsa_->describe() + ")";
	}
    };

    template<class _Automaton>
    typename _Automaton::ConstRef projectOutput(typename _Automaton::ConstRef f) {
	if (f->type() == Fsa::TypeAcceptor) return f;
	return typename _Automaton::ConstRef(new ProjectOutputAutomaton<_Automaton>(f));
    }

    template<class _Automaton>
    class InvertAutomaton : public ModifyAutomaton<_Automaton> {
	typedef ModifyAutomaton<_Automaton> Precursor;
    public:
	typedef typename _Automaton::ConstRef _ConstAutomatonRef;
	typedef typename _Automaton::State _State;
    public:
	InvertAutomaton(_ConstAutomatonRef f) : Precursor(f) {
	    this->unsetProperties(Fsa::PropertySorted);
	    if (hasProperties<_Automaton>(Precursor::fsa_, Fsa::PropertySortedByInput))
		this->setProperties(Fsa::PropertySortedByOutput);
	    if (hasProperties<_Automaton>(Precursor::fsa_, Fsa::PropertySortedByOutput))
		this->setProperties(Fsa::PropertySortedByInput);
	    if (hasProperties<_Automaton>(Precursor::fsa_, Fsa::PropertySortedByWeight))
		this->setProperties(Fsa::PropertySortedByWeight);
	}
	virtual Fsa::ConstAlphabetRef getInputAlphabet() const { return Precursor::fsa_->getOutputAlphabet(); }
	virtual Fsa::ConstAlphabetRef getOutputAlphabet() const { return Precursor::fsa_->getInputAlphabet(); }
	virtual void modifyState(_State *sp) const {
	    for (typename _State::iterator a = sp->begin(); a != sp->end(); ++a) {
		Fsa::LabelId tmp = a->input_;
		a->input_ = a->output_;
		a->output_ = tmp;
	    }
	}
	virtual std::string describe() const {
	    return "invert(" + Precursor::fsa_->describe() + ")";
	}
    };

    template<class _Automaton>
    typename _Automaton::ConstRef invert(typename _Automaton::ConstRef f) {
	return typename _Automaton::ConstRef(new InvertAutomaton<_Automaton>(f));
    }

} // namespace Ftl

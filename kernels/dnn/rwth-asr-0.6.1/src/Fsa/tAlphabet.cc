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

#include "tAutomaton.hh"
#include "tAlphabet.hh"

namespace Ftl {
    template<class _Automaton>
    class ChangeInputAlphabetAutomaton : public SlaveAutomaton<_Automaton> {
	typedef SlaveAutomaton<_Automaton> Precursor;
    public:
	typedef typename _Automaton::ConstRef ConstAutomatonRef;
	typedef typename _Automaton::ConstStateRef ConstStateRef;
    protected:
	Fsa::ConstAlphabetRef inputAlphabet_;
    public:
	ChangeInputAlphabetAutomaton(ConstAutomatonRef f, const Fsa::ConstAlphabetRef a) :
	    Precursor(f), inputAlphabet_(a) {}
	virtual Fsa::ConstAlphabetRef getInputAlphabet() const {
	    return inputAlphabet_;
	}
	virtual ConstStateRef getState(Fsa::StateId s) const { return Precursor::fsa_->getState(s); }
	virtual std::string describe() const { return "ChangeInputAlphabetAutomaton(" + Precursor::fsa_->describe() + ")"; }
    };

    template<class _Automaton>
    class ChangeOutputAlphabetAutomaton : public SlaveAutomaton<_Automaton> {
	typedef SlaveAutomaton<_Automaton> Precursor;
    public:
	typedef typename _Automaton::ConstRef ConstAutomatonRef;
	typedef typename _Automaton::ConstStateRef ConstStateRef;
    protected:
	Fsa::ConstAlphabetRef outputAlphabet_;
    public:
	ChangeOutputAlphabetAutomaton(ConstAutomatonRef f, const Fsa::ConstAlphabetRef a) :
	    Precursor(f), outputAlphabet_(a) {}
	virtual Fsa::ConstAlphabetRef getOutputAlphabet() const {
	    return outputAlphabet_;
	}
	virtual ConstStateRef getState(Fsa::StateId s) const { return Precursor::fsa_->getState(s); }
	virtual std::string describe() const { return "ChangeOutputAlphabetAutomaton(" + Precursor::fsa_->describe() + ")"; }
    };

    template<class _Automaton>
    class ChangeInputOutputAlphabetAutomaton : public SlaveAutomaton<_Automaton> {
	typedef SlaveAutomaton<_Automaton> Precursor;
    public:
	typedef typename _Automaton::ConstRef ConstAutomatonRef;
	typedef typename _Automaton::ConstStateRef ConstStateRef;
    protected:
	Fsa::ConstAlphabetRef alphabet_;
    public:
	ChangeInputOutputAlphabetAutomaton(ConstAutomatonRef f, const Fsa::ConstAlphabetRef a) :
	    Precursor(f), alphabet_(a) {}
	virtual Fsa::ConstAlphabetRef getInputAlphabet() const {
	    return alphabet_;
	}
	virtual Fsa::ConstAlphabetRef getOutputAlphabet() const {
	    return alphabet_;
	}
	virtual ConstStateRef getState(Fsa::StateId s) const { return Precursor::fsa_->getState(s); }
	virtual std::string describe() const { return "ChangeInputOutputAlphabetAutomaton(" + Precursor::fsa_->describe() + ")"; }
    };

    template<class _Automaton>
    class MappedAutomaton : public ModifyAutomaton<_Automaton> {
	typedef ModifyAutomaton<_Automaton> Precursor;
    public:
	typedef typename _Automaton::ConstRef ConstAutomatonRef;
	typedef typename _Automaton::ConstStateRef ConstStateRef;
    protected:
	Fsa::AlphabetMapping mapping_;
    public:
	MappedAutomaton(ConstAutomatonRef f, const Fsa::AlphabetMapping &mapping) :
	    Precursor(f)
	{
	    require(mapping.isModifyingType());
	    mapping_ = mapping;
	    if (!(mapping_.type() & Fsa::AlphabetMapping::typeUnmapped))
		this->unsetProperties(Fsa::PropertySorted);
	}

	virtual Fsa::Type type() const {
	    if (!mapping_.isModifyingType()) return Precursor::fsa_->type();
	    return Fsa::TypeTransducer;
	}
    };

    template<class _Automaton>
    class InputMappedAutomaton : public MappedAutomaton<_Automaton> {
	typedef MappedAutomaton<_Automaton> Precursor;
    public:
	typedef typename _Automaton::ConstRef ConstAutomatonRef;
	typedef typename _Automaton::State State;
	typedef typename _Automaton::ConstStateRef ConstStateRef;
    public:
	InputMappedAutomaton(ConstAutomatonRef f, const Fsa::AlphabetMapping &mapping) :
	    Precursor(f, mapping) {}
	virtual Fsa::ConstAlphabetRef getInputAlphabet() const {
	    return Precursor::mapping_.to();
	}
	virtual void modifyState(State *sp) const {
	    if (Precursor::mapping_.type() & Fsa::AlphabetMapping::typeComplete) {
		for (typename State::iterator a = sp->begin(); a != sp->end(); ++a)
		    a->input_ = Precursor::mapping_[a->input()];
	    } else {
		typename State::iterator b = sp->begin();
		for (typename State::iterator a = sp->begin(); a != sp->end(); ++a) {
		    if (Precursor::mapping_[a->input()] != Fsa::InvalidLabelId) {
			if (a != b) *b = *a;
			b->input_ = Precursor::mapping_[a->input()];
			++b;
		    }
		}
		sp->truncate(b);
	    }
	}
	virtual std::string describe() const { return "InputMappedAutomaton(" + Precursor::fsa_->describe() + ")"; }
    };

    template<class _Automaton>
    class OutputMappedAutomaton : public MappedAutomaton<_Automaton> {
	typedef MappedAutomaton<_Automaton> Precursor;
    public:
	typedef typename _Automaton::ConstRef ConstAutomatonRef;
	typedef typename _Automaton::State State;
	typedef typename _Automaton::ConstStateRef ConstStateRef;
    public:
	OutputMappedAutomaton(ConstAutomatonRef f, const Fsa::AlphabetMapping &mapping) :
	    Precursor(f, mapping) {}
	virtual Fsa::ConstAlphabetRef getOutputAlphabet() const {
	    return Precursor::mapping_.to();
	}
	virtual void modifyState(State *sp) const {
	    if (Precursor::mapping_.type() & Fsa::AlphabetMapping::typeComplete) {
		for (typename State::iterator a = sp->begin(); a != sp->end(); ++a)
		    a->output_ = Precursor::mapping_[a->output()];
	    } else {
		typename State::iterator b = sp->begin();
		for (typename State::iterator a = sp->begin(); a != sp->end(); ++a) {
		    if (Precursor::mapping_[a->output()] != Fsa::InvalidLabelId) {
			if (a != b) *b = *a;
			b->output_ = Precursor::mapping_[a->output()];
			++b;
		    }
		}
		sp->truncate(b);
	    }
	}
	virtual std::string describe() const { return "OutputMappedAutomaton(" + Precursor::fsa_->describe() + ")"; }
    };

    template<class _Automaton>
    class InputOutputMappedAutomaton : public MappedAutomaton<_Automaton> {
	typedef MappedAutomaton<_Automaton> Precursor;
    public:
	typedef typename _Automaton::ConstRef ConstAutomatonRef;
	typedef typename _Automaton::State State;
	typedef typename _Automaton::ConstStateRef ConstStateRef;
    public:
	InputOutputMappedAutomaton(ConstAutomatonRef f, const Fsa::AlphabetMapping &mapping) :
	    Precursor(f, mapping) {}
	virtual Fsa::Type type() const {
	    return Precursor::fsa_->type();
	}
	virtual Fsa::ConstAlphabetRef getInputAlphabet() const {
	    return Precursor::mapping_.to();
	}
	virtual Fsa::ConstAlphabetRef getOutputAlphabet() const {
	    return Precursor::mapping_.to();
	}
	virtual void modifyState(State *sp) const {
	    if (Precursor::mapping_.type() & Fsa::AlphabetMapping::typeComplete) {
		for (typename State::iterator a = sp->begin(); a != sp->end(); ++a) {
		    a->input_  = Precursor::mapping_[a->input()];
		    a->output_ = Precursor::mapping_[a->output()];
		}
	    } else {
		typename State::iterator b = sp->begin();
		for (typename State::iterator a = sp->begin(); a != sp->end(); ++a) {
		    if ((Precursor::mapping_[a->input() ] != Fsa::InvalidLabelId) &&
			(Precursor::mapping_[a->output()] != Fsa::InvalidLabelId)) {
			if (a != b) *b = *a;
			b->input_  = Precursor::mapping_[a->input() ];
			b->output_ = Precursor::mapping_[a->output()];
			++b;
		    }
		}
		sp->truncate(b);
	    }
	}
	virtual std::string describe() const { return "InputOutputMappedAutomaton(" + Precursor::fsa_->describe() + ")"; }
    };


    template<class _Automaton>
    typename _Automaton::ConstRef mapInput(typename _Automaton::ConstRef f, const Fsa::AlphabetMapping &mapping) {
	require(f->getInputAlphabet() == mapping.from() || !mapping.to());
	if (mapping.type() & Fsa::AlphabetMapping::typeIdentity)
	    return f;
	if (!mapping.isModifyingType())
	    return typename _Automaton::ConstRef(new ChangeInputAlphabetAutomaton<_Automaton>(f, mapping.to()));
	return typename _Automaton::ConstRef(new InputMappedAutomaton<_Automaton>(f, mapping));
    }
    template<class _Automaton>
    typename _Automaton::ConstRef mapOutput(typename _Automaton::ConstRef f, const Fsa::AlphabetMapping &mapping) {
	require(f->getOutputAlphabet() == mapping.from() || !mapping.to());
	if (mapping.type() & Fsa::AlphabetMapping::typeIdentity)
	    return f;
	if (!mapping.isModifyingType())
	    return typename _Automaton::ConstRef(new ChangeOutputAlphabetAutomaton<_Automaton>(f, mapping.to()));
	return typename _Automaton::ConstRef(new OutputMappedAutomaton<_Automaton>(f, mapping));
    }
    template<class _Automaton>
    typename _Automaton::ConstRef mapInputOutput(typename _Automaton::ConstRef f, const Fsa::AlphabetMapping &mapping) {
	require(f->getInputAlphabet()  == mapping.from() || !mapping.to());
	require(f->getOutputAlphabet() == mapping.from() || !mapping.to());
	if (mapping.type() & Fsa::AlphabetMapping::typeIdentity)
	    return f;
	if (!mapping.isModifyingType())
	    return typename _Automaton::ConstRef(new ChangeInputOutputAlphabetAutomaton<_Automaton>(f, mapping.to()));
	return typename _Automaton::ConstRef(new InputOutputMappedAutomaton<_Automaton>(f, mapping));
    }

    template<class _Automaton>
    typename _Automaton::ConstRef mapInput(typename _Automaton::ConstRef f, Fsa::ConstAlphabetRef a, u32 reportUnknowns) {
	Fsa::AlphabetMapping mapping;
	mapAlphabet(f->getInputAlphabet(), a, mapping, Fsa::InvalidLabelId, reportUnknowns);
	return mapInput<_Automaton>(f, mapping);
    }
    template<class _Automaton>
    typename _Automaton::ConstRef mapOutput(typename _Automaton::ConstRef f, Fsa::ConstAlphabetRef a, u32 reportUnknowns) {
	Fsa::AlphabetMapping mapping;
	mapAlphabet(f->getOutputAlphabet(), a, mapping, Fsa::InvalidLabelId, reportUnknowns);
	return mapOutput<_Automaton>(f, mapping);
    }
    template<class _Automaton>
    typename _Automaton::ConstRef mapInputOutput(typename _Automaton::ConstRef f, Fsa::ConstAlphabetRef a, u32 reportUnknowns) {
	require(f->getInputAlphabet() == f->getOutputAlphabet());
	Fsa::AlphabetMapping mapping;
	mapAlphabet(f->getInputAlphabet(), a, mapping, Fsa::InvalidLabelId, reportUnknowns);
	return mapInputOutput<_Automaton>(f, mapping);
    }
} // namespace Ftl

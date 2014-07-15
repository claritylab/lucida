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
#ifndef _T_FSA_STORAGE_HH
#define _T_FSA_STORAGE_HH

#include <Core/ReferenceCounting.hh>
#include "Alphabet.hh"
#include "Types.hh"

namespace Ftl {
    template<class _Automaton>
    class StorageAutomaton : public _Automaton {
	typedef _Automaton Precursor;
    public:
	typedef typename _Automaton::Weight _Weight;
	typedef typename _Automaton::State _State;
	typedef typename _Automaton::Semiring _Semiring;
	typedef Core::Ref<const _State> _ConstStateRef;
	typedef Core::Ref<const _Semiring> _ConstSemiringRef;
    protected:
	Fsa::Type type_;
	_ConstSemiringRef semiring_;
	Fsa::StateId initial_;
	Fsa::ConstAlphabetRef input_;
	Fsa::ConstAlphabetRef output_;
    public:
	StorageAutomaton(Fsa::Type type = Fsa::TypeUnknown);
	virtual Fsa::Type type() const;
	virtual void setType(Fsa::Type type);
	virtual void addProperties(Fsa::Property properties) const;
	virtual void setProperties(Fsa::Property knownProperties, Fsa::Property properties) const;
	virtual void unsetProperties(Fsa::Property unknownProperties) const;
	virtual _ConstSemiringRef semiring() const;
	virtual void setSemiring(_ConstSemiringRef semiring) { semiring_ = semiring; }
	virtual Fsa::StateId initialStateId() const;
	virtual void setInitialStateId(Fsa::StateId initial) { initial_ = initial; }

	virtual Fsa::ConstAlphabetRef getInputAlphabet() const;
	virtual void setInputAlphabet(Fsa::ConstAlphabetRef alphabet);
	virtual Fsa::ConstAlphabetRef getOutputAlphabet() const;
	virtual void setOutputAlphabet(Fsa::ConstAlphabetRef alphabet);

	// created state is NOT stored; use setState to make the state persistent
	virtual _State* createState(Fsa::StateId id = 0, Fsa::StateTag tags = Fsa::StateTagNone) const;
	virtual _State* createState(Fsa::StateId id, Fsa::StateTag tags, const _Weight&) const;

	virtual bool hasState(Fsa::StateId sid) const = 0;
	virtual void setState(_State *sp) = 0;

	virtual void clear() = 0;
	virtual void deleteState(Fsa::StateId) = 0;
	virtual void normalize();
	/** Highest valid state ID. */
	virtual Fsa::StateId maxStateId() const = 0;
	/** Smallest unused state ID, i.e. maxStateId()+1. */
	virtual Fsa::StateId size() const = 0;
    };
} // namespace Ftl

#include "tStorage.cc"

#endif // _T_FSA_STORAGE_HH

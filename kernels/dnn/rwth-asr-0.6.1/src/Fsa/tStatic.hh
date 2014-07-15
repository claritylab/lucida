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
#ifndef _T_FSA_STATIC_HH
#define _T_FSA_STATIC_HH

#include "tStorage.hh"
#include "Types.hh"
#include <Core/Vector.hh>

namespace Ftl {
    template<class _Automaton>
    class StaticAutomaton : public StorageAutomaton<_Automaton> {
	typedef StorageAutomaton<_Automaton> Precursor;
    public:
	typedef typename _Automaton::Weight _Weight;
	typedef typename _Automaton::Arc _Arc;
	typedef typename _Automaton::State _State;
	typedef typename _Automaton::Semiring _Semiring;
	typedef Core::Ref<_State> _StateRef;
	typedef Core::Ref<const _State> _ConstStateRef;
	typedef Core::Ref<const _Semiring> _ConstSemiringRef;
    private:
	std::string desc_;
	mutable size_t memoryUsed_;
	Core::Vector<_StateRef> states_;
	std::pair<size_t, size_t> getStateAndArcMemoryUsed() const;
    public:
	StaticAutomaton(Fsa::Type type = Fsa::TypeUnknown) : Precursor(type), desc_("static"), memoryUsed_(0) {}
	StaticAutomaton(const std::string &desc, Fsa::Type type = Fsa::TypeUnknown) : Precursor(type), desc_(desc), memoryUsed_(0) {}
	virtual ~StaticAutomaton();
	virtual void clear();
	void setDescription(const std::string &desc) { desc_ = desc; }
	virtual bool hasState(Fsa::StateId sid) const;
	_State* newState(Fsa::StateId tags = 0);
	_State* newState(Fsa::StateId tags, const _Weight &finalWeight);
	_State* newFinalState(const _Weight &finalWeight);
	void setStateFinal(_State*, const _Weight &finalWeight);
	void setStateFinal(_State*);
	_StateRef state(Fsa::StateId s);
	_State* fastState(Fsa::StateId s) { return states_[s].get(); }
	const _State* fastState(Fsa::StateId s) const { return states_[s].get(); }
	virtual void setState(_State *sp);
	virtual void deleteState(Fsa::StateId);
	virtual _ConstStateRef getState(Fsa::StateId s) const;
	virtual void normalize();
	virtual Fsa::StateId maxStateId() const { return states_.size() - 1; }
	virtual Fsa::StateId size() const { return states_.size(); }
	virtual size_t getMemoryUsed() const;
	virtual void dumpMemoryUsage(Core::XmlWriter &o) const;
	virtual std::string describe() const { return desc_; }
	void compact(Fsa::StateMap &mapping);
    };
} // namespace Ftl

#include "tStatic.cc"

#endif // _T_FSA_STATIC_HH

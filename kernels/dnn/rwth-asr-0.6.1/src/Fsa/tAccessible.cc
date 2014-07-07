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
#ifndef _T_FSA_ACCESSIBLE_CC
#define _T_FSA_ACCESSIBLE_CC

#include "tAutomaton.hh"
#include "tAccessible.hh"

namespace Ftl {

    template<class _Automaton>
    CoaccessibleDfsState<_Automaton>::CoaccessibleDfsState(_ConstAutomatonRef f) :
	Precursor(f), time_(0), scc_(0) {}

    template<class _Automaton>
    void CoaccessibleDfsState<_Automaton>::discoverState(_ConstStateRef sp) {
	Fsa::StateId s = sp->id();
	unfinished_.push(s);
	roots_.push(s);
	flags_.grow(s, 0);
	flags_[s] = time_++ | FlagUnfinished; // | FlagAccessible; // states are accessible by default
	if (sp->isFinal()) flags_[s] |= FlagCoaccessible;
	flags_[this->predecessor(s)] |= flags_[s] & FlagCoaccessible;
    }

    template<class _Automaton>
    void CoaccessibleDfsState<_Automaton>::finishState(Fsa::StateId s) {
	if (s == roots_.top()) {
	    // if any state of the scc is coaccessible we mark the whole scc
	    Fsa::StateId w, coaccessible = flags_[s] & FlagCoaccessible;
	    if (!coaccessible) {
		for (Fsa::Stack<Fsa::StateId>::const_iterator i = unfinished_.begin(); *i != s; ++i)
		    if (flags_[*i] & FlagCoaccessible) {
			coaccessible = FlagCoaccessible;
			break;
		    }
	    }
	    do {
		w = unfinished_.pop();
		flags_[w] &= ~FlagUnfinished;
		flags_[w] |= coaccessible;
		flags_[this->predecessor(w)] |= flags_[w] & FlagCoaccessible;
	    } while (s != w);
	    roots_.pop();
	    ++scc_;
	}
    }

    template<class _Automaton>
    void CoaccessibleDfsState<_Automaton>::exploreNonTreeArc(_ConstStateRef from, const _Arc &a) {
	    flags_[from->id()] |= flags_[a.target()] & FlagCoaccessible;
	    if (flags_[a.target()] & FlagUnfinished)
		while ((flags_[roots_.top()] & ~FlagAll) > (flags_[a.target()] & ~FlagAll))
		    roots_.pop();
    }

} // namespace Ftl

#endif // _T_FSA_ACCESSIBLE_CC

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
#ifndef _T_FSA_ACCESSIBLE_HH
#define _T_FSA_ACCESSIBLE_HH

#include "tAutomaton.hh"
#include "tDfs.hh"

namespace Ftl {

    /*
     * single-pass SCC algorithm from "graphs and their representations in a computer",
     * chapter 4, Mehlhorn, 1999 modifed to fit into DFS algorithm from Cormen et al.,
     * see also Dfs.cc
     **/

    template<class _Automaton>
    class CoaccessibleDfsState : public DfsState<_Automaton> {
	typedef DfsState<_Automaton> Precursor;
    public:
	typedef typename _Automaton::Arc _Arc;
	typedef typename _Automaton::ConstStateRef _ConstStateRef;
	typedef typename _Automaton::ConstRef _ConstAutomatonRef;
    public:
//	static const Fsa::StateId FlagAccessible = 0x80000000;
	static const Fsa::StateId FlagCoaccessible = 0x80000000;
	static const Fsa::StateId FlagUnfinished = 0x40000000;
	static const Fsa::StateId FlagAll = 0xc0000000;
    private:
	Fsa::StateId time_, scc_;
	Core::Vector<Fsa::StateId> flags_;
	Fsa::Stack<Fsa::StateId> roots_;
	Fsa::Stack<Fsa::StateId> unfinished_;
    public:
	CoaccessibleDfsState(_ConstAutomatonRef);
	virtual ~CoaccessibleDfsState() {}

	const Core::Vector<Fsa::StateId>& flags() const { return flags_; }
	virtual void discoverState(_ConstStateRef);
	virtual void finishState(Fsa::StateId);
	virtual void exploreNonTreeArc(_ConstStateRef from, const _Arc&);
    };

} // namespace Ftl

#endif // _T_FSA_ACCESSIBLE_HH

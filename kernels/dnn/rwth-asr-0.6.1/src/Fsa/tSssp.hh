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
#ifndef _T_FSA_SSSP_HH
#define _T_FSA_SSSP_HH

#include "hSssp.hh"
#include "Types.hh"

namespace Ftl {
    template<class _Automaton>
    StatePotentials<typename _Automaton::Weight> sssp(typename _Automaton::ConstRef f, Fsa::StateId start,
			 const SsspArcFilter<_Automaton> &arcFilter = SsspArcFilter<_Automaton>(), bool progress = false);

    /**
     * see Weighted Automata Algorithms, Mehryar Mohri, Handbook of weighted automata, 2009, page 10 (http://www.cs.nyu.edu/~mohri/postscript/hwa.pdf)
     */
    template<class _Automaton>
    StatePotentials<typename _Automaton::Weight> sssp(typename _Automaton::ConstRef f, bool progress = false);
    template<class _Automaton>
    StatePotentials<typename _Automaton::Weight> ssspBackward(typename _Automaton::ConstRef f,
		const SsspArcFilter<_Automaton> &arcFilter = SsspArcFilter<_Automaton>(), bool progress = false);
    template<class _Automaton>
    StatePotentials<typename _Automaton::Weight> ssspBackward(typename _Automaton::ConstRef f, bool progress = false);




    template<class _Automaton>
    typename _Automaton::ConstRef pushToInitial(typename _Automaton::ConstRef f, bool progress = false);
    template<class _Automaton>
    typename _Automaton::ConstRef pushToFinal(typename _Automaton::ConstRef f, bool progress = false);
    template<class _Automaton>
    typename _Automaton::ConstRef posterior(typename _Automaton::ConstRef f);
    template<class _Automaton>
    typename _Automaton::ConstRef posterior(typename _Automaton::ConstRef f, const StatePotentials<typename _Automaton::Weight> &forward);
    template<class _Automaton>
    typename _Automaton::ConstRef posterior(typename _Automaton::ConstRef f, typename _Automaton::Weight &totalInv);

    /**
     * Calculates the number of paths through the given automaton starting
     * from the initial state to one of the final states.
     * Complexity: see sssp()
     * @param f the automaton
     * @return returns the number of paths or Core::Type<size_t>::max(=2147483647 for s32)
     *    if the number was larger than that
     **/
    template<class _Automaton>
    size_t countPaths(typename _Automaton::ConstRef f);

} // namespace Ftl

#include "tSssp.cc"

#endif // _T_FSA_SSSP_HH

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
#ifndef _T_FSA_SORT_HH
#define _T_FSA_SORT_HH

#include "Types.hh"
#include "hSort.hh"

namespace Ftl {
    /**
     * Sorts arcs leaving a state.
     * Category: on-demand
     * Complexity: average case: O(V + E' log E'), worst-case: O(V + E log E)
     * @param f automaton
     * @param type what to sort
     * @return returns an abstract automaton with arcs begin sorted
     */
    template<class _Automaton>
    typename _Automaton::ConstRef sort(typename _Automaton::ConstRef f, Fsa::SortType type);

    /**
     * Topologically sorts states of the given automaton. As this may take
     * some time this function displays a progress indicator on demand.
     * Complexity: O(V + E)
     * @param f an arbitrary automaton
     * @param progress if true displays a progress indicator on current tty
     * @return returns a structure that maps automaton state ids to topologically
     *    sorted times
     */
    template<class _Automaton>
    Fsa::StateMap topologicallySort(typename _Automaton::ConstRef f, bool progress = false);

    /**
     * Test if automaton is acyclic using DFS.
     * @return true is automatic contains no cycles (except trivial
     * ones).  i.e. automaton is topologically sortable.
     **/
    template<class _Automaton>
    bool isAcyclic(typename _Automaton::ConstRef);

} // namespace Ftl

#include "tSort.cc"

#endif // _T_FSA_SORT_HH

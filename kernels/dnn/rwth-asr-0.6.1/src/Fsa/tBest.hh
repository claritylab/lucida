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
#ifndef _T_FSA_BEST_HH
#define _T_FSA_BEST_HH

/**
 * @file Best.hh
 * Contains functions dealing with best path problems
 **/

#include "hSssp.hh"

namespace Ftl {

    /**
     * Calculates the score of the best path through a weighted automaton.
     * Currently, this is only well defined in the tropical semiring.
     * Complexity: O(sssp(f) + )
     * @param f the automaton
     * @return total score of the best path
     **/
    template<class _Automaton>
    typename _Automaton::Weight bestscore(typename _Automaton::ConstRef f);

    /**
     * Calculates the best path through a weighted automaton.
     * Category: on-demand with precalculations
     * Complexity: worst-case: O(sssp(f) + V + E), best-case: O(sssp(f) + n * E')
     *    where n is the length if the best path
     * @param f the automaton
     * @return the best path represented as automaton
     **/
    template<class _Automaton>
    typename _Automaton::ConstRef best(typename _Automaton::ConstRef f);

    /**
     * Calculates the best path through a weighted automaton using precalculated
     * backward scores. This simply skips the additional precalculation
     * step.
     * Category: on-demand
     * @param f the automaton
     * @param backward backward state potentials
     * @return the best path represented as automaton
     **/
    template<class _Automaton>
    typename _Automaton::ConstRef best
    (typename _Automaton::ConstRef f, const StatePotentials<typename _Automaton::Weight> &backward);

    /**
     * Calculates the n best paths through a weighted automaton.
     * The result is an acyclic automaton where all state except the
     * initial one have only a single successor:
     * 0:       initial state
     * 1 .. n:  start states for n-best paths
     * So you can extract the i-th best path using partial(i).
     * Category: on-demand with precalculations
     * Complexity: O(sssp(determinize(remove-epsilons(f)))
     * @param f the original automaton
     * @param n is an upper limit on the number of paths to generate.
     * The actual number of paths in the result automaton may be
     * smaller (if the original automaton does not contain that many
     * paths.)  The actual number of n-bests can be determined by
     * counting the number of arcs leaving the initial state.
     * @param bestSequences set to false if you want to calculate the n-best
     * paths instead of the n-best unique sequences, true by default
     * @return an automaton representing the n-best sequences
     **/
    template<class _Automaton>
    typename _Automaton::ConstRef nbest(typename _Automaton::ConstRef f, size_t n = 1, bool bestSequences = true);

    /**
     * Compute best path through an automaton using Dijkstra's
     * algorithm (best first search).
     * This routine is functionally equivalent to best(), but uses a
     * different search alogrithm.  In particular firstBest() is not exhaustive.
     * @param f the automaton
     * @return the subset of @f containing only the best path
     */
    template<class _Automaton>
    typename _Automaton::ConstRef firstbest(typename _Automaton::ConstRef f);

} // namespace Ftl

#include "tBest.cc"

#endif // _T_FSA_BEST_HH

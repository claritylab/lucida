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
#ifndef _T_FSA_INFO_HH
#define _T_FSA_INFO_HH

#include <Core/XmlStream.hh>

#include "hInfo.hh"
#include "Types.hh"

namespace Ftl {
    /**
     * Calculates some interesting statistics of the size of an automaton.
     * This function uses dfs().
     * Complexity: O(V + E)
     * @param f the automaton
     * @param progress set to true if function should show a progress indicator
     *    on current tty
     * @return returns a data structure containing the counts
     **/
    template<class _Automaton>
    Fsa::AutomatonCounts count(typename _Automaton::ConstRef f, bool progress = false);

    /**
     * Test for emptyness.
     * @return true if the automaton represents the empty language (or
     * relation).
     */
    template<class _Automaton>
    bool isEmpty(typename _Automaton::ConstRef f);

    /**
     * Calculates the in-degree for each state, i.e. the number of incoming
     * arcs to a state. This function uses dfs().
     * Complexity: O(V + E)
     * @param f the automaton
     * @param progress set to true if function should show a progress indicator
     *    on current tty
     * @return returns a vector that contains the in-degree for each state
     **/
    template<class _Automaton>
    Core::Vector<u32> inDegree(typename _Automaton::ConstRef f, bool progress = false);

    /**
     * Calculates some number of linear states, i.e. the number of states with
     * an in-degree of 1. This function uses dfs().
     * Complexity: O(V + E)
     * @param f the automaton
     * @param progress set to true if function should show a progress indicator
     *    on current tty
     * @return returns the number of linear states
     **/
   template<class _Automaton>
   size_t countLinearStates(typename _Automaton::ConstRef f, bool progress = false);


    /**
     * Calculates some number of occurances of the given input label in an
     * automaton. This function uses dfs().
     * Complexity: O(V + E)
     * @param f the automaton
     * @param label the input label
     * @param progress set to true if function should show a progress indicator
     *    on current tty
     * @return returns the number of occurances of label as an input label
     **/
    template<class _Automaton>
    size_t countInput(typename _Automaton::ConstRef f, Fsa::LabelId label, bool progress = false);

    /**
     * Calculates some number of occurances of the given output label in an
     * automaton. This function uses dfs().
     * Complexity: O(V + E)
     * @param f the automaton
     * @param label the output label
     * @param progress set to true if function should show a progress indicator
     *    on current tty
     * @return returns the number of occurances of label as an output label
     **/
    template<class _Automaton>
    size_t countOutput(typename _Automaton::ConstRef f, Fsa::LabelId label, bool progress = false);

    /**
     * Report information about an FSA automaton.
     * @warning info() exhaustively counts all states and arcs, which
     * may take a considerable amount of time. Be careful when
     * applying to large automata.
     * Complexity: O(I + O + V + E)
     **/
    template<class _Automaton>
    void info(typename _Automaton::ConstRef f, Core::XmlWriter &o, bool progress = false);

    /**
     * Report some information about an FSA automaton.
     * Like info(), but restricted to information that can be obtained
     * in O(1), i.e. omits state and arc statistics.
     * Complexity: O(1)
     **/
    template<class _Automaton>
    void cheapInfo(typename _Automaton::ConstRef f, Core::XmlWriter &o);

    /**
     * Report some information about an FSA automaton.
     * Like info(), but restricted to information that can be obtained
     * in O(1), i.e. omits state and arc statistics.
     * Complexity: as the full lazy cascade of automata may be traversed
     *    there is no upper bound for this function. Look at specific
     *    implementations for a more detailled analysis
     **/
    template<class _Automaton>
    void memoryInfo(typename _Automaton::ConstRef f, Core::XmlWriter &o);
} // namespace Ftl


#include "tInfo.cc"

#endif // _T_FSA_INFO_HH

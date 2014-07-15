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
#ifndef _T_FSA_ARITHMETIC_HH
#define _T_FSA_ARITHMETIC_HH

namespace Ftl {

    /**
     * Collect each arc weight with the specified weight.
     * Category: on-demand
     * Complexity: O(V + E)
     * @param f the input automaton
     * @param value the weight
     * @return returns an automaton with all arc weights being collected with value
     **/
    template<class _Automaton>
    typename _Automaton::ConstRef collect(typename _Automaton::ConstRef f, typename _Automaton::Weight value);

    /**
     * Extend each arc weight with the specified weight.
     * Category: on-demand
     * Complexity: O(V + E)
     * @param f the input automaton
     * @param value the weight
     * @return returns an automaton with all arc weights being extended by value
     **/
    template<class _Automaton>
    typename _Automaton::ConstRef extend(typename _Automaton::ConstRef f, typename _Automaton::Weight value);

    /**
     * Extend each final state weight with the specified weight.
     * Category: on-demand
     * Complexity: O(V + E)
     * @param f the input automaton
     * @param value the weight
     * @return returns an automaton with all final state weights being extended by value
     **/
    template<class _Automaton>
    typename _Automaton::ConstRef extendFinal(typename _Automaton::ConstRef f, typename _Automaton::Weight value);

    /**
     * Modify each weight using the given functor of type Modifier.
     *
     * concept Modifier {
     *     Weight operator() (const Weight &) const;
     *     std::string describe() const;
     * }
     *
     * Category: on-demand
     * Complexity: O(V + E)
     * @param f the input automaton
     * @param value the weight
     * @return returns an automaton with all arc weights being multiplied by value
     **/
    template<class _Automaton, class Modifier>
    typename _Automaton::ConstRef modify(typename _Automaton::ConstRef f, const Modifier &modifier = Modifier());

} // namespace Ftl

#include "tArithmetic.cc"

#endif // _T_FSA_ARITHMETIC_H

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
#ifndef _FSA_ARITHMETIC_HH
#define _FSA_ARITHMETIC_HH

#include "Automaton.hh"

namespace Fsa {
    ConstAutomatonRef collect(ConstAutomatonRef f, Weight value);
    ConstAutomatonRef extend(ConstAutomatonRef f, Weight value);

    /**
     * Multiply each arc weight with the specified weight. Currently, this is only
     * possibly in real-valued semirings like the log or tropical semiring.
     * Category: on-demand
     * Complexity: O(V + E)
     * @param f the input automaton
     * @param value the weight
     * @return returns an automaton with all arc weights being multiplied by value
     **/
    ConstAutomatonRef multiply(ConstAutomatonRef f, Weight value);
    ConstAutomatonRef expm(ConstAutomatonRef f);
    ConstAutomatonRef logm(ConstAutomatonRef f);

    ConstAutomatonRef extend(ConstAutomatonRef f1, ConstAutomatonRef f2);
    ConstAutomatonRef extendFinal(ConstAutomatonRef f, Weight value);

    ConstAutomatonRef isGreaterEqual(ConstAutomatonRef f, Weight threshold);

} // namespace Fsa

#endif // _FSA_ARITHMETIC_H

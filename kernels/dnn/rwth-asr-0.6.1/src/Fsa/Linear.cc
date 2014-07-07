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
// $Id: Linear.cc 4892 2005-07-19 18:17:40Z hoffmeister $

#include "tLinear.hh"
#include "Linear.hh"
#include "Types.hh"

namespace Fsa {
    bool isLinear(ConstAutomatonRef la)
    { return Ftl::isLinear<Automaton>(la); }

    void getLinearInput(ConstAutomatonRef la, std::vector<LabelId> &result)
    { return Ftl::getLinearInput<Automaton>(la, result); }

    void getLinearOutput(ConstAutomatonRef la, std::vector<LabelId> &result)
    { return Ftl::getLinearOutput<Automaton>(la, result); }

    Weight getLinearWeight(ConstAutomatonRef la)
    { return Ftl::getLinearWeight<Automaton>(la); }
} // namespace Fsa

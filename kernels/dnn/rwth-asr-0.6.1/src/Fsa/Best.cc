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
#include "tBest.hh"
#include "Best.hh"

namespace Fsa {
    Weight bestscore(ConstAutomatonRef f)
    { return Ftl::bestscore<Automaton>(f); }

    ConstAutomatonRef best(ConstAutomatonRef f)
    { return Ftl::best<Automaton>(f); }

    ConstAutomatonRef best(ConstAutomatonRef f, const StatePotentials &backward)
    { return Ftl::best<Automaton>(f, backward); }

    ConstAutomatonRef nbest(ConstAutomatonRef f, size_t n, bool bestSequences)
    { return Ftl::nbest<Automaton>(f, n, bestSequences); }

    ConstAutomatonRef firstbest(ConstAutomatonRef f)
    { return Ftl::firstbest<Automaton>(f); }
} // namespace Fsa

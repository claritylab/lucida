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
#include "tSort.hh"
#include "Sort.hh"

namespace Fsa {
    ConstAutomatonRef sort(ConstAutomatonRef f, SortType type)
    { return Ftl::sort<Automaton>(f, type); }

    StateMap topologicallySort(ConstAutomatonRef f, bool progress)
    { return Ftl::topologicallySort<Automaton>(f, progress); }

    bool isAcyclic(ConstAutomatonRef f)
    { return Ftl::isAcyclic<Automaton>(f); }
} // namespace Fsa

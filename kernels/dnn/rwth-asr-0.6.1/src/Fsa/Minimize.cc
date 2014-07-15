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
#include "tMinimize.hh"
#include "Minimize.hh"

namespace Fsa {

    ConstAutomatonRef minimize(ConstAutomatonRef f, bool progress)
    { return Ftl::minimize<Automaton>(f, progress); }

    ConstAutomatonRef minimize(ConstAutomatonRef f, Fsa::OptimizationHint hint, bool progress)
    { return Ftl::minimize<Automaton>(f, hint, progress); }

    ConstAutomatonRef minimizeSimple(ConstAutomatonRef f)
    { return Ftl::minimizeSimple<Automaton>(f); }
} // namespace Fsa

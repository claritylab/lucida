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
#include "tPrune.hh"
#include "Prune.hh"

namespace Fsa {
    ConstAutomatonRef prunePosterior(ConstAutomatonRef f, const Weight &threshold, bool relative)
    { return Ftl::prunePosterior<Automaton>(f, threshold, relative); }

    ConstAutomatonRef prunePosterior(ConstAutomatonRef f, const Weight &threshold, const StatePotentials &fw, const StatePotentials& bw, bool relative)
    { return Ftl::prunePosterior<Automaton>(f, threshold, fw, bw, relative); }

    ConstAutomatonRef pruneSync(ConstAutomatonRef f, const Weight &threshold)
    { return Ftl::pruneSync<Automaton>(f, threshold); }
} // namespace Fsa

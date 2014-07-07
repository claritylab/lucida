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
#include "tCompose.hh"
#include "Compose.hh"

namespace Fsa {
    ConstAutomatonRef composeMatching(ConstAutomatonRef fl, ConstAutomatonRef fr, bool reportUnknowns)
    { return Ftl::composeMatching<Automaton>(fl, fr, reportUnknowns); }

    ConstAutomatonRef composeSequencing(ConstAutomatonRef fl, ConstAutomatonRef fr, bool reportUnknowns)
    { return Ftl::composeSequencing<Automaton>(fl, fr, reportUnknowns); }

    ConstAutomatonRef difference(ConstAutomatonRef fl, ConstAutomatonRef fr)
    { return Ftl::difference<Automaton>(fl, fr); }

    ConstMappingRef mapToLeft(ConstAutomatonRef f)
    { return Ftl::mapToLeft<Automaton>(f); }

    ConstMappingRef mapToRight(ConstAutomatonRef f)
    { return Ftl::mapToRight<Automaton>(f); }
} // namespace Fsa

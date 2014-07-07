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
#ifndef _FSA_COMPOSE_HH
#define _FSA_COMPOSE_HH

#include "Automaton.hh"
#include "Mapping.hh"

namespace Fsa {
    ConstAutomatonRef composeMatching(ConstAutomatonRef fl, ConstAutomatonRef fr, bool reportUnknowns = true);
    ConstAutomatonRef composeSequencing(ConstAutomatonRef fl, ConstAutomatonRef fr, bool reportUnknowns = true);
    ConstAutomatonRef difference(ConstAutomatonRef fl, ConstAutomatonRef fr);
    ConstMappingRef mapToLeft(ConstAutomatonRef f);
    ConstMappingRef mapToRight(ConstAutomatonRef f);
} // namespace Fsa

#endif // _FSA_COMPOSE_HH

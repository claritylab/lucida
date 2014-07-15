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
#ifndef _FSA_STATIC_HH
#define _FSA_STATIC_HH

#include "Automaton.hh"
// inclusion of Storage.hh is crucial here, because it declares
// a specialized constructor for Fsa::StorageAutomaton, which is
// the (not obvious) base classes of Fsa::StaticAutomaton
#include "Storage.hh"
#include "Types.hh"
#include "tStatic.hh"

namespace Fsa {
    typedef Ftl::StaticAutomaton<Automaton> StaticAutomaton;

    void removeNonAccessibleStates(Core::Ref<StaticAutomaton> s);
    void trimInPlace(Core::Ref<StaticAutomaton>);
    void removeInvalidArcsInPlace(Core::Ref<StaticAutomaton>);

    Core::Ref<StaticAutomaton> staticCopy(ConstAutomatonRef);
    Core::Ref<StaticAutomaton> staticCompactCopy(ConstAutomatonRef);
    Core::Ref<StaticAutomaton> staticCopy(const std::string &, ConstSemiringRef semiring = UnknownSemiring);

} // namespace Fsa

#endif // _FSA_STATIC_HH

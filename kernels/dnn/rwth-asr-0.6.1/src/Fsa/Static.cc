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
#include "Static.hh"
#include "tStaticAlgorithm.hh"
#include "tCopy.hh"

namespace Fsa {
    void removeNonAccessibleStates(Core::Ref<StaticAutomaton> s)
    { Ftl::removeNonAccessibleStates<Automaton>(s); }

    void trimInPlace(Core::Ref<StaticAutomaton> s) {
	Ftl::trimInPlace<Automaton>(s);
    }

    void removeInvalidArcsInPlace(Core::Ref<StaticAutomaton> s) {
	Ftl::removeInvalidArcsInPlace<Automaton>(s);
    }

    Core::Ref<StaticAutomaton> staticCopy(ConstAutomatonRef f)
    { return Ftl::staticCopy<Automaton>(f); }

    Core::Ref<StaticAutomaton> staticCompactCopy(ConstAutomatonRef f)
    { return Ftl::staticCompactCopy<Automaton>(f); }

    Core::Ref<StaticAutomaton> staticCopy(const std::string &str, ConstSemiringRef semiring)
    { return Ftl::staticCopy<Automaton>(str, semiring); }
} // namespace Fsa

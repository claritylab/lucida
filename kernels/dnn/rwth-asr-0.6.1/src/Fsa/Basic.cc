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
#include "tAlphabet.hh"
#include "tBasic.hh"
#include "tInfo.hh"
#include "Basic.hh"

namespace Fsa {
    ConstAutomatonRef mapInput(ConstAutomatonRef f, ConstAlphabetRef a, u32 reportUnknowns)
    { return Ftl::mapInput<Automaton>(f, a, reportUnknowns); }

    ConstAutomatonRef mapOutput(ConstAutomatonRef f, ConstAlphabetRef a, u32 reportUnknowns)
    { return Ftl::mapOutput<Automaton>(f, a, reportUnknowns); }

    ConstAutomatonRef mapInputOutput(ConstAutomatonRef f, ConstAlphabetRef a, u32 reportUnknowns)
    { return Ftl::mapInputOutput<Automaton>(f, a, reportUnknowns); }

    ConstAutomatonRef mapInput(ConstAutomatonRef f, const AlphabetMapping &m)
    { return Ftl::mapInput<Automaton>(f, m); }

    ConstAutomatonRef mapOutput(ConstAutomatonRef f, const AlphabetMapping &m)
    { return Ftl::mapOutput<Automaton>(f, m); }

    ConstAutomatonRef mapInputOutput(ConstAutomatonRef f, const AlphabetMapping &m)
    { return Ftl::mapInputOutput<Automaton>(f, m); }
} //namespace Fsa


namespace Fsa {
    ConstAutomatonRef normalize(ConstAutomatonRef f)
    { return Ftl::normalize<Automaton>(f); }

    ConstAutomatonRef trim(ConstAutomatonRef f, bool progress)
    { return Ftl::trim<Automaton>(f, progress); }

    ConstAutomatonRef partial(ConstAutomatonRef f, StateId initialStateOfPartialAutomaton)
    { return Ftl::partial<Automaton>(f, initialStateOfPartialAutomaton); }

    ConstAutomatonRef partial(ConstAutomatonRef f, StateId initialStateOfPartialAutomaton, Weight additionalFinalWeight)
    { return Ftl::partial<Automaton>(f, initialStateOfPartialAutomaton, additionalFinalWeight); }

    ConstAutomatonRef hypothesis(ConstAutomatonRef f, StateId n)
    { return Ftl::hypothesis<Automaton>(f, n); }

    ConstAutomatonRef changeSemiring(ConstAutomatonRef f, ConstSemiringRef semiring)
    { return Ftl::changeSemiring<Automaton>(f, semiring); }

    ConstMappingRef mapNormalized(ConstAutomatonRef f)
    { return Ftl::mapNormalized<Automaton>(f); }
} //namespace Fsa


namespace Fsa {
    AutomatonCounts count(ConstAutomatonRef f, bool progress)
    { return Ftl::count<Automaton>(f, progress); }

    bool isEmpty(ConstAutomatonRef f)
    { return Ftl::isEmpty<Automaton>(f); }

    Core::Vector<u32> inDegree(ConstAutomatonRef f, bool progress)
    { return Ftl::inDegree<Automaton>(f, progress); }

    size_t countLinearStates(ConstAutomatonRef f, bool progress)
    { return Ftl::countLinearStates<Automaton>(f, progress); }

    size_t countInput(ConstAutomatonRef f, LabelId label, bool progress)
    { return Ftl::countInput<Automaton>(f, label, progress); }

    size_t countOutput(ConstAutomatonRef f, LabelId label, bool progress)
    { return Ftl::countOutput<Automaton>(f, label, progress); }

    void info(ConstAutomatonRef f, Core::XmlWriter &o, bool progress)
    { return Ftl::info<Automaton>(f, o, progress); }

    void cheapInfo(ConstAutomatonRef f, Core::XmlWriter &o)
    { return Ftl::cheapInfo<Automaton>(f, o); }

    void memoryInfo(ConstAutomatonRef f, Core::XmlWriter &o)
    { return Ftl::memoryInfo<Automaton>(f, o); }
} // namespace Fsa

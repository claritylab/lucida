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
#ifndef _FSA_BASIC_HH
#define _FSA_BASIC_HH

#include <Core/XmlStream.hh>

#include "Alphabet.hh"
#include "AlphabetUtility.hh"
#include "Automaton.hh"
#include "Mapping.hh"

#include "hInfo.hh"

/*
 * tAlphabet
 */
namespace Fsa {
    ConstAutomatonRef mapInput(ConstAutomatonRef f, ConstAlphabetRef alphabet, u32 reportUnknowns = 10);
    ConstAutomatonRef mapOutput(ConstAutomatonRef f, ConstAlphabetRef alphabet, u32 reportUnknowns = 10);
    ConstAutomatonRef mapInputOutput(ConstAutomatonRef, ConstAlphabetRef, u32 reportUnknowns = 10);
    ConstAutomatonRef mapInput(ConstAutomatonRef f, const AlphabetMapping&);
    ConstAutomatonRef mapOutput(ConstAutomatonRef f, const AlphabetMapping&);
    ConstAutomatonRef mapInputOutput(ConstAutomatonRef, const AlphabetMapping&);
} // namespace Fsa

/*
 * tBasic
 */
namespace Fsa {
    ConstAutomatonRef normalize(ConstAutomatonRef f);
    ConstAutomatonRef trim(ConstAutomatonRef f, bool progress = false);
    ConstAutomatonRef partial(ConstAutomatonRef f, StateId initial);
    ConstAutomatonRef partial(ConstAutomatonRef f, StateId initial, Weight additionalFinalWeight);
    ConstAutomatonRef hypothesis(ConstAutomatonRef f, StateId n);
    ConstAutomatonRef changeSemiring(ConstAutomatonRef f, ConstSemiringRef semiring);
    ConstMappingRef mapNormalized(ConstAutomatonRef f);
} // namespace Fsa

/*
 * tInfo
 */
namespace Fsa {
    AutomatonCounts count(ConstAutomatonRef f, bool progress = false);
    bool isEmpty(ConstAutomatonRef f);
    Core::Vector<u32> inDegree(ConstAutomatonRef f, bool progress = false);
    size_t countLinearStates(ConstAutomatonRef f, bool progress = false);
    size_t countInput(ConstAutomatonRef f, LabelId label, bool progress = false);
    size_t countOutput(ConstAutomatonRef f, LabelId label, bool progress = false);
    void info(ConstAutomatonRef f, Core::XmlWriter &o, bool progress = false);
    void cheapInfo(ConstAutomatonRef f, Core::XmlWriter &o);
    void memoryInfo(ConstAutomatonRef f, Core::XmlWriter &o);
} // namespace Fsa

#endif // _FSA_BASIC_HH

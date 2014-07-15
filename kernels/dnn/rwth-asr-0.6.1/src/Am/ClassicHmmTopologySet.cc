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
#include "ClassicHmmTopologySet.hh"

using namespace Am;


const Core::ParameterInt ClassicHmmTopologySet::paramPhoneStates(
    "states-per-phone",
    "number of (acoustically different) states per phone HMM (fka HMM_NOFSEG)",
    3, 1, Core::Type<s8>::max);
const Core::ParameterInt ClassicHmmTopologySet::paramRepeatedStates(
    "state-repetitions",
    "number of times each HMM state is repeated (fka DURATION)",
    2, 1, Core::Type<s8>::max);
// currently limited to two, because of TransitionModel
const Core::ParameterBool ClassicHmmTopologySet::paramAcrossWordModel(
    "across-word-model",
    "enable modeling of co-articulation across word boundaries",
    false);

ClassicHmmTopologySet::ClassicHmmTopologySet(const Core::Configuration &c, Bliss::Phoneme::Id silenceId) :
    Component(c),
    silenceId_(silenceId),
    silence_(1, 1),
    default_(paramPhoneStates(c), paramRepeatedStates(c)),
    isAcrossWordModelEnabled_(false)
{
    isAcrossWordModelEnabled_ = paramAcrossWordModel(c);
}

void ClassicHmmTopologySet::getDependencies(Core::DependencySet &deps) const {
    deps.add(paramPhoneStates.name(), default_.nPhoneStates());
    deps.add(paramRepeatedStates.name(), default_.nSubStates());
    deps.add(paramAcrossWordModel.name(), isAcrossWordModelEnabled_);
}

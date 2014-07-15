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
#include "Module.hh"
#include <Modules.hh>
#include <Core/Application.hh>
#include "ClassicAcousticModel.hh"
#include <Legacy/DecisionTree.hh>
#ifdef MODULE_CART
#include "DecisionTreeStateTying.hh"
#endif
#ifdef MODULE_ADAPT_MLLR
#include "AdaptedAcousticModel.hh"
#endif

using namespace Am;

const Core::Choice Module_::choiceAmType(
    "classic", typeClassic,
    "adapted", typeAdapted,
    "fast-adapted", typeFastAdapted,
    "combined", typeCombined,
    Core::Choice::endMark());

const Core::ParameterChoice Module_::paramAmType(
    "type", &choiceAmType, "type of acoustic model", typeClassic);


Module_::Module_()
{
    registerStateTying<NoStateTying>(ClassicAcousticModel::noTying);
    registerStateTying<MonophoneStateTying>(ClassicAcousticModel::monophoneTying);
    registerStateTying<LutStateTying>(ClassicAcousticModel::lutTying);
    registerStateTying<Legacy::PhoneticDecisionTree>(ClassicAcousticModel::oldCartTying);
#ifdef MODULE_CART
    registerStateTying<DecisionTreeStateTying>(ClassicAcousticModel::cartTying);
#endif

}

Core::Ref<AcousticModel> Module_::createAcousticModel(
    const Core::Configuration &c, Bliss::LexiconRef l, AcousticModel::Mode mode)
{
    Core::Ref<AcousticModel> result;

    switch (paramAmType(c)) {
    case typeClassic:
	Core::Application::us()->log("Load classic acoustic model.");
	result = Core::ref(new ClassicAcousticModel(c, l));
	break;
#ifdef MODULE_ADAPT_MLLR
    case typeAdapted:
	Core::Application::us()->log("Load adapted acoustic model.");
	result = Core::ref(new AdaptedAcousticModel(c, l));
	break;
#endif
    default:
	defect();
    }
    result->load(mode);
    if (result->hasFatalErrors())
	result.reset();
    return result;
}

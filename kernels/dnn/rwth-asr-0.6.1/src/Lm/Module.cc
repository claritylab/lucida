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
#include <Modules.hh>
#include <Core/Application.hh>
#include "Module.hh"
#include "ClassLm.hh"
#ifdef MODULE_LM_ARPA
#include "ArpaLm.hh"
#endif
#ifdef MODULE_LM_FSA
#include "FsaLm.hh"
#endif
#ifdef MODULE_LM_ZEROGRAM
#include "Zerogram.hh"
#endif

using namespace Lm;

namespace Lm {
    enum LanguageModelType {
	lmTypeArpa,
	lmTypeArpaWithClasses,
	lmTypeDump,
	lmTypeEstar,
	lmTypeFsa,
	lmTypePhilips,
	lmTypeRwth,
	lmTypeSimple,
	lmTypeZerogram
    };
}

const Core::Choice Module_::lmTypeChoice(
    "ARPA",          lmTypeArpa,
    "ARPA+classes",  lmTypeArpaWithClasses,
    "Philips",       lmTypePhilips,
    "RWTH",          lmTypeRwth,
    "dump",          lmTypeDump,
    "estar",         lmTypeEstar,
    "fsa",           lmTypeFsa,
    "simple",        lmTypeSimple,
    "zerogram",      lmTypeZerogram,
    Core::Choice::endMark());

const Core::ParameterChoice Module_::lmTypeParam(
    "type", &Module_::lmTypeChoice, "type of language model", lmTypeZerogram);

Core::Ref<LanguageModel> Module_::createLanguageModel(
    const Core::Configuration &c,
    Bliss::LexiconRef l)
{
    Core::Ref<LanguageModel> result;

    switch (lmTypeParam(c)) {
#ifdef MODULE_LM_ARPA
    case lmTypeArpa:            result = Core::ref(new ArpaLm(c, l));      break;
    case lmTypeArpaWithClasses: result = Core::ref(new ArpaClassLm(c, l)); break;
#endif
#ifdef MODULE_LM_FSA
    case lmTypeFsa:             result = Core::ref(new FsaLm(c, l));       break;
#endif
#ifdef MODULE_LM_ZEROGRAM
    case lmTypeZerogram:        result = Core::ref(new Zerogram(c, l));    break;
#endif
    default:
	Core::Application::us()->criticalError("unknwon language model type: %d",  lmTypeParam(c));
    }
    result->init();
    if (result->hasFatalErrors())
	result.reset();
    return result;
}

Core::Ref<ScaledLanguageModel> Module_::createScaledLanguageModel(
    const Core::Configuration &c, Core::Ref<LanguageModel> languageModel)
{
    return languageModel ?
	Core::Ref<ScaledLanguageModel>(new LanguageModelScaling(c, languageModel)) :
	Core::Ref<ScaledLanguageModel>();
}

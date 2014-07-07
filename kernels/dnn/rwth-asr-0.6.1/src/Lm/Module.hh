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
#ifndef _LM_MODULE_HH
#define _LM_MODULE_HH

#include <Core/Singleton.hh>
#include "LanguageModel.hh"
#include "ScaledLanguageModel.hh"

namespace Lm {

    class Module_
    {
    private:
	static const Core::Choice lmTypeChoice;
	static const Core::ParameterChoice lmTypeParam;

    public:
	Module_() {}

	/**
	 * Creates and initializes a LanguageModel as configured.
	 * @return a newly created instance of LanguageModel or a void
	 * reference if an error occured.
	 */
	Core::Ref<LanguageModel> createLanguageModel(
	    const Core::Configuration&, Bliss::LexiconRef);

	/**
	 * Creates a scaled language model from @param languageModel.
	 * @return a valid reference to the newly created instance of
	 * ScaledLanguageModel or an invalid reference if an error
	 * occured.
	 */
	Core::Ref<ScaledLanguageModel> createScaledLanguageModel(
	    const Core::Configuration &, Core::Ref<LanguageModel>);

	/**
	 * Creates a scaled language model as configured.
	 * @return a valid reference to the newly created instance of
	 * ScaledLanguageModel or an invalid reference if an error
	 * occured.
	 */
	Core::Ref<ScaledLanguageModel> createScaledLanguageModel(
	    const Core::Configuration &c, Bliss::LexiconRef l) {
	    return createScaledLanguageModel(c, createLanguageModel(c, l));
	}

    };

    typedef Core::SingletonHolder<Module_> Module;
}

#endif // _LM_MODULE_HH

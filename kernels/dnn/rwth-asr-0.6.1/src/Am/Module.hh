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
#ifndef _AM_MODULE_HH
#define _AM_MODULE_HH

#include <Core/Factory.hh>
#include <Core/Singleton.hh>
#include "AcousticModel.hh"
#include "ClassicAcousticModel.hh"

namespace Am {

    class Module_
    {
    private:
	enum Type {
	    typeClassic,
	    typeAdapted,
	    typeFastAdapted,
	    typeCombined
	};

	class ClassicStateTyingFactory :
	    public Core::Factory<ClassicStateTying,
				 ClassicStateTying* (*)(const Core::Configuration&, ClassicStateModelRef),
				 ClassicAcousticModel::StateTyingType>
	{
	public:
	    typedef ClassicStateTying* (*CreationFunction)(const Core::Configuration&, ClassicStateModelRef);
	    typedef ClassicAcousticModel::StateTyingType Identifier;
	public:
	    ClassicStateTying* getObject(const Identifier &id, const Core::Configuration &c, ClassicStateModelRef m) {
		CreationFunction create = getCreationFunction(id);
		if(create)
		    return create(c, m);
		else
		    return 0;
	    }
	    template<class T>
	    static ClassicStateTying* create(const Core::Configuration &c, ClassicStateModelRef m) {
		return new T(c, m);
	    }
	};

	ClassicStateTyingFactory stateTyingFactory_;
	static const Core::Choice choiceAmType;
	static const Core::ParameterChoice paramAmType;
    public:
	Module_();

	/**
	 * Creates and initializes a AcousticModel as configured.
	 * @return a newly created instance of AcousticModel or 0 if
	 * an error occured.
	 */
	Core::Ref<AcousticModel> createAcousticModel(
	    const Core::Configuration&, Bliss::LexiconRef,
	    AcousticModel::Mode = AcousticModel::complete);

	template<class T>
	void registerStateTying(ClassicAcousticModel::StateTyingType id) {
	    stateTyingFactory_.registerClass(id, ClassicStateTyingFactory::create<T>);
	}

	ClassicStateTying* getStateTying(
	    ClassicAcousticModel::StateTyingType id,
	    const Core::Configuration &c, ClassicStateModelRef m) {
	    return stateTyingFactory_.getObject(id, c, m);
	}

    };

    typedef Core::SingletonHolder<Module_> Module;

}

#endif // _AM_MODULE_HH

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
#ifndef _FEATURE_SCORER_FACTORY_HH
#define _FEATURE_SCORER_FACTORY_HH

#include <Core/Choice.hh>
#include <Mm/Module.hh>
#include <Mm/MixtureSetLoader.hh>

namespace Mm
{

    /**
     * Factory for FeatureScorer objects
     */
    class FeatureScorerFactory
    {
    private:
	typedef Mm::Module::Instance::FeatureScorerType FeatureScorerType;
	typedef FeatureScorer* (*CreationFunction)(
	    const Core::Configuration &, Core::Ref<const AbstractMixtureSet>);
	typedef std::map<u32, CreationFunction> Registry;
	Registry registry_;
	typedef std::map<u32, AbstractMixtureSetLoader*> LoaderRegistry;
	LoaderRegistry loader_;
	Core::Choice names_;

    public:
	~FeatureScorerFactory() {
	    for (LoaderRegistry::iterator l = loader_.begin(); l != loader_.end(); ++l)
		delete l->second;
	}

	/**
	 * Register a FeatureScorer class.
	 * T: FeatureScorer class.
	 * ModelType: MixtureSet class required by the feature scorer.
	 * Loader: MixtureSetLoader class used to read the model.
	 * id: unique feature scorer id
	 * name: name associated with the feature scorer type
	 *
	 * a Core::Choice is maintained for the id to name mapping.
	 */
	template<class T, class ModelType, class Loader>
	bool registerFeatureScorer(u32 id, const char *name) {
	    if (registry_.find(id) == registry_.end()) {
		CreationFunction c = createInstance<T, ModelType>;
		registry_.insert(Registry::value_type(id, c));
		loader_.insert(LoaderRegistry::value_type(id, new Loader()));
		names_.addChoice(name, id);
		return true;
	    } else {
		return false;
	    }
	}

	/**
	 * Create a FeatureScorer object.
	 * The actual class is determined from the id.
	 */
	FeatureScorer* createFeatureScorer(u32 id,
					   const Core::Configuration &c,
					   Core::Ref<const AbstractMixtureSet> m) const
	{
	    CreationFunction create = getCreationFunction(id);
	    if (create) {
		return create(c, m);
	    } else {
		return 0;
	    }
	}

	/**
	 * Return a MixtureSetLoader to read the model (AbstractMixtureSet or derivative)
	 * for the given feature scorer id.
	 */
	const AbstractMixtureSetLoader* getMixtureSetLoader(u32 id) const {
	    LoaderRegistry::const_iterator i = loader_.find(id);
	    if (i != loader_.end())
		return i->second;
	    else
		return 0;
	}

	/**
	 * Return the Core::Choice for the feature scorer id to name mapping.
	 */
	const Core::Choice& featureScorerNames() const { return names_; }
    protected:
	CreationFunction getCreationFunction(u32 id) const {
	    Registry::const_iterator item = registry_.find(id);
	    if (item != registry_.end()) {
		return item->second;
	    } else {
		return 0;
	    }
	}

	template<class T, class ModelType>
	static FeatureScorer* createInstance(const Core::Configuration &c, Core::Ref<const AbstractMixtureSet> m) {
	    verify(m);
	    // Core::Ref::Ref(T *ptr) doesn't check if ptr is valid.
	    const ModelType *castedModel = dynamic_cast<const ModelType*>(m.get());
	    ensure(castedModel);
	    const Core::Ref<const ModelType> castedModelRef(castedModel);
	    return new T(c, castedModelRef);
	}

    };
} // namespace Mm

#endif // _FEATURE_SCORER_FACTORY_HH

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
#ifndef _CORE_FACTORY_HH
#define _CORE_FACTORY_HH

#include <map>
#include <Core/Configuration.hh>
#include <Core/Extensions.hh>

namespace Core
{

    /**
     * Object factory
     * based on Alexandrescu, Andrei "Modern C++ Design"
     */
    template<class BaseClass, typename CreationFunction, typename Identifier>
    class Factory
    {
    protected:
	typedef std::map<Identifier, CreationFunction> Registry;
    public:
	Factory() {}

	/**
	 * register a class
	 * @param id identifier for this class
	 * @param c  function that creates a new instance of this class
	 * @return class registered
	 */
	bool registerClass(const Identifier &id, CreationFunction c) {
	    if(registry_.find(id) == registry_.end()) {
		registry_.insert(typename Registry::value_type(id, c));
		return true;
	    }
	    else
		return false;
	}

	/**
	 * Create an instance of the class which is identified by @c id
	 * If no such classes exists 0 is returned !
	 */
	BaseClass* getObject(const Identifier &id) const {
	    CreationFunction create = getCreationFunction(id);
	    if(create)
		return create();
	    else
		return 0;
	}

	std::vector<Identifier> identifiers() const {
	    typedef Core::select1st< typename Registry::value_type > SelectKey;
	    std::vector<Identifier> ids(registry_.size());
	    std::transform(registry_.begin(), registry_.end(), ids.begin(), SelectKey());
	    return ids;
	}

    protected:
	CreationFunction getCreationFunction(const Identifier &id) const {
	    typename Registry::const_iterator item = registry_.find(id);
	    if(item != registry_.end())
		return item->second;
	    else
		return 0;
	}

	Registry registry_;
    };


    /**
     * Object factory for subclasses of Core::Component
     */
    template<class BaseClass, typename Identifier>
    class ComponentFactory :
	public Factory<BaseClass, BaseClass* (*)(const Core::Configuration&), Identifier>
    {
	typedef Factory<BaseClass, BaseClass* (*)(const Core::Configuration&), Identifier> Precursor;
	typedef BaseClass* (*CreationFunction)(const Core::Configuration&);
      public:
	BaseClass* getObject(const Identifier &id, const Core::Configuration &c) const {
	    CreationFunction create = Precursor::getCreationFunction(id);
	    if(create)
		return create(c);
	    else
		return 0;
	}
    };

}

#endif // _CORE_FACTORY_HH

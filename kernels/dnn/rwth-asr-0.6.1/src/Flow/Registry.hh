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
#ifndef _FLOW_REGISTRY_HH
#define _FLOW_REGISTRY_HH

#include <iostream>
#include <map>
#include <string>
#include <Core/Singleton.hh>
#include "Filter.hh"

namespace Flow
{

class Datatype;

/**
 * central registry for Flow filters and datatypes.
 * never use this class directly, use Flow::Registry.
 *
 * Registry can be accessed using Flow::Registry::instance()
 */
class Registry_
{
private:
    typedef std::map<std::string, _Filter*> FilterMap;
    typedef std::map<std::string, const Datatype*> DatatypeMap;
    FilterMap filters_;
    DatatypeMap datatypes_;

    void registerFilter_(_Filter *f);
    void registerDatatype_(const Datatype *d);

public:
    // filter registry
    template<class T> void registerFilter()
    {
	registerFilter_(new Filter<T> (T::filterName()));
    }
    const _Filter* getFilter(const std::string &name) const;
    void dumpFilters(std::ostream &o) const;

    // datatype registry
    template<class T> void registerDatatype()
    {
	registerDatatype_(T::type());
    }
    const Datatype* getDatatype(const std::string &name) const;
    void dumpDatatypes(std::ostream &o) const;
};

typedef Core::SingletonHolder<Registry_> Registry;

}
#endif /* _FLOW_REGISTRY_HH */

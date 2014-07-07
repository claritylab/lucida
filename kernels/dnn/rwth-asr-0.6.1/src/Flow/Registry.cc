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
#include <algorithm>
#include <iterator>
#include <iostream>
#include <Core/Application.hh>
#include <Core/Extensions.hh>
#include "Registry.hh"
#include "Datatype.hh"

using namespace Flow;

/*****************************************************************************/
void Registry_::dumpFilters(std::ostream &o) const
/*****************************************************************************/
{
    transform(filters_.begin(), filters_.end(), std::ostream_iterator<Flow::_Filter*>(o, "\n"),
	      Core::select2nd<FilterMap::value_type>());
}

/*****************************************************************************/
void Registry_::registerFilter_(_Filter *f)
/*****************************************************************************/
{
    if (filters_.find(f->getName()) == filters_.end())
	filters_[f->getName()] = f;
    else {
	Core::Application::us()->criticalError("filter '%s' already registered", f->getName().c_str());
	delete f;
    }
}

/*****************************************************************************/
const Flow::_Filter* Registry_::getFilter(const std::string &name) const
/*****************************************************************************/
{
    FilterMap::const_iterator found = filters_.find(name);
    if (found == filters_.end()) return 0;
    return (*found).second;
}

/*****************************************************************************/
void Registry_::registerDatatype_(const Datatype *d)
/*****************************************************************************/
{
    require(d != 0 && !d->name().empty());

    if (datatypes_.find(d->name()) != datatypes_.end())
	Core::Application::us()->criticalError("datatype '%s' already registered", d->name().c_str());
    datatypes_[d->name()] = d;
}

/*****************************************************************************/
void Registry_::dumpDatatypes(std::ostream &o) const
/*****************************************************************************/
{
    transform(datatypes_.begin(), datatypes_.end(), std::ostream_iterator<const Flow::Datatype*>(o, "\n"),
	      Core::select2nd<DatatypeMap::value_type>());
}

/*****************************************************************************/
const Flow::Datatype* Registry_::getDatatype(const std::string &name) const
/*****************************************************************************/
{
    DatatypeMap::const_iterator found = datatypes_.find(name);
    if (found == datatypes_.end()) return 0;
    return (*found).second;
}

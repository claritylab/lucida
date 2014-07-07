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
#include "Data.hh"
#include "Datatype.hh"


using namespace Flow;

/*****************************************************************************/
const Datatype* Data::type()
/*****************************************************************************/
{
    static DatatypeTemplate<Self> dt("sentinel");
    return &dt;
}

/*****************************************************************************/
void Data::free() const
/*****************************************************************************/
{
    verify_(isNotSentinel(this));
    delete this;
}

/*****************************************************************************/
Core::XmlOpen Data::xmlOpen() const
/*****************************************************************************/
{
    return Core::XmlOpen(datatype()->name());
}

/*****************************************************************************/
Core::XmlClose Data::xmlClose() const
/*****************************************************************************/
{
    return Core::XmlClose(datatype()->name());
}

/*****************************************************************************/
Core::XmlWriter& Data::dump(Core::XmlWriter &o) const
/*****************************************************************************/
{
    if (this == sentinel()) o << Core::XmlEmpty("null");
    else if (this == eos()) o << Core::XmlEmpty("eos");
    else if (this == ood()) o << Core::XmlEmpty("ood");
    else defect();
    return o;
}

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
#include "Aggregate.hh"
#include "Registry.hh"

using namespace Flow;

//==========================================================================================
const Datatype *Aggregate::type()
{
    static Core::NameHelper<Self> name;
    static DatatypeTemplate<Self> dt(name);
    return &dt;
}

bool Aggregate::read(Core::BinaryInputStream &is)
{
    bool sameDatatype; is >> sameDatatype;
    return Timestamp::read(is) && (sameDatatype ? readSameType(is) : readDifferentTypes(is));
}

bool Aggregate::write(Core::BinaryOutputStream &os) const
{
    bool sameDatatype = isSameDataType();
    os << sameDatatype;
    return Timestamp::write(os) && (sameDatatype ? writeSameType(os) : writeDifferentTypes(os));
}

bool Aggregate::isSameDataType() const
{
    const Datatype *dt = 0;
    for(const_iterator i = begin(); i != end(); ++i) {
	if (dt == 0)
	    dt = (*i)->datatype();
	else if (dt != (*i)->datatype())
	    return false;
    }
    return true;
}

bool Aggregate::readSameType(Core::BinaryInputStream &is)
{
    std::string datatypeName;
    if (is >> datatypeName) {
	const Datatype *dt = Flow::Registry::instance().getDatatype(datatypeName);
	std::vector<DataPtr<Data> > v;
	if (dt && dt->readGatheredData(is, v)) {
	    resize(v.size());
	    for(u32 i = 0; i < v.size(); ++ i) {
		operator[](i) = v[i];
		ensure(operator[](i));
	    }
	    return true;
	}
    }
    return false;
}

bool Aggregate::writeSameType(Core::BinaryOutputStream &os) const
{
    const Datatype *dt = empty() ? datatype() : front()->datatype();
    if (os << dt->name()) {
	std::vector<DataPtr<Data> > v(begin(), end());
	return dt->writeGatheredData(os, v);
    }
    return false;
}

bool Aggregate::readDifferentTypes(Core::BinaryInputStream &is)
{
    u32 s; is >> s; resize(s);
    for(iterator i = begin(); i != end(); ++i) {
	std::string datatypeName;
	if (is >> datatypeName) {
	    const Datatype *dt = Flow::Registry::instance().getDatatype(datatypeName);
	    DataPtr<Data> v;
	    if (dt && dt->readData(is, v)) {
		*i = v;
		ensure(*i);
		continue;
	    }
	}
	clear();
	return false;
    }
    return true;
}

bool Aggregate::writeDifferentTypes(Core::BinaryOutputStream &os) const
{
    os << (u32)size();
    for(const_iterator i = begin(); i != end(); ++i) {
	const Datatype *dt = (*i)->datatype();
	DataPtr<Data> v(*i);
	if ((os << dt->name()) && dt->writeData(os, v))
	    continue;
	return false;
    }
    return true;
}

Core::XmlWriter& Aggregate::dump(Core::XmlWriter &o) const
{
    o << xmlOpen();
    for (const_iterator i = begin(); i != end(); ++i)
	(*i)->dump(o);
    o << xmlClose();
    return o;
}

//==========================================================================================
bool AggregationNode::configure()
{
    Core::Ref<Attributes> a(new Flow::Attributes);
    for (PortId i = 0; i < nInputs(); i++) {
	Core::Ref<const Attributes> b = getInputAttributes(i);
	if (b) a->merge(*b);
    }
    a->set("datatype", Aggregate::type()->name());
    return putOutputAttributes(0, a);
}

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
#include "Properties.hh"

#include <map>

namespace Cart {

    // ============================================================================
    void PropertyMap::reset() {
	if (values_) {
	    for (ChoicePtrList::iterator it = values_->begin();
		 it != values_->end(); ++it)
		delete *it;
	    delete values_;
	    values_ = 0;
	}
	delete keys_;
	keys_ = 0;
    }

    void PropertyMap::set(const StringList & keys,
			  const ListOfIndexedStringList & values) {
	reset();
	require(!keys.empty());
	require(keys.size() == values.size());
	keys_ = new Choice(keys);
	values_ = new ChoicePtrList(values.size());
	ChoicePtrList::iterator choicePtrIt;
	ListOfIndexedStringList::const_iterator valueIt;
	for (choicePtrIt = values_->begin(), valueIt = values.begin();
	     choicePtrIt != values_->end(); ++choicePtrIt, ++valueIt) {
	    // it makes no sense to have only one possible answer
	    verify(valueIt->size() > 1);
	    *choicePtrIt = new Choice(*valueIt);
	}
    }

    bool PropertyMap::operator==(const PropertyMap & map) const {
	if (!(*keys_ == *(map.keys_)))
	    return false;
	ChoicePtrList::const_iterator it1 = values_->begin();
	ChoicePtrList::const_iterator it2 = map.values_->begin();
	for (; it1 !=  values_->end(); ++it1, ++it2)
	    if (!(*(*it1) == *(*it2)))
		return false;
	return true;
    }

    void PropertyMap::write(std::ostream & os) const {
	size_t maxKeySize = 0;
	for (size_t i = 0; i < size(); ++i)
	    maxKeySize = std::max(maxKeySize, key(i).size());
	os << "property-map:" << std::endl;
	for (size_t i = 0; i < size(); ++i) {
	    os << std::setw(3) << std::right << (i + 1) << ". "
	       << std::setw(maxKeySize) << std::left << key(i) << " : ";
	    Core::Choice::const_iterator it = operator[](i).begin();
	    os << it->ident() << "/" << it->value();
	    for (++it; it != operator[](i).end(); ++it)
		os << ", " << it->ident() << "/" << it->value();
	    os << std::endl;
	}
    }

    void PropertyMap::writeXml(Core::XmlWriter & xml) const {
	xml << Core::XmlOpen("properties-definition");
	for (size_t i = 0; i < size(); ++i) {
	    xml << Core::XmlFull("key", key(i))
		<< Core::XmlOpen ("value-map");
	    for (Core::Choice::const_iterator it = operator[](i).begin();
		 it != operator[](i).end(); ++it)
		xml << Core::XmlFull("value", it->ident()) + Core::XmlAttribute("id", it->value());
	    xml << Core::XmlClose("value-map");
	}
	xml << Core::XmlClose("properties-definition");
    }
    // ============================================================================


    // ============================================================================
    bool PropertyMapDiff::equal(const std::string & key) {
	return map1_[key] == map2_[key];
    }

    // quite inefficient implementation, but we won't do a diff to often, won't we?
    bool PropertyMapDiff::equalSymbolic(const std::string & key) {
	const Core::Choice & values1 = map1_[key];
	const Core::Choice & values2 = map2_[key];
	for (Core::Choice::const_iterator it = values1.begin(); it != values1.end(); ++it)
	    if (values2[it->ident()] == Core::Choice::IllegalValue)
		return false;
	for (Core::Choice::const_iterator it = values2.begin(); it != values2.end(); ++it)
	    if (values1[it->ident()] == Core::Choice::IllegalValue)
		return false;
	return true;
    }

    void PropertyMapDiff::logKey(
	Core::XmlWriter & xml,
	const std::string & name,
	const PropertyMap & map,
	std::string key) {
	const Core::Choice & values = map[key];
	xml << Core::XmlOpen(name) + Core::XmlAttribute("key", key);
	Core::Choice::const_iterator it = values.begin();
	if (it != values.end()) {
	    xml << it->ident() << "/" << it->value();
	    for (++it; it != values.end(); ++it)
		xml << ", " << it->ident() << "/" << it->value();
	}
	xml << Core::XmlClose(name);
    }

    void PropertyMapDiff::logUniq(
	Core::XmlWriter & xml,
	std::string key,
	u16 diff) {
	xml << Core::XmlOpen("diff") + Core::XmlAttribute("difference", "uniq");
	logKey(xml, (diff == 1) ? "left" : "right", (diff == 1) ? map1_ : map2_, key);
	xml << Core::XmlClose("diff");
    }

    void PropertyMapDiff::logDiff(
	Core::XmlWriter & xml,
	std::string key) {
	xml << Core::XmlOpen("diff") + Core::XmlAttribute("difference", "values");
	logKey(xml, "left", map1_, key);
	logKey(xml, "right", map2_, key);
	xml << Core::XmlClose("diff");
    }

    PropertyMapDiff::PropertyMapDiff(
	const Core::Configuration & config,
	const PropertyMap & map1,
	const PropertyMap & map2,
	bool compareSymbolic) :
	Core::Component(config),
	map1_(map1),
	map2_(map2),
	differences_(0) {

	typedef std::map<std::string, u16> DiffMap;
	DiffMap diffMap;
	for (size_t i = 0; i < map1_.size(); ++i) {
	    diffMap.insert(DiffMap::value_type(map1_.key(i), 1));
	}
	for (size_t i = 0; i < map2_.size(); ++i) {
	    DiffMap::iterator it = diffMap.find(map2_.key(i));
	    if (it != diffMap.end()) {
		it->second |= 2;
		if ( compareSymbolic ? !equalSymbolic(it->first) : !equal(it->first) )
		    it->second |= 4;
	    } else {
		diffMap.insert(DiffMap::value_type(map2_.key(i), 2));
	    }
	}

	Core::Component::Message msg = log();
	Core::XmlWriter & xml = (Core::XmlWriter &)msg;
	for (DiffMap::iterator it = diffMap.begin(); it != diffMap.end(); ++it) {
	    switch (it->second) {
	    case 1:
		differences_ |= 1;
		logUniq(xml, it->first, 1);
		break;
	    case 2:
		differences_ |= 2;
		logUniq(xml, it->first, 2);
		break;
	    case 7:
		differences_ |= 4;
		logDiff(xml, it->first);
		break;
	    }
	}
	if (!hasDifferences())
	    xml << Core::XmlFull("diff", "no differences");
    }
    // ============================================================================


    // ============================================================================
    const std::string & Properties::operator[](const std::string & key) const {
	Index keyIndex = map_->key(key);
	if (map_->exists(keyIndex))
	    return (*map_)[keyIndex][operator[](keyIndex)];
	else
	    return map_->undefinedString;
    }

    /**
     * \todo the hash function should be checked/improved some time
     */
    size_t Properties::HashFcn::operator()(const Properties & props) const {
	size_t hash = 0;
	for (Index i = 0; i < Index(props.map_->size()); ++i)
	    hash = size_t(props[i]) ^ (hash << 4 | hash >> (sizeof(size_t) * 8 - 4));
	return hash;
    }

    bool Properties::operator==(const Properties & props) const {
	if (map_->size() != props.map().size())
	    return false;
	for (Index i = 0; i < Index(map_->size()); ++i)
	    if (operator[](i) != props[i])
		return false;
	return true;
    }

    void Properties::write(std::ostream & os) const {
	size_t maxKeySize = 0;
	for (size_t i = 0; i < map_->size(); ++i)
	    maxKeySize = std::max(maxKeySize, map_->key(i).size());
	PropertyMap::Index id;
	os << "properties:" << std::endl;
	for (size_t i = 0; i < map_->size(); ++i)
	    os << std::setw(3) << std::right << (i + 1) << ". "
	       << std::setw(maxKeySize) << std::left << map_->key(i) << " : "
	       << ((map_->isDefined(id = operator[](i))) ?
		(*map_)[i][id] : "<undefined>")
	       << std::endl;
    }

    void Properties::writeXml(Core::XmlWriter & xml) const {
	PropertyMap::Index id;
	xml << Core::XmlOpen("properties");
	for (size_t i = 0; i < map_->size(); ++i) {
	    xml << Core::XmlFull("key", map_->key(i));
	    if (map_->isDefined(id = operator[](i)))
		xml << Core::XmlFull("value", (*map_)[i][id]);
	}
	xml << Core::XmlClose("properties");
    }
    // ============================================================================



    // ============================================================================
    StoredProperties::StoredProperties(
	const Properties & props
	) :
	Properties(props.getMap()),
	valueIndexes_(map_->size(), map_->undefinedIndex) {
	for (Index i = 0; i < Index(map_->size()); ++i)
	    valueIndexes_[i] = props[i];
    }

    StoredProperties::StoredProperties(
	PropertyMapRef map,
	const StringList & keys,
	const StringList & values
	) :
	Properties(map),
	valueIndexes_(map->size(), map_->undefinedIndex) {
	require(keys.size() == values.size());
	StringList::const_iterator keyIt = keys.begin();
	StringList::const_iterator valueIt = values.begin();
	for (;keyIt != keys.end(); ++keyIt, ++valueIt) {
	    Index keyIndex = map_->key(*keyIt);
	    if (map_->exists(keyIndex))
		valueIndexes_[keyIndex] = (*map_)[keyIndex][*valueIt];
	}
    }
    // ============================================================================


} // namespace Cart

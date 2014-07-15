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
#include "../../src/Fsa/Types.hh"

#include <Bliss/Phoneme.hh>
#include <Bliss/Phonology.hh>
#include <Cart/Parser.hh>
#include <Core/XmlStream.hh>

#include "ClassicDecisionTree.hh"

using namespace Am;

// ============================================================================
void mapHmmStates(
    PropertyMap::StringList & keys,
    PropertyMap::ListOfIndexedStringList & values,
    const Am::ClassicHmmTopology & hmmTopology) {
    keys.push_back("hmm-state");
    values.push_back(PropertyMap::IndexedStringList(hmmTopology.nPhoneStates()));
    PropertyMap::IndexedStringList & pairs = values.back();
    PropertyMap::IndexedStringList::iterator it = pairs.begin();
    for (size_t i = 0; i < size_t(hmmTopology.nPhoneStates()); ++i, ++it) {
	std::stringstream ss; ss << i; ss >> it->first;
	it->second = i;
    }
}

void mapBoundaries(
    PropertyMap::StringList & keys,
    PropertyMap::ListOfIndexedStringList & values) {
    keys.push_back("boundary");
    values.push_back(PropertyMap::IndexedStringList());
    PropertyMap::IndexedStringList & pairs = values.back();
    pairs.resize(4);
    pairs[0].first = "within-lemma";
    pairs[0].second = 0;
    pairs[1].first = "begin-of-lemma";
    pairs[1].second = PropertyMap::Index(Allophone::isInitialPhone);
    pairs[2].first = "end-of-lemma";
    pairs[2].second = PropertyMap::Index(Allophone::isFinalPhone);
    pairs[3].first = "single-phoneme-lemma";
    pairs[3].second = PropertyMap::Index(Allophone::isInitialPhone | Allophone::isFinalPhone);
}

void mapContext(
    PropertyMap::StringList & keys,
    PropertyMap::ListOfIndexedStringList & values,
    const Phonology & phonology) {
    const Bliss::PhonemeInventory & inventory = *(phonology.getPhonemeInventory());
    PropertyMap::IndexedStringList phonemeMap(inventory.nPhonemes() + 1);
    phonemeMap[0].first = "#";
    phonemeMap[0].second = PropertyMap::Index(Bliss::Phoneme::term);
    Bliss::PhonemeInventory::PhonemeIterator it = inventory.phonemes().first;
    for (size_t i = 1; i <= inventory.nPhonemes(); ++i, ++it) {
	phonemeMap[i].first = (const std::string)(*it)->symbol();
	phonemeMap[i].second = PropertyMap::Index((*it)->id());
    }

    for (s32 i = phonology.maximumHistoryLength() - 1; 0 <= i; --i) {
	keys.push_back("");
	std::stringstream ss; ss << "history[" << i << "]"; ss >> keys.back();
	values.push_back(phonemeMap);
    }
    keys.push_back("central");
    values.push_back(phonemeMap);
    for (s32 i = 0; i < phonology.maximumFutureLength(); ++i) {
	keys.push_back("");
	std::stringstream ss; ss << "future[" << i << "]"; ss >> keys.back();
	values.push_back(phonemeMap);
    }
}

void mapConditions(
    PropertyMap::StringList & keys,
    PropertyMap::ListOfIndexedStringList & values,
    const PropertyMap::StringList & conditions) {
    PropertyMap::IndexedStringList boolMap(2);
    boolMap[0].first = "false";
    boolMap[0].second = PropertyMap::Index(false);
    boolMap[1].first = "true";
    boolMap[1].second = PropertyMap::Index(true);
    for (PropertyMap::StringList::const_iterator it = conditions.begin();
	 it != conditions.end(); ++it) {
	keys.push_back(*it);
	values.push_back(boolMap);
    }
}

PropertyMap::PropertyMap() :
    Precursor(),
    termIndex(0),
    termString("#") {}

PropertyMap::PropertyMap(ClassicStateModelRef stateModel) :
    Precursor(),
    termIndex(0),
    termString("#") {
    StringList keys;
    ListOfIndexedStringList values;
    mapHmmStates(keys, values, stateModel->hmmTopologySet().getDefault());
    mapBoundaries(keys, values);
    verify(stateModel->getPhonology());
    mapContext(keys, values, stateModel->phonology());
    mapConditions(keys, values, stateModel->conditions());

    set(keys, values);
}


void PropertyMap::set(
    const StringList & keys,
    const ListOfIndexedStringList & values) {


    StringList::const_iterator it = keys.begin();
    verify(it != keys.end());
    verify(*it == "hmm-state");
    ++it;
    verify(it != keys.end());
    verify(*it == "boundary");
    ++it;
    for (historySize_ = 0; (it != keys.end()) && (it->find("history") == 0);
	 ++it, ++historySize_) ;
    verify(it != keys.end());
    verify(*it == "central");
    ++it;
    for (futureSize_ = 0; (it != keys.end()) && (it->find("future") == 0);
	 ++it, ++futureSize_);

    Precursor::set(keys, values);
}

// ============================================================================


// ============================================================================
void logKey(
    Core::XmlWriter & xml,
    const PropertyMap & map,
    PropertyMap::Index i,
    const std::string & name)
{
    xml << Core::XmlOpen(name) + Core::XmlAttribute("key", map.key(i));
    Core::Choice::const_iterator it = map[i].begin();
    if (it != map[i].end()) {
	xml << it->ident() << "/" << it->value();
	for (++it; it != map[i].end(); ++it)
	    xml << ", " << it->ident() << "/" << it->value();
    }
    xml << Core::XmlClose(name);
}

void PropertyMapDiff::logDiff(
    Core::XmlWriter & xml,
    PropertyMap::Index l,
    PropertyMap::Index r)
{
    xml << Core::XmlOpen("diff") ;
    logKey(xml, map1_, l, "left");
    logKey(xml, map2_, r, "right");
    xml << Core::XmlClose("diff");
}

PropertyMapDiff::PropertyMapDiff(
    const Core::Configuration & config,
    const PropertyMap & map1,
    const PropertyMap & map2
    ) :
    Core::Component(config),
    map1_(map1),
    map2_(map2),
    differences_(0)
{
    Core::Component::Message msg = log();
    Core::XmlWriter & xml = (Core::XmlWriter &)msg;

    size_t l = 0, r = 0;
    if (!(map1_[l] == map2_[r])) {
	logDiff(xml, l, r);
	differences_ |= 1;
    }
    ++l; ++r;
    if (!(map1_[l] == map2_[r])) {
	logDiff(xml, l, r);
	differences_ |= 2;
    }
    ++l; ++r;
    if (map1.historySize() != map2.historySize()) {
	xml << Core::XmlOpen("diff")
	    << Core::XmlOpen("left")
	    << Core::XmlFull("history-size", map1.historySize())
	    << Core::XmlClose("left")
	    << Core::XmlOpen("right")
	    << Core::XmlFull("history-size", map2.historySize())
	    << Core::XmlClose("right")
	    << Core::XmlClose("diff");
	differences_ |= 8;
    }
    l += map1.historySize();
    r += map2.historySize();
    if (!(map1_[l] == map2_[r])) {
	logDiff(xml, l, r);
	differences_ |= 4;
    }
    ++l; ++r;
    if (map1.futureSize() != map2.futureSize()) {
	xml << Core::XmlOpen("diff")
	    << Core::XmlOpen("left")
	    << Core::XmlFull("future-size", map1.futureSize())
	    << Core::XmlClose("left")
	    << Core::XmlOpen("right")
	    << Core::XmlFull("future-size", map2.futureSize())
	    << Core::XmlClose("right")
	    << Core::XmlClose("diff");
	differences_ |= 16;
    }
    l += map1.futureSize();
    r += map2.futureSize();
    for (; l < map1.size() && r < map2.size(); ++l, ++r) {
	if (map1.key(l) != map2.key(r)) {
	    xml << Core::XmlOpen("diff")
		<< Core::XmlOpen("left")
		<< Core::XmlFull("condtion", map1.key(l))
		<< Core::XmlClose("left")
		<< Core::XmlOpen("right")
		<< Core::XmlFull("condtion", map2.key(r))
		<< Core::XmlClose("right")
		<< Core::XmlClose("diff");
	    differences_ |= 32;
	}
    }
    if ((l != map1.size()) || (r != map2.size())) {
	xml << Core::XmlOpen("diff")
	    << Core::XmlOpen("left")
	    << Core::XmlFull("number-of-conditions",
			     map1.size() - map1.historySize() - map1.futureSize() - 3)
	    << Core::XmlClose("left")
	    << Core::XmlOpen("right")
	    << Core::XmlFull("number-of-conditions",
			     map2.size() - map2.historySize() - map2.futureSize() - 3)
	    << Core::XmlClose("right")
	    << Core::XmlClose("diff");
	differences_ |= 32;
    }

    if (!hasDifferences())
	xml << Core::XmlFull("diff", "no differences");
}
// ============================================================================


// ============================================================================
class XmlDecisionTreeParser :
    public Cart::XmlDecisionTreeParser {
protected:
    typedef XmlDecisionTreeParser Self;
    typedef Cart::XmlDecisionTreeParser Precursor;

public:
    XmlDecisionTreeParser(
	const Core::Configuration & config
	) :
	Precursor(config, new Cart::TrainingInformationBuilder()) {}
};

bool DecisionTree::loadFromString(const std::string & str) {
    XmlDecisionTreeParser parser(config);
    return parser.parseString(str, this);
}

bool DecisionTree::loadFromStream(std::istream & i) {
    XmlDecisionTreeParser parser(config);
    return parser.parseStream(i, this);
}

bool DecisionTree::loadFromFile(const std::string & filename) {
    log("load decision tree from file \"%s\"", filename.c_str());
    XmlDecisionTreeParser parser(config);
    return parser.parseFile(filename, this);
}
// ============================================================================

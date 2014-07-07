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
#include <stdarg.h>

#include "Assertions.hh"
#include "Choice.hh"

using namespace Core;

const Choice::Value Choice::IllegalValue = -1;
const char* const Choice::IllegalIdentifier = "";
const char *Choice::endMark() {
    return (const char*) 0;
}

enum choice_mode {
    choice_parse_state_ident = 0,
    choice_parse_state_value = 1,
    choice_parse_state_exit  = 2
};

void Choice::addChoice(const char *ident, Value value) {
    require(ident != Choice::IllegalIdentifier);
    require(value != Choice::IllegalValue);
    items_by_ident.insert(Item(ident, value));
    items_by_value.insert(Item(ident, value));
}

Choice::Choice() :
    illegalIdentifier(Choice::IllegalIdentifier)
{}

Choice::Choice(const char * ident, const Value value, ...) :
    illegalIdentifier(Choice::IllegalIdentifier)
{
    int mode;
    Value va_value;
    const char* va_ident=0;
    va_list ap;

    addChoice(ident, value);

    mode = choice_parse_state_ident;
    va_start(ap, value);
    while (mode != choice_parse_state_exit) {
	switch(mode) {
	case choice_parse_state_ident:
	    va_ident = va_arg(ap, const char*);
	    if (va_ident == endMark()) mode = choice_parse_state_exit;
	    else mode = choice_parse_state_value;
	    break;
	case choice_parse_state_value:
	    va_value = va_arg(ap, Value);
	    addChoice(va_ident, va_value);
	    mode = choice_parse_state_ident;
	    break;
	default:
	    break;
	}
    }
    va_end(ap);
}

Choice::Choice(const IdentifierValuePairList & pairs) :
    illegalIdentifier(Choice::IllegalIdentifier) {
    for (IdentifierValuePairList::const_iterator it = pairs.begin();
	 it != pairs.end(); ++it) addChoice(it->first.c_str(), it->second);
}

Choice::Choice(const std::vector<std::string> & keys) :
    illegalIdentifier(Choice::IllegalIdentifier) {
    Choice::Value value = 0;
    for (std::vector<std::string>::const_iterator it = keys.begin();
	 it != keys.end(); ++it, ++value) addChoice(it->c_str(), value);
}

Choice::Value Choice::addChoice(const char *ident) {
    Value maxValue = 0;
    for (std::set<Item, comp_by_value>::const_iterator it = items_by_value.begin(); it != items_by_value.end() ; ++it)
	maxValue = std::max(maxValue, it->value());
    addChoice(ident, maxValue + 1);
    return maxValue + 1;
}

void Choice::getValues(std::vector<Value> & out) const {
    for (std::set<Item, comp_by_value>::const_iterator it = items_by_value.begin(); it != items_by_value.end() ; ++it)
	out.push_back(it->value());
}

void Choice::getIdentifiers(std::vector<std::string> & out) const {
    for (std::set<Item, comp_by_value>::const_iterator it = items_by_value.begin(); it != items_by_value.end() ; ++it)
	out.push_back(it->ident());
}

Choice::Value Choice::operator[](const std::string& ident) const {
    std::set<Item>::iterator it = items_by_ident.find(Item(ident.c_str(), 0));
    if (it == items_by_ident.end())
	return IllegalValue;
    return it->value();
}

const std::string & Choice::operator[](const Value& value) const {
    std::set<Item, comp_by_value>::iterator it = items_by_value.find(Item("", value));
    if (it == items_by_value.end())
	return illegalIdentifier;
    return it->ident();
}

bool Choice::operator==(const Choice & choice) const {
    return items_by_ident == choice.items_by_ident;
}

void Choice::printIdentifiers(std::ostream &os) const {
    std::set<Item>::const_iterator i = items_by_ident.begin();
    if (i != items_by_ident.end()) os << i->ident();
    for (++i ; i != items_by_ident.end() ; ++i) os << ", " << i->ident();
}

const Choice Core::boolChoice(
    "1",     true,  "0",     false,
    "yes",   true,  "no",    false,
    "true",  true,  "false", false,
    "on",    true,  "off",   false,
    Choice::endMark());

const Choice Core::infinityChoice(
    "infinite",   1,
    "+infinite",  1,
    "-infinite",  2,
    "infinity",   1,
    "+infinity",  1,
    "-infinity",  2,
    "inf",        1,
    "+inf",       1,
    "-inf",       2,
    "unlimited",  1,
    Choice::endMark());

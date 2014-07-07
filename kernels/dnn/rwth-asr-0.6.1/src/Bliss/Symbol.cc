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
// $Id: Symbol.cc 8249 2011-05-06 11:57:02Z rybach $

#include "Symbol.hh"

using namespace Bliss;

const Bliss::Token::Id Bliss::Token::invalidId;

SymbolSet::SymbolSet() {}

Symbol SymbolSet::operator[] (const Symbol::Char *s) {
    const Symbol::Char *result = 0;
    Map::const_iterator i = map_.find(s);
    if (i == map_.end()) {
	result = strings_.add0(s, s + strlen((char*)s));
	map_.insert(result);
    } else {
	result = *i;
    }
    return Symbol(result);
}

Symbol SymbolSet::operator[] (const Symbol::String &s) {
    const Symbol::Char *result = 0;
    Map::const_iterator i = map_.find(s.c_str());
    if (i == map_.end()) {
	//result = strings_.add0(&*(s.begin()),&*(s.end()));
	result = strings_.add0(&*(s.begin()),(&*(s.end()-1))+1);
	map_.insert(result);
    } else {
	result = *i;
    }
    return Symbol(result);
}

Symbol SymbolSet::get(const Symbol::Char *s) const {
    Map::const_iterator i = map_.find(s);
    return Symbol((i != map_.end()) ? *i : 0);
}

Symbol SymbolSet::get(const Symbol::String &s) const {
    return this->get(s.c_str());
}

bool SymbolSet::contains(const Symbol::Char *s) const {
    return map_.find(s) != map_.end();
}

bool SymbolSet::contains(const Symbol::String &s) const {
    return this->contains(s.c_str());
}

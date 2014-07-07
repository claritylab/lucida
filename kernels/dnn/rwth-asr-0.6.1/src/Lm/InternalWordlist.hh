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
// $Id: InternalWordlist.hh 5439 2005-11-09 11:05:06Z bisani $

#ifndef _LM_INTERNAL_WORDLIST_HH
#define _LM_INTERNAL_WORDLIST_HH

#include "WordlistInterface.hh"
#include <Core/Obstack.hh>
#include <Core/Types.hh>

namespace Lm {

    class WordlistInterfaceLm::InternalWordlist {
    private:
	Core::Obstack<char> symbols;
	std::vector<const char*> symbol;

    public:
	void reserve(u32 n) { symbol.reserve(n); }

	void addWord(const char *synt) {
	    const char *sym = synt;
	    const char *sym_end = sym + strlen(synt);
	    symbol.push_back(symbols.add0(sym, sym_end));
	}

	const char *word(InternalClassIndex c) const {
	    require(0 <= c &&  c < symbol.size());
	    return symbol[c];
	}
	const char *const *words() const { return &*symbol.begin(); }
	u32 nWords() const { return symbol.size(); }
    };

}

#endif // _LM_INTERNAL_WORDLIST_HH

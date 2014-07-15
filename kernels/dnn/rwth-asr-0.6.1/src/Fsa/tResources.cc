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
#include <iostream>
#include <iomanip>

#include "tResources.hh"

namespace Ftl {
    template<class _Automaton>
    void Resources<_Automaton>::dump(std::ostream &o) const {
	typedef typename Resources<_Automaton>::SemiringMap::const_iterator SemiringMapConstIterator;
	typedef typename Resources<_Automaton>::TypeToNameMap::const_iterator TypeToNameConstIterator;
	typedef typename Resources<_Automaton>::FormatMap::const_iterator FormatMapConstIterator;
	{
	    size_t l = 0;
	    for (SemiringMapConstIterator it = semirings_.begin(); it != semirings_.end(); ++it)
		l = std::max(l, it->first.size());
	    o << "registered semirings:" << std::endl;
	    for (SemiringMapConstIterator it = semirings_.begin(); it != semirings_.end(); ++it) {
		o << "  " << std::setw(l) << std::left << it->first
		  << "  " << it->second->name() << " semiring"
		  << std::endl;
	    }
	    if (defaultSemiring_)
		o << "default: " << defaultSemiring_->name() << std::endl;
	    if (!typeToName_.empty()) {
		o << std::endl;
		o << "registered semiring types (for backward compatibility)" << std::endl;
		for (TypeToNameConstIterator it = typeToName_.begin(); it != typeToName_.end(); ++it) {
		    o << "  " << std::setw(l) << std::right << it->first
		      << "  " << it->second << " semiring"
		      << std::endl;
		}
	    }
	}
	o << std::endl;
	{
	    size_t l = 0;
	    for (FormatMapConstIterator it = formats_.begin(); it != formats_.end(); ++it)
		l = std::max(l, it->first.size());
	    o << "registered formats:" << std::endl;
	    for (FormatMapConstIterator it = formats_.begin(); it != formats_.end(); ++it) {
		o << "  " << std::setw(l) << std::left << it->first
		  << "  " << ((it->second->reader) ? "i" : " ") << ((it->second->writer) ? "o" : " ")
		  << "  " << it->second->desc
		  << std::endl;
	    }
	    if (defaultFormat_)
		o << "default: " << defaultFormat_->name << std::endl;
	}
	o << std::endl;
    }
} // namespace Ftl

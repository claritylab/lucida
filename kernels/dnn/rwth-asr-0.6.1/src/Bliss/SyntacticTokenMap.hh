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
#ifndef _BLISS_SYNTACTIC_TOKEN_MAP
#define _BLISS_SYNTACTIC_TOKEN_MAP

#include <vector>
#include "Lexicon.hh"

namespace Bliss {

    template <typename T>
    class SyntacticTokenMap {
    public:
	typedef T Value;
	typedef typename std::vector<Value>::iterator iterator;
    private:
	std::vector<Value> store_;
    public:
	SyntacticTokenMap(LexiconRef _lexicon) :
	    store_(_lexicon->nSyntacticTokens() + 1) {}

	void fill(const Value &v) {
	    std::fill(store_.begin(), store_.end(), v);
	}

	iterator begin() {
	    return store_.begin();
	}

	iterator end() {
	    return store_.end();
	}

	void operator= (const SyntacticTokenMap<T> &map) {
	    require_(store_.size() == map.store_.size());
	    store_ = map.store_;
	}

	Value &operator[](const SyntacticToken *s) {
	    require_(s);
	    verify_(s->id() < store_.size());
	    return store_[s->id()];
	}

	const Value &operator[](const SyntacticToken *s) const {
	    require_(s);
	    verify_(s->id() < store_.size());
	    return store_[s->id()];
	}
    };

} // namespace Bliss

#endif // _BLISS_SYNTACTIC_TOKEN_MAP

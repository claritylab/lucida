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
#include <Core/Assertions.hh>

#include "Conditions.hh"

namespace Cart {
    bool Conditions::addCondition(const std::string & condition) {
	require(!condition.empty());
	std::vector<std::string>::iterator it = conditions_.begin();
	for (; (*it < condition) && (it != conditions_.end()); ++it) {}
	if (it == conditions_.end()) {
	    conditions_.push_back(condition);
	    return true;
	} else {
	    if (*it != condition) {
		conditions_.insert(it, condition);
		return true;
	    } else return false;
	}
    }

    bool Conditions::removeCondition(const std::string & condition) {
	std::vector<std::string>::iterator it = conditions_.begin();
	for (; (*it < condition) && (it != conditions_.end()); ++it) {}
	if (it == conditions_.end()) {
	    return false;
	} else {
	    if (*it == condition) {
		conditions_.erase(it);
		return true;
	    } else return false;
	}
    }

    void Conditions::removeAllConditions() {
	conditions_.clear();
    }

    bool Conditions::hasCondition(const std::string & condition) const {
	size_t low = 0, high = conditions_.size();
	while (low <= high) {
	    size_t mid = (low + high) / 2;
	    if (condition == conditions_[mid])
		return true;
	    int cmp = conditions_[mid].compare(condition);
	    if (cmp < 0)
		high = mid - 1;
	    else if (cmp > 0)
		low = mid + 1;
	    else
		return true;
	}
	return false;
    }

} // namespace Cart

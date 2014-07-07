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
#ifndef _CART_CONDITIONS_HH
#define _CART_CONDITIONS_HH

#include <vector>

namespace Cart {
    /**
       Representing a set of conditions;
       each condition is in turn represented
       by a std::string.
       The condition class is comparable and hashable.
    */
    class Conditions {

    private:
	std::vector<std::string> conditions_;

    public:
	Conditions() :
	    conditions_() {}

	bool addCondition(const std::string & condition);

	bool removeCondition(const std::string & condition);

	void removeAllConditions();

	bool hasCondition(const std::string & condition) const;

	bool operator==(const Conditions & c) const {
	    return conditions_ == c.conditions_;
	}

	struct HashFcn {
	    size_t operator() (const std::vector<std::string> & strings) const {
		size_t result = 0;
		for (std::vector<std::string>::const_iterator it = strings.begin();
		     it != strings.end(); ++it)
		    for(std::string::const_iterator itt = it->begin();
			itt != it->end(); ++itt)
			result = 5 * result + size_t(*itt);
		return result;
	    }
	    size_t operator() (const Conditions  & c) const {
		return c.conditions_.empty() ? 0 : operator()(c.conditions_);
	    }
	};
    };


} // namespace Cart

#endif // _CART_CONDITIONS_HH

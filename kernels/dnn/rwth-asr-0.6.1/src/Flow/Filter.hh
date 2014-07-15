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
#ifndef _FLOW_FILTER_HH
#define _FLOW_FILTER_HH

#include <iostream>
#include <Core/Configuration.hh>

#include "Node.hh"

namespace Flow {

    class Registry_;

    /** Flow filter identifier */
    class _Filter {
    private:
	std::string name;
    protected:
	_Filter(const std::string &name) { this->name = name; }
	void brand(Node *n) const {
	    n->filter_ = this;
	}
    public:
	virtual ~_Filter() {}

	inline bool operator < (const _Filter &f) const
	    { return name < f.name; }
	friend std::ostream& operator << (std::ostream& o, const _Filter &f)
	    { o << f.name; return o; }
	friend std::ostream& operator << (std::ostream& o, const _Filter *f)
	    { o << f->name; return o; }

	const std::string& getName() const { return name; }
	virtual Node* newNode(const Core::Configuration &c) const = 0;
    };

    /** Flow filter definition */
    template<class T> class Filter : public _Filter {
    private:
	friend class Flow::Registry_;
	Filter(const std::string &name) : _Filter(name) {}
    public:
	virtual Node* newNode(const Core::Configuration &c) const {
	    Node *n = new T(c);
	    brand(n);
	    return n;
	}
    };

} // namespace Flow

#endif // _FLOW_FILTER_HH

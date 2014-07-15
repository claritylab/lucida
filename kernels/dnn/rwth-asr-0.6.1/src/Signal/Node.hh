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
#ifndef _SIGNAL_NODE_HH
#define _SIGNAL_NODE_HH

#include <algorithm>
#include <list>

#include <Core/Configuration.hh>
#include <Flow/Node.hh>

namespace Signal {

    /**
     * Signal processing network node.
     * Manages datatypes.
     */
    class SleeveNode : public Flow::SleeveNode {
    private:
	std::list<const Flow::Datatype*> datatypes_;

    public:
	SleeveNode(const Core::Configuration &c) : Core::Component(c), Flow::SleeveNode(c) {}
	virtual ~SleeveNode() {}

    protected:
	virtual void reset() {}
	virtual bool configure() {
	    Core::Ref<const Flow::Attributes> a = getInputAttributes(0);
	    if (a) {
		std::list<const Flow::Datatype*>::const_iterator i;
		for (i = datatypes_.begin(); i != datatypes_.end(); i++)
		    if (configureDatatype(a, *i)) break;
		if (i == datatypes_.end())
		    return false;
	    }
	    reset();
	    if (a) return putOutputAttributes(0, a);
	    return true;
	}
	void addDatatype(const Flow::Datatype *dt) {
	    if (find(datatypes_.begin(), datatypes_.end(), dt) == datatypes_.end())
		datatypes_.push_front(dt);
	}
    };

} // namespace Signal

#endif // _SIGNAL_NODE_HH

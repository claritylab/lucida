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
#ifndef _FLOW_VECTOR_SPLIT_HH
#define _FLOW_VECTOR_SPLIT_HH

#include <map>
#include <typeinfo>
#include <vector>

#include <Core/Types.hh>
#include <Core/Parameter.hh>

#include "Data.hh"
#include "Link.hh"
#include "Node.hh"
#include "Timestamp.hh"
#include "Types.hh"
#include "Vector.hh"

#include "Node.hh"

namespace Flow {

    const Core::ParameterBool paramVectorSplitNodeReverse(
	"reverse", "if true, indices run backwards starting at the back of the input vector", false);
    const Core::ParameterBool paramVectorSplitNodeTimeStamp(
	"timestamp", "if true, split timestamp too", false);

    template<class T> class VectorSplitNode : public SinkNode {
	typedef SinkNode Precursor;
    private:
	std::map<std::string, Flow::PortId> map_;
	std::vector<std::vector<s32> > outputs_;
	bool reverse_,timestamp_;
    public:
	static std::string filterName() {
	    return std::string("generic-vector-") + Core::Type<T>::name + "-split";
	}
	VectorSplitNode(const Core::Configuration &c) : Core::Component(c), SinkNode(c), reverse_(false), timestamp_(false) {
	    reverse_ = paramVectorSplitNodeReverse(c);
	    timestamp_ = paramVectorSplitNodeTimeStamp(c);
	}
	virtual ~VectorSplitNode() {}

	virtual bool setParameter(const std::string &name, const std::string &value) {
	    if (paramVectorSplitNodeReverse.match(name))
		reverse_ = paramVectorSplitNodeReverse(value);
	    else if (paramVectorSplitNodeTimeStamp.match(name))
		timestamp_ = paramVectorSplitNodeTimeStamp(value);
	    else
		return false;
	    return true;
	}
	virtual bool configure() {
	    Core::Ref<const Attributes> a = getInputAttributes(0);
	    if (!configureDatatype(a, Vector<T>::type()))
		return false;
	    bool status = true;
	    for (PortId i = 0; i < nOutputs(); i++) {
		if (!putOutputAttributes(i, a))
		    status = false;
	    }
	    return status;
	}
	virtual Flow::PortId getOutput(const std::string &name) {
	    std::vector<s32> ins;
	    if (name.empty()) ins.push_back(-1);
	    else {
		std::vector<bool> range;
		if (Core::ParameterBitVector::parseRangeList(name, range))
		    for(u32 i = 0; i < range.size(); i++)
			if (range[i]) ins.push_back(i);
	    }

	    std::map<std::string, Flow::PortId>::iterator pos;
	    if ((pos = map_.find(name)) != map_.end()) return pos->second;

	    PortId out = map_[name] = addOutput();
	    verify(out == (PortId) outputs_.size());
	    outputs_.push_back(ins);
	    return out;
	}
	virtual bool work(Flow::PortId p) {
	    Flow::DataPtr<Flow::Vector<T> > in;
	    if (!getData(0, in)) {
		bool ret = false;
		for (u32 i = 0; i < outputs_.size(); i++)
		    if (putData(i, in.get())) ret = true;
		return ret;
	    }



	    Time step = (in->getEndTime() - in->getStartTime()) / outputs_.size();

	    for (u32 i = 0; i < outputs_.size(); i++) {
		Flow::Vector<T> *out = new Flow::Vector<T>;
		if ((outputs_[i].size() == 1) && (outputs_[i][0] == -1)) {
		    *out = *in;
		} else {
		    for (u32 j = 0; j < outputs_[i].size(); j++) {
			size_t index = outputs_[i][j];
			if (index < (size_t)in->size()) {
			    if (reverse_) index = in->size() - 1 - index;
			    out->push_back((*in)[index]);
			} else
			    out->push_back(0);
		    }
		}
		if(timestamp_){
		    out->setStartTime(in->getStartTime() + i * step);
		    out->setEndTime(in->getStartTime() + (i+1) * step);
		}else{
		    out->setStartTime(in->getStartTime());
		    out->setEndTime(in->getEndTime());
		}
		if (!putData(i, out)) return false;
	    }
	    return true;
	}
    };

} // namespace Flow

#endif // _FLOW_VECTOR_SPLIT_HH

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
#ifndef _SIGNAL_VECTORRESIZE_HH
#define _SIGNAL_VECTORRESIZE_HH

#include <Flow/Vector.hh>
#include "Node.hh"

namespace Signal {

    /** VectorResizeNode resizes input vector to a given size
     *  If new size is larger than the current size, the vector is extended by initial-value.
     *
     *  Size is given in continuous units.
     */
    extern const Core::ParameterFloat paramVectorResizeNewSize;
    extern const Core::ParameterInt paramVectorResizeNewDiscreteSize;
    extern const Core::ParameterFloat paramVectorResizeInitialValue;
    extern const Core::ParameterBool paramVectorResizeChangeFront;
    extern const Core::ParameterBool paramVectorResizeRelativeChange;

    template<class T>
    class VectorResizeNode : public SleeveNode {
    public:
	typedef SleeveNode Precursor;

    private:
	s32 newSize_;
	f64 continuousNewSize_;
	u32 discreteNewSize_;
	T initialValue_;
	bool changeFront_;
	bool relativeChange_;
    public:
	static std::string filterName() {
	    return std::string("signal-vector-") + Core::Type<T>::name + "-resize";
	}
	VectorResizeNode(const Core::Configuration &c) :
	    Core::Component(c), Precursor(c), newSize_(0) {
	    continuousNewSize_ = paramVectorResizeNewSize(c);
	    discreteNewSize_ = paramVectorResizeNewDiscreteSize(c);
	    initialValue_ = paramVectorResizeInitialValue(c);
	    changeFront_ = paramVectorResizeChangeFront(c);
	    relativeChange_ = paramVectorResizeRelativeChange(c);
	}
	virtual ~VectorResizeNode() {}

	virtual bool configure() {
	    Core::Ref<const Flow::Attributes> a = getInputAttributes(0);
	    if (!configureDatatype(a, Flow::Vector<T>::type()))
		return false;
	    std::string sampleRateAttribute = a->get("sample-rate");
	    f64 sampleRate = sampleRateAttribute.empty() ? 1 : atof(sampleRateAttribute.c_str());
	    if(sampleRate == 0)
		sampleRate = 1;
	    newSize_ = (discreteNewSize_ != 0) ? discreteNewSize_ : (u32)rint(continuousNewSize_ * sampleRate);
	    if (discreteNewSize_ != 0 && continuousNewSize_ != 0.0)
		warning("Continuous units resize will be overwritten by discrete units");
	    return putOutputAttributes(0, a);
	}

	virtual bool setParameter(const std::string &name, const std::string &value) {
	    if (paramVectorResizeNewSize.match(name))
		continuousNewSize_ = paramVectorResizeNewSize(value);
	    else if (paramVectorResizeNewDiscreteSize.match(name))
		discreteNewSize_ = paramVectorResizeNewDiscreteSize(value);
	    else if (paramVectorResizeInitialValue.match(name))
		initialValue_ = paramVectorResizeInitialValue(value);
	    else if (paramVectorResizeChangeFront.match(name))
		changeFront_ = paramVectorResizeChangeFront(value);
	    else if (paramVectorResizeRelativeChange.match(name))
		relativeChange_ = paramVectorResizeRelativeChange(value);
	    else
		return false;
	    return true;
	}

	virtual bool work(Flow::PortId p) {
	    Flow::DataPtr<Flow::Vector<T> > in;
	    if (!getData(0, in))
		return putData(0, in.get());
	    in.makePrivate();
	    s32 difference = relativeChange_ ? newSize_ : (s32)newSize_ - (s32)in->size();
	    if (((s32)in->size() + difference) < 0) {
		warning("Negative size set to zero");
		difference= -(s32)in->size();
	    }
	    if (difference <= 0)
		if (changeFront_) in->erase(in->begin(), in->begin() - difference);
		else in->erase(in->end() + difference, in->end());
	    else
		if (changeFront_) in->insert(in->begin(), difference, initialValue_);
		else in->insert(in->end(), difference, initialValue_);
	    return putData(0, in.get());
	}
    };
}


#endif // _SIGNAL_VECTORRESIZE_HH

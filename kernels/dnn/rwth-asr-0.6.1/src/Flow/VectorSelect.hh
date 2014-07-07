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
#ifndef _FLOW_VECTOR_SELECT_HH
#define _FLOW_VECTOR_SELECT_HH

#include <Core/Parameter.hh>
#include <Math/Vector.hh>
#include <Math/Matrix.hh>

#include "Node.hh"
#include "Vector.hh"

namespace Flow {

    /**
     * Allow to select elemnts from a feature vector without resorting to concat and split
     * Parameter selection takes form of 0-2,2,4,5,23-234,2-3 where 23-234 is the range
     * from dimension 23 up to 234 (234 included)
     * Original code by Christian Plahl
     */

    template <class T>
    class VectorSelectNode : public SleeveNode {
	typedef SleeveNode Precursor;
	typedef f32 Value;
	typedef s32 Range;
    private:
	std::vector<Range> featureRange_;
    public:
	void setFeatureRange(std::vector<std::string> range);
	size_t getOutputSize() { return featureRange_.size(); }

    private:
	static const Core::ParameterStringVector paramFeatureSelect;
	static const Core::ParameterIntVector paramFeatureSelectTemp;

    private:
	void applyFeatureSelection(Flow::Vector<T> &in, Flow::Vector<T> &out);

    public:
	static std::string filterName() { return std::string("generic-vector-" + Core::NameHelper<T>() + "-select"); };

    public:
	VectorSelectNode(const Core::Configuration &c);
	virtual ~VectorSelectNode() {};

	virtual bool configure();
	virtual bool setParameter(const std::string &name, const std::string &value);
	virtual bool work(Flow::PortId p);

    };

    //////////////////////////////////////////////////

    template<class T>
    const Core::ParameterStringVector VectorSelectNode<T>::paramFeatureSelect(
	"select", "vector of features to select, default all features", ",", 0,
	Core::Type<s32>::max);

    template<class T>
    const Core::ParameterIntVector VectorSelectNode<T>::paramFeatureSelectTemp(
	"select-temp", "", "-", 0, Core::Type<s32>::max);

    template<class T>
    VectorSelectNode<T>::VectorSelectNode(const Core::Configuration &c) :
	Core::Component(c), Precursor(c) {
	setFeatureRange(paramFeatureSelect(c));
    }

    template<class T>
    void VectorSelectNode<T>::applyFeatureSelection(Flow::Vector<T> &in, Flow::Vector<T> &out) {
	if (featureRange_.size() == 0)
	    out = in;
	else
	    for (u32 i = 0; i < featureRange_.size(); i++)
	    {
		if(featureRange_[i] >= in.size())
		    criticalError() << "Selection mismatch: " << featureRange_[i] << " >= " << in.size();
		require(i < out.size());
		out[i] = in[featureRange_[i]];
    }
    }

    template <class T>
    bool VectorSelectNode<T>::configure() {
	Core::Ref<Flow::Attributes> attributes(new Flow::Attributes());
	getInputAttributes(0, *attributes);

	if (!configureDatatype(attributes, Flow::Vector<T>::type())){
	    return false;
	}

	attributes->set("datatype", Flow::Vector<Value>::type()->name());
	return putOutputAttributes(0, attributes);
    }

    template<class T>
    bool VectorSelectNode<T>::work(Flow::PortId p) {
	Flow::DataPtr<Flow::Vector<T> > ptrFeatures;

	if (getData(0, ptrFeatures)) {
	    // select the features
	    require(getOutputSize()); // If this fails, then no selection was specified
	    Flow::Vector<T> *out = new Flow::Vector<T>(getOutputSize());
	    applyFeatureSelection(*(ptrFeatures.get()), *out);

	    out->setTimestamp(*ptrFeatures);
	    return putData(0, out);
	}
	return putData(0, ptrFeatures.get());
    }

    template<class T>
    void VectorSelectNode<T>::setFeatureRange(std::vector<std::string> rangeToken) {
	std::vector<Range> featureRange;

	if (rangeToken.size() != 0) { // get the range:
	    for (u32 i = 0; i < rangeToken.size(); i++) {
		featureRange = paramFeatureSelectTemp(rangeToken[i]);

		switch (featureRange.size()) {
		case 1: {
		    featureRange_.push_back(featureRange[0]);
		} break;
		case 2: {
		    for (Range j = featureRange[0]; j <= (Range) featureRange[1]; j++){
			featureRange_.push_back(j);
		    }
		} break;
		default:
		    error("parse error in feature selection list");
		}
	    }
	}
    }

    template<class T>
    bool VectorSelectNode<T>::setParameter(const std::string &name, const std::string &value) {
	if (paramFeatureSelect.match(name))
	    setFeatureRange(paramFeatureSelect(value));
	else
	    return false;

	return true;
    }


} // end namespace

#endif // _FLOW_VECTOR_SELECT_HH

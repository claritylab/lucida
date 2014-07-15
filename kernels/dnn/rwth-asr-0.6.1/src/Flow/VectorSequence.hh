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
#ifndef _FLOW_VECTOR_SEQUENCE_HH
#define _FLOW_VECTOR_SEQUENCE_HH

#include <map>
#include <typeinfo>
#include <vector>

#include <Core/Types.hh>
#include <Core/Parameter.hh>

#include "Collector.hh"
#include "Timestamp.hh"
#include "Vector.hh"

namespace Flow {

    /**
     * Vector sequence filter.
     * All input packets must be vectors, which will be put into a sequence of vectors.
     *
     * Inputs: many (e.g extracted at same timestamp), dynamically generated.
     * Outputs: the same, but on one stream. All inputs consume the same number
     * of packets.  The dimension of each output vector will be the same than
     * each input vector.
     *
     * @todo check the time stamps of the input and output vectors
     */
    const Core::ParameterBool paramVectorSequenceNodeReverse("reverse", "if true, indices run backwards starting at the back of the inputs", false);
    const Core::ParameterBool paramVectorSequenceNodeTimeStamp("timestamp", "if true, change timestamp in the sequence", false);

    template <typename T>
    class VectorSequenceNode :
	public CollectorNode< Vector<T>, Vector<T> >
    {
	typedef  CollectorNode< Vector<T>, Vector<T> > Precursor;
    private:
	bool reverse_, timestamp_;
    public:
	VectorSequenceNode(const Core::Configuration &c) : Core::Component(c), Precursor(c), reverse_(false), timestamp_(false) {
	    reverse_ = paramVectorSequenceNodeReverse(c);
	    timestamp_ = paramVectorSequenceNodeTimeStamp(c);
	}
	virtual ~VectorSequenceNode() {}

	virtual bool setParameter(const std::string &name, const std::string &value) {
	    if (paramVectorSequenceNodeReverse.match(name))
		reverse_ = paramVectorSequenceNodeReverse(value);
	    else if (paramVectorSequenceNodeTimeStamp.match(name))
		timestamp_ = paramVectorSequenceNodeTimeStamp(value);
	    else
		return Precursor::setParameter(name,value);
	    return true;
	}

	/// instead of merging we put each vector on the output stream
	virtual bool work(Flow::PortId p) {
	    if (Precursor::needInit_)
		this->init();

	    Precursor::inputData_.resize(this->nInputs());

	    if (this->nInputs() == 0) {
		this->error("No input connected.");
		return this->putNullData();
	    }

	    Time startTime, endTime;
	    if (!Precursor::getData(startTime, endTime))
		return Precursor::putNullData();

	    Time step = (endTime - startTime) / Precursor::inputData_.size();

	    for (u32 i = 0; i < Precursor::inputData_.size(); i++) {
		u32 index = i;
		if (reverse_) index = Precursor::inputData_.size() - 1 - index;
		Flow::DataPtr<Flow::Vector<T> > &out(Precursor::inputData_[index]);
		if(timestamp_){
		    out->setStartTime(startTime + i * step);
		    out->setEndTime(startTime + (i+1) * step);
		}
		if(!this->putData(0, out.get()))
		    return false;
	    }
	    return true;
	}

	static std::string filterName() {
	    return std::string("generic-vector-") + Core::Type<T>::name + "-sequence";
	}
    };

} // namespace Flow

#endif // _FLOW_VECTOR_SPLIT_HH

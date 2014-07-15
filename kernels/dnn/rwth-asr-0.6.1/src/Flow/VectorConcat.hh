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
#ifndef _FLOW_VECTOR_CONCAT_HH
#define _FLOW_VECTOR_CONCAT_HH

#include <Core/Types.hh>
#include "Merger.hh"
#include "Vector.hh"

namespace Flow {

    /**
     * Vector concatenation filter.
     * All input packets must be vectors, which will be concatenated
     * into a single vector.
     * Inputs: many, dynamically generated.
     * Outputs: one
     * All inputs consume the same nummber of packets.  The dimension of
     * the output vector will be the sum of the dimensions of all input
     * vectors.  The time stamp of the output vector is set so that it
     * contains the time ranges of all input vectors.
     */

    template <typename T>
    class VectorConcatNode :
	public MergerNode< Vector<T>, Vector<T> >
    {
	typedef  MergerNode< Vector<T>, Vector<T> > Precursor;
    private:
	u32 output_size_;

    public:
	static std::string filterName() {
	    return std::string("generic-vector-") + Core::NameHelper<T>() + "-concat";
	}
	VectorConcatNode(const Core::Configuration &c) :
	    Core::Component(c), Precursor(c), output_size_(0) {}
	virtual ~VectorConcatNode() {}

	virtual bool configure() {
	    output_size_ = 0;
	    return Precursor::configure();
	}

	virtual Vector<T> *merge(std::vector< DataPtr< Vector<T> > > &inputData) {
	    Flow::Vector<T> *out = new Flow::Vector<T>;
	    if (output_size_ > 0) out->reserve(output_size_);

	    for (u32 i = 0; i < inputData.size(); i++) {
		Flow::DataPtr<Flow::Vector<T> > &in(inputData[i]);
		out->insert(out->end(), in->begin(), in->end());
	    }
	    if (out->size() > output_size_) output_size_ = out->size();
	    return out;
	}
    };

} // namespace Flow

#endif // _FLOW_VECTOR_CONCAT_HH

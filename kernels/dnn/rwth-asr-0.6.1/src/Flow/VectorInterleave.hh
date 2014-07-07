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
#ifndef _FLOW_VECTOR_INTERLEAVE_HH
#define _FLOW_VECTOR_INTERLEAVE_HH

#include <Core/Types.hh>
#include "Merger.hh"
#include "Vector.hh"

namespace Flow {

    /**
     * Vector interleave filter.
     * All input packets must be vectors, which will be interleaved
     * into a single vector.
     * Inputs: many, dynamically generated.
     * Outputs: one
     * All inputs consume the same nummber of packets.  The dimension of
     * the output vector will be the min of the dimensions of all input
     * vectors.  The time stamp of the output vector is set so that it
     * contains the time ranges of all input vectors.
     */

    template <typename T>
    class VectorInterleaveNode :
	public MergerNode< Vector<T>, Vector<T> >
    {
	typedef  MergerNode< Vector<T>, Vector<T> > Precursor;
    private:
	u32 minimumSize(const std::vector<DataPtr<Vector<T> > > &inputData) const {
	    u32 result = Core::Type<u32>::max;

	    for(u32 d = 0; d < inputData.size(); ++ d)
		result = std::min(result, (inputData[d]) ? u32(inputData[d]->size()) : 0);

	    return result;
	}
    public:
	static std::string filterName() {
	    return std::string("generic-vector-") + Core::Type<T>::name + "-interleave";
	}
	VectorInterleaveNode(const Core::Configuration &c) : Core::Component(c), Precursor(c) {}
	virtual ~VectorInterleaveNode() {}

	virtual Vector<T> *merge(std::vector< DataPtr< Vector<T> > > &inputData) {
	    Flow::Vector<T> *out = new Flow::Vector<T>;

	    u32 nInput = inputData.size();
	    u32 size = minimumSize(inputData);

	    out->resize(size * nInput);

	    for(u32 i = 0; i < size; ++ i) {
		for (u32 d = 0; d < nInput; ++ d)
		    (*out)[i * nInput + d] = (*inputData[d])[i];
	    }

	    return out;
	}
    };

} // namespace Flow

#endif // _FLOW_VECTOR_INTERLEAVE_HH

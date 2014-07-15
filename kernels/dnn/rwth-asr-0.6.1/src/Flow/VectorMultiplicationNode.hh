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
#ifndef _FLOW_VECTOR_VECTOR_MULT_HH
#define _FLOW_VECTOR_VECTOR_MULT_HH

#include "Merger.hh"
#include "Vector.hh"

namespace Flow {

    template <typename T>
    class VectorMultNode :
	public MergerNode< Vector<T>, Vector<T> >
    {
	typedef  MergerNode< Vector<T>, Vector<T> > Precursor;
    public:
	static std::string filterName() {
	    return std::string("generic-vector-vector") + Core::Type<T>::name + "-multiplication";
	}
	VectorMultNode(const Core::Configuration &c) :
	    Core::Component(c), Precursor(c) {}
	virtual ~VectorMultNode() {}

	/**
	 * @todo This would be more efficient, if vector dimensions were advertised attributes.
	 */
	virtual Vector<T> *merge(std::vector< DataPtr< Vector<T> > > &inputData) {
	    require(inputData.size() > 0);
	    Flow::Vector<T> *out = new Flow::Vector<T>(*inputData[0]);
	    for (u32 i = 1; i < inputData.size(); ++i) {
		Flow::Vector<T> &in(*inputData[i]);
		if (out->size() < in.size())
		    out->resize(in.size(), T());
		for (u32 c = 0; c < in.size(); ++c)
		    (*out)[c] *= in[c];
	    }
	    return out;
	}

    };

} // namespace Flow
#endif

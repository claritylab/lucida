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
#ifndef _MM_VECTOR_ACCUMULATOR_HH
#define _MM_VECTOR_ACCUMULATOR_HH

#include "Utilities.hh"
#include <vector>
#include <Core/BinaryStream.hh>
#include <Core/XmlStream.hh>
#include "Types.hh"
#include "GaussDensity.hh"

namespace Mm {

    template<class InputType, class PlusOperation, class PlusWeightedOperation>
    class VectorAccumulator {
	typedef VectorAccumulator<InputType, PlusOperation, PlusWeightedOperation> Self;
    public:
	typedef typename PlusOperation::first_argument_type SumType;
    protected:
	std::vector<SumType> sum_;
	Weight weight_;
    public:
	VectorAccumulator(ComponentIndex size = 0) : weight_(0) {
	    resize(size);
	}
	VectorAccumulator(const std::vector<SumType> &sum, Weight weight) :
	    sum_(sum), weight_(weight) {}

	void resize(ComponentIndex size) {
	    sum_.resize(size);
	    reset();
	}
	ComponentIndex size() const { return sum_.size(); }

	void reset() {
	    std::fill(sum_.begin(), sum_.end(), 0);
	    weight_ = 0;
	}

	void accumulate(const std::vector<InputType> &v) {
	    unrolledTransform(sum_.begin(), sum_.end(), v.begin(), sum_.begin(), PlusOperation());
	    ++ weight_;
	}
	void accumulate(const std::vector<InputType> &v, Weight weight) {
	    unrolledTransform(sum_.begin(), sum_.end(), v.begin(), sum_.begin(), PlusWeightedOperation(weight));
	    weight_ += weight;
	}
	void accumulate(const Self &v) {
	    unrolledTransform(sum_.begin(), sum_.end(), v.sum_.begin(), sum_.begin(), std::plus<SumType>());
	    weight_ += v.weight_;
	}

	const std::vector<SumType>& sum() const { return sum_; }
	Weight weight() const { return weight_; }

	void read(Core::BinaryInputStream &i, u32 version) {
	    u32 size; i >> size; sum_.resize(size);
	    for(typename std::vector<SumType>::iterator it = sum_.begin(); it != sum_.end();
		it ++) i >> (*it);
	    if (version > 0) {
		i >> weight_;
	    } else {
		Count tmp;
		i >> tmp;
		weight_ = tmp;
	    }
	}
	void write(Core::BinaryOutputStream &o) const {
	    o << (u32)sum_.size();
	    std::copy(sum_.begin(), sum_.end(), Core::BinaryOutputStream::Iterator<SumType>(o));
	    o << weight_;
	}
	void write(Core::XmlWriter &o, const std::string &name = "vector-accumulator") const {
	    o << Core::XmlOpen(name)
		+ Core::XmlAttribute("size", sum_.size()) + Core::XmlAttribute("weight", weight_);
	    std::copy(sum_.begin(), sum_.end(), std::ostream_iterator<SumType>(o, " "));
	    o << Core::XmlClose(name);
	}

	bool operator==(const Self &toCompare) const {
	    return weight_ == toCompare.weight_ && sum_ == toCompare.sum_;
	}
	bool operator!=(const Self &toCompare) const { return !operator==(toCompare); }
    };

} // namespace Mm

#endif //_MM_VECTOR_ACCUMULATOR_HH

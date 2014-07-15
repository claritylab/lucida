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
#ifndef _SIGNAL_VECTOR_SEQUENCE_AGGREGATION_HH
#define _SIGNAL_VECTOR_SEQUENCE_AGGREGATION_HH

#include "Delay.hh"
#include <Flow/TypedAggregate.hh>
#include <Flow/Vector.hh>

namespace Signal {

	/** Filter collects a vector sequence into an aggregate
	 *  Input is sequence of vectors
	 *  Output is an aggregate with vectors as aggregate streams
	 *  Input vectors are collected with preceding and succeeding vectors.
	 *  Parameters
	 *    -expand-timestamp: if true timestamp of output encompasses the timestamps of its element;
	 *                       if false timestamp of the "present" element is copied to timestamp of output.
	 *    -for further parameters @see DelayNode
	 *  Note: in output vector oldest frames are first and recent frames last.
	 */

	extern const Core::ParameterBool paramVectorSequenceAggregationExpandTimestamp;

	template<class T> class VectorSequenceAggregation : public DelayNode {
		typedef DelayNode Precursor;
		typedef typename Flow::TypedAggregate<Flow::Vector<T> >::DataType DataType;
	private:
		bool expandTimestamp_;
	protected:
		virtual bool putData();
	public:
		static std::string filterName() {
			return std::string("signal-vector-") + Core::Type<T>::name + "-sequence-aggregation";
		}
		VectorSequenceAggregation(const Core::Configuration &c);
		virtual ~VectorSequenceAggregation() {
		}

		virtual bool setParameter(const std::string &name, const std::string &value);
		virtual bool configure();
		virtual Flow::PortId getOutput(const std::string &name) {
			return 0;
		}
	};

	template<class T> VectorSequenceAggregation<T>::VectorSequenceAggregation(const Core::Configuration &c) :
		Component(c), Precursor(c) {
		expandTimestamp_ = paramVectorSequenceAggregationExpandTimestamp(c);
		addOutput(0);
	}

	template<class T> bool VectorSequenceAggregation<T>::setParameter(const std::string &name, const std::string &value) {
		if (paramVectorSequenceAggregationExpandTimestamp.match(name))
			expandTimestamp_ = paramVectorSequenceAggregationExpandTimestamp(value);
		else
			return Precursor::setParameter(name, value);
		return true;
	}

	template<class T> bool VectorSequenceAggregation<T>::configure() {
		clear();

		Core::Ref<Flow::Attributes> attr(new Flow::Attributes);
		Core::Ref<const Flow::Attributes> a = getInputAttributes(0);
		if (!configureDatatype(a, Flow::Vector<T>::type()))
			return false;
		attr->merge(*a);
		attr->set("datatype", Flow::TypedAggregate<DataType>::type()->name());
		return putOutputAttributes(0, attr);
	}

	template<class T> bool VectorSequenceAggregation<T>::putData() {

		// collect sequence packets
		std::vector<Flow::DataPtr<DataType> > inputData;
		Flow::Timestamp* dummy = new Flow::Timestamp();
		dummy->invalidateTimestamp();

		for (int relativeIndex = -(int)maxPastSize(); relativeIndex <= (int)maxFutureSize(); ++relativeIndex) {
			Flow::DataPtr<DataType> o;
			get(relativeIndex, o);
			inputData.push_back(o);
			if (expandTimestamp_)
				dummy->expandTimestamp(*o);
			else if (relativeIndex == 0)
				dummy->setTimestamp(*o);
		}

		// create new TypedAggegrate
		Flow::TypedAggregate<DataType>* out = new Flow::TypedAggregate<DataType>(inputData);
		out->invalidateTimestamp();
		out->setTimestamp(*dummy);
		delete dummy;
		return Node::putData(0, out);
	}

} // namespace Signal

#endif //_SIGNAL_VECTOR_SEQUENCE_CONCATENATION_HH

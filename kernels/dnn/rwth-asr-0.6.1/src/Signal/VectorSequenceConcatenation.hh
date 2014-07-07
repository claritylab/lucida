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
#ifndef _SIGNAL_VECTOR_SEQUENCE_CONCATENATION_HH
#define _SIGNAL_VECTOR_SEQUENCE_CONCATENATION_HH

#include "Delay.hh"
#include <Flow/Vector.hh>

namespace Signal {

    /** Filter concatenating of vector sequence
     *  Input is sequence of vectors
     *  Output is a sequence of concatenated vectors.
     *  Input vectors are concatenated with preceding and succeeding vectors.
     *  Parameters
     *    -expand-timestamp: if true timestamp of output encompasses the timestamps of its element;
     *                        if false timestamp if the "present" element is copied to timestamp of output.
     *    -for further parameters @see DelayNode
     *  Note: in output vector oldest frames are first and recent frames last.
     */

    extern const Core::ParameterBool paramVectorSequenceConcatExpandTimestamp;

    template<class T>
    class VectorSequenceConcatenation : public DelayNode
    {
	typedef DelayNode Precursor;
    protected:
	bool expandTimestamp_;
    protected:
	virtual bool putData();
    public:
	static std::string filterName() {
	    return std::string("signal-vector-") + Core::Type<T>::name + "-sequence-concatenation";
	}
	VectorSequenceConcatenation(const Core::Configuration &c);
	virtual ~VectorSequenceConcatenation() {}

	virtual bool setParameter(const std::string &name, const std::string &value);
	virtual bool configure();
	virtual Flow::PortId getOutput(const std::string &name) { return 0; }
    };

    template<class T>
    VectorSequenceConcatenation<T>::VectorSequenceConcatenation(const Core::Configuration &c) :
	Component(c), Precursor(c)
    {
	expandTimestamp_ = paramVectorSequenceConcatExpandTimestamp(c);
	addOutput(0);
    }

    template<class T>
    bool VectorSequenceConcatenation<T>::setParameter(const std::string &name, const std::string &value)
    {
	if (paramVectorSequenceConcatExpandTimestamp.match(name))
	    expandTimestamp_ = paramVectorSequenceConcatExpandTimestamp(value);
	else
	    return Precursor::setParameter(name, value);
	return true;
    }

    template<class T>
    bool VectorSequenceConcatenation<T>::configure()
    {
	clear();

	Core::Ref<const Flow::Attributes> a = getInputAttributes(0);
	if (!configureDatatype(a, Flow::Vector<T>::type()))
	    return false;
	return putOutputAttributes(0, a);
    }

    template<class T>
    bool VectorSequenceConcatenation<T>::putData()
    {
	Flow::Vector<T> *out = new Flow::Vector<T>;
	out->invalidateTimestamp();
	for(int relativeIndex = -(int)maxPastSize(); relativeIndex <= (int)maxFutureSize(); ++ relativeIndex) {
	    Flow::DataPtr<Flow::Vector<T> > o;
	    get(relativeIndex, o);
	    out->insert(out->end(), o->begin(), o->end());
	    if (expandTimestamp_)
		out->expandTimestamp(*o);
	    else if (relativeIndex == 0)
		out->setTimestamp(*o);
	}
	return Node::putData(0, out);
    }

} // namespace Signal

#endif //_SIGNAL_VECTOR_SEQUENCE_CONCATENATION_HH

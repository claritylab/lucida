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
#ifndef _FLOW_SYNCHRONIZATION_HH
#define _FLOW_SYNCHRONIZATION_HH

#include <Core/Types.hh>
#include <Core/Utility.hh>
#include "Timestamp.hh"
#include "Node.hh"

namespace Flow {

    /** Synchronization: ensures that the input stream has the same number of elements and
     *	  the same start-times than "target-time" stream.
     *
     *  Algorithm:
     *    -remove all elements with a start-time that do not occure in the "target-time" stream.
     *    -Error messages:
     *       -there is an element in the "target-time" stream whose
     *         start-time do not occure in the input stream.
     */
    class Synchronization {

    public:
	typedef DataPtr<Timestamp> DataPointer;
    private:
	std::string lastError_;
    protected:
	/** override nextData to supply input data on demand */
	virtual bool nextData(DataPointer &dataPointer) = 0;
    public:
	static std::string name() { return "generic-synchronization"; }
	Synchronization() {}
	virtual ~Synchronization() {}

	bool work(const Timestamp &time, DataPointer &dataPointer);
	void reset() {}
	std::string lastError() const { return lastError_; }
    };

    /**
     * TimestampCopy: copy timestamps of the target stream to the input stream
     */
    class TimestampCopy {
    public:
	typedef DataPtr<Timestamp> DataPointer;
    private:
	std::string lastError_;
    protected:
	virtual bool nextData(DataPointer &dataPointer) = 0;
    public:
	static std::string name() { return "timestamp-copy"; }
	TimestampCopy() {}
	virtual ~TimestampCopy() {}

	bool work(const Timestamp &time,  DataPointer &dataPointer);
	void reset() {}
	std::string lastError() const { return lastError_; }
    };


    /** Ignore if the algorithm could not create a synchronised element for a target start-time */
    extern Core::ParameterBool paramSynchronizationIgnoreErrors;

    /** Base class for algorithms which synchronise the input stream to a "target" stream
     *  according to the start-times
     *
     *  Inputs:
     *    -target: series of target start-times
     *    -(any name): input elements to get synchronised
     *
     *  Outputs:
     *    -target: elements of the target input without any changes
     *    -(any name): synchronised series of elements
     *
     *
     *  Algorithm has to provide the following interface:
     *    typename DataPointer
     *    typename Time
     *    void reset()
     *    bool work(Time time, DataPointer out);
     *    std::string lastError() const;
     *
     */
    template<class Algorithm>
    class SynchronizationNode : public Node, public Algorithm {

    public:
	typedef typename Algorithm::DataPointer DataPointer;
    private:
	bool ignoreErrors_;
	bool firstData_;
	Time previousStartTime_;
    private:
	void reset();
    protected:
	/** Delivers next element of the input stream and checks its consistent
	 *
	 *  @return is the result of the call to Node::getData
	 *
	 *  Consistent checks:
	 *    -Empty input stream
	 *    -Null input element
	 *    -Monotonously increasing start-times
	 */
	virtual bool nextData(DataPointer &dataPointer);
    public:
	static std::string filterName() {
	    return Algorithm::name();
	}

	SynchronizationNode(const Core::Configuration &c);
	virtual ~SynchronizationNode() {}

	virtual Flow::PortId getInput(const std::string &name) { return name == "target" ? 1 : 0; }
	virtual Flow::PortId getOutput(const std::string &name) { return getInput(name); }


	virtual bool configure();
	bool setParameter(const std::string &name, const std::string &value);


	/** Reads one element from the target stream and
	 *  creates a synchronized element of the input stream by calling Algorithm::work
	 */
	 virtual bool work(Flow::PortId p);
    };

    template<class Algorithm>
    class WeakSynchronizationNode : public SynchronizationNode<Algorithm> {

	typedef SynchronizationNode<Algorithm> Precursor;
	typedef typename Precursor::DataPointer DataPointer;
    protected:
	virtual bool nextData(DataPointer &dataPointer);
    public:
	static std::string fileterName() {
	    return Algorithm::name();
	}
	WeakSynchronizationNode(const Core::Configuration &c)
	    : Core::Component(c), Precursor(c) {}
	virtual ~WeakSynchronizationNode() {}
    };


    template<class Algorithm>
    SynchronizationNode<Algorithm>::SynchronizationNode(const Core::Configuration &c) :
	Core::Component(c),  Node(c),
	firstData_(true),
	previousStartTime_(Core::Type<Time>::min)
    {
	ignoreErrors_ = paramSynchronizationIgnoreErrors(c);

	addInputs(2);
	addOutputs(2);
    }


    template<class Algorithm>
    bool SynchronizationNode<Algorithm>::setParameter(const std::string &name,
						      const std::string &value) {
	if (paramSynchronizationIgnoreErrors.match(name))
	    ignoreErrors_ = paramSynchronizationIgnoreErrors(value);
	else
	    return false;
	return true;
    }


    template<class Algorithm>
    bool SynchronizationNode<Algorithm>::configure() {

	reset();

	verify(nInputs() == nOutputs());

	for(Flow::PortId port = 0; port < nInputs(); ++ port) {
	    Core::Ref<const Attributes> attributes = getInputAttributes(port);
	    if (!putOutputAttributes(port, attributes))
		return false;
	}

	return true;
    }


    template<class Algorithm>
    bool SynchronizationNode<Algorithm>::work(Flow::PortId p) {

	DataPtr<Timestamp> interpolationTime;
	if (!getData(1, interpolationTime)) {
	    putData(0, interpolationTime.get());
	    putData(1, interpolationTime.get());
	    return true;
	}

	DataPointer out;
	if (Algorithm::work(*interpolationTime, out)) {
	    verify((bool)out);
	    putData(0, out.get());
	    putData(1, interpolationTime.get());
	    return true;
	}

	if (!ignoreErrors_)
	    this->criticalError("%s", this->lastError().c_str());
	// Synchronization failed, typically at the end of the segment, so put eos
	putData(0, Flow::Data::eos());
	putData(1, interpolationTime.get());
	return true;
    }


    template<class Algorithm>
    bool SynchronizationNode<Algorithm>::nextData(DataPointer &dataPointer) {
	bool result = Node::getData(0, dataPointer);
	if (result) {
	    if (!dataPointer)
		criticalError("Input is null.");

	    if (!firstData_ &&
		!Core::isSignificantlyLess(previousStartTime_, dataPointer->startTime(), timeTolerance)) {
		criticalError("Input start-times do not increase monotonously: %f after %f",
			      dataPointer->startTime(), previousStartTime_);
	    }

	    firstData_ = false;
	    previousStartTime_ = dataPointer->startTime();
	} else {
	    if (firstData_)
		warning("Input stream is empty.");
	}
	return result;
    }

    template<class Algorithm>
    bool WeakSynchronizationNode<Algorithm>::nextData(DataPointer &dataPointer) {
	bool result = Node::getData(0, dataPointer);
	if (result && !dataPointer)
	    result = false;
	return result;
    }


    template<class Algorithm>
    void SynchronizationNode<Algorithm>::reset() {
	Algorithm::reset();
	firstData_ = true;
	previousStartTime_ = Core::Type<Time>::min;
    }


} // namespace Flow

#endif // _FLOW_SYNCHRONIZATION_HH

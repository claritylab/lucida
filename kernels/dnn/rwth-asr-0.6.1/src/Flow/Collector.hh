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
#ifndef _FLOW_COLLECTOR_HH
#define _FLOW_COLLECTOR_HH

#include <map>
#include "Node.hh"
#include "Timestamp.hh"


namespace Flow {

    /** Switches on/off the check of length input streams.
     *  If on and input streams have different length, a critical error is raised.
     */
    const Core::ParameterBool paramCollectorNodeCheckSameLength(
	"check-same-length", "if true, error raised if input streams are of different length", false);
    const Core::ParameterString paramCollectorNodeTimestampPortName(
	"timestamp-port", "name of port to take timestamps from");

    /**
     * Abstract base class for nodes merging several packets into one stream,
     * e.g. VectorConcat, VectorSum, VectorSequence
     */

    template <class I, class O>
    class CollectorNode :
	public SourceNode
    {
	typedef SourceNode Precursor;
    public:
	typedef I InputData;
	typedef O OutputData;
	typedef Flow::DataPtr<InputData> InputFrame;
    protected:
	std::map<std::string, Flow::PortId> inputs_;
	std::vector<InputFrame> inputData_;
	bool checkSameLength_;

	std::string timestampPortName_;
	PortId timestampPortId_;
	bool needInit_;
    protected:
	void init();

	void setCheckSameLength(bool check) {
	    if (checkSameLength_ != check) { checkSameLength_ = check; needInit_ = true; }
	}
	void setTimestampPortName(std::string name) {
	    if (timestampPortName_ != name) { timestampPortName_ = name; needInit_ = true; }
	}
	/** Checks if all imputs have failed to deliver new data and
	 *  puts the corresponding object into the output.
	 */
	bool putNullData();

	/** applies Precursor::getData to all elements of inputData_
	 *
	 *  In @param startTime @param endTime is set according to timestampPortId_:
	 *    if Illegal: @param startTime is the minimal start time and @param endTime the maximal end time;
	 *    if not Illegal: timestamp of timestampPortId_ is copied into @param startTime and @param endTime.
	 *
	 *  @return is false if one of the inputs failed to deliver new data.
	 */
	bool getData(Time &startTime, Time &endTime);
    public:
	CollectorNode(const Core::Configuration &c);
	virtual ~CollectorNode() {}

	virtual bool setParameter(const std::string &name, const std::string &value);
	virtual bool configure();
	virtual Flow::PortId getInput(const std::string &name);

	virtual bool work(Flow::PortId p) = 0;
    };

    template <class I, class O>
    CollectorNode<I, O>::CollectorNode(const Core::Configuration &c) :
	Core::Component(c),
	Precursor(c),
	checkSameLength_(paramCollectorNodeCheckSameLength.defaultValue()),
	timestampPortName_(paramCollectorNodeTimestampPortName.defaultValue()),
	timestampPortId_(IllegalPortId),
	needInit_(true)
    {
	setCheckSameLength(paramCollectorNodeCheckSameLength(c));
	setTimestampPortName(paramCollectorNodeTimestampPortName(c));
    }

    template <class I, class O>
    void CollectorNode<I, O>::init()
    {
	if (!timestampPortName_.empty()) {
	    std::map<std::string, Flow::PortId>::iterator p = inputs_.find(timestampPortName_);
	    if (p == inputs_.end())
		error("Could not find input port '%s' as time-stamp-port.", timestampPortName_.c_str());
	    else
		timestampPortId_ = p->second;
	}
	needInit_ = false;
    }

    template <class I, class O>
    bool CollectorNode<I, O>::putNullData()
    {
	if (checkSameLength_) {
	    typename std::vector<InputFrame>::iterator hasData =
		std::find_if(inputData_.begin(), inputData_.end(),
			     std::bind2nd(std::equal_to<bool>(), true));

	    if (hasData != inputData_.end())
		criticalError("Input streams have different length.");
	}

	typename std::vector<InputFrame>::iterator noData =
	    std::find_if(inputData_.begin(), inputData_.end(),
			 std::bind2nd(std::equal_to<bool>(), false));

	if (noData == inputData_.end())
	    return putEos(0);
	return putData(0, noData->get());
    }

    template <class I, class O>
    bool CollectorNode<I, O>::getData(Time &startTime, Time &endTime)
    {
	startTime = Core::Type<Time>::max;
	endTime = Core::Type<Time>::min;

	bool result = true;
	for (PortId i = 0; i < (PortId)inputData_.size(); i++) {
	    InputFrame &in(inputData_[i]);
	    if (!Precursor::getData(i, in)) {
		result = false;
		continue;
	    }

	    if (timestampPortId_ == IllegalPortId) {
		if (startTime > in->getStartTime())
		    startTime = in->getStartTime();
		if (endTime < in->getEndTime())
		    endTime = in->getEndTime();
	    } else if (timestampPortId_ == i) {
		startTime = in->startTime();
		endTime = in->endTime();
	    }
	}
	return result;
    }

    template <class I, class O>
    bool CollectorNode<I, O>::setParameter(const std::string &name, const std::string &value)
    {
	if (paramCollectorNodeCheckSameLength.match(name))
	    setCheckSameLength(paramCollectorNodeCheckSameLength(value));
	else if (paramCollectorNodeTimestampPortName.match(name))
	    setTimestampPortName(paramCollectorNodeTimestampPortName(value));
	else
	    return false;
	return true;
    }

    template <class I, class O>
    bool CollectorNode<I, O>::configure()
    {
	Core::Ref<Attributes> a(new Flow::Attributes);
	for (PortId i = 0; i < nInputs(); i++) {
	    Core::Ref<const Attributes> b = getInputAttributes(i);
	    if (!configureDatatype(b, InputData::type())) return false;
	    a->merge(*b);
	}
	a->set("datatype", OutputData::type()->name());
	return putOutputAttributes(0, a);
    }

    template <class I, class O>
    Flow::PortId CollectorNode<I, O>::getInput(const std::string &name)
    {
	if (inputs_.find(name) != inputs_.end()) return Flow::IllegalPortId;
	Flow::PortId id = addInput();
	inputs_[name] = id;
	needInit_ = true;
	return id;
    }

} // namespace Flow

#endif //_FLOW_COLLECTOR_HH

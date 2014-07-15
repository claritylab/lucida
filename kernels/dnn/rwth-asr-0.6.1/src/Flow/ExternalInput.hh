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
#ifndef _FLOW_EXTERNAL_INPUT_HH
#define _FLOW_EXTERNAL_INPUT_HH

#include "Node.hh"
#include "Network.hh"
#include "Vector.hh"

namespace Flow {

    const Core::ParameterFloat paramExternalInputSampleRate(
	"sample-rate", "sample rate/frequency of input the data", 16000, 1);

    /** Wrapper for network input port
     *  Supported features:
     *     - sets sample rate,
     *     - handles startTime, endTime.
     */
    template <class T> class ExternalInput: public virtual Core::Component {
    private:
	typedef Core::Component Precursor;
    public:
	ExternalInput(const Core::Configuration &c, Network *network) : Precursor(c) {
	    require(network != 0);
	    network_ = network;
	    setSampleRate(paramExternalInputSampleRate(c));
	    configure();
	    setInput();
	}
	virtual ~ExternalInput() {}

	void setSampleRate(Time sampleRate) {
	    require(sampleRate > 0);
	    sampleRate_ = sampleRate;
	    configure();
	}

	void setInput() {
	    if (network_->nInputs()) {
		if (network_->nInputs() > 1)
		    warning("the first network input has been set as an input");
		inputPortId_ = 0;
	    }
	    else
		criticalError("network doesn't have any input port");
	}

	void setInput(const std::string &name) {
	    inputPortId_ = network_->getInput(name);
	    if (inputPortId_ == IllegalPortId)
		criticalError("input port with name \"%s\" doesn't exist",name.c_str());
	    else {
		reset();
		configure();
	    }
	}

	void putData(Vector<T> *v) {
	    startTime_ = endTime_;
	    endTime_ = endTime_ + Time(v->size())/sampleRate_;
	    v->setStartTime(startTime_);
	    v->setEndTime(endTime_);
	    network_->putData(0,v);
	}

	virtual void putEos() {
	    network_->putEos(0);
	}

	virtual void putOod() {
	    network_->putOod(0);
	}

	virtual void reset() {
	    startTime_ = 0;
	    endTime_ = 0;
	}

	virtual bool configure() {
	    bool isConfigured = network_->configure();
	    Core::Ref<Attributes> a(new Attributes());
	    a->set("sample-rate", sampleRate_);
	    a->set("sample-size", sizeof(T)*8);
	    a->set("datatype", Vector<T>::type()->name());
	    isConfigured = isConfigured && network_->putAttributes(0, a);
	    reset();
	    return isConfigured;
	}
    private:
	Time sampleRate_; // [Hz]
	Time startTime_, endTime_;
	Network *network_;
	PortId inputPortId_;
    };

} // namespace Flow

#endif // _FLOW_EXTERNAL_INPUT

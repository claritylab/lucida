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
#ifndef _FLOW_CUTTER_HH
#define _FLOW_CUTTER_HH

#include "Node.hh"
#include "Timestamp.hh"

namespace Flow {

    /** Flow network. */
    class CutterNode : public SleeveNode {
    protected:
	static Core::ParameterFloat paramStartTime;
	static Core::ParameterFloat paramEndTime;
	static Core::ParameterString paramId;

	std::vector<DataPtr<Data> > featureSequence_;
	size_t position_;
	Time startTime_;
	Time endTime_;
	std::string id_;

	void fillCache();
	void seekToStartTime();

    public:
	static std::string filterName() { return "generic-cutter"; }
	CutterNode(const Core::Configuration &c);
	virtual ~CutterNode() {}

	void setStartTime(Time time) { startTime_ = time; }
	void setEndTime(Time time) { endTime_ = time; }
	void setId(const std::string &id);

	virtual bool setParameter(const std::string &name, const std::string &value);
	virtual bool work(PortId output);
	virtual bool configure();
    };
}

#endif // _FLOW_CUTTER_HH

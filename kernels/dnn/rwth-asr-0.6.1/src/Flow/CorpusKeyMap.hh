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
#ifndef _FLOW_CORPUS_KEY_MAP_HH
#define _FLOW_CORPUS_KEY_MAP_HH

#include "Node.hh"

namespace Flow {

    class CoprusKeyMapNode : public SourceNode {
	typedef SourceNode Precursor;
    private:
	static const Core::ParameterString paramKey;
	static const Core::ParameterString paramMapFilename;
	static const Core::ParameterString paramDefaultOutput;
	static const Core::ParameterFloat paramStartTime;
	static const Core::ParameterFloat paramEndTime;
    private:
	bool sent_;
	bool send(const std::string &value);

	std::string key_;
	void setKey(const std::string &key);

	typedef Core::StringHashMap<std::string> Map;
	Map map_;
	void setMapFile(const std::string &filename);

	std::string defaultOutput_;
	void setDefaultOutput(const std::string &defaultOutput) { defaultOutput_ = defaultOutput; }

	Time startTime_;
	void setStartTime(Time time) { startTime_ = time; }
	Time endTime_;
	void setEndTime(Time time) { endTime_ = time; }

	void reset() { sent_ = false; }
    public:
	static std::string filterName() { return "generic-coprus-key-map"; }

	CoprusKeyMapNode(const Core::Configuration &c);
	virtual ~CoprusKeyMapNode();

	virtual bool setParameter(const std::string &name, const std::string &value);
	virtual bool configure();
	virtual bool work(PortId output);
    };
} // namespace Flow

#endif //_FLOW_CORPUS_KEY_MAP_HH

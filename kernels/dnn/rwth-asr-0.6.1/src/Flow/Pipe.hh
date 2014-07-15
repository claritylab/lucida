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
#ifndef _FLOW_PIPE_HH
#define _FLOW_PIPE_HH


/*
 * flow network pipe:
 */


//#include <stdio.h>
#include <cstdio>

#include "Node.hh"


namespace Flow {

    class PipeNode : public SourceNode {
    private:
	bool changed_;
	std::string command_;
	FILE *pipe_;

    public:
	static std::string filterName() { return "generic-pipe"; }
	PipeNode(const Core::Configuration &c) :
	    Core::Component(c), SourceNode(c),
	    changed_(true), pipe_(0) {}
	virtual ~PipeNode() { if (pipe_) pclose(pipe_); }

	virtual bool setParameter(const std::string &name, const std::string &value) {
	    if (name == "command") {
		command_ = value;
		changed_ = true;
	    } else return false;
	    return true;
	}
	virtual bool work(PortId out) {
	    if (changed_) {
		if (pipe_) pclose(pipe_);
		pipe_ = popen(command_.c_str(), "r");
		if (!pipe_) return putEos(0);
	    }
	    // which data to read? datatypes necessary?
	    defect();
	    return putData(out, Data::eos());
	}
    };

}


#endif // _FLOW_PIPE_HH

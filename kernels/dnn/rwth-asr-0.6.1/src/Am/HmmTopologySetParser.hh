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
#ifndef _AM_HMM_TOPOLOGY_SET_PARSER_HH
#define _AM_HMM_TOPOLOGY_SET_PARSER_HH


#include <Core/XmlParser.hh>
#include "HmmTopologySet.hh"

namespace Am {

    class HmmTopologySetParser : public Core::XmlSchemaParser {
    private:
	typedef HmmTopologySetParser Self;

	HmmTopologySet &hset_;

	void end_hmm_topology_set(const Core::XmlAttributes atts);
	void start_hmm_topology(const Core::XmlAttributes atts);
	void end_hmm_topology(const Core::XmlAttributes atts);
	void start_default_silence(const Core::XmlAttributes atts);
	void start_default_acoustic_unit(const Core::XmlAttributes atts);

    public:
	HmmTopologySetParser(HmmTopologySet &hset, const Core::Configuration &c);

	bool buildFromString(const std::string &str);
	bool buildFromFile(const std::string &filename);
    };

}


#endif // _AM_HMM_TOPOLOGY_SET_PARSER_HH

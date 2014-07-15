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
#include <Core/Unicode.hh>
#include "HmmTopologySetParser.hh"

//#include "Network.hh"
//#include "Node.hh"


using namespace Core;
using namespace Am;

/*****************************************************************************/
HmmTopologySetParser::HmmTopologySetParser
(HmmTopologySet &hset, const Core::Configuration &c) :
    XmlSchemaParser(c), hset_(hset)
    /*****************************************************************************/
{
    XmlElement *parameter_decl = new XmlEmptyElementRelay
	("param", this, startHandler(&Self::start_hmm_topology));
    XmlElement *input_decl = new XmlEmptyElementRelay
	("in", this, startHandler(&Self::start_input));
    XmlElement *output_decl = new XmlEmptyElementRelay
	("out", this, startHandler(&Self::start_output));
    XmlElement *node_decl = new XmlEmptyElementRelay
	("node", this, startHandler(&Self::start_node));
    XmlElement *link_decl = new XmlEmptyElementRelay
	("link", this, startHandler(&Self::start_link));
    XmlElement *network_decl = new XmlMixedElementRelay
	("hmm-topology-set", this,
	 startHandler(&Self::start_network),
	 endHandler(&Self::end_network),
	 0,
	 XML_CHILD(parameter_decl),
	 XML_CHILD(input_decl),
	 XML_CHILD(output_decl),
	 XML_CHILD(node_decl),
	 XML_CHILD(link_decl),
	 XML_NO_MORE_CHILDREN);

    setRoot(network_decl);
}

/*****************************************************************************/
bool HmmTopologySetParser::buildFromString(const std::string &str)
    /*****************************************************************************/
{
    return (parseString(str.c_str()) == 0);
}

/*****************************************************************************/
bool HmmTopologySetParser::buildFromFile(const std::string &filename)
    /*****************************************************************************/
{
    return (parseFile(filename.c_str()) == 0);
}

/*****************************************************************************/
void HmmTopologySetParser::end_hmm_topology_set(const XmlAttributes atts)
    /*****************************************************************************/
{
}

/*****************************************************************************/
void HmmTopologySetParser::start_hmm_topology(const XmlAttributes atts)
    /*****************************************************************************/
{
    const char *name = atts["name"];
    if (!name) error("hmm topology has no name");
    else network.addParameter(reinterpret_cast<const char*>(name));
}

/*****************************************************************************/
void HmmTopologySetParser::end_hmm_topology(const XmlAttributes atts)
    /*****************************************************************************/
{
}

/*****************************************************************************/
void HmmTopologySetParser::start_default_silence(const XmlAttributes atts)
    /*****************************************************************************/
{
    const char *name = atts["name"];
    hset_.setDefaultSilence((char*) name);
}

/*****************************************************************************/
void HmmTopologySetParser::start_default_acoustic_unit(const XmlAttributes atts)
    /*****************************************************************************/
{
    const char *name = atts["name"];
    hset_.setDefaultAcousticUnit((char*) name);
}

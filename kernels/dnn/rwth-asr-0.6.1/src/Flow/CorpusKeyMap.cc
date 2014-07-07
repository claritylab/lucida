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
#include "CorpusKeyMap.hh"
#include "DataAdaptor.hh"
#include <Core/MapParser.hh>

using namespace Flow;


const Core::ParameterString CoprusKeyMapNode::paramKey(
    "key", "corpus key");

const Core::ParameterString CoprusKeyMapNode::paramMapFilename(
    "map-file", "coprus-key map filename");
const Core::ParameterString CoprusKeyMapNode::paramDefaultOutput(
    "default-output", "defualt output if map-file does not contain a corpus-key");

const Core::ParameterFloat CoprusKeyMapNode::paramStartTime(
    "start-time", "start-time of output", 0.0);
const Core::ParameterFloat CoprusKeyMapNode::paramEndTime(
    "end-time", "end-time of output", 0.0);

CoprusKeyMapNode::CoprusKeyMapNode(const Core::Configuration &c) :
    Component(c),
    Precursor(c),
    sent_(false)
{
    setKey(paramKey(c));
    setMapFile(paramMapFilename(c));
    setDefaultOutput(paramDefaultOutput(c));
    setStartTime(paramStartTime(c));
    setEndTime(paramEndTime(c));
}

CoprusKeyMapNode::~CoprusKeyMapNode() {
}

void CoprusKeyMapNode::setMapFile(const std::string &filename) {
    if (filename != "") {
	Core::XmlMapDocument<Map> parser(config, map_, "coprus-key-map", "map-item", "key", "value");
	parser.parseFile(filename.c_str());
    }
}

void CoprusKeyMapNode::setKey(const std::string &key) {
    if (key_ != key) {
	key_ = key;
	sent_ = false;
    }
}

bool CoprusKeyMapNode::setParameter(const std::string &name, const std::string &value) {
    if (paramKey.match(name))
	setKey(paramKey(value));
    else if (paramMapFilename.match(name))
	setMapFile(paramMapFilename(value));
    else if (paramDefaultOutput.match(name))
	setDefaultOutput(paramDefaultOutput(value));
    else if (paramStartTime.match(name))
	setStartTime(paramStartTime(value));
    else if (paramEndTime.match(name))
	setEndTime(paramEndTime(value));
    else
	return false;
    return true;
}

bool CoprusKeyMapNode::configure() {
    reset();

    Core::Ref<Attributes> a(new Flow::Attributes);
    a->set("datatype", String::type()->name());
    return putOutputAttributes(0, a);
}

bool CoprusKeyMapNode::work(PortId output) {
    if (!sent_) {
	Map::const_iterator i = map_.find(key_);
	if (i != map_.end())
	    return send(i->second);
	else if (!defaultOutput_.empty())
	    send(defaultOutput_);
	else
	    error("Key '%s' not found and default is empty.", key_.c_str());
    }

    return putEos(0);
}

bool CoprusKeyMapNode::send(const std::string &value) {
    String *out = new String(value);
    out->setStartTime(startTime_);
    out->setEndTime(endTime_);
    return (sent_ = putData(0, out));
}

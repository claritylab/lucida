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
#include "StringExpressionNode.hh"

using namespace Flow;

const std::string StringExpressionNode::openTag = "$input(";
const std::string StringExpressionNode::closeTag = ")";

StringExpressionNode::StringExpressionNode(const Core::Configuration &c, PortId firstPortId) :
    Component(c),
    Precursor(c),
    nextPortId_(firstPortId),
    newTemplate_(false),
    newData_(false)
{}

PortId StringExpressionNode::getInput(const std::string &name)
{
    if (inputPorts_.find(name) != inputPorts_.end() || !stringExpression_.hasVariable(name))
	return IllegalPortId;

    inputPorts_[name].portId_ = addInput(nextPortId_ ++);
    inputPorts_[name].timestamp_ = Timestamp(Core::Type<Time>::min, Core::Type<Time>::min);
    return inputPorts_[name].portId_;
}

void StringExpressionNode::setTemplate(const std::string &t)
{
    stringExpression_ = Core::makeStringExpression(template_ = t, openTag, closeTag);
    newTemplate_ = true;
}

bool StringExpressionNode::configure(Attributes &result)
{
    reset();

    Core::StringHashMap<InputPort>::iterator i;
    for(i = inputPorts_.begin(); i != inputPorts_.end(); ++ i) {
	Core::Ref<const Attributes> attributes = getInputAttributes(i->second.portId_);
	if (!configureDatatype(attributes, String::type())) return false;
	result.merge(*attributes);
    }
    return true;
}

bool StringExpressionNode::update(const Timestamp &timestamp)
{
    Core::StringHashMap<InputPort>::iterator i;
    for(i = inputPorts_.begin(); i != inputPorts_.end(); ++ i) {
	while(!i->second.timestamp_.contains(timestamp)) {
	    DataPtr<String> in;
	    if (getData(i->second.portId_, in))
		update(i, *in);
	    else {
		criticalError("In input stream '%s', no object contained the interval [%f..%f].",
			      i->first.c_str(), timestamp.startTime(), timestamp.endTime());
	    }
	}
    }
    return newData_ || newTemplate_;
}

bool StringExpressionNode::update()
{
    Core::StringHashMap<InputPort>::iterator i;
    for(i = inputPorts_.begin(); i != inputPorts_.end(); ++ i) {
	DataPtr<String> in;
	if (getData(i->second.portId_, in))
	    update(i, *in);
    }
    return newData_ || newTemplate_;
}


void StringExpressionNode::update(
    Core::StringHashMap<InputPort>::iterator inputPort, const String &value)
{
    inputPort->second.timestamp_ = value;
    stringExpression_.setVariable(inputPort->first, value());
    newData_ = true;
}

std::string StringExpressionNode::value()
{
    if (newData_ || newTemplate_) {
	if (!stringExpression_.value(resolvedValue_))
	    criticalError("Could not resolve string expression '%s'.", template_.c_str());
	newData_ = newTemplate_ = false;
    }
    return resolvedValue_;
}

void StringExpressionNode::reset()
{
    stringExpression_.clear();
    resolvedValue_.clear();
    newData_ = false;

    Core::StringHashMap<InputPort>::iterator i;
    for(i = inputPorts_.begin(); i != inputPorts_.end(); ++ i)
	i->second.timestamp_ = Timestamp(Core::Type<Time>::min, Core::Type<Time>::min);
}

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
#include "Repeater.hh"

using namespace Flow;


RepeaterNode::RepeaterNode(const Core::Configuration &c) :
    Component(c),
    Node(c)
{
    addOutput(0);
}


PortId RepeaterNode::getInput(const std::string &name)
{
    if (name == "data") {
	addInput(0);
	return 0;
    }
    return IllegalPortId;
}


PortId RepeaterNode::getOutput(const std::string &name)
{
    if (name == "data")
	return 0;
    return IllegalPortId;
}


bool RepeaterNode::work(PortId out)
{
    if (nInputs() == 0)
	return putEos(0);

    DataPtr<Data> d;
    getData(0, d);
    return putData(0, d.get());
}

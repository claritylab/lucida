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
#include "Delay.hh"

using namespace Signal;

const Core::ParameterInt DelayNode::parameterMaxSize(
    "max-size", "maximum length of sliding window", 1, 1);
const Core::ParameterInt DelayNode::parameterRight(
    "right", "position of zero time index from right", 0, 0);

const Core::Choice DelayNode::choiceMarginPolicyType(
    "copy", marginPolicyCopy,
    "mean", marginPolicyMean,
    "zero", marginPolicyZero,
    "one", marginPolicyOne,
    Core::Choice::endMark());
const Core::ParameterChoice DelayNode::paramMarginPolicy(
    "margin-policy", &choiceMarginPolicyType, "type of margin policy", marginPolicyCopy);

const Core::Choice DelayNode::choiceMarginConditionType(
    "not-empty", marginConditionNotEmpty,
    "present-not-empty", marginConditionPresentNotEmpty,
    "full", marginConditionFull,
    Core::Choice::endMark());
const Core::ParameterChoice DelayNode::paramMarginCondition(
    "margin-condition", &choiceMarginConditionType, "condition when to generate an output",
    marginConditionPresentNotEmpty);

DelayNode::DelayNode(const Core::Configuration &c) :
    Component(c),
    Node(c),
    maxSize_(0),
    right_(0),
    marginPolicy_(0),
    marginCondition_(0),
    needInit_(true)
{
    setMaxSize(parameterMaxSize(c));
    setRight(parameterRight(c));
    setMarginPolicy((MarginPolicyType)paramMarginPolicy(c));
    setMarginCondition((MarginConditionType)paramMarginCondition(c));

    addInput(0);
}

DelayNode::~DelayNode()
{
    delete marginPolicy_;
    delete marginCondition_;
}

void DelayNode::setMarginPolicy(MarginPolicyType type)
{
    delete marginPolicy_;
    switch(type) {
    case marginPolicyCopy: marginPolicy_ = new copyMarginPolicy(*this);
	break;
    case marginPolicyMean: marginPolicy_ = new meanMarginPolicy(*this);
	break;
    case marginPolicyZero: marginPolicy_ = new zeroMarginPolicy(*this);
	break;
    case marginPolicyOne: marginPolicy_ = new oneMarginPolicy(*this);
	break;
    default:
	defect();
    };
    needInit_ = true;
}

void DelayNode::setMarginCondition(MarginConditionType condition)
{
    delete marginCondition_;
    switch(condition) {
    case marginConditionNotEmpty: marginCondition_ = new notEmptyMarginCondition(*this);
	break;
    case marginConditionPresentNotEmpty: marginCondition_ = new presentNotEmptyMarginCondition(*this);
	break;
    case marginConditionFull: marginCondition_ = new fullMarginCondition(*this);
	break;
    default:
	defect();
    };
    needInit_ = true;
}

void DelayNode::init()
{
    if (!DataSlidingWindow::init(maxSize_, right_)) {
	error("Cannot initialize with parameters max-size (%zd), right (%zd).", maxSize_, right_);
	return;
    }
    needInit_ = false;
}

bool DelayNode::setParameter(const std::string &name, const std::string &value)
{
    if (parameterMaxSize.match(name))
	setMaxSize(parameterMaxSize(value));
    else if (parameterRight.match(name))
	setRight(parameterRight(value));
    else if (paramMarginPolicy.match(name))
	setMarginPolicy((MarginPolicyType)paramMarginPolicy(value));
    else if (paramMarginCondition.match(name))
	setMarginCondition((MarginConditionType)paramMarginCondition(value));
    else
	return false;
    return true;
}

bool DelayNode::configure()
{
    clear();
    return Node::configure();
}

Flow::PortId DelayNode::getOutput(const std::string &name)
{
    char *endptr;
    int result = (int)right_ - strtol(name.c_str(), &endptr, 10);
    if (*endptr != '\0' || result < 0 || result >= (int)maxSize_)
	return Flow::IllegalPortId;
    return addOutput(result);
}

bool DelayNode::work(Flow::PortId port)
{
    if (needInit_) init();

    Flow::DataPtr<Flow::Data> i;
    while(getData(0, i)) {
	add(i);
	if (marginCondition_->isSatisfied())
	    return putData();
    }
    if (i == Flow::Data::eos()) {
	while(!empty()) {
	    flush();
	    if (marginCondition_->isSatisfied())
		return putData();
	}
    }
    return putData(i);
}

bool DelayNode::putData()
{
    bool result = false;
    for(Flow::PortId port = 0; port < nOutputs(); ++ port) {
	if (nOutputLinks(port) > 0) {
	    Flow::DataPtr<Flow::Data> out;
	    int relativeIndex = (int)right_ - (int)port;
	    get(relativeIndex, out);
	    if (Node::putData(port, out.get()))
		result = true;
	}
    }
    return result;
}

void DelayNode::get(int relativeIndex, Flow::DataPtr<Flow::Data> &out) const
{
    if (!DataSlidingWindow::get(relativeIndex, out))
	marginPolicy_->get(relativeIndex, out);
}

bool DelayNode::putData(Flow::DataPtr<Flow::Data> out)
{
    bool result = false;
    for(Flow::PortId port = 0; port < nOutputs(); ++ port) {
	if (nOutputLinks(port) > 0) {
	    if (Node::putData(port, out.get()))
		result = true;
	}
    }
    return result;
}

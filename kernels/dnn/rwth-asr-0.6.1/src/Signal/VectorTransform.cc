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
#include "VectorTransform.hh"
#include <Math/AnalyticFunctionFactory.hh>
#include <Flow/Vector.hh>

using namespace Signal;

const Core::ParameterString ContinuousVectorTransformNode::paramF(
    "f", "declaration of unary function f");

const Core::ParameterString ContinuousVectorTransformNode::paramOperation(
    "operation", "declaration of the binary function operation");

ContinuousVectorTransformNode::ContinuousVectorTransformNode(const Core::Configuration &c) :
    Core::Component(c),
    Precursor(c),
    sampleRate_(0),
    inputSize_(0),
    needInit_(true)
{
    setF(paramF(c));
    setOperation(paramOperation(c));
}

ContinuousVectorTransformNode::~ContinuousVectorTransformNode()
{}

bool ContinuousVectorTransformNode::createF()
{
    Math::AnalyticFunctionFactory factory(select(paramF.name()));
    factory.setSampleRate(sampleRate_);
    factory.setDomainType(Math::AnalyticFunctionFactory::continuousDomain);
    factory.setMaximalArgument(inputSize_ - 1);

    f_ = factory.createUnaryFunction(fDeclaration_);
    if (!f_) error("Failed to create f.");
    return f_;
}

bool ContinuousVectorTransformNode::createOperation()
{
    Math::AnalyticFunctionFactory factory(select(paramOperation.name()));
    factory.setSampleRate(sampleRate_);
    factory.setDomainType(Math::AnalyticFunctionFactory::continuousDomain);

    operation_ = factory.createBinaryFunction(operationDeclaration_);
    if (!operation_) error("Failed to create f.");
    return operation_;
}

void ContinuousVectorTransformNode::init(size_t inputSize)
{
    inputSize_ = inputSize;
    if (inputSize_ == 0)
	warning("Input size is zero.");
    if (sampleRate_ <= 0)
	error("Sample rate (%f) has an incorrect value.", sampleRate_);
    respondToDelayedErrors();
    createF();
    createOperation();
    respondToDelayedErrors();
    needInit_ = false;
}

void ContinuousVectorTransformNode::apply(std::vector<Data> &in)
{
    require(in.size() == inputSize_);
    for(u32 i = 0; i < in.size(); ++ i)
	in[i] = operation_->value(in[i], f_->value(i));
}

bool ContinuousVectorTransformNode::configure()
{
    Core::Ref<const Flow::Attributes> attributes = getInputAttributes(0);
    if (!configureDatatype(attributes, Flow::Vector<Data>::type()))
	return false;
    setSampleRate(atof(attributes->get("sample-rate").c_str()));
    return putOutputAttributes(0, attributes);
}

bool ContinuousVectorTransformNode::setParameter(const std::string &name, const std::string &value)
{
    if (paramF.match(name))
	setF(paramF(value));
    else if (paramOperation.match(name))
	setOperation(paramOperation(value));
    else
	return false;
    return true;
}

bool ContinuousVectorTransformNode::work(Flow::PortId p)
{
    Flow::DataPtr<Flow::Vector<Data> > in;
    if (getData(0, in)) {
	if (needInit_) init(in->size());
	in.makePrivate();
	apply(*in);
    }
    return putData(0, in.get());
}

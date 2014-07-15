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
#include "CosineTransform.hh"

using namespace Signal;

//==================================================================================================
CosineTransform::CosineTransform() : N_(0), normalize_(false)
{}

void CosineTransform::init(
    InputType inputType, size_t inputSize, size_t outputSize, bool normalize,
    Math::UnaryAnalyticFunctionRef warpingFunction, bool shouldWarpDifferentialUnit)
{
    normalize_ = normalize;

    Math::UnaryAnalyticFunctionRef derivedWarpingFunction = shouldWarpDifferentialUnit ?
	warpingFunction->derive() : Math::AnalyticFunctionFactory::createConstant(1);
    ensure(derivedWarpingFunction);

    switch(inputType) {
    case NplusOneData: initNplusOneData(inputSize, outputSize,
					warpingFunction, derivedWarpingFunction);
	break;
    case evenAboutNminusHalf: initEvenAboutNminusHalf(inputSize, outputSize,
						      warpingFunction, derivedWarpingFunction);
	break;
    default: defect();
    }
}

void CosineTransform::initNplusOneData(
    size_t inputSize, size_t outputSize,
    Math::UnaryAnalyticFunctionRef warpingFunction, Math::UnaryAnalyticFunctionRef derivedWarpingFunction)
{
    transformation_.resize(outputSize, inputSize);
    N_ = transformation_.nColumns() - 1;
    for(size_t k = 0; k < transformation_.nRows(); ++ k) {
	transformation_[k][0] = 0.5;
	transformation_[k][N_] = 0.5 * pow(-1, k);
	for(size_t n = 1; n < N_; ++ n) {
	    f64 omega = M_PI * n / N_;
	    transformation_[k][n] = cos(warpingFunction->value(omega) * k) *
		derivedWarpingFunction->value(omega);
	}
    }
}

void CosineTransform::initEvenAboutNminusHalf(
    size_t inputSize, size_t outputSize,
    Math::UnaryAnalyticFunctionRef warpingFunction, Math::UnaryAnalyticFunctionRef derivedWarpingFunction)
{
    transformation_.resize(outputSize, inputSize);
    N_ = transformation_.nColumns();
    for(size_t k = 0; k < transformation_.nRows(); ++ k) {
	for(size_t n = 0; n < N_; ++ n) {
	    f64 omega = M_PI * (n + 0.5) / N_;
	    transformation_[k][n] = cos(warpingFunction->value(omega) * k) *
		derivedWarpingFunction->value(omega);
	}
    }
}

void CosineTransform::apply(const std::vector<Value> &in, std::vector<Value> &out) const
{
    require_(in.size() == transformation_.nColumns());
    out = transformation_ * in;
    if (normalize_) {
	verify_(N_ > 0);
	std::transform(out.begin(), out.end(), out.begin(), std::bind2nd(std::divides<Value>(), N_));
    }
}

//==================================================================================================
const Core::Choice CosineTransformNode::choiceInputType(
    "N-plus-one", NplusOneData,
    "even-about-N-minus-half", evenAboutNminusHalf,
    Core::Choice::endMark());
const Core::ParameterChoice CosineTransformNode::paramInputType(
    "input-type", &choiceInputType, "Input: (N+1) / (N and even about N-0.5)", evenAboutNminusHalf);

const Core::ParameterInt CosineTransformNode::paramOutputSize(
    "nr-outputs", "number of outputs", 1, 1);
const Core::ParameterBool CosineTransformNode::paramNormalize(
    "normalize", "normalize output by N (yes/no)", false);

const Core::ParameterString CosineTransformNode::paramWarpingFunction(
    "warping-function", "warping function declaration");
const Core::ParameterBool CosineTransformNode::paramWarpDifferentialUnit(
    "warp-differential-unit", "Controls if derivative of warping function is applied.", true);

CosineTransformNode::CosineTransformNode(const Core::Configuration &c) :
    Component(c),
    Node(c),
    StringExpressionNode(c, 1),
    inputType_(evenAboutNminusHalf),
    outputSize_(0),
    frequencyDomainSampleRate_(0),
    normalize_(false),
    shouldWarpDifferenctialUnit_(true),
    needInit_(false)
{
    addInput(0);
    addOutput(0);

    setInputType((InputType)paramInputType(c));
    setOutputSize(paramOutputSize(c));
    setNormalize(paramNormalize(c));
    setTemplate(paramWarpingFunction(c));
    setWarpDifferentialUnit(paramWarpDifferentialUnit(c));
}

void CosineTransformNode::init(size_t inputSize)
{
    if (inputSize == 0)
	error("Empty input vector.");
    if (outputSize_ == 0)
	warning("Output size is set to zero.");
    f64 sampleRate = timeDomainSampleRate(inputSize);
    respondToDelayedErrors();
    Math::UnaryAnalyticFunctionRef warpingFunction = createWarpingFunction(sampleRate);
    respondToDelayedErrors();

    CosineTransform::init(inputType_, inputSize, outputSize_, normalize_,
			  warpingFunction, shouldWarpDifferenctialUnit_);
    needInit_ = false;
}

f64 CosineTransformNode::timeDomainSampleRate(size_t inputSize)
{
    f64 result = 0;
    switch(inputType_) {
    case NplusOneData:
	result = 2 * (inputSize - 1) / frequencyDomainSampleRate_;
    case evenAboutNminusHalf:
	result = 2 * inputSize / frequencyDomainSampleRate_;
    }
    if (result <= 0)
	error("Sample rate (%f) is not set correctly.", result);
    return result;
}

Math::UnaryAnalyticFunctionRef CosineTransformNode::createWarpingFunction(f64 sampleRate)
{
    require(sampleRate > 0);
    Math::UnaryAnalyticFunctionRef result;

    Math::AnalyticFunctionFactory factory(select(paramWarpingFunction.name()));
    factory.setSampleRate(sampleRate);
    factory.setDomainType(Math::AnalyticFunctionFactory::normalizedOmegaDomain);
    factory.setMaximalArgument(M_PI);

    result = factory.createIdentity();
    std::string declaration = StringExpressionNode::value();
    if (!declaration.empty()) {
	result = factory.createUnaryFunction(declaration);
	if (!result)
	    error("Failed to create warping function.");
	else if (shouldWarpDifferenctialUnit_ && !result->derive()) {
	    result.reset();
	    error("Warping function not derivable.");
	}
    }
    return result;
}

bool CosineTransformNode::setParameter(const std::string &name, const std::string &value)
{
    if (paramInputType.match(name))
	setInputType((InputType)paramInputType(value));
    else if (paramOutputSize.match(name))
	setOutputSize(paramOutputSize(value));
    else if (paramNormalize.match(name))
	setNormalize(paramNormalize(value));
    else if (paramWarpingFunction.match(name))
	setTemplate(paramWarpingFunction(value));
    else if (paramWarpDifferentialUnit.match(name))
	setWarpDifferentialUnit(paramWarpDifferentialUnit(value));
    else
	return false;
    return true;
}

bool CosineTransformNode::configure()
{
    Core::Ref<Flow::Attributes> attributes(new Flow::Attributes());
    getInputAttributes(0, *attributes);
    if (!configureDatatype(attributes, Flow::Vector<f32>::type()))
	return false;
    setFrequencyDomainSampleRate(atof(attributes->get("sample-rate").c_str()));
    if (!StringExpressionNode::configure(*attributes))
	return false;
    attributes->set("sample-rate", 1);
    attributes->set("datatype", Flow::Vector<f32>::type()->name());
    return putOutputAttributes(0, attributes);
}

bool CosineTransformNode::work(Flow::PortId p)
{
    Flow::DataPtr<Flow::Vector<f32> > in;
    if (!getData(0, in))
	return putData(0, in.get());

    if (StringExpressionNode::update(*in) || needInit_)
	init(in->size());

    if (in->size() != inputSize())
	criticalError("Input size (%zd) does not match the expected input size (%zd)",
		      in->size(), inputSize());

    Flow::Vector<f32> *out = new Flow::Vector<f32>;
    out->setTimestamp(*in);
    apply(*in, *out);
    return putData(0, out);
}

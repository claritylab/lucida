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
#include "GammaTone.hh"
#include <Math/Complex.hh>
#include <sstream>
using namespace Signal;

WarpingFunction::WarpingFunction(f32 warpingFactor /*= 1.0*/,
	f32 freqBreak /*= 6600.0*/, f32 maxFreq /*= 8000.0*/) {
    setWarpingFactor(warpingFactor);
    setFreqBreak(freqBreak);
    setMaxFreq(maxFreq);
}

bool WarpingFunction::checkParam() {
    if (freqBreak_ - maxFreq_ == 0) {
        std::cerr << ("freqBreak == maxFreq") << std::endl;;
        return false;
    }
    if (warpingFactor_ <= 0) {
        std::cerr << ("warpingFactor <= 0") << std::endl;;
        return false;
    }
    if (warpingFactor_ * freqBreak_ >= maxFreq_) {
        std::cerr << ("warpingFactor_ * freqBreak_ >= maxFreq_") << std::endl;;
        return false;
    }
    return true;
}

void WarpingFunction::init() {
    if (!checkParam()) {
	warpingFactor_ = 1.0;
	freqBreak_ = 6600.0;
	maxFreq_ = 8000.0;
    }

    beta_ = (warpingFactor_ * freqBreak_ - maxFreq_) / (freqBreak_ - maxFreq_);
    b_ = maxFreq_ * (1 - beta_);
    needInit_ = false;
    warpedFreqBreak_ = warping(freqBreak_);
}

f32 WarpingFunction::warping(f32 f) {
    if (needInit_) init();
    if (f < freqBreak_)
	return warpingFactor_ * f;
    else
	return beta_ * f + b_;
}

f32 WarpingFunction::inverseWarping(f32 f) {
    if (needInit_) init();
    if (f < warpedFreqBreak_)
	return f / warpingFactor_;
    else
	return (f - b_) / beta_;
}

// ===========================================================================

GammaTone::GammaTone() :
    warp_(1.0, 6600.0, 8000.0),
    minFreq_(400),
    maxFreq_(8000),
    centerFrequencyMode_(Human),
    channels_(50),
    sampleRate_(0),
    cascade_(0),
    needInit_(true) { }

/**
 * Calculates the inverse of the Greenwood function
 * (frequency to basilar membrane position mapping)
 */
f32 GammaTone::invGreenWoodFunction(f32 cf, const std::vector<f32> &parameters){
    return log10(cf / parameters[0] + parameters[1]) / parameters[2];
}

/**
 * Initialization of the filter center frequencies
 */
void GammaTone::initializeCenterFrequencyList() {
    std::vector<f32> greenWoodParameters;
    greenWoodParameters.resize(3);

    if (centerFrequencyMode_ == Human) {
	greenWoodParameters[0] = 165.4;
	greenWoodParameters[1] = 0.88;
	greenWoodParameters[2] = 2.1;
    }

    centerFrequencyList_.resize(channels_);

    f32 xMin = invGreenWoodFunction(minFreq_, greenWoodParameters);
    f32 xMax = invGreenWoodFunction(maxFreq_, greenWoodParameters);
    f32 scale = (xMax - xMin) / f32(channels_ - 1);

    // center frequencies according to the Greenwood function
    for (u32 i=0; i < channels_; i++) {
	f32 exponent = greenWoodParameters[2] * (xMin + i * scale);
	centerFrequencyList_[i] = warp_.inverseWarping(greenWoodParameters[0] * (pow(10.0, exponent) - greenWoodParameters[1]));
    }
}

/**
 * Pre-calculation of the filter coefficients
 */
void GammaTone::initCoefficients() {
    coefficients_.resize(channels_);
    f32 dt=1./sampleRate_;

    f32 theta;
    f32 Phi;
    f32 alpha;
    f32 a0, a1, b1, b2;

    for (u32 freq=0; freq < channels_; freq++){
	theta = 2. * M_PI * centerFrequencyList_[freq] * dt ;
	Phi   = 2. * M_PI * bandWidthList_[freq] * dt ;

	alpha = -exp(-Phi) * cos(theta);

	b1    = 2. * alpha;
	b2    = exp(-2 * Phi);

	std::complex<f32> b1C(b1 * cos(theta),       -b1 * sin(theta));
	std::complex<f32> b2C(b2 * cos(2 * theta),   -b2 * sin(2 * theta));
	std::complex<f32> alphaC(alpha * cos(theta), -alpha * sin(theta));

	a0 = std::abs((b1C + b2C + 1.0f) / (alphaC + 1.0f));
	a1 = alpha * a0;

	coefficients_[freq].a0 = a0;
	coefficients_[freq].a1 = a1;
	coefficients_[freq].b1 = b1;
	coefficients_[freq].b2 = b2;
    }
}

/**
 * Filter bandwidth calculation
 */
void GammaTone::initBandWidths() {
    f32 k1Erb = 24.7;
    f32 k2Erb = 4.37e-3;

    bandWidthList_.resize(channels_);
    for (u32 i=0; i < channels_; i++){
	bandWidthList_[i] = k1Erb * (k2Erb * centerFrequencyList_[i] + 1.0);
    }
}


void GammaTone::init() {
    initializeCenterFrequencyList();
    initBandWidths();
    initCoefficients();

    buffer_.resize(2);
    buffer_[0].clear();
    buffer_[1].clear();
    buffer_[0].resize(channels_);
    buffer_[1].resize(channels_);

    for (u32 ch=0; ch < channels_; ch++){
	buffer_[0][ch].resize(cascade_);
	buffer_[1][ch].resize(cascade_);
    }

    needInit_ = false;
}


void GammaTone::apply(const Flow::Vector<f32> &in, Flow::Vector<Flow::Vector<f32> > &out){
    f32 wn;
    if (needInit_) init();
    out.resize(in.size());

    for (u32 i=0; i < in.size(); i++){
	out[i].resize(channels_);
	for (u32 ch=0; ch < channels_; ch++){
	    out[i][ch] = in[i];
	    for (u32 c=0; c < cascade_; c++){
		out[i][ch] -= coefficients_[ch].b1 * buffer_[0][ch][c];
		out[i][ch] -= coefficients_[ch].b2 * buffer_[1][ch][c];
		wn = out[i][ch];
		out[i][ch] *= coefficients_[ch].a0;
		out[i][ch] += (coefficients_[ch].a1 * buffer_[0][ch][c]);

		buffer_[1][ch][c] = buffer_[0][ch][c];
		buffer_[0][ch][c] = wn;
	    }
	}
    }
}

// ===========================================================================

const Core::ParameterInt GammaToneNode::paramCascade("cascade", "filter cascade", 4);
const Core::ParameterFloat GammaToneNode::paramMinFreq("minfreq", "min center frequency", 100);
const Core::ParameterFloat GammaToneNode::paramMaxFreq("maxfreq", "max center frequency", 6000);
const Core::ParameterInt GammaToneNode::paramChannels("channels","number of channels", 50);
const Core::Choice GammaToneNode::choiceCenterFrequencyMode("human", Signal::GammaTone::Human, Core::Choice::endMark());
const Core::ParameterChoice GammaToneNode::paramCenterFrequencyMode("cfmode", &choiceCenterFrequencyMode, "center frequency mode", Human);
const Core::ParameterFloat GammaToneNode::paramWarpingFreqBreak("warp-freqbreak", "2-piece linear function breakpoint", 6600);
const Core::ParameterString GammaToneNode::paramWarpingFactor("warping-factor", "warping factor", "1");

GammaToneNode::GammaToneNode(const Core::Configuration &c) :
	Core::Component(c),
	Node(c),
	StringExpressionNode(c, 1) {
    addInput(0);
    addOutput(0);

    setCenterFrequencyMode(CenterFrequencyModeType(paramCenterFrequencyMode(c)));
    setMinFreq(paramMinFreq(c));
    setMaxFreq(paramMaxFreq(c));
    setChannels(paramChannels(c));
    setCascade(paramCascade(c));

    setWarpFreqBreak(paramWarpingFreqBreak(c));
    Flow::StringExpressionNode::setTemplate(paramWarpingFactor(c));
}

bool GammaToneNode::setParameter(const std::string &name, const std::string &value) {
    if (paramMinFreq.match(name))
	setMinFreq(paramMinFreq(value));
    else if (paramMaxFreq.match(name))
	setMaxFreq(paramMaxFreq(value));
    else if (paramChannels.match(name))
	setChannels(paramChannels(value));
    else if (paramCenterFrequencyMode.match(name))
	setCenterFrequencyMode((CenterFrequencyModeType)paramCenterFrequencyMode(value));
    else if (paramCascade.match(name))
	setCascade(paramCascade(value));
    else if (paramWarpingFreqBreak.match(name))
	setWarpFreqBreak(paramWarpingFreqBreak(value));
    else if (paramWarpingFactor.match(name))
	setTemplate(paramWarpingFactor(value));
    else
	return false;
    return true;
}


bool GammaToneNode::configure(){
    Core::Ref<Flow::Attributes> a(new Flow::Attributes());
    getInputAttributes(0, *a);
    if (!configureDatatype(a, Flow::Vector<f32>::type()))
	return false;

    setSampleRate(atof(a->get("sample-rate").c_str()));
    setWarpMaxFreq(atof(a->get("sample-rate").c_str()) / 2);
    if (!StringExpressionNode::configure(*a))
	return false;

    a->set("datatype", Flow::Vector<Flow::Vector<f32> >::type()->name());
    reset();
    return putOutputAttributes(0, a);
}


void GammaToneNode::init() {
    if (checkParam())
	GammaTone::init();
    else
	error("Maybe there is a problem with the warping function.");
}


bool GammaToneNode::work(Flow::PortId p) {
    Flow::DataPtr<Flow::Vector<f32> > in;
    if (!getData(0, in)) {
	if (in == Flow::Data::eos())
	    reset();
	return putData(0, in.get());
    }

    if (StringExpressionNode::update(*in)) {
	// Updating warping factor
	f32 warpingValue;
	std::istringstream iss(StringExpressionNode::value());
	iss >> warpingValue;
	setWarpWarpingFactor(warpingValue);
    }

    Flow::Vector<Flow::Vector<f32> > *out = new Flow::Vector<Flow::Vector<f32> >();

    in.makePrivate();
    apply(*in, *out);
    out->setTimestamp(*in);
    return putData(0, out);
}

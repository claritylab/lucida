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
#include <cmath>
#include <Core/Assertions.hh>
#include "TemporalIntegration.hh"

using namespace Flow;
using namespace Signal;

TemporalIntegration::TemporalIntegration() :
    lengthInS_(0),
    shiftInS_(0.0),
    windowFunction_(0)
{}


TemporalIntegration::~TemporalIntegration() {
    if (windowFunction_)
	delete windowFunction_;
}


void TemporalIntegration::setWindowFunction(WindowFunction* windowFunction) {
    if (windowFunction_)
	delete windowFunction_;

    windowFunction_ = windowFunction;
}


void TemporalIntegration::setLengthInS(Time length) {
    if (lengthInS_ != length) {
	lengthInS_ = length;
	setNeedInit();
    }
}


void TemporalIntegration::setShiftInS(Time shift) {
    if (shiftInS_ != shift) {
	shiftInS_ = shift;
	setNeedInit();
    }
}


void TemporalIntegration::setSampleRate(f64 sampleRate) {
    require(sampleRate > 0.0);
    if (Precursor::sampleRate() != sampleRate) {
	Precursor::setSampleRate(sampleRate);
      setNeedInit();
    }
}


void TemporalIntegration::init() {
    verify(windowFunction_);
    verify(sampleRate() > 0);

    setLength((u32)rint(lengthInS_ * sampleRate()));
    setShift((u32)rint(shiftInS_ * sampleRate()));
    Precursor::init();
}


void TemporalIntegration::transform(Vector<Sample> &out) {
    u32 channels = out[0].size();
    windowFunction_->setLength(out.size());
    for (u32 ch = 0; ch < channels; ch++){
	out[0][ch] *= (windowFunction_->getWindow())[0];
	for (u32 i = 1; i < out.size(); i++){
	    out[0][ch] += fabs(out[i][ch]) * (windowFunction_->getWindow())[i];
	}
    }
    Vector<Sample>::iterator iter = out.begin() + 1;
    out.erase(iter,out.end());
}

//----------------------------------------------------------------------------

const Core::ParameterFloat TemporalIntegrationNode::paramShift(
    "shift", "shift of window");

const Core::ParameterFloat TemporalIntegrationNode::paramLength(
    "length", "length of window");

const Core::ParameterBool TemporalIntegrationNode::paramFlushAll(
    "flush-all", "if false, segments stops after the last sample was delivered", false);

const Core::ParameterBool TemporalIntegrationNode::paramFlushBeforeGap(
    "flush-before-gap", "if true, flushes before a gap in the input samples", true);

TemporalIntegrationNode::TemporalIntegrationNode(const Core::Configuration &c) :
    Component(c), Predecessor(c)
{
    setWindowFunction(WindowFunction::create((WindowFunction::Type)WindowFunction::paramType(c)));
    setShiftInS(paramShift(c));
    setLengthInS(paramLength(c));
    setFlushAll(paramFlushAll(c));
    setFlushBeforeGap(paramFlushBeforeGap(c));
}

bool TemporalIntegrationNode::setParameter(const std::string &name, const std::string &value) {
    if (WindowFunction::paramType.match(name))
	setWindowFunction(WindowFunction::create((WindowFunction::Type)WindowFunction::paramType(value)));
    else if (paramShift.match(name))
	setShiftInS(paramShift(value));
    else if (paramLength.match(name))
	setLengthInS(paramLength(value));
    else if (paramFlushAll.match(name))
	setFlushAll(paramFlushAll(value));
    else if (paramFlushBeforeGap.match(name))
	setFlushBeforeGap(paramFlushBeforeGap(value));
    else
	return false;

    return true;
}


bool TemporalIntegrationNode::configure() {
    Core::Ref<Flow::Attributes> a(new Flow::Attributes());;
    getInputAttributes(0, *a);

    if (!configureDatatype(a, Flow::Vector<Sample>::type()))
	return false;

    a->set("frame-shift", shiftInS());
    f64 sampleRate = atof(a->get("sample-rate").c_str());
    if (sampleRate > 0.0) {
	setSampleRate(sampleRate);
    } else {
	criticalError("Sample rate is not positive: %f", sampleRate);
    }
    reset();

    return putOutputAttributes(0, a);
}

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
#include <Core/Utility.hh>
#include "DcDetection.hh"

using namespace Core;
using namespace Flow;
using namespace Signal;

// DcDetection
//////////////

DcDetection::DcDetection() :
    sampleRate_(0),
    maxDcIncrement_(.9),
    minDcLength_(0),
    minDcLengthInS_(.0125),
    maximalOutputSize_(0),
    nonDcLength_(0),
    dcLength_(0),
    nOutputs_(0),
    nonDcSegmentLength_(0),
    minNonDcSegmentLength_(0),
    minNonDcSegmentLengthInS_(.02),
    totalRejected_(0),
    totalAccepted_(0),
    bufferStartTime_(0),
    needInit_(true)
{}


void DcDetection::setMaxDcIncrement(Sample maxDcIncrement) {

    if (maxDcIncrement_ != maxDcIncrement) {

	maxDcIncrement_ = maxDcIncrement;
	needInit_ = true;
    }
}


void DcDetection::setMinDcLengthInS(Time minDcLengthInS) {

    if (minDcLengthInS_ != minDcLengthInS) {

	minDcLengthInS_ = minDcLengthInS;
	needInit_ = true;
    }
}


void DcDetection::setMinNonDcSegmentLengthInS(Time minNonDcSegmentLengthInS) {

    if (minNonDcSegmentLengthInS_ != minNonDcSegmentLengthInS) {

	minNonDcSegmentLengthInS_ = minNonDcSegmentLengthInS;
	needInit_ = true;
    }
}


void DcDetection::setMaximalOutputSize(u32 maximalOutputSize)
{

    if (maximalOutputSize_ != maximalOutputSize) {

	maximalOutputSize_ = maximalOutputSize;
	needInit_ = true;
    }
}


void DcDetection::setSampleRate(Time sampleRate) {

    if (sampleRate_ != sampleRate) {

	sampleRate_ = sampleRate;
	needInit_ = true;
    }
}


void DcDetection::init() {

    verify(sampleRate_ > 0);
    verify(maximalOutputSize_ > 0);

    minDcLength_ = (u32)rint(minDcLengthInS_ * sampleRate_);

    minNonDcSegmentLength_ = (u32)rint(minNonDcSegmentLengthInS_ * sampleRate_);

    reset();

    needInit_ = false;
}


void DcDetection::reset() {

    nonDcLength_ = 1;
    dcLength_ = 0;

    nonDcSegmentLength_ = 0;

    nOutputs_ = 0;

    bufferStartTime_ = 0;
    buffer_.clear();
}


bool DcDetection::put(const Vector<Sample> &in) {

    if (needInit_)
	init();

    Time bufferEndTime = bufferStartTime_ + (Time)buffer_.size() / sampleRate_;
    if (!in.equalsToStartTime(bufferEndTime)) {
	if (!buffer_.empty()) {
	    if(in.startTime() < bufferEndTime)
	    return false;
	    verify(maxDcIncrement_ /* Gaps in input are not allowed if dc-detection is disabled */);
	    // Fill gaps with zeroes (will be discarded in output)
	    u32 gap = (in.startTime() - bufferEndTime) * sampleRate_;
	    buffer_.insert(buffer_.end(), gap, 0);
	}else{
	// set the new start time
	bufferStartTime_ = in.startTime();
	nOutputs_ = 0;
	}
    }

    buffer_.insert(buffer_.end(), in.begin(), in.end());
    return true;
}


bool DcDetection::get(Vector<Sample> &out) {

    if (needInit_)
	init();

    out.reserve(maximalOutputSize_);
    out.clear();

    do {
	if (!nextBlock())
	    return false;

    } while(!flushBlock(out));

   return true;
}


bool DcDetection::nextBlock() {

    while((nonDcLength_ + dcLength_) < buffer_.size()) {

	if (isNonDC(buffer_[nonDcLength_ + dcLength_])) {

	    if (isDcDetected()) {
		return true;
	    } else {

		nonDcLength_ += dcLength_; // include the DC hypotheses
		dcLength_ = 0;

		if (nonDcLength_ >= std::max(minNonDcSegmentLength_, maximalOutputSize_))
		    return true;
	    }

	    nonDcLength_ ++; // include the new nonDC sample
	} else
	    dcLength_ ++;
    }

    return false;
}


bool DcDetection::lastBlock() {

    if (buffer_.empty())
	return false;

    verify((nonDcLength_ + dcLength_) == buffer_.size());

    if (isDcDetected())
	return true;

    nonDcLength_ += dcLength_; // include the DC hypotheses
    dcLength_ = 0;

    return true;
}


bool DcDetection::copyBlock(Vector<Sample> &out) {

    verify(out.empty());

    bool result = false;

    if ((nonDcSegmentLength_ += nonDcLength_) >= minNonDcSegmentLength_) {

	out.insert(out.end(), buffer_.begin(), buffer_.begin() + nonDcLength_);

	out.setStartTime(bufferStartTime_);
	out.setEndTime(bufferStartTime_ + (Time)out.size() / sampleRate_);

	nOutputs_ ++;

	result = true;
    }

    if (dcLength_ > 0)
	nonDcSegmentLength_ = 0;

    return result;
}


void DcDetection::eraseBlock() {

    buffer_.erase(buffer_.begin(), buffer_.begin() + nonDcLength_ + dcLength_);

    totalRejected_ += dcLength_;
    totalAccepted_ += nonDcLength_;

    bufferStartTime_ += (Time)(nonDcLength_ + dcLength_) / sampleRate_;

    nonDcLength_ = 1;
    dcLength_ = 0;
}


bool DcDetection::flushBlock(Vector<Sample> &out) {

    bool result = copyBlock(out);

    eraseBlock();

    return result;
}


bool DcDetection::flush(Vector<Sample> &out) {

    if (needInit_)
	init();

    out.clear();

    if (!lastBlock())
	return false;

    return flushBlock(out);
}


// DcDetectionNode
//////////////////


ParameterFloat DcDetectionNode::paramMinDcLength(
    "min-dc-length", "minimum length (in seconds) of DC necesseary for the decision", .0125, 0);

ParameterFloat DcDetectionNode::paramMaxDcIncrement(
    "max-dc-increment", "interval with less variation taken as DC, 0 disables DC detection", 0.9, 0);

ParameterFloat DcDetectionNode::paramMinNonDcSegmentLength(
    "min-non-dc-segment-length", "smaller segments (given in seconds) are discarded ", .02, 0);

ParameterInt DcDetectionNode::paramMaximalOutputSize(
    "maximal-output-size", "maximal size of output", 4096, 1);


DcDetectionNode::DcDetectionNode(const Core::Configuration &c) :
    Component(c), Predecessor(c) {

    setMinDcLengthInS(paramMinDcLength(c));
    setMaxDcIncrement(paramMaxDcIncrement(c));
    setMinNonDcSegmentLengthInS(paramMinNonDcSegmentLength(c));
    setMaximalOutputSize(paramMaximalOutputSize(c));
}


bool DcDetectionNode::setParameter(const std::string &name, const std::string &value) {
    if (paramMinDcLength.match(name))
	setMinDcLengthInS(paramMinDcLength(value));
    else if (paramMaxDcIncrement.match(name))
	setMaxDcIncrement(paramMaxDcIncrement(value));
    else  if (paramMinNonDcSegmentLength.match(name))
	setMinNonDcSegmentLengthInS(paramMinNonDcSegmentLength(value));
    else  if (paramMaximalOutputSize.match(name))
	setMaximalOutputSize(paramMaximalOutputSize(value));
    else
	return false;

    return true;
}


bool DcDetectionNode::configure() {

    Core::Ref<const Flow::Attributes> a = getInputAttributes(0);
    if (!configureDatatype(a, Flow::Vector<f32>::type()))
	return false;

    setSampleRate(atof(a->get("sample-rate").c_str()));
    reset();

    return putOutputAttributes(0, a);
}

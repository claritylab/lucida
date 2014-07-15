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
// $Id: Node.cc 9621 2014-05-13 17:35:55Z golik $

#include "Node.hh"

#include <cmath>
#include <Flow/Datatype.hh>
#include <Flow/Vector.hh>


using namespace Audio;

// ===========================================================================
const Node::SampleCount Node::invalidSampleCount =
Core::Type<SampleCount>::max;

const Core::ParameterString Node::paramFilename(
    "file", "name of audio file");

Node::Node(const Core::Configuration &c) :
    Core::Component(c), Flow::Node(c),
    sampleRate_(0), sampleSize_(0), trackCount_(0), sampleCount_(0)
{
    filename_ = paramFilename(config);
}

bool Node::isFileOpen() const { return false; }

bool Node::setParameter(const std::string &name, const std::string &value) {
    if (paramFilename.match(name)) {
	setFilename(paramFilename(value));
    } else
	return Predecessor::setParameter(name, value);
    return true;
}

// ===========================================================================
const Core::ParameterInt SourceNode::paramBlockSize(
    "block-size", "number of samples per block sent (controls latency)", 4096);
const Core::ParameterFloat SourceNode::paramStartTime(
    "start-time", "time in seconds to skip at beginning of file",
    0.0, 0.0);
const Core::ParameterFloat SourceNode::paramEndTime(
    "end-time", "time in seconds to stop extraction at",
    Core::Type<Flow::Time>::max, 0.0);

SourceNode::SourceNode(const Core::Configuration &c) :
    Core::Component(c), Node(c)
{
    addOutput(0);
    blockSize_ = paramBlockSize(config);
    startTime_ = paramStartTime(config);
    endTime_   = paramEndTime(config);
    totalSampleCount_ = invalidSampleCount;
}

bool SourceNode::setParameter(const std::string &name, const std::string &value) {
    if (paramBlockSize.match(name)) {
	blockSize_ = paramBlockSize(value);
    } else if (paramStartTime.match(name)) {
	return setStartTime(paramStartTime(value));
    } else if (paramEndTime.match(name)) {
	return setEndTime(paramEndTime(value));
    } else
	return Predecessor::setParameter(name, value);
    return true;
}

bool SourceNode::configure() {
    if (!(isFileOpen() || openFile()))
	return false;

    SampleCount startSample = getStartSample();
    if (!seek(startSample)) {
	closeFile();
	return false;
    }
    verify(sampleCount_ == startSample);

    Core::Ref<Flow::Attributes> a(new Flow::Attributes());
    a->set("file", filename_);
    a->set("sample-rate", sampleRate_);
    a->set("sample-size", sampleSize_);
    a->set("track-count", trackCount_);
    switch (sampleSize_) {
    case 8:
	a->set("datatype", Flow::Vector<s8>::type()->name());
	break;
    case 16:
	a->set("datatype", Flow::Vector<s16>::type()->name());
	break;
    case 32:
	a->set("datatype", Flow::Vector<f32>::type()->name());
	break;
    default:
	error("unsupported sample size: %d bit", sampleSize_);
	return false;
    }
    if (totalSampleCount_ != invalidSampleCount) {
	a->set("total-duration",
	       Flow::Time(totalSampleCount_) / sampleRate_);
    }
    return putOutputAttributes(0, a);
}

void SourceNode::setTotalSampleCount(SampleCount c) {
    totalSampleCount_ = c;
}

bool SourceNode::seek(SampleCount newSamplePos) {
    if (newSamplePos != 0) {
	error("This audio source node does not support seeking");
	return false;
    }
    return true;
}

bool SourceNode::setStartTime(Flow::Time _startTime) {
    startTime_ = _startTime;
    bool success = true;
    if (isFileOpen()) {
	SampleCount startSample = getStartSample();
	success = seek(startSample);
	verify(!success || sampleCount_ == startSample);
    }
    return success;
}

SourceNode::SampleCount SourceNode::getStartSample() const {
    return (SampleCount)ceil(startTime_ * sampleRate_);
}

bool SourceNode::setEndTime(Flow::Time _endTime) {
    endTime_ = _endTime;
    return true;
}

SourceNode::SampleCount SourceNode::getEndSample() const {
    Flow::Time discreteEndTime = endTime_ * sampleRate_;
    if (std::isnormal(discreteEndTime))
	return (SampleCount)floor(discreteEndTime);
    return Core::Type<SampleCount>::max;
}

bool SourceNode::work(Flow::PortId out) {
    if (!isFileOpen())
	return false;

    SampleCount endSample = getEndSample();
    if (sampleCount_ >= endSample)
	return putEos(0);

    u32 samplesToRead = (sampleCount_ + blockSize_ <= endSample) ? blockSize_ : endSample - sampleCount_;

    Flow::Timestamp *v = 0;
    u32 samplesRead = read(samplesToRead, v);

    if (!v)
	return putEos(0);

    v->setStartTime(Flow::Time(sampleCount_) / Flow::Time(sampleRate_));
    sampleCount_ += samplesRead;
    v->setEndTime(Flow::Time(sampleCount_) / Flow::Time(sampleRate_));

    ensure(v->startTime() >= startTime_ || Core::isAlmostEqual(v->startTime(), startTime_));
    ensure(v->endTime()   <= endTime_   || Core::isAlmostEqual(v->endTime(),   endTime_));
    return putData(0, v);
}

// ===========================================================================
Audio::SinkNode::SinkNode(const Core::Configuration &c) :
    Core::Component(c), Node(c)
{
    addInput(0);
    addOutput(0);
}

bool SinkNode::configure() {
    Core::Ref<const Flow::Attributes> a = getInputAttributes(0);

    //  configureDatatype(a);
    sampleRate_ = atoi(a->get("sample-rate").c_str());
    sampleSize_ = atoi(a->get("sample-size").c_str());
    trackCount_ = atoi(a->get("track-count").c_str());

    return putOutputAttributes(0, a);
}

bool SinkNode::work(Flow::PortId out) {
    Flow::DataPtr<Flow::Data> in;
    if (getData(0, in)) {
	if (isFileOpen() || openFile())
	    write(in.get());
    } else {
	if (isFileOpen()) closeFile();
    }
    return putData(0, in.get());
}

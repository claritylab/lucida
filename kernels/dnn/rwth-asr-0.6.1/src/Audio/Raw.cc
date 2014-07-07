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
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#include "Raw.hh"

#include <Flow/Vector.hh>

using namespace Audio;

// ===========================================================================
const Core::ParameterFloat RawSourceNode::paramSampleRate(
    "sample-rate", "sampling rate in Hz", 16000, 0);
const Core::ParameterInt RawSourceNode::paramSampleSize(
    "sample-size", "sampling resolution in bits", 16, 1);
const Core::ParameterInt RawSourceNode::paramTracks(
    "track-count", "number of tracks", 1, 1);

RawSourceNode::RawSourceNode(const Core::Configuration &c) :
    Core::Component(c), Node(c), SourceNode(c)
{
    sampleRate_ = paramSampleRate(config);
    sampleSize_ = paramSampleSize(config);
    trackCount_ = paramTracks(config);
}

bool RawSourceNode::setParameter(const std::string &name, const std::string &value) {
    if (paramSampleRate.match(name)) {
	setSampleRate(paramSampleRate(value));
    } else if (paramSampleSize.match(name)) {
	setSampleSize(paramSampleSize(value));
    } else if (paramTracks.match(name)) {
	setTrackCount(paramTracks(value));
    } else
	return Predecessor::setParameter(name, value);
    return true;
}

// ===========================================================================
const Core::ParameterInt RawFileInputNode::paramOffset(
    "offset", "number of bytes to skip at start of file", 0, 0);


RawFileInputNode::RawFileInputNode(const Core::Configuration &c) :
    Core::Component(c), Node(c), RawSourceNode(c)
{
    offset_ = paramOffset(c);
}

bool RawFileInputNode::setParameter(const std::string &name, const std::string &value) {
    if (paramOffset.match(name)) {
	offset_ = paramOffset(value);
    } else
	return Predecessor::setParameter(name, value);
    return true;
}

bool RawFileInputNode::openFile_() {
    bis_.open(filename_);
    if (!isFileOpen()) {
	error("could not open raw file '%s' for reading", filename_.c_str());
	return false;
    }

    readHeader();

    return true;
}

void RawFileInputNode::closeFile_() {
    bis_.close();
}

bool RawFileInputNode::seek(SampleCount newSamplePos) {
    require(isFileOpen());
    bis_.seek(position(newSamplePos), std::ios_base::beg);
    if (!bis_) {
	error("seek failed");
	return false;
    }
    sampleCount_ = newSamplePos;
    return true;
}

template <typename T>
u32 RawFileInputNode::readTyped(u32 nSamples, Flow::Timestamp* &d) {
    require(isFileOpen());
    Flow::Vector<T> *v = new Flow::Vector<T>(nSamples * trackCount_);
    u32 samplesRead = bis_.readSome(&*(v->begin()), v->size());
    if (samplesRead == 0) {
	if (!bis_.eof()) error("readSome failed");
	delete v; d = 0; return 0;
    }
    v->resize(samplesRead * trackCount_);
    d = v;
    return samplesRead;
}

u32 RawFileInputNode::read(u32 nSamples, Flow::Timestamp* &d) {
    require(isFileOpen());
    require(d == 0);
    switch (sampleSize_) {
    case 8:
	return readTyped<s8>(nSamples, d);
    case 16:
	return readTyped<s16>(nSamples, d);
    case 32:
	return readTyped<f32>(nSamples, d);
    default:
	error("unsupported sample size: %d bit", sampleSize_);
	return false;
    }
}

void RawFileInputNode::readHeader() {
    struct stat statistic;
    if (stat(filename_.c_str(), &statistic) == 0) {
	setTotalSampleCount(statistic.st_size / (sampleSize_ / 8));
    }
}

std::streampos RawFileInputNode::position(SampleCount newSamplePos) const {
    return offset_ + newSamplePos * trackCount_ * (sampleSize_ / 8);
}

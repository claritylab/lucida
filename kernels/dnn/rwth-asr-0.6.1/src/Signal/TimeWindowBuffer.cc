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
#include <math.h>
#include <Core/Assertions.hh>
#include <Core/Utility.hh>
#include "TimeWindowBuffer.hh"

using namespace Signal;

template <typename T>
TimeWindowBuffer<T>::TimeWindowBuffer() :
    length_(0),
    shift_(0),
    bufferStartTime_(0),
    sampleRate_(0),
    flushBeforeGap_(true),
    nOutputs_(0),
    flushed_(true),
    flushAll_(false),
    needInit_(true)
{}


template <typename T>
void TimeWindowBuffer<T>::init() {
    verify(sampleRate_ > 0);

    reset();
    needInit_ = false;
}


template <typename T>
void TimeWindowBuffer<T>::reset() {
    nOutputs_ = 0;
    flushed_ = false;
    buffer_.clear();
    bufferStartTime_ = 0;
}


template <typename T>
bool TimeWindowBuffer<T>::put(const Flow::Vector<T> &in) {
    if (needInit_) {
	init();
	ensure(!needInit_);
    }

    if (buffer_.empty()) {
	bufferStartTime_ = in.startTime();
    } else {
	Time bufferEndTime = bufferStartTime_ + (Time)buffer_.size() / sampleRate_;
	if (flushBeforeGap_ && !in.equalsToStartTime(bufferEndTime))
	    return false;
    }

    buffer_.insert(buffer_.end(), in.begin(), in.end());
    return true;
}


template <typename T>
void TimeWindowBuffer<T>::copy(Flow::Vector<T> &out, u32 length) {
    out.clear();
    out.insert(out.end(), buffer_.begin(), buffer_.begin() + length);
    out.setStartTime(bufferStartTime_);
    out.setEndTime(bufferStartTime_ + (Time)out.size() / (Time)sampleRate_);

    ++ nOutputs_;
    transform(out);
}


template <typename T>
bool TimeWindowBuffer<T>::get(Flow::Vector<T> &out) {
    if (needInit_) {
	init();
	verify(!needInit_);
    }

    if (buffer_.size() < 2 * std::max(shift_, length_))
	return false;

    copy(out, length_);
    buffer_.erase(buffer_.begin(), buffer_.begin() + shift_);
    bufferStartTime_ += (Time)shift_ / (Time)sampleRate_;
    return true;
}


template <typename T>
bool TimeWindowBuffer<T>::flush(Flow::Vector<T> &out) {
    if (needInit_) {
	init();
	verify(!needInit_);
    }

    verify(buffer_.size() < 2 * std::max(shift_, length_));

    if (buffer_.empty())
	return false;

    if (flushAll())
	flushed_ = (shift_ >= buffer_.size());
    else
	flushed_ = (std::max(shift_, length_) >= buffer_.size());
    copy(out, std::min(length_, u32(buffer_.size())));

    if (flushed())
	needInit_ = true;
    else {

	buffer_.erase(buffer_.begin(), buffer_.begin() + shift_);

	bufferStartTime_ += (Time)shift_ / (Time)sampleRate_;
    }

    return true;
}

namespace Signal {

//=============================================================================
// explicit template instantiation
template class TimeWindowBuffer<Flow::Vector<f32> >;

}

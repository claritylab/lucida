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
#include <Flow/Timestamp.hh>
#include "RepeatingFramePrediction.hh"

using namespace Signal;


// RepeatingFramePrediction
///////////////////////////


RepeatingFramePrediction::RepeatingFramePrediction() :
    slidingWindow_(2, 0),
    predictOnlyMissing_(true),
    syncEndTimes_(false)
    { }


bool RepeatingFramePrediction::work(const Flow::Timestamp &timestamp, DataPointer &out) {
    Time time = timestamp.startTime();
    Time endTime;
    if (syncEndTimes_)
	endTime = timestamp.endTime();
    else
	endTime = -1;

    seek(time);
    if (copyLatest(time, out, endTime))
	return true;

    return copyPrevious(time, out, endTime);
}


void RepeatingFramePrediction::seek(Time time) {
    while(slidingWindow_.size() == 0 ||
      Core::isSignificantlyLess(slidingWindow_.front()->startTime(), time, Flow::timeTolerance)) {

    DataPointer dataPointer;

    if (nextData(dataPointer))
	slidingWindow_.add(dataPointer);
    else if (slidingWindow_.size() > 1)
	slidingWindow_.flushOut();
    else
	break;
    }
}


bool RepeatingFramePrediction::copyLatest(Time time, DataPointer &out, Time endTime=-1) {
    verify(slidingWindow_.size() >= 1);

    if (predictOnlyMissing_ && slidingWindow_.front()->equalsToStartTime(time)) {
	out = slidingWindow_.front();
	if (endTime != -1) out->setEndTime(endTime);
    return true;
    }

    return false;
}


bool RepeatingFramePrediction::copyPrevious(Time time, DataPointer &out, Time endTime=-1) {
    verify(slidingWindow_.size() >= 1);

    out = slidingWindow_.back();

    out.makePrivate();

    out->setStartTime(time);
    if (endTime != -1)
	out->setEndTime(endTime);
    else
	out->setEndTime(time);
    return true;
}

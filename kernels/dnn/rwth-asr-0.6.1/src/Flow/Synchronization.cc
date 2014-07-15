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
#include <Core/StringUtilities.hh>
#include "Synchronization.hh"

using namespace Flow;


bool Synchronization::work(const Timestamp &time, DataPointer &dataPointer) {

    DataPointer in;
    Time startTime = time.startTime();
    do {
	if (!nextData(in)) {
	    lastError_ = Core::form("Input stream ended before the start-time %f.", startTime);
	    return false;
	}
    } while(Core::isSignificantlyGreater(startTime, in->startTime(), timeTolerance));


    if (!Core::isAlmostEqual(startTime, in->startTime(), timeTolerance)) {
	lastError_ = Core::form("Input stream has no element with the start-time %f.", startTime);
	return false;
    }

    dataPointer = in;
    return true;
}

bool TimestampCopy::work(const Timestamp &time, DataPointer &dataPointer) {
    if (!nextData(dataPointer)) {
	lastError_ = "input stream endet before target stream";
	return false;
    }
    dataPointer->setTimestamp(time);
    return true;
}

// SynchronizationNode
//////////////////////

namespace Flow {

    Core::ParameterBool paramSynchronizationIgnoreErrors
    ("ignore-errors", "ignore if the synchronization algorithm fails", false);

} // namespace Flow

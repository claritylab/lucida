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
#include "FeatureExtractor.hh"

using namespace Core;
using namespace Flow;

// FeatureExtractor
///////////////////

void FeatureExtractor::processSegment(Bliss::Segment *segment)
{
    std::string frameShift = dataSource()->getAttribute(dataSource()->mainPortId(), "frame-shift");

    if (!frameShift.empty()) {
	Flow::Time tmp = atof(frameShift.c_str());
	if (tmp != frameShift_)
	    log("frame shift changed to %f", tmp);
	frameShift_ = tmp;
    }

    Precursor::processSegment(segment);
}

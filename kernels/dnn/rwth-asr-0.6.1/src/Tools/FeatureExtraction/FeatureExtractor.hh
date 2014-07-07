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
#ifndef _TOOLS_FEATURE_EXTRACTION_FEATURE_EXTRACTOR_HH
#define _TOOLS_FEATURE_EXTRACTION_FEATURE_EXTRACTOR_HH

#include <Speech/DataExtractor.hh>

/**
 * FeatureExtractor: pulls all output out of the network
 */

class FeatureExtractor :
    public Speech::FeatureVectorExtractor
{
    typedef FeatureVectorExtractor Precursor;
protected:
    Flow::Time frameShift_;
public:
    FeatureExtractor(const Core::Configuration &c) :
	Core::Component(c), Precursor(c), frameShift_(0) {}
    virtual ~FeatureExtractor() {}

    Flow::Time frameShift() { return frameShift_; } // current frame-shift in seconds

    virtual  void processSegment(Bliss::Segment *segment);
};

#endif // _TOOLS_FEATURE_EXTRACTION_FEATURE_EXTRACTOR_HH

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
#ifndef _SPEECH_CONFIDENCES_HH
#define _SPEECH_CONFIDENCES_HH

#include <Core/Component.hh>
#include <Core/Parameter.hh>
#include <Flow/Cache.hh>
#include "Alignment.hh"

namespace Bliss {
    class SpeechSegment;
}

namespace Speech {

    /**
     *  used for weighted accumulation
     */
    class Confidences : public Core::Component
    {
	typedef Core::Component Precursor;
    private:
	static const Core::ParameterFloat paramThreshold;
    private:
	const Alignment *alignment_;
	Mm::Weight threshold_;
	struct Statistics { u32 nZero, nOne; } statistics_;
    private:
	void updateStatistics(const Alignment &);
	void dumpStatistics();
    public:
	Confidences(const Core::Configuration &);
	~Confidences();

	void clear();
	void setAlignment(const Alignment *);
	Mm::Weight operator[](TimeframeIndex) const;
	bool isValid() const;
    };

    /**
     *  used for weighted accumulation
     */
    class ConfidenceArchive : public Flow::Cache
    {
	typedef Flow::Cache Precursor;
    public:
	ConfidenceArchive(const Core::Configuration &);
	virtual ~ConfidenceArchive();

	void get(Confidences &, const std::string &);
	void get(Confidences &, Bliss::SpeechSegment *);
    };

}

#endif // _SPEECH_CONFIDENCES_HH

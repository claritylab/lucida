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
#ifndef _FLF_TIMEFRAME_ERROR_HH
#define _FLF_TIMEFRAME_ERROR_HH

#include <Core/Vector.hh>

#include "FlfCore/Lattice.hh"
#include "Network.hh"


namespace Flf {

    class TimeframeError;
    typedef Core::Ref<TimeframeError> TimeframeErrorRef;
    class TimeframeError : public Core::ReferenceCounted {
    public:

    public:
	Score alpha;

	Score nWords;
	Score nFrames;
	Score nFrameErrors;
	Score nSmoothedFrames;
	Score nSmoothedFrameErrors;
    private:
	TimeframeError(Score alpha);
    public:
	void add(const TimeframeError &);
	void add(ConstLatticeRef lHyp, ConstLatticeRef lRef);
	void add(ConstLatticeRef lHyp, ConstPosteriorCnRef cnRef);

	Score error() const;
	Score smoothedError() const;

	// static builder method
	static TimeframeErrorRef create(Score alpha = 0.05);
	static TimeframeErrorRef create(ConstLatticeRef lHyp, ConstLatticeRef lRef, Score alpha = 0.05);
	static TimeframeErrorRef create(ConstLatticeRef lHyp, ConstPosteriorCnRef cnRef, Score alpha = 0.05);
    };
    NodeRef createTimeframeErrorNode(const std::string &name, const Core::Configuration &config);


    NodeRef createMinimumFrameWerDecoderNode(const std::string &name, const Core::Configuration &config);

} // namespace Flf

#endif // _FLF_TIMEFRAME_ERROR_HH

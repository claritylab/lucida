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
#ifndef _FLF_TIMEFRAME_CONFUSION_NETWORK_HH
#define _FLF_TIMEFRAME_CONFUSION_NETWORK_HH

#include "FlfCore/Lattice.hh"
#include "Network.hh"


namespace Flf {

    ConstLatticeRef posteriorCn2lattice(ConstPosteriorCnRef cn);

    ConstLatticeRef decodePosteriorCn(ConstPosteriorCnRef cn);

    void normalizePosteriorCn(PosteriorCn &cn);
    bool isNormalizedPosteriorCn(ConstPosteriorCnRef cn);

    class FramePosteriorCnFeatures;
    typedef Core::Ref<FramePosteriorCnFeatures> FramePosteriorCnFeaturesRef;
    class FramePosteriorCnFeatures : public Core::ReferenceCounted {
    public:
	static const Core::ParameterFloat paramAlpha;
    private:
	class Internal;
	Internal *internal_;
	FramePosteriorCnFeatures(Internal *internal);
    public:
	~FramePosteriorCnFeatures();
	void update(Fsa::LabelId label, Time begin, Time duration);
	/*
	  Confidence a la Frank Wessel
	 */
	Probability confidence() const;
	/*
	  Smoothed, expected time frame error
	  - alpha=0.0 -> unsmoothed
	  - fCN[t]=0.0|1.0 -> (smoothed) time frame error
	*/
	Score error(Score alpha = 0.05) const;
	/*
	  smoothed duration, i.e. equals duration for alpha = 0.0
	*/
	Score norm(Score alpha = 0.05) const;

	/*
	  Static creator function
	*/
	static FramePosteriorCnFeaturesRef create(ConstPosteriorCnRef cn);

	/*
	  Smoothed error
	  - error / (1.0 + alpha * (duration - 1.0))
	  - some experiments with normalization dependency on word label, e.g. words vs. non-words;
	    not implemented yet
	*/
	static Score smooth(Score error, Score duration, Score alpha = 0.05);
    };

    ConstLatticeRef extendByFCnConfidence(ConstLatticeRef l, ConstPosteriorCnRef cn, ScoreId id, RescoreMode rescoreMode = RescoreModeClone);
    // Deprecated, perhaps replaced by "createFramePosteriorCnFeatureNode"
    NodeRef createExtendByPosteriorCnConfidenceNode(const std::string &name, const Core::Configuration &config);

    NodeRef createFramePosteriorCnFeatureNode(const std::string &name, const Core::Configuration &config);

} // namespace Flf

#endif // _FLF_TIMEFRAME_CONFUSION_NETWORK_HH

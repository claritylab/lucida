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
#ifndef _SPEECH_FEATURE_SHIFT_ADAPTOR_HH
#define _SPEECH_FEATURE_SHIFT_ADAPTOR_HH

#include <Core/ReferenceCounting.hh>
#include <Core/IoRef.hh>
#include <Core/ObjectCache.hh>
#include <Flow/Node.hh>
#include <Flow/Vector.hh>
#include <Flow/DataAdaptor.hh>
#include <Mm/MllrAdaptation.hh>
#include "AlignmentNode.hh"

namespace Speech {

    /**
     *  Apply shift adaptor in reverse to a feature stream, using a given
     *  alignment to choose regression class.
     */
    class FeatureShiftAdaptor : public Flow::Node {
    private:
	u32 featureIndex_;
	Flow::DataPtr<Flow::DataAdaptor<Alignment> > alignment_;
    private:
	void updateAlignment(const Flow::Timestamp &);
    protected:
	typedef Core::ObjectCache<Core::MruObjectCacheList<
	    std::string,
	    Core::IoRef<Mm::Adaptor>,
	    Core::StringHash,
	    Core::StringEquality
	> > AdaptorCache;

	typedef Core::ObjectCache<Core::MruObjectCacheList<
	    std::string,
	    Core::IoRef<Mm::AdaptorEstimator>,
	    Core::StringHash,
	    Core::StringEquality
	> > AdaptorEstimatorCache;

	Core::Configuration adaptationConfiguration_;

	AdaptorCache adaptorCache_;
	AdaptorEstimatorCache adaptorEstimatorCache_;

	bool useCorpusKey_;

	Core::Ref<Bliss::CorpusKey> corpusKey_;
	Core::StringHashMap<std::string> corpusKeyMap_;

	std::string currentKey_;
	std::string currentMappedKey_;

	Core::Ref<Am::AdaptationTree> adaptationTree_;

	Core::Ref<const Am::AcousticModel> acousticModel_;

	Core::Ref<Mm::Adaptor> currentAdaptor_;

	void loadCorpusKeyMap();

    public:
	static const Core::ParameterString paramCorpusKeyMap;
	static const Core::ParameterString paramKey;
	static const Core::ParameterBool paramReuseAdaptors;
    public:

	FeatureShiftAdaptor(const Core::Configuration &);
	static std::string filterName() { return "speech-feature-shift-adaptor"; }

	virtual Flow::PortId getInput(const std::string &name) {
	    return name == "alignment" ? 1 : 0; }
	virtual Flow::PortId getOutput(const std::string &name) {
	    return 0; }

	bool setKey(const std::string &key);

	virtual bool configure();
	virtual bool work(Flow::PortId out);
	virtual bool setParameter(const std::string &name, const std::string &value);
    };

} // namespace Speech

#endif // _SPEECH_FEATURE_SHIFT_ADAPTOR_HH

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
#ifndef _AM_ADAPTED_ACOUSTIC_MODEL_HH
#define _AM_ADAPTED_ACOUSTIC_MODEL_HH

#include <Core/ObjectCache.hh>
#include <Core/IoRef.hh>
#include <Am/ClassicAcousticModel.hh>
#include <Mm/MllrAdaptation.hh>
#include <Bliss/CorpusDescription.hh>
#include <Am/AdaptationTree.hh>
#include <Speech/CorpusVisitor.hh>
#include <Bliss/CorpusKey.hh>
#include <Modules.hh>

namespace Am {

    /** AdaptedAcousticModel
     */
    class AdaptedAcousticModel : public ClassicAcousticModel {
	    typedef ClassicAcousticModel Precursor;
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
	Core::Ref<Mm::MixtureSet> adaptMixtureSet_;

	virtual void loadCorpusKeyMap();
	virtual void checkIfModelNeedsUpdate();
    public:
	static const Core::ParameterString paramCorpusKeyMap;
	static const Core::ParameterBool paramReuseAdaptors;

	AdaptedAcousticModel(const Core::Configuration &, Bliss::LexiconRef);
	virtual ~AdaptedAcousticModel();

	virtual Core::Ref<Mm::AbstractMixtureSet> mixtureSet();

	virtual Core::Ref<const Mm::ScaledFeatureScorer> featureScorer();

	virtual void signOn(Speech::CorpusVisitor &corpusVisitor);
	virtual bool setKey(const std::string &key);

    };


} // namespace Am

#endif //_AM_ADAPTED_ACOUSTIC_MODEL_HH

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
#ifndef _SPEECH_KEYED_ESTIMATOR_HH
#define _SPEECH_KEYED_ESTIMATOR_HH

#include <Am/AcousticModel.hh>
#include "AcousticModelTrainer.hh"
#include <Mm/AffineFeatureTransformAccumulator.hh>
#include <Core/ObjectCache.hh>
#include <Core/IoRef.hh>
#include <Mm/AbstractAdaptationAccumulator.hh>
#include <Math/Matrix.hh>
#include <Bliss/CorpusKey.hh>
#include "Feature.hh"

namespace Speech {

    /**
     * KeyedEstimator
     */
    class KeyedEstimator : public AcousticModelTrainer {
	typedef AcousticModelTrainer Precursor;
    public:
	typedef Core::IoRef<Mm::AbstractAdaptationAccumulator> Accumulator;
	typedef Core::ObjectCache< Core::MruObjectCacheList<
	    std::string,
	    Accumulator,
	    Core::StringHash,
	    Core::StringEquality
	> > AccumulatorCache;
	enum Operation {estimate, calculate, combines};
    public:
	static const Core::ParameterFloat paramCombinationWeight;
	static const Core::ParameterInt paramFeatureStream;
	static const Core::ParameterString paramCorpusKeyMap;
    protected:
	Core::Ref<Bliss::CorpusKey> corpusKey_;
	Core::StringHashMap<std::string> corpusKeyMap_;
	size_t featureDimension_;
	size_t modelDimension_;
	size_t featureStream_;
	Operation operation_;
	Accumulator *currentAccumulator_;
	AccumulatorCache accumulatorCache_;
	AccumulatorCache* accumulatorCacheToAdd_;
	Core::Ref<const Mm::AssigningFeatureScorer> assigningFeatureScorer_;
	Core::Ref<Mm::MixtureSet> mixtureSet_;
    protected:
	virtual void createAccumulator(std::string key) = 0;
	void loadMixtureSet();
	void createAssigningFeatureScorer();
	void loadCorpusKeyMap();
    public:
	KeyedEstimator(const Core::Configuration &c, Operation op = estimate);
	virtual ~KeyedEstimator();

	virtual void signOn(CorpusVisitor &corpusVisitor);
	virtual void setFeatureDescription(const Mm::FeatureDescription &);
	virtual void processAlignedFeature(
		Core::Ref<const Feature>, Am::AllophoneStateIndex);
	virtual void processAlignedFeature(
		Core::Ref<const Feature>, Am::AllophoneStateIndex, Mm::Weight);
	virtual void combine();
    };


} // namespace Speech

#endif // _SPEECH_KEYED_ESTIMATOR_HH

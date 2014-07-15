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
#ifndef _SPEECH_LINEAR_FEATURE_TRANSFORM_ESTIMATOR_HH
#define _SPEECH_LINEAR_FEATURE_TRANSFORM_ESTIMATOR_HH

#include <Am/AcousticModel.hh>
#include "AcousticModelTrainer.hh"
#include <Mm/AffineFeatureTransformAccumulator.hh>
#include <Mm/AffineFeatureTransformAccumulator.hh>
#include <Core/ObjectCache.hh>
#include <Math/Matrix.hh>
#include <Bliss/CorpusKey.hh>
#include "Feature.hh"
#include "KeyedEstimator.hh"

namespace Speech {

    /**
     * AffineFeatureTransformEstimator
     */
    class AffineFeatureTransformEstimator : public KeyedEstimator {
	typedef KeyedEstimator Precursor;
	typedef Mm::AffineFeatureTransformAccumulator ConcreteAccumulator;
    public:
	static const Core::ParameterString paramInitialTransform;
	static const Core::ParameterString paramInitialTransformDirectory;
	static const Core::ParameterString paramTransformDirectory;
	static const Core::ParameterInt paramEstimationIterations;
	static const Core::ParameterFloat paramMinObservationWeight;
	static const Core::Choice optimizationCriterionChoice;
	static const Core::ParameterChoice paramOptimizationCriterion;
    protected:
	void createAccumulator(std::string key);
	std::string transformExtension() const { return ".matrix"; }
    public:
	AffineFeatureTransformEstimator(const Core::Configuration &c, Operation op = estimate);
	virtual ~AffineFeatureTransformEstimator();

	void postProcess();
	void scoreTransforms();
    };


} // namespace Speech

#endif // _SPEECH_LINEAR_FEATURE_TRANSFORM_ESTIMATOR_HH

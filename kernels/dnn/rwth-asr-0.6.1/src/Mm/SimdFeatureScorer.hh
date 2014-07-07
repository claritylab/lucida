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
#ifndef _MM_SIMD_FEATURE_SCORER_HH
#define _MM_SIMD_FEATURE_SCORER_HH

#include "AssigningFeatureScorer.hh"
#include "MixtureSet.hh"
#include "IntelOptimization.hh"

namespace Mm {

    /** Accelerated feature-scorer for MixtureSet of Gauss densities with diagonal covariance matrices
     *  Scores are calculated on quantized (8 bit) features with maximum approximation.
     *
     *  Data Structure:
     *    -Precalculated data is stored without tying, i.e. the precalculated means and constant weights
     *     are stored for each density separated in a QuantizedMixtureFeatureScorerElement object.
     *    -The current feature vector is preprocessed by each covariance matrix and the resulting vectors
     *     are stored in the Context object.
     *
     *  Acceleration:
     *    -constants are precalculated for each density
     *    -each mean vector is divided by the variance and quantized beforehand
     *    -feature vectors are divided by the variances and quantized once before the distance calculation
     *    -distance calculation is reduced (in the previos steps) to addition of u8 integers
     *    -addition of u8 integers can be further paralelized using u32 addition
     *
     *  Prerequisite of Acceleration:
     *    -#densities >> #covariances since feature vectors are first divided by all variances
     *     thus eventually also with not necesseary ones.
     */
    class SimdGaussDiagonalMaximumFeatureScorer : public CachedAssigningFeatureScorer {
	typedef CachedAssigningFeatureScorer Precursor;
    public:
	typedef FeatureScorerIntelOptimization FeatureScorerOptimization;
	typedef FeatureScorerOptimization::QuantizedType QuantizedType;
	typedef FeatureScorerOptimization::MixtureElement MixtureElement;
	typedef FeatureScorerOptimization::PreparedFeatureVector PreparedFeatureVector;
    private:
	FeatureScorerOptimization optimization_;

	std::vector<MixtureElement> mixtureTable_;
	std::vector<CovarianceFeatureScorerElement> covarianceTable_;

	FeatureType scalingSquared_;
	FeatureType inverseQuantizationFactor_;
	ComponentIndex dimension_;
    public:
	class Context : public CachedAssigningContextScorer {
	    friend class SimdGaussDiagonalMaximumFeatureScorer;

	    std::vector<PreparedFeatureVector> featureVectorPerCovariance_;

	    Context(const FeatureVector &featureVector,
		    const SimdGaussDiagonalMaximumFeatureScorer *featureScorer,
		    size_t cacheSize);
	};

    /**
     * Returns a scaled and quantized version of the given feature-vector for each covariance-matrix
     */
	std::vector<PreparedFeatureVector> multiplyAndQuantize(const FeatureVector& features) const;

	friend class Context;
    private:
	void buildMixtureTable(const MixtureSet &mixtureSet);

	/** @return quantizedIntervalSize / 125% * intervalSize, where
	 *    quantizedIntervalSize is 255 for the type u8,
	 *    125% is a reserve for extreme values, and
	 *    intervalSize = (maxValue - minValue) is approximated by 2 * max(abs(maxValue), ans(minValue)).
	 *
	 *  For type u8 the final calculation is:
	 *    100.0 / max(abs(maxValue)n, abs(minValue)).
	 */
	static FeatureType quantizationScalingFactor(FeatureType minValue, FeatureType maxValue);

	/** @return is scaling factor for quantization
	 *
	 *  Assumption: mean values of densities represent the interval of feature vector values.
	 *
	 *  Algorithm: collect the maximal and minimal mean value over all densities and all dimensions and
	 *  call the function quantizationScalingFactor which calculates the scaling between this interval
	 *  and quantized interval. (@see quantizationScalingFactor)
	 */
	MeanType getScaling(const MixtureSet &mixtureSet) const;

	ScoreAndBestDensity calculateScoreAndDensity(
	    const CachedAssigningContextScorer *cs, MixtureIndex mixtureIndex) const;

	std::pair<int, DensityIndex> quantizedScore(
	    const MixtureElement& mixture,
	    const std::vector<PreparedFeatureVector> &featuresPerCovariance) const;
    public:
	SimdGaussDiagonalMaximumFeatureScorer(const Core::Configuration &c, Core::Ref<const MixtureSet> mixtureSet);
	virtual ~SimdGaussDiagonalMaximumFeatureScorer() {}

	void init(const MixtureSet &mixtureSet);

	virtual MixtureIndex nMixtures() const { return mixtureTable_.size(); }
	virtual ComponentIndex dimension() const { return dimension_; }

	virtual AssigningScorer getAssigningScorer(const FeatureVector &featureVector) const {
	    return AssigningScorer(new Context(featureVector, this, nMixtures()));
	}
	/**
	 * Returns the factor which transforms a quantized score (see quantizedScore()) into the real score
	 * (see calculateScoreAndDensity()).
	 *
	 * */
	FeatureType inverseQuantizationFactor() const {
	    return inverseQuantizationFactor_;
	}
    };

} //namespace Mm

#endif //_MM_SIMD_FEATURE_SCORER_HH

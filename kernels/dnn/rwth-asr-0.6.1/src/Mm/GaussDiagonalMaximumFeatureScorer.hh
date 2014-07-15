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
#ifndef _MM_GAUSS_DIAGONAL_MAXIMUM_FEATURE_SCORER_HH
#define _MM_GAUSS_DIAGONAL_MAXIMUM_FEATURE_SCORER_HH

#include "AssigningFeatureScorer.hh"
#include "MixtureSet.hh"
#include "MixtureFeatureScorerElement.hh"
#include "CovarianceFeatureScorerElement.hh"

namespace Mm {

    /** Feature-scorer for MixtureSet of Gauss densities with diagonal covariance matrices
     *  Scores are calculated with maximum approximation.
     */
    class GaussDiagonalMaximumFeatureScorer : public CachedAssigningFeatureScorer {
    public:
	typedef CachedAssigningFeatureScorer Precursor;
    protected:
	class Context : public CachedAssigningContextScorer {
	public:
	    FeatureVector featureVector_;
	    Context(const FeatureVector &featureVector,
		    const GaussDiagonalMaximumFeatureScorer *featureScorer,
		    size_t cacheSize);
	};
    protected:
	static const Core::ParameterFloat paramMixtureWeightScale;
	static const Core::ParameterFloat paramGaussianScale;

	Score mixtureWeightScale_;
	Score gaussianScale_;
    protected:
	std::vector<MixtureFeatureScorerElement> mixtureTable_;
	std::vector<GaussDensity> densityTable_;
	std::vector<Mean> meanTable_;
	std::vector<CovarianceFeatureScorerElement> covarianceTable_;

	ComponentIndex dimension_;

    protected:
	Score distance(const std::vector<FeatureType> &feature,
		       const std::vector<MeanType> &mean,
		       const std::vector<VarianceType> &inverseSquareRootVar) const;


	virtual ScoreAndBestDensity calculateScoreAndDensity(const CachedAssigningContextScorer *cs, MixtureIndex mixtureIndex) const;


    public:
	GaussDiagonalMaximumFeatureScorer(const Core::Configuration &c, Core::Ref<const MixtureSet> mixtureSet);
	GaussDiagonalMaximumFeatureScorer(const Core::Configuration &c);

	void init(const MixtureSet &mixtureSet);


	virtual AssigningScorer getAssigningScorer(const FeatureVector &featureVector) const {
	    return AssigningScorer(new Context(featureVector, this, nMixtures()));
	}

	virtual MixtureIndex nMixtures() const { return mixtureTable_.size(); }
	virtual ComponentIndex dimension() const { return dimension_; }
	virtual DensityIndex nDensities() const { return densityTable_.size(); }
	virtual const MixtureFeatureScorerElement& mixture(MixtureIndex mix) const { return mixtureTable_[mix]; }
	virtual const std::vector<DensityIndex>& densitiesInMixture(MixtureIndex mix) const {
	    return mixtureTable_[mix].densityIndices();
	}

	// virtual Score calculateScore(const CachedAssigningContextScorer*, MixtureIndex, DensityIndex) const;

	// debug
	void outputMeanVectors() const;
    };


    /** Feature-scorer for MixtureSet of Gauss densities with diagonal covariance matrices
     *  The maximum approximation is not applied here.
     */
    class GaussDiagonalSumFeatureScorer : public GaussDiagonalMaximumFeatureScorer {
    public:
	typedef GaussDiagonalMaximumFeatureScorer Precursor;
    protected:
	mutable const CachedAssigningContextScorer *lastContext_;
	mutable MixtureIndex lastMixtureIndex_;
	mutable Score *scores_;     /**< array to cache scores */
	mutable size_t nDensities_; /**< cache for number of densities */
	void calculateScoresAndNumberOfDensities(
	    const CachedAssigningContextScorer *cs, MixtureIndex mixtureIndex) const;
	size_t maximumNumberOfDensities() const;
    public:
	GaussDiagonalSumFeatureScorer(const Core::Configuration &c, Core::Ref<const MixtureSet> mixtureSet);

	virtual ScoreAndBestDensity calculateScoreAndDensity(const CachedAssigningContextScorer *cs, MixtureIndex mixtureIndex) const;
	virtual void calculateDensityPosteriorProbabilities(const CachedAssigningContextScorer*, Score denominator,
							    EmissionIndex e, std::vector<Mm::Weight> &result) const;
    };

} //namespace Mm

#endif //_MM_GAUSS_DIAGONAL_MAXIMUM_FEATURE_SCORER_HH

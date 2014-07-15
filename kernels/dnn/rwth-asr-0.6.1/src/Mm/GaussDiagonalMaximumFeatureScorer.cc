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
#include "GaussDiagonalMaximumFeatureScorer.hh"

using namespace Mm;

//GaussDiagonalMaximumFeatureScorer::Context
////////////////////////////////////////////


GaussDiagonalMaximumFeatureScorer::Context::Context(const FeatureVector &featureVector,
													const GaussDiagonalMaximumFeatureScorer *featureScorer,
													size_t cacheSize) :
    CachedAssigningContextScorer(featureScorer, cacheSize),
    featureVector_(featureVector)
{
    require(featureVector.size() == featureScorer->dimension());
}


// GaussDiagonalMaximumFeatureScorer
////////////////////////////////////

const Core::ParameterFloat GaussDiagonalMaximumFeatureScorer::paramMixtureWeightScale(
    "mixture-weight-scale", "scaling of the logarithmized mixture weights", 1.0,
    Core::Type<Score>::epsilon);

const Core::ParameterFloat GaussDiagonalMaximumFeatureScorer::paramGaussianScale(
    "gaussian-scale", "scaling of the logarithmized gaussian probability", 1.0,
    Core::Type<Score>::epsilon);

GaussDiagonalMaximumFeatureScorer::GaussDiagonalMaximumFeatureScorer(
    const Core::Configuration &c,
    Core::Ref<const MixtureSet> mixtureSet)  :
    Core::Component(c),
    Precursor(c),
    mixtureWeightScale_(paramMixtureWeightScale(c)),
    gaussianScale_(std::sqrt(paramGaussianScale(c))),
    dimension_(0)
{
    init(*mixtureSet);
}

GaussDiagonalMaximumFeatureScorer::GaussDiagonalMaximumFeatureScorer(const Core::Configuration &c) :
    Core::Component(c),
    Precursor(c),
    mixtureWeightScale_(paramMixtureWeightScale(c)),
    gaussianScale_(std::sqrt(paramGaussianScale(c))),
    dimension_(0)
{}

void GaussDiagonalMaximumFeatureScorer::init(const MixtureSet &mixtureSet)
{
    dimension_ = mixtureSet.dimension();

    mixtureTable_.resize(mixtureSet.nMixtures());
    for(size_t i = 0; i < mixtureTable_.size(); ++ i) {
		mixtureTable_[i] = *mixtureSet.mixture(i);
		mixtureTable_[i].scale(mixtureWeightScale_);
    }

    densityTable_.resize(mixtureSet.nDensities());
    for(size_t i = 0; i < densityTable_.size(); ++ i)
		densityTable_[i] = *mixtureSet.density(i);

    meanTable_.resize(mixtureSet.nMeans());
    for(size_t i = 0; i < meanTable_.size(); ++ i)
		meanTable_[i] = *mixtureSet.mean(i);

    covarianceTable_.resize(mixtureSet.nCovariances());
    for(size_t i = 0; i < covarianceTable_.size(); ++ i) {
		covarianceTable_[i] = *mixtureSet.covariance(i);
		covarianceTable_[i].scale(gaussianScale_);
    }

}

//debug
void GaussDiagonalMaximumFeatureScorer::outputMeanVectors() const {
	// pure debug method ! delete me after the bug is exterminated

	u32 nMixtures = mixtureTable_.size();

	for( u32 i = 0; i < nMixtures ; ++i){
		// get mixture and number of densities
		const MixtureFeatureScorerElement &mixture = mixtureTable_[i];
		size_t nDensities = mixture.nDensities();
		std::cout << "Mixture Number " << i << std::endl;

		for(u32 dns = 0; dns < nDensities ; ++dns){
			std::cout << "Density " << dns << std::endl;
			// get density
			const GaussDensity &density = densityTable_[mixture.densityIndex(dns)];
			const std::vector<MeanType> &mean = meanTable_[density.meanIndex()];

			for( u32 j = 0; j < mean.size() ; ++j){
				std::cout << mean[j] << " ";
			}
			std::cout << std::endl;
		}
		std::cout << "////////////////////" << std::endl;
	}
}
//end debug


AssigningFeatureScorer::ScoreAndBestDensity GaussDiagonalMaximumFeatureScorer::calculateScoreAndDensity(
    const CachedAssigningContextScorer* cs, MixtureIndex mixtureIndex) const
{
    const Context *c = required_cast(const Context*, cs);
    Score bestScore = Core::Type<Score>::max;
    size_t bestDensity = Core::Type<size_t>::max;
    const MixtureFeatureScorerElement &mixture = mixtureTable_[mixtureIndex];
    size_t nDensities = mixture.nDensities();
    for(size_t dns = 0; dns < nDensities; ++ dns) {
	const GaussDensity &density = densityTable_[mixture.densityIndex(dns)];
	const CovarianceFeatureScorerElement &covariance = covarianceTable_[density.covarianceIndex()];

	// code is architecture dependent without (f64)
	// this can lead to unwanted behavior
	f64 score = (f64)mixture.minus2LogWeights()[dns] +
		(f64)covariance.logNormalizationFactor() +
		(f64)distance(c->featureVector_, meanTable_[density.meanIndex()],
			covariance.inverseSquareRootDiagonal());
	if (bestScore > score) {
	    bestScore = score;
	    bestDensity = dns;
	}
    }
    ScoreAndBestDensity result;
    result.score = 0.5 * bestScore;
    result.bestDensity = bestDensity;
    return result;
}


Score GaussDiagonalMaximumFeatureScorer::distance(
    const std::vector<FeatureType> &feature,
    const std::vector<MeanType> &mean,
    const std::vector<VarianceType> &inverseSquareRootVar) const
{
    ComponentIndex cmp = 0;
    ComponentIndex dimension = feature.size();
    Score result = 0;
    Score df;

    switch ((dimension - 1) % 8) {
	while (cmp < dimension) {
	    case 7: df = (mean[cmp] - feature[cmp]) * inverseSquareRootVar[cmp]; result += df * df; ++ cmp;
	    case 6: df = (mean[cmp] - feature[cmp]) * inverseSquareRootVar[cmp]; result += df * df; ++ cmp;
	    case 5: df = (mean[cmp] - feature[cmp]) * inverseSquareRootVar[cmp]; result += df * df; ++ cmp;
	    case 4: df = (mean[cmp] - feature[cmp]) * inverseSquareRootVar[cmp]; result += df * df; ++ cmp;
	    case 3: df = (mean[cmp] - feature[cmp]) * inverseSquareRootVar[cmp]; result += df * df; ++ cmp;
	    case 2: df = (mean[cmp] - feature[cmp]) * inverseSquareRootVar[cmp]; result += df * df; ++ cmp;
	    case 1: df = (mean[cmp] - feature[cmp]) * inverseSquareRootVar[cmp]; result += df * df; ++ cmp;
	    case 0: df = (mean[cmp] - feature[cmp]) * inverseSquareRootVar[cmp]; result += df * df; ++ cmp;
	}
    }
    return result;
}


// ================================================================================


GaussDiagonalSumFeatureScorer::GaussDiagonalSumFeatureScorer(
    const Core::Configuration &c,
    Core::Ref<const MixtureSet> mixtureSet)
    :
    Core::Component(c),
    Precursor(c, mixtureSet),
    scores_(new Score[maximumNumberOfDensities()])
{
    // optionally: load lookup table for exp/log
}

size_t GaussDiagonalSumFeatureScorer::maximumNumberOfDensities() const {
    size_t result = 0;
    for(size_t m = 0; m < mixtureTable_.size(); ++ m) {
		const MixtureFeatureScorerElement &mixture = mixtureTable_[m];
		result = std::max(result, mixture.nDensities());
    }
    return result;
}

void GaussDiagonalSumFeatureScorer::calculateScoresAndNumberOfDensities(
    const CachedAssigningContextScorer* cs, MixtureIndex mixtureIndex) const
{
    // check, if we must recompute scores_ and nDensities_
    if (mixtureIndex == lastMixtureIndex_  && cs == lastContext_)
		return;
    lastContext_ = cs;
    lastMixtureIndex_ = mixtureIndex;
    const Context *c = required_cast(const Context*, cs);

    const MixtureFeatureScorerElement &mixture = mixtureTable_[mixtureIndex];
    nDensities_ = mixture.nDensities();

    for(size_t dns = 0; dns < nDensities_; ++ dns) {
		const GaussDensity &density = densityTable_[mixture.densityIndex(dns)];
		const CovarianceFeatureScorerElement &covariance = covarianceTable_[density.covarianceIndex()];

		Score score = mixture.minus2LogWeights()[dns] +
			covariance.logNormalizationFactor() +
			distance(c->featureVector_, meanTable_[density.meanIndex()],
					 covariance.inverseSquareRootDiagonal());

		scores_[dns] = 0.5*score;
    }
}


AssigningFeatureScorer::ScoreAndBestDensity GaussDiagonalSumFeatureScorer::calculateScoreAndDensity(
    const CachedAssigningContextScorer* cs, MixtureIndex mixtureIndex) const
{
    // 1.) calculate best (= viterbi) score
    Score bestScore = Core::Type<Score>::max;
    size_t bestDensity = Core::Type<size_t>::max;

    calculateScoresAndNumberOfDensities(cs, mixtureIndex);
    for(size_t dns = 0; dns < nDensities_; ++ dns) {
		Score score = scores_[dns];
		if (bestScore > score) {
			bestScore = score;
			bestDensity = dns;
		}
    }

    // 2.) compute sum of exponentiated truncated scores
    Score sumExp = 0;
    for (size_t dns = 0; dns < nDensities_; ++ dns) {
		sumExp += std::exp(bestScore - scores_[dns]);
    }

    // 3.) subtract from viterbi score
    ScoreAndBestDensity result;
    result.score = bestScore - std::log(sumExp);
    result.bestDensity = bestDensity;
    return result;
}

void GaussDiagonalSumFeatureScorer::calculateDensityPosteriorProbabilities(
    const CachedAssigningContextScorer* cs, Score logDenominator,
    EmissionIndex e, std::vector<Mm::Weight> &result) const
{
    calculateScoresAndNumberOfDensities(cs, e);
    result.resize(nDensities_);
    for (size_t dns = 0; dns < nDensities_; ++ dns) {
		result[dns] = std::exp(logDenominator - scores_[dns]);
    }
}

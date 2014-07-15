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
#include "SimdFeatureScorer.hh"

using namespace Mm;

// SimdGaussDiagonalMaximumFeatureScorer::Context
/////////////////////////////////////////////////

SimdGaussDiagonalMaximumFeatureScorer::Context::Context(
    const FeatureVector &featureVector,
    const SimdGaussDiagonalMaximumFeatureScorer *featureScorer,
    size_t cacheSize) :
    CachedAssigningContextScorer(featureScorer, cacheSize)
{
    require(featureVector.size() == featureScorer->dimension());

    featureVectorPerCovariance_.resize(featureScorer->covarianceTable_.size());
    for(u32 c = 0; c < featureVectorPerCovariance_.size(); ++ c) {
	FeatureScorerOptimization::multiplyAndQuantize(
	    featureVector,
	    featureScorer->covarianceTable_[c].inverseSquareRootDiagonal(),
	    featureVectorPerCovariance_[c]);
    }
}

std::vector<SimdGaussDiagonalMaximumFeatureScorer::PreparedFeatureVector> SimdGaussDiagonalMaximumFeatureScorer::multiplyAndQuantize(const Mm::FeatureVector& featureVector) const
{
    require(featureVector.size() == dimension());

    std::vector<SimdGaussDiagonalMaximumFeatureScorer::PreparedFeatureVector> ret;
    ret.resize(covarianceTable_.size());

    for(u32 c = 0; c < ret.size(); ++ c) {
	FeatureScorerOptimization::multiplyAndQuantize(
	    featureVector,
	    covarianceTable_[c].inverseSquareRootDiagonal(),
	    ret[c]);
    }

    return ret;
}

// SimdGaussDiagonalMaximumFeatureScorer
////////////////////////////////////////

SimdGaussDiagonalMaximumFeatureScorer::SimdGaussDiagonalMaximumFeatureScorer(
    const Core::Configuration &c, Core::Ref<const MixtureSet> mixtureSet) :
    Core::Component(c),
    Precursor(c),
    optimization_(select("code"), mixtureSet->dimension())
{
    init(*mixtureSet);
}

void SimdGaussDiagonalMaximumFeatureScorer::init(const MixtureSet &mixtureSet)
{
    covarianceTable_.resize(mixtureSet.nCovariances());
    for(size_t i = 0; i < covarianceTable_.size(); ++ i)
	covarianceTable_[i] = *mixtureSet.covariance(i);

    MeanType scaling = getScaling(mixtureSet);
    log("Scaling factor: ") << scaling;

    scalingSquared_ = scaling * scaling;
    inverseQuantizationFactor_ = 0.5 / scalingSquared_;

    for(u32 i = 0; i < covarianceTable_.size(); ++ i)
	covarianceTable_[i].scale(scaling);

    buildMixtureTable(mixtureSet);
}

void SimdGaussDiagonalMaximumFeatureScorer::buildMixtureTable(const MixtureSet &mixtureSet)
{
    dimension_ = mixtureSet.dimension();

    // create quantized mixture set
    mixtureTable_.resize(mixtureSet.nMixtures());
    for (MixtureIndex mix = 0; mix < mixtureTable_.size(); ++ mix) {

	const Mixture *mixture = mixtureSet.mixture(mix);
	MixtureElement &quantizedMixture = mixtureTable_[mix];

	quantizedMixture.setNumberOfDensities(mixture->nDensities());
	for (size_t dns = 0; dns < mixture->nDensities(); ++ dns) {
	    const GaussDensity* density = mixtureSet.density(mixture->densityIndex(dns));

	    quantizedMixture.density(dns).covarianceIndex_ = density->covarianceIndex();

	    Weight scaledMinus2LogWeight = scalingSquared_ * -2 * mixture->logWeight(dns);

	    FeatureScorerOptimization::createDensityElement(scaledMinus2LogWeight,
							    *mixtureSet.mean(density->meanIndex()),
							    covarianceTable_[density->covarianceIndex()],
							    quantizedMixture.density(dns));
	}
    }
}

MeanType SimdGaussDiagonalMaximumFeatureScorer::getScaling(const MixtureSet &mixtureSet) const
{
    MeanType minMean = Core::Type<MeanType>::max;
    MeanType maxMean = Core::Type<MeanType>::min;

    for (DensityIndex i = 0; i < mixtureSet.nDensities(); ++ i) {
	const GaussDensity *density = mixtureSet.density(i);
	const std::vector<MeanType> *mean = mixtureSet.mean(density->meanIndex());
	const std::vector<VarianceType>& inverseSquareRootVariance =
	    covarianceTable_[density->covarianceIndex()].inverseSquareRootDiagonal();

	verify(mean->size() == inverseSquareRootVariance.size());

	for (ComponentIndex cmp = 0; cmp < mean->size(); ++ cmp) {
	    MeanType dividedMean = (*mean)[cmp] * inverseSquareRootVariance[cmp];
	    minMean = std::min(minMean, dividedMean);
	    maxMean = std::max(maxMean, dividedMean);
	}
    }
    log() << "Mean bounds: min= " << minMean << ", max= " << maxMean;
    return quantizationScalingFactor(minMean, maxMean);
}

FeatureType SimdGaussDiagonalMaximumFeatureScorer::quantizationScalingFactor(
    FeatureType minValue, FeatureType maxValue)
{
    int quantizedIntervalSize =
	(int)Core::Type<QuantizedType>::max - (int)Core::Type<QuantizedType>::min;
    FeatureType intervalSize = 2 * std::max(Core::abs(minValue), Core::abs(maxValue));
    return (FeatureType)quantizedIntervalSize / (1.25 * intervalSize);
}

AssigningFeatureScorer::ScoreAndBestDensity
SimdGaussDiagonalMaximumFeatureScorer::calculateScoreAndDensity(
    const CachedAssigningContextScorer *cs, MixtureIndex mixtureIndex) const
{
    const Context *c = required_cast(const Context*, cs);

    std::pair<int, size_t> quantizedResult =
	quantizedScore(mixtureTable_[mixtureIndex], c->featureVectorPerCovariance_);

    ScoreAndBestDensity result;
    result.score = 0.5 * quantizedResult.first / scalingSquared_;
    result.bestDensity = quantizedResult.second;
    return result;
}

/**
 * correctness:
 * Do not use floating point operations after optimization_.distance(),
 * but before calling optimization_.resetFloatingPointCalculation(), because
 * this may result in incorrect behaviour.
 *
 * efficiency:
 * For the sake of some processor types which can not switch quickly
 * from optimized integer modus to floating point modus,
 * use INTEGER OPERATIONS ONLY in this function!
 */
std::pair<int, DensityIndex> SimdGaussDiagonalMaximumFeatureScorer::quantizedScore(
    const MixtureElement& mixture,
    const std::vector<PreparedFeatureVector> &featuresPerCovariance) const
{
    int minScore = Core::Type<int>::max;
    size_t bestDensity = Core::Type<size_t>::max;

    for (size_t dns = 0; dns < mixture.nDensities(); ++ dns) {
	const MixtureElement::Density &density = mixture.density(dns);

	int score = density.constantWeight_ +
	    optimization_.distance(density.preparedMean_,
				   featuresPerCovariance[density.covarianceIndex_]);

	if (score < minScore) {
	    minScore = score;
	    bestDensity = dns;
	}
    }
    optimization_.resetFloatingPointCalculation();
    return std::make_pair(minScore, bestDensity);
}

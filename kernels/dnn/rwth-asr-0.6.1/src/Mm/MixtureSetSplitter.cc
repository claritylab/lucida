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
#include "MixtureSetSplitter.hh"
#include <Core/Types.hh>

using namespace Mm;

const Core::ParameterFloat MixtureSetSplitter::paramMinMeanObservationWeight(
    "minimum-mean-observation-weight", "only means with more observation-weight get splitted", 20, 0);
const Core::ParameterFloat MixtureSetSplitter::paramMinCovarianceObservationWeight(
    "minimum-covariance-observation-weight", "only covariances with more observation-weight get splitted",
    Core::Type<f32>::max, 0);
const Core::ParameterFloat MixtureSetSplitter::paramPerturbationWeight(
    "perturbation-weight", "empirical weight to optimize the epsilon for splitting means", 0.1, 0);
const Core::ParameterBool MixtureSetSplitter::paramNormalizeMixtureWeights(
    "normalize-mixture-weights", "normalize mixture weights after split, not required in the Viterbi approximation ", false);

MixtureSetSplitter::MixtureSetSplitter(const Core::Configuration &configuration) :
    Precursor(configuration),
    minMeanObservationWeight_(paramMinMeanObservationWeight(configuration)),
    perturbationWeight_(paramPerturbationWeight(configuration)),
    minCovarianceObservationWeight_(paramMinCovarianceObservationWeight(configuration)),
    normalizeMixtureWeights_(paramNormalizeMixtureWeights(configuration))
{}

Core::Ref<MixtureSet> MixtureSetSplitter::split(AbstractMixtureSetEstimator &estimator)
{
    std::vector<Weight> meanObservationWeight;
    std::vector<Weight> covarianceObservationWeight;
    Core::Ref<MixtureSet> result(new MixtureSet);

    estimator.estimate(*result, meanObservationWeight, covarianceObservationWeight);
    splitMeans(*result, meanObservationWeight);
    splitCovariances(*result, covarianceObservationWeight);
    splitDensities(*result);
    splitDensitiesInMixtures(*result);
    return result;
}

void MixtureSetSplitter::splitMeans(MixtureSet &mixtureSet,
				    const std::vector<Weight>& weights)
{
    splittedMeans_.resize(mixtureSet.nMeans());
    splittedDensities_.resize(mixtureSet.nDensities());
    for(DensityIndex densityIndex = 0; densityIndex < splittedDensities_.size(); ++ densityIndex) {
	const GaussDensity* densityToSplit = mixtureSet.density(densityIndex);
	MeanIndex meanIndex = densityToSplit->meanIndex();
	CovarianceIndex covarianceIndex = densityToSplit->covarianceIndex();

	std::vector<MeanType> perturbation = mixtureSet.covariance(covarianceIndex)->diagonal();
	for(std::vector<MeanType>::iterator elem = perturbation.begin(); elem != perturbation.end(); ++ elem) {
	    *elem = std::sqrt(*elem) * perturbationWeight_ * Core::Type<f32>::epsilon;
	}

	splittedMeans_[meanIndex] = (weights[meanIndex] > minMeanObservationWeight_ ? splitMean(mixtureSet, meanIndex, perturbation) : meanIndex);
    }
}

MeanIndex MixtureSetSplitter::splitMean(MixtureSet &mixtureSet, MeanIndex index, std::vector<MeanType> &perturbation)
{
    Mean *toSplit = mixtureSet.mean(index);
    Mean *newMean = new Mean(toSplit->size());

    std::transform(toSplit->begin(), toSplit->end(), perturbation.begin(),
		   newMean->begin(), std::minus<MeanType>());
    std::transform(toSplit->begin(), toSplit->end(), perturbation.begin(),
		   toSplit->begin(), std::plus<MeanType>());
    return mixtureSet.addMean(newMean);
}

void MixtureSetSplitter::splitCovariances(MixtureSet &mixtureSet,
					  const std::vector<Weight>& weights)
{
    splittedCovariances_.resize(mixtureSet.nCovariances());
    for(CovarianceIndex i = 0; i < splittedCovariances_.size(); ++ i) {
	splittedCovariances_[i] = (weights[i] > minCovarianceObservationWeight_ ?
				   splitCovariance(mixtureSet, i) : i);
    }
}

CovarianceIndex MixtureSetSplitter::splitCovariance(MixtureSet &mixtureSet, CovarianceIndex index)
{
    return mixtureSet.addCovariance(mixtureSet.covariance(index)->clone());
}

void MixtureSetSplitter::splitDensities(MixtureSet &mixtureSet)
{
    splittedDensities_.resize(mixtureSet.nDensities());
    for(DensityIndex i = 0; i < splittedDensities_.size(); ++ i) {
	const GaussDensity* toSplit = mixtureSet.density(i);
	MeanIndex splittedMeanIndex = splittedMeans_[toSplit->meanIndex()];
	CovarianceIndex splittedCovarianceIndex = splittedCovariances_[toSplit->covarianceIndex()];

	if (splittedMeanIndex != toSplit->meanIndex() ||
	    splittedCovarianceIndex != toSplit->covarianceIndex()) {
	    splittedDensities_[i] = mixtureSet.addDensity(
		new GaussDensity(splittedMeanIndex, splittedCovarianceIndex));
	} else
	    splittedDensities_[i] = i;
    }
}

void MixtureSetSplitter::splitDensitiesInMixtures(MixtureSet &mixtureSet)
{
    for(MixtureIndex i = 0; i < mixtureSet.nMixtures(); ++ i) {
	Mixture *mixture = mixtureSet.mixture(i);
	size_t nDensitiesToSplit = mixture->nDensities();

	for(size_t dns = 0; dns < nDensitiesToSplit; ++ dns) {
	    DensityIndex splittedDensityIndex = splittedDensities_[mixture->densityIndex(dns)];
	    if (splittedDensityIndex != mixture->densityIndex(dns)) {
		mixture->addLogDensity(splittedDensityIndex, mixture->logWeight(dns));
	    }
	}
	if (normalizeMixtureWeights_) {
	    mixture->normalizeWeights();
	}
    }
}

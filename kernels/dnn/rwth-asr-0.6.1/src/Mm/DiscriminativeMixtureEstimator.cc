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
#include <Core/Application.hh>
#include "DiscriminativeMixtureEstimator.hh"
#include "MixtureSet.hh"

using namespace Mm;

/**
 *  DiscriminativeMixtureEstimator
 */
DiscriminativeMixtureEstimator::DiscriminativeMixtureEstimator(
    const Core::Configuration &c)
{}

DiscriminativeMixtureEstimator::~DiscriminativeMixtureEstimator()
{}

void DiscriminativeMixtureEstimator::removeDensity(DensityIndex indexInMixture)
{
    verify(previousMixtureWeights_.size() == nDensities());
    Precursor::removeDensity(indexInMixture);
    previousMixtureWeights_.erase(previousMixtureWeights_.begin() + indexInMixture);
}

void DiscriminativeMixtureEstimator::removeDensitiesWithLowWeight(
    Weight minObservationWeight, Weight minRelativeWeight)
{
    DensityIndex densityMax = densityIndexWithMaxWeight();
    Weight minWeight = minObservationWeight;
    for (DensityIndex dns = 0; dns < densityEstimators_.size(); ) {
	bool hasEnoughWeight = weights_[dns] >= minWeight;
	bool hasEnoughRelativeWeight = previousMixtureWeight(dns) >= minRelativeWeight;
	bool isDeletable = dns != densityMax;
	if ((!hasEnoughWeight || !hasEnoughRelativeWeight) && isDeletable) {
	    removeDensity(dns);
	    -- densityMax;
	} else {
	    ++ dns;
	}
    }
}

void DiscriminativeMixtureEstimator::accumulate(const AbstractMixtureEstimator &toAdd)
{
    require(nDensities() == toAdd.nDensities());
    const DiscriminativeMixtureEstimator *_toAdd =
	required_cast(const DiscriminativeMixtureEstimator *, &toAdd);
    std::transform(weights_.begin(), weights_.end(),
		   _toAdd->weights_.begin(), weights_.begin(), std::plus<Weight>());
}

void DiscriminativeMixtureEstimator::accumulateDenominator(
    DensityIndex indexInMixture, const FeatureVector &featureVector, Weight weight)
{
    require_(0 <= indexInMixture && indexInMixture < nDensities());
    weights_[indexInMixture] -= weight;
    required_cast(DiscriminativeGaussDensityEstimator*,
		  densityEstimators_[indexInMixture].get())->accumulateDenominator(
		      featureVector, weight);
}

Weight DiscriminativeMixtureEstimator::logPreviousMixtureWeight(DensityIndex dns) const
{
    const Weight _previousMixtureWeight =
	std::max(previousMixtureWeight(dns), Core::Type<Weight>::delta);
    if (_previousMixtureWeight != previousMixtureWeight(dns)) {
	Core::Application::us()->log("previous-mixture-weight[")
	    << dns << "]=" << previousMixtureWeight(dns) << " floored for logarithm";
    }
    return std::log(_previousMixtureWeight);
}

/**
 *  @param mixture: previous mixture
 */
void DiscriminativeMixtureEstimator::setPreviousMixture(const Mixture *mixture)
{
    require(mixture && mixture->nDensities() == nDensities());
    previousMixtureWeights_.resize(nDensities());
    for (DensityIndex dnsInMix = 0; dnsInMix < previousMixtureWeights_.size(); ++ dnsInMix) {
	previousMixtureWeights_[dnsInMix] = mixture->weight(dnsInMix);
    }
}

DensityIndex DiscriminativeMixtureEstimator::accumulate(
    Core::BinaryInputStreams &is,
    Core::BinaryOutputStream &os)
{
    return Precursor::accumulate(is, os);
}

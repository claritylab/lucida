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
#include "RpropDiscriminativeMixtureEstimator.hh"
#include "MixtureSet.hh"

using namespace Mm;

/**
 * RpropDiscriminativeMixtureEstimator: Resilient Backpropagation (Rprop)
 */
RpropDiscriminativeMixtureEstimator::RpropDiscriminativeMixtureEstimator(
    const Core::Configuration &c)
    :
    Precursor(c)
{}

void RpropDiscriminativeMixtureEstimator::removeDensity(DensityIndex indexInMixture)
{
    Precursor::removeDensity(indexInMixture);
    previousToPrevious_.erase(previousToPrevious_.begin() + indexInMixture);
    stepSizes_.erase(stepSizes_.begin() + indexInMixture);
}

Mixture* RpropDiscriminativeMixtureEstimator::estimate(
    const ReferenceIndexMap<GaussDensityEstimator> &densityMap,
    bool normalizeWeights)
{
    std::vector<Weight> logMixtureWeights(nDensities());
    for (DensityIndex dns = 0; dns < nDensities(); ++ dns) {
	logMixtureWeights[dns] = logPreviousMixtureWeight(dns);
    }
    Rprop::apply(logMixtureWeights);
    Mixture *result = new Mixture;
    for (DensityIndex dns = 0; dns < densityEstimators_.size(); ++ dns) {
	result->addLogDensity(
	    densityMap[densityEstimators_[dns]],
	    logMixtureWeights[dns]);
    }
    if (normalizeWeights) {
	result->normalizeWeights();
    }
    return result;
}

DensityIndex RpropDiscriminativeMixtureEstimator::accumulate(
    Core::BinaryInputStreams &is,
    Core::BinaryOutputStream &os)
{
    return Precursor::accumulate(is, os);
}

void RpropDiscriminativeMixtureEstimator::setStepSizes(
    const Mixture *mixture)
{
    require(mixture and mixture->nDensities() == nDensities());
    Rprop::setStepSizes(mixture->logWeights());
}

void RpropDiscriminativeMixtureEstimator::setStepSizes(
    Weight stepSize)
{
    Rprop::setStepSizes(nDensities(), stepSize);
}

Mixture* RpropDiscriminativeMixtureEstimator::collectStepSizes(
    const ReferenceIndexMap<GaussDensityEstimator> &densityMap)
{
    Mixture *result = new Mixture;
    for (DensityIndex dns = 0; dns < densityEstimators_.size(); ++ dns) {
	result->addLogDensity(densityMap[densityEstimators_[dns]], Rprop::stepSizes()[dns]);
    }
    return result;
}

void RpropDiscriminativeMixtureEstimator::setPreviousToPreviousMixture(
    const Mixture *mixture)
{
    require(mixture and mixture->nDensities() == nDensities());
    Rprop::setPreviousToPrevious(mixture->logWeights());
}

/**
 * DiscriminativeMixtureEstimatorWithISmoothing: Rprop
 */
RpropDiscriminativeMixtureEstimatorWithISmoothing::RpropDiscriminativeMixtureEstimatorWithISmoothing(
    const Core::Configuration &c)
    :
    Precursor(c)
{
    ISmoothing::set(this);
}

RpropDiscriminativeMixtureEstimatorWithISmoothing::~RpropDiscriminativeMixtureEstimatorWithISmoothing()
{}

void RpropDiscriminativeMixtureEstimatorWithISmoothing::removeDensity(DensityIndex indexInMixture)
{
    Precursor::removeDensity(indexInMixture);
    ISmoothing::removeDensity(indexInMixture);
}

void RpropDiscriminativeMixtureEstimatorWithISmoothing::clear()
{
    Precursor::clear();
    ISmoothing::clear();
}

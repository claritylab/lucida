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
#include "DiscriminativeGaussDensityEstimator.hh"
#include "MixtureSet.hh"
#include "Utilities.hh"

using namespace Mm;

/**
 * DiscriminativeGaussDensityEstimator
 */
void DiscriminativeGaussDensityEstimator::accumulateDenominator(
    const FeatureVector &featureVector, Weight weight)
{
    required_cast(DiscriminativeMeanEstimator*,
		  meanEstimator_.get())->accumulateDenominator(
		      featureVector, weight);
    required_cast(DiscriminativeCovarianceEstimator*,
		  covarianceEstimator_.get())->accumulateDenominator(
		      featureVector, weight);
}

/**
 * DiscriminativeMeanEstimator
 */
DiscriminativeMeanEstimator::DiscriminativeMeanEstimator(ComponentIndex dimension) :
    Precursor(dimension)
{}

void DiscriminativeMeanEstimator::accumulateDenominator(
    const std::vector<FeatureType> &v, Weight w)
{
    accumulator_.accumulate(v, -w);
}

void DiscriminativeMeanEstimator::setPreviousMean(const Mean *previousMean)
{
    require(previousMean);
    previousMean_.resize(previousMean->size());
    std::copy(previousMean->begin(), previousMean->end(), previousMean_.begin());
}

/**
 * DiscriminativeCovarianceEstimator
 */
DiscriminativeCovarianceEstimator::DiscriminativeCovarianceEstimator(ComponentIndex dimension) :
    Precursor(dimension)
{}

void DiscriminativeCovarianceEstimator::accumulateDenominator(
    const std::vector<FeatureType>& v, Weight w)
{
    accumulator_.accumulate(v, -w);
}

void DiscriminativeCovarianceEstimator::setPreviousCovariance(const Covariance *previousCovariance)
{
    require(previousCovariance);
    previousCovariance_.resize(previousCovariance->dimension());
    std::copy(previousCovariance->diagonal().begin(), previousCovariance->diagonal().end(), previousCovariance_.begin());
}

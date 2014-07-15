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
#include "ISmoothingMixtureEstimator.hh"
#include "MixtureSet.hh"
#include "DiscriminativeMixtureEstimator.hh"

using namespace Mm;

/**
 *  ISmoothingMixtureEstimator
 */
ISmoothingMixtureEstimator::ISmoothingMixtureEstimator() :
    parent_(0),
    constant_(0)
{}

ISmoothingMixtureEstimator::~ISmoothingMixtureEstimator()
{}

void ISmoothingMixtureEstimator::set(DiscriminativeMixtureEstimator *parent)
{
    parent_ = parent;
}

void ISmoothingMixtureEstimator::removeDensity(DensityIndex indexInMixture)
{
    verify(indexInMixture < iMixtureWeights_.size());
    iMixtureWeights_.erase(iMixtureWeights_.begin() + indexInMixture);
}

void ISmoothingMixtureEstimator::clear()
{
    iMixtureWeights_.clear();
}

/**
 *  @param mixture: i-smoothing mixture
 */
void ISmoothingMixtureEstimator::setIMixture(const Mixture *mixture)
{
    require(mixture);
    verify(iMixtureWeights_.empty());
    for (DensityIndex dnsInMix = 0; dnsInMix < mixture->nDensities(); ++ dnsInMix) {
	iMixtureWeights_.push_back(mixture->weight(dnsInMix));
    }
}

Sum ISmoothingMixtureEstimator::getObjectiveFunction() const
{
    Sum iOf = 0;
    verify(parent_);
    for (DensityIndex dns = 0; dns < parent_->nDensities(); ++ dns) {
	iOf += iMixtureWeights_[dns] * parent_->logPreviousMixtureWeight(dns);
    }
    return constant() * iOf;
}

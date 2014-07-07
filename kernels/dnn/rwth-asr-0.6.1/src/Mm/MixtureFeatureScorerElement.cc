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
#include "MixtureFeatureScorerElement.hh"
#include "Utilities.hh"
#include <numeric>

using namespace Mm;

void MixtureFeatureScorerElement::operator=(const Mixture &mixture)
{

    densityIndices_ = mixture.densityIndices();

    minus2LogWeights_.resize(mixture.nDensities());
    for (DensityIndex dnsInMix = 0; dnsInMix < mixture.nDensities(); ++ dnsInMix) {
	minus2LogWeights_[dnsInMix] = -2 * mixture.logWeight(dnsInMix);
    }
}

void MixtureFeatureScorerElement::scale(Score scale)
{
    std::transform(minus2LogWeights_.begin(), minus2LogWeights_.end(),
		   minus2LogWeights_.begin(), std::bind2nd(std::multiplies<Score>(), scale));
}

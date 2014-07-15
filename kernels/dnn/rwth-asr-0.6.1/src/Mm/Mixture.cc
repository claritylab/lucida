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
#include <numeric>
#include "Mixture.hh"
#include "MixtureSet.hh"
#include "Utilities.hh"

using namespace Mm;

Mixture::Mixture(const MixtureTopology &mixtureTopology)
{
    Precursor::operator=(mixtureTopology);

    logWeights_.resize(densityIndices_.size());
    std::fill(logWeights_.begin(), logWeights_.end(), Core::Type<Weight>::min);
}

Mixture::~Mixture()
{
	clear();
}

Mixture::Mixture(const Mixture& mixture)
{
    operator=(mixture);
}

Mixture::Mixture(const MixtureSet &mixtureSet)
{
    MixtureIndex nMixtures = mixtureSet.nMixtures();
    for (MixtureIndex i=0; i<nMixtures; ++i) {
	const Mixture& recentMixture = *(mixtureSet.mixture(i));
	DensityIndex nDens = recentMixture.nDensities();
	for (DensityIndex di=0; di<nDens; ++di) {
	    f32 logWeight = recentMixture.logWeight(di);
	    addLogDensity(recentMixture.densityIndex(di), logWeight);
	}
    }
    normalizeWeights();
}

Mixture& Mixture::operator=(const Mixture& mixture)
{
    Precursor::operator=(mixture);
    logWeights_ = mixture.logWeights_;
    return *this;
}

void Mixture::addLogDensity(DensityIndex index, Weight logWeight)
{
    Precursor::addDensity(index);
    logWeights_.push_back(logWeight);

    verify(logWeights_.size() == densityIndices_.size());
}

void Mixture::addDensity(DensityIndex index, Weight weight)
{
    Weight logWeight = weight > 0 ? log(weight) : Core::Type<Weight>::min;
    addLogDensity(index, logWeight);
}

void Mixture::normalizeWeights()
{
    if(logWeights_.empty())
	return;
    Weight logNorm = logExpNorm(logWeights_.begin(), logWeights_.end());
    std::transform(logWeights_.begin(), logWeights_.end(),
		   logWeights_.begin(), std::bind2nd(std::minus<Weight>(), logNorm));
}

void Mixture::clear()
{
	Precursor::clear();
	logWeights_.clear();
}

bool Mixture::write(std::ostream& o) const
{
    o << nDensities();
    for (unsigned int i=0; i < nDensities(); ++i)
	o << " " << densityIndex(i) << " " << logWeight(i);
    o << std::endl;

    return o.good();
}

bool Mixture::read(std::istream& i, f32 version)
{
    clear();
    DensityIndex ndns;
    i >> ndns;
    DensityIndex dns;
    Weight w;
    while (0<ndns--) {
	i >> dns >> w;
	if (version < 2.0) {
	    addDensity(dns, w);
	} else {
	    addLogDensity(dns, w);
	}
    }

    return i.good();
}

DensityIndex Mixture::densityIndexWithMaxWeight() const
{
    return std::max(logWeights_.begin(), logWeights_.end()) - logWeights_.begin();
}

void Mixture::removeDensitiesWithLowWeight(Weight minWeight, bool normalizeWeights)
{
    const Weight _minWeight = exp(logExpNorm(logWeights_.begin(), logWeights_.end())) * minWeight;
    DensityIndex dnsMax = densityIndexWithMaxWeight();
#if 1
    for (DensityIndex dnsInMix = 0; dnsInMix < nDensities(); ) {
	if ((weight(dnsInMix) < _minWeight) and (dnsInMix != dnsMax)) {
	    removeDensity(dnsInMix);
	    if (dnsMax > dnsInMix) -- dnsMax;
	} else {
	    ++ dnsInMix;
	}
    }
#endif
    if (normalizeWeights) {
	this->normalizeWeights();
    }
}

#if 1
void Mixture::removeDensity(DensityIndex index)
{
    verify(logWeights_.size() == nDensities());
    logWeights_.erase(logWeights_.begin() + index);
    Precursor::removeDensity(index);
}
#endif

void Mixture::map(const std::vector<DensityIndex> &densityMap)
{
    for (DensityIndex dns = 0; dns < nDensities(); ++ dns) {
	verify(densityMap[densityIndex(dns)] != Core::Type<DensityIndex>::max);
	densityIndices_[dns] = densityMap[densityIndex(dns)];
    }
}

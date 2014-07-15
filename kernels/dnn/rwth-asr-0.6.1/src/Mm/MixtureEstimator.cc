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
#include <Core/StringUtilities.hh>
#include "MixtureEstimator.hh"

using namespace Mm;

/**
 *  AbstractMixtureEstimator: base class
 */
void AbstractMixtureEstimator::addDensity(Core::Ref<GaussDensityEstimator> densityEstimator)
{
    require(std::find_if(densityEstimators_.begin(), densityEstimators_.end(),
			 std::bind2nd(std::equal_to<Core::Ref<GaussDensityEstimator> >(),
				      densityEstimator)) ==
	    densityEstimators_.end());

    densityEstimators_.push_back(densityEstimator);
    weights_.resize(densityEstimators_.size());
    reset();
}

void AbstractMixtureEstimator::clear()
{
    densityEstimators_.clear();
    weights_.clear();
}

DensityIndex AbstractMixtureEstimator::densityIndexWithMaxWeight() const
{
    verify(!densityEstimators_.empty());
    DensityIndex result = 0;
    for (DensityIndex dns = 1; dns < densityEstimators_.size(); ++ dns) {
	if (weights_[dns] > weights_[result]) {
	    result = dns;
	}
    }
    return result;
}

void AbstractMixtureEstimator::removeDensitiesWithZeroWeight()
{
    DensityIndex densityMax = densityIndexWithMaxWeight();
    for (DensityIndex dns = 0; dns < densityEstimators_.size(); ) {
	if (densityEstimators_[dns]->mean()->weight() == 0 &&
	    dns != densityMax) {
	    removeDensity(dns);
	    if (densityMax > dns) -- densityMax;
	} else {
	    ++ dns;
	}
    }
}

void AbstractMixtureEstimator::removeDensitiesWithLowWeight(
    Weight minObservationWeight, Weight minRelativeWeight)
{
    DensityIndex densityMax = densityIndexWithMaxWeight();
    Weight minWeight = std::max(minObservationWeight, getWeight() * minRelativeWeight);
    for (DensityIndex dns = 0; dns < densityEstimators_.size(); ) {
	bool hasEnoughWeight = weights_[dns] >= minWeight;
	bool isDeletable = dns != densityMax;
	if (!hasEnoughWeight && isDeletable) {
	    removeDensity(dns);
	    if (densityMax > dns) -- densityMax;
	} else {
	    ++ dns;
	}
    }
}

void AbstractMixtureEstimator::removeDensity(DensityIndex indexInMixture)
{
    densityEstimators_.erase(densityEstimators_.begin() + indexInMixture);
    weights_.erase(weights_.begin() + indexInMixture);
}

void AbstractMixtureEstimator::accumulate(DensityIndex indexInMixture, const FeatureVector &featureVector)
{
    require_(0 <= indexInMixture && indexInMixture < nDensities());
    ++ weights_[indexInMixture];
    densityEstimators_[indexInMixture]->accumulate(featureVector);
}

void AbstractMixtureEstimator::accumulate(
    DensityIndex indexInMixture, const FeatureVector &featureVector, Weight weight)
{
    require_(0 <= indexInMixture && indexInMixture < nDensities());
    weights_[indexInMixture] += weight;
    densityEstimators_[indexInMixture]->accumulate(featureVector, weight);
}

void AbstractMixtureEstimator::accumulate(const AbstractMixtureEstimator &toAdd)
{
    require(nDensities() == toAdd.nDensities());
    std::transform(weights_.begin(), weights_.end(),
		   toAdd.weights_.begin(), weights_.begin(), std::plus<Weight>());
}

void AbstractMixtureEstimator::reset()
{
    std::fill(weights_.begin(), weights_.end(), 0);

    for (DensityIndex dns = 0; dns < densityEstimators_.size(); ++ dns)
	densityEstimators_[dns]->reset();
}

Mixture* AbstractMixtureEstimator::estimate(
    const ReferenceIndexMap<GaussDensityEstimator>& densityMap,
    bool normalizeWeights)
{
    Mixture* result = new Mixture;

    for (DensityIndex dns = 0; dns < densityEstimators_.size(); ++ dns)
	result->addDensity(densityMap[densityEstimators_[dns]], weights_[dns]);

    if (normalizeWeights)
	result->normalizeWeights();

    return result;
}

bool AbstractMixtureEstimator::checkEventsWithZeroWeight(std::string &message)
{
    bool result = true;

    std::string m("zero observation-weight for means of density(s):");
    for (DensityIndex dns = 0; dns < densityEstimators_.size(); ++ dns) {
	if (densityEstimators_[dns]->mean()->weight() == 0) {
	    m += Core::form(" %d", dns);
	    result = false;
	}
    }
    if (!result)
	message += m;
    return result;
}

void AbstractMixtureEstimator::read(
    Core::BinaryInputStream &i,
    const std::vector<Core::Ref<GaussDensityEstimator> > &densityEstimators,
    u32 version)
{
    u32 nDensities; i >> nDensities;
    densityEstimators_.resize(nDensities);
    weights_.resize(nDensities);

    for (DensityIndex dns = 0; dns < densityEstimators_.size(); ++ dns) {
	DensityIndex densityIndex; i >> densityIndex;
	densityEstimators_[dns] = densityEstimators[densityIndex];
	if (version > 0) {
	    i >> weights_[dns];
	} else {
	    Count tmp;
	    i >> tmp;
	    weights_[dns] = tmp;
	}
    }
}

void AbstractMixtureEstimator::write(
    Core::BinaryOutputStream &o,
    const ReferenceIndexMap<GaussDensityEstimator> &densityMap) const
{
    o << (u32) densityEstimators_.size();
    for (DensityIndex dns = 0; dns < densityEstimators_.size(); ++ dns) {
	o << (DensityIndex) densityMap[densityEstimators_[dns]];
	o << weights_[dns];
    }
}

void AbstractMixtureEstimator::write(
    Core::XmlWriter &o,
    const ReferenceIndexMap<GaussDensityEstimator> &densityMap) const
{
    o << Core::XmlOpen("mixture-estimator");
    for (DensityIndex dns = 0; dns < densityEstimators_.size(); ++ dns) {
	o << Core::XmlEmpty("density") +
	    Core::XmlAttribute("estimator-index", densityMap[densityEstimators_[dns]]) +
	    Core::XmlAttribute("weight", weights_[dns]);
    }
    o << Core::XmlClose("mixture-estimator");
}

bool AbstractMixtureEstimator::equalTopology(
    const AbstractMixtureEstimator &toCompare,
    const ReferenceIndexMap<GaussDensityEstimator>& densityMap,
    const ReferenceIndexMap<GaussDensityEstimator>& densityMapToCompare) const
{
    if (nDensities() != toCompare.nDensities())
	return false;
    for (DensityIndex dns = 0; dns < densityEstimators_.size(); ++ dns) {
	if (densityMap[densityEstimators_[dns]] != densityMapToCompare[toCompare.densityEstimators_[dns]])
	    return false;
    }
    return true;
}

void AbstractMixtureEstimator::addMixture(const AbstractMixtureEstimator &toAdd)
{
    for (DensityIndex i = 0; i < toAdd.nDensities(); ++ i) {
	Core::Ref<GaussDensityEstimator> densityEstimator = toAdd.densityEstimators()[i];
	require(std::find_if(densityEstimators_.begin(), densityEstimators_.end(),
			     std::bind2nd(std::equal_to<Core::Ref<GaussDensityEstimator> >(),
					  densityEstimator)) ==
		densityEstimators_.end());

	densityEstimators_.push_back(densityEstimator);
	weights_.push_back(toAdd.weights()[i]);
    }
}

DensityIndex AbstractMixtureEstimator::accumulate(
    Core::BinaryInputStreams &is,
    Core::BinaryOutputStream &os)
{
    DensityIndex nDensities;
    is.front() >> nDensities;
    for (u32 n = 1; n < is.size(); ++ n) {
	DensityIndex _nDensities;
	is[n] >> _nDensities;
	require(_nDensities == nDensities);
    }
    os << nDensities;

    for (DensityIndex dns = 0; dns < nDensities; ++ dns) {
	DensityIndex densityIndex;
	is.front() >> densityIndex;
	Weight weight;
	is.front() >> weight;
	for (u32 n = 1; n < is.size(); ++ n) {
	    DensityIndex _densityIndex;
	    is[n] >> _densityIndex;
	    require(_densityIndex == densityIndex);
	    Weight _weight;
	    is[n] >> _weight;
	    weight += _weight;
	}
	os << densityIndex;
	os << weight;
    }
    return nDensities;
}

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
#include "EbwDiscriminativeMixtureEstimator.hh"
#include "MixtureSet.hh"

using namespace Mm;

/**
 * EbwDiscriminativeMixtureEstimator: extended Baum Welch (EBW)
 */
const Core::ParameterInt EbwDiscriminativeMixtureEstimator::paramNumberOfIterations(
    "number-of-iterations",
    "number of iterations in the Cambridge re-estimation procedure of mixture weights",
    100, 0);

EbwDiscriminativeMixtureEstimator::EbwDiscriminativeMixtureEstimator(
    const Core::Configuration &c)
    :
    Precursor(c),
    nIterations_(paramNumberOfIterations(c))
{}

void EbwDiscriminativeMixtureEstimator::removeDensity(DensityIndex indexInMixture)
{
    Precursor::removeDensity(indexInMixture);
    denWeights_.erase(denWeights_.begin() + indexInMixture);
}

void EbwDiscriminativeMixtureEstimator::estimateMixtureWeights(
    std::vector<Weight> &weights,
    bool normalizeWeights) const
{
    /**
     * Cambridge University mixture weight update scheme
     */
    if (nDensities() > 1 and getWeight() > Core::Type<Weight>::epsilon) {
	weights.resize(nDensities());
	Weight kMax = 0;
	for (DensityIndex dns = 0; dns < nDensities(); ++ dns) {
	    weights[dns] = previousMixtureWeights_[dns];
	    if (weights[dns] > Core::Type<f64>::epsilon) {
		kMax = std::max(kMax, denWeights_[dns] / weights[dns]);
	    } else {
		weights[dns] = 0;
	    }
	}
	std::vector<Weight> k(nDensities(), 0);
	for (DensityIndex dns = 0; dns < nDensities(); ++ dns) {
	    if (weights[dns] > 0) {
		k[dns] = kMax - denWeights_[dns] / weights[dns];
	    }
	}
	for (u32 iter = 0; iter < nIterations_; ++ iter) {
	    for (DensityIndex dns = 0; dns < nDensities(); ++ dns) {
		if (weights[dns] > 0) {
		    weights[dns] = weight(dns) + k[dns] * weights[dns];
		}
		if (weights[dns] < 0) {
		    Core::Application::us()->log("iteration ")
			<< iter << ": set negative weights["
			<< dns << "]=" << weights[dns] << " to 0";
		    weights[dns] = 0;
		}
	    }
	    if (normalizeWeights) {
		Weight sum = std::accumulate(weights.begin(), weights.end(), Weight(0));
		if (sum > Core::Type<Weight>::delta) {
		    std::transform(weights.begin(), weights.end(),
				   weights.begin(), std::bind2nd(std::divides<Weight>(), sum));
		} else {
		    Core::Application::us()->warning("cannot normalize mixture weights because of vanishing sum");
		    std::fill(weights.begin(), weights.end(), 1 / Weight(nDensities()));
		}
	    }
	}
    } else {
	weights.resize(nDensities(), normalizeWeights ? 1 / Weight(nDensities()) : 1);
    }
}

void EbwDiscriminativeMixtureEstimator::addDensity(Core::Ref<GaussDensityEstimator> densityEstimator)
{
    denWeights_.resize(nDensities() + 1);
    Precursor::addDensity(densityEstimator);
}

void EbwDiscriminativeMixtureEstimator::clear()
{
    Precursor::clear();
    denWeights_.clear();
}

void EbwDiscriminativeMixtureEstimator::accumulate(const AbstractMixtureEstimator &toAdd)
{
    Precursor::accumulate(toAdd);
    const EbwDiscriminativeMixtureEstimator *_toAdd =
	required_cast(const EbwDiscriminativeMixtureEstimator *, &toAdd);
    std::transform(denWeights_.begin(), denWeights_.end(),
		   _toAdd->denWeights_.begin(), denWeights_.begin(), std::plus<Weight>());
}

void EbwDiscriminativeMixtureEstimator::accumulateDenominator(
    DensityIndex indexInMixture, const FeatureVector &featureVector, Weight weight)
{
    require_(0 <= indexInMixture && indexInMixture < nDensities());
    denWeights_[indexInMixture] += weight;
    required_cast(DiscriminativeGaussDensityEstimator*,
		  densityEstimators_[indexInMixture].get())->accumulateDenominator(
		      featureVector, weight);
}

void EbwDiscriminativeMixtureEstimator::reset()
{
    Precursor::reset();
    std::fill(denWeights_.begin(), denWeights_.end(), 0);
}

Mixture* EbwDiscriminativeMixtureEstimator::estimate(
    const ReferenceIndexMap<GaussDensityEstimator> &densityMap,
    bool normalizeWeights)
{
    Mixture *result = new Mixture;

    std::vector<Weight> weights;
    estimateMixtureWeights(weights, normalizeWeights);
    for (DensityIndex dns = 0; dns < densityEstimators_.size(); ++ dns) {
	result->addDensity(densityMap[densityEstimators_[dns]], weights[dns]);
    }
    if (normalizeWeights) {
	result->normalizeWeights();
    }
    return result;
}

void EbwDiscriminativeMixtureEstimator::read(
    Core::BinaryInputStream &i,
    const std::vector<Core::Ref<GaussDensityEstimator> > &densityEstimators,
    u32 version)
{
    Precursor::read(i, densityEstimators, version);
    denWeights_.resize(nDensities());
    for (DensityIndex dns = 0; dns < densityEstimators_.size(); ++ dns) {
	i >> denWeights_[dns];
    }
}

void EbwDiscriminativeMixtureEstimator::write(
    Core::BinaryOutputStream &o,
    const ReferenceIndexMap<GaussDensityEstimator> &densityMap) const
{
    Precursor::write(o, densityMap);
    for (DensityIndex dns = 0; dns < densityEstimators_.size(); ++ dns) {
	o << denWeights_[dns];
    }
}

void EbwDiscriminativeMixtureEstimator::write(
    Core::XmlWriter &o,
    const ReferenceIndexMap<GaussDensityEstimator> &densityMap) const
{
    o << Core::XmlOpen("mixture-estimator");
    for (DensityIndex dns = 0; dns < densityEstimators_.size(); ++ dns) {
	o << Core::XmlEmpty("density") +
	    Core::XmlAttribute("estimator-index", densityMap[densityEstimators_[dns]]) +
	    Core::XmlAttribute("numerator-weight", weights_[dns]) +
	    Core::XmlAttribute("denominator-weight", denWeights_[dns]);
    }
    o << Core::XmlClose("mixture-estimator");
}

bool EbwDiscriminativeMixtureEstimator::equalWeights(const AbstractMixtureEstimator &toCompare) const
{
    return Precursor::equalWeights(toCompare)
	&& required_cast(const EbwDiscriminativeMixtureEstimator*, &toCompare)->denWeights_ == denWeights_;
}

DensityIndex EbwDiscriminativeMixtureEstimator::accumulate(
    Core::BinaryInputStreams &is,
    Core::BinaryOutputStream &os)
{
    DensityIndex nDensities = Precursor::accumulate(is, os);
    for (DensityIndex dns = 0; dns < nDensities; ++ dns) {
	Weight weight;
	is.front() >> weight;
	for (u32 n = 1; n < is.size(); ++ n) {
	    Weight _weight;
	    is[n] >> _weight;
	    weight += _weight;
	}
	os << weight;
    }
    return nDensities;
}

/**
 *  DiscriminativeMixtureEstimatorWithISmoothing: EBW
 */
EbwDiscriminativeMixtureEstimatorWithISmoothing::EbwDiscriminativeMixtureEstimatorWithISmoothing(
    const Core::Configuration &c)
    :
    Precursor(c)
{
    ISmoothing::set(this);
}

EbwDiscriminativeMixtureEstimatorWithISmoothing::~EbwDiscriminativeMixtureEstimatorWithISmoothing()
{}

void EbwDiscriminativeMixtureEstimatorWithISmoothing::removeDensity(DensityIndex indexInMixture)
{
    verify(ISmoothing::nMixtureWeights() == this->nDensities());
    Precursor::removeDensity(indexInMixture);
    ISmoothing::removeDensity(indexInMixture);
}

void EbwDiscriminativeMixtureEstimatorWithISmoothing::clear()
{
    Precursor::clear();
    ISmoothing::clear();
}

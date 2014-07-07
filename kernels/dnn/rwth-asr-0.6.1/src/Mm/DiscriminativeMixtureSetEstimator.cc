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
#include "DiscriminativeMixtureSetEstimator.hh"
#include "MixtureSetEstimator.hh"
#include "MixtureSet.hh"
#include "Module.hh"
#include "Types.hh"
#include <Core/Assertions.hh>
#include "DiscriminativeGaussDensityEstimator.hh"


using namespace Mm;

/**
 * DiscriminativeMixtureSetEstimator
 */
DiscriminativeMixtureSetEstimator::DiscriminativeMixtureSetEstimator(const Core::Configuration &c) :
    Core::Component(c),
    Precursor(c),
    objectiveFunction_(0),
    mixtureEstimatorConfig_(select("mixture-estimator"))
{}

DiscriminativeMixtureSetEstimator::~DiscriminativeMixtureSetEstimator()
{}

/**
 *  Set the previous mixture set, i.e., the means and covariances.
 */
void DiscriminativeMixtureSetEstimator::loadPreviousMixtureSet()
{
    Core::Ref<MixtureSet> previousMixtureSet =
	Mm::Module::instance().readMixtureSet(select("previous-mixture-set"));
    if (previousMixtureSet) {
	if (!distributePreviousMixtureSet(*previousMixtureSet)) {
	    criticalError("previous mixture set differs in topology");
	}
    } else {
	criticalError("previous mixture set could not be read");
    }
}

bool DiscriminativeMixtureSetEstimator::distributePreviousMixtureSet(
    const MixtureSet &previousMixtureSet)
{
    if (dimension() != previousMixtureSet.dimension()) {
	return false;
    }
    MixtureSetEstimatorIndexMap indexMaps(*this);
    if (previousMixtureSet.nMixtures() != nMixtures()) {
	return false;
    }
    for (MixtureIndex i = 0; i < previousMixtureSet.nMixtures(); ++ i) {
	mixtureEstimator(i).setPreviousMixture(previousMixtureSet.mixture(i));
    }
    if (previousMixtureSet.nDensities() != indexMaps.densityMap().size()) {
	return false;
    }
    for (DensityIndex i = 0; i < previousMixtureSet.nDensities(); ++ i) {
	required_cast(DiscriminativeGaussDensityEstimator*,
		      indexMaps.densityMap()[i].get())->setPreviousDensity(
			  previousMixtureSet.density(i));
    }
    if (previousMixtureSet.nMeans() != indexMaps.meanMap().size()) {
	return false;
    }
    for (MeanIndex i = 0; i < previousMixtureSet.nMeans(); ++ i) {
	required_cast(DiscriminativeMeanEstimator*,
		      indexMaps.meanMap()[i].get())->setPreviousMean(
			  previousMixtureSet.mean(i));
    }
    if (previousMixtureSet.nCovariances() != indexMaps.covarianceMap().size()) {
	return false;
    }
    CovarianceToMeanSetMap meanSetMap(
	indexMaps.densityMap().indexToPointerMap());
    for (CovarianceIndex i = 0; i < previousMixtureSet.nCovariances(); ++ i) {
	required_cast(DiscriminativeCovarianceEstimator*,
		      indexMaps.covarianceMap()[i].get())->setPreviousCovariance(
			  previousMixtureSet.covariance(i));
    }
    log("distribute previous mixture set ...done");
    return true;
}

void DiscriminativeMixtureSetEstimator::load()
{
    loadPreviousMixtureSet();
}

void DiscriminativeMixtureSetEstimator::finalize(
    MixtureSet &toEstimate,
    const MixtureSetEstimatorIndexMap &,
    const CovarianceToMeanSetMap &)
{
    if (minRelativeWeight() != 0) {
	toEstimate.removeDensitiesWithLowWeight(minRelativeWeight(), normalizeMixtureWeights_);
    }
}

void DiscriminativeMixtureSetEstimator::reset()
{
    Precursor::reset();
    objectiveFunction_ = 0;
}

void DiscriminativeMixtureSetEstimator::read(Core::BinaryInputStream &is)
{
    Precursor::read(is);
    if (version_ > 1) {
	Sum tmp;
	is >> tmp;
	objectiveFunction_ = tmp;
    }
    log("objective-function ") << objectiveFunction();
}

void DiscriminativeMixtureSetEstimator::write(Core::BinaryOutputStream &os)
{
    Precursor::write(os);
    os << (Sum) objectiveFunction();
    log("objective-function ") << objectiveFunction();
}

void DiscriminativeMixtureSetEstimator::write(Core::XmlWriter &os)
{
    os << Core::XmlOpen("mixture-set-estimator")
	+ Core::XmlAttribute("version", version_)
	+ Core::XmlAttribute("objective-function", objectiveFunction());

    Precursor::write(os);

    os << Core::XmlClose("mixture-set-estimator");
}

bool DiscriminativeMixtureSetEstimator::operator==(const AbstractMixtureSetEstimator &toCompare) const
{
    if (!Precursor::operator==(toCompare)) {
	return false;
    }
    return objectiveFunction() == required_cast(
	const DiscriminativeMixtureSetEstimator*, &toCompare)->objectiveFunction();
}

void DiscriminativeMixtureSetEstimator::estimate(MixtureSet &toEstimate)
{
    load();
#if 1
    if (minObservationWeight() != 0 or minRelativeWeight() != 0) {
	removeDensitiesWithLowWeight(minObservationWeight(), minRelativeWeight());
    }
#endif
    toEstimate.clear();
    toEstimate.setDimension(dimension_);
    MixtureSetEstimatorIndexMap indexMaps(*this);
    CovarianceToMeanSetMap meanSetMap(indexMaps.densityMap().indexToPointerMap());
    initialize(indexMaps, meanSetMap);
    for (MixtureIndex i = 0; i < mixtureEstimators_.size(); ++ i) {
	toEstimate.addMixture(i, mixtureEstimators_[i]->estimate(indexMaps.densityMap(), normalizeMixtureWeights_));
    }
    for (DensityIndex i = 0; i < indexMaps.densityMap().size(); ++ i) {
	toEstimate.addDensity(i, indexMaps.densityMap()[i]->estimate(
				  indexMaps.meanMap(), indexMaps.covarianceMap()));
    }
    for (MeanIndex i = 0; i < indexMaps.meanMap().size(); ++ i) {
	toEstimate.addMean(i, indexMaps.meanMap()[i]->estimate());
    }
    for (CovarianceIndex i = 0; i < indexMaps.covarianceMap().size(); ++ i) {
	toEstimate.addCovariance(i, indexMaps.covarianceMap()[i]->estimate(meanSetMap, minVariance_));
    }
    finalize(toEstimate, indexMaps, meanSetMap);
}

bool DiscriminativeMixtureSetEstimator::accumulate(
    Core::BinaryInputStreams &is,
    Core::BinaryOutputStream &os)
{
    if (!AbstractMixtureSetEstimator::accumulate(is, os)) {
	return false;
    }
    Sum objectiveFunction;
    is.front() >> objectiveFunction;
    for (u32 n = 1; n < is.size(); ++ n) {
	Sum _objectiveFunction;
	is[n] >> _objectiveFunction;
	objectiveFunction += _objectiveFunction;
    }
    os << objectiveFunction;
    return true;
}

bool DiscriminativeMixtureSetEstimator::accumulate(const AbstractMixtureSetEstimator &toAdd)
{
    if (!Precursor::accumulate(toAdd)) {
	return false;
    }
    objectiveFunction_ += required_cast(
	const DiscriminativeMixtureSetEstimator*, &toAdd)->objectiveFunction();
    return true;
}

void DiscriminativeMixtureSetEstimator::accumulateDenominator(
    MixtureIndex mixtureIndex, Core::Ref<const Feature::Vector> featureVector, Weight weight)
{
    if (viterbi_) {
	DensityIndex index = densityIndex(mixtureIndex, featureVector);
	mixtureEstimator(mixtureIndex).accumulateDenominator(
	    index, *featureVector, weight);
    } else {
	std::vector<Mm::Weight> dnsPosteriors;
	getDensityPosteriorProbabilities(mixtureIndex, featureVector, dnsPosteriors);
	for (DensityIndex index = 0; index < dnsPosteriors.size(); ++ index) {
	    Weight finalWeight = weight * dnsPosteriors[index];
	    if (finalWeight > weightThreshold_) {
		mixtureEstimator(mixtureIndex).accumulateDenominator(
		    index, *featureVector, finalWeight);
	    }
	}
    }
}

void DiscriminativeMixtureSetEstimator::accumulateObjectiveFunction(Score f)
{
    objectiveFunction_ += (Sum) f;
}

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
#include "RpropDiscriminativeMixtureSetEstimator.hh"
#include "MixtureSetEstimator.hh"
#include "MixtureSet.hh"
#include "Module.hh"
#include "Types.hh"
#include <Core/Assertions.hh>
#include "RpropDiscriminativeGaussDensityEstimator.hh"

using namespace Mm;

/**
 * RpropDiscriminativeMixtureSetEstimator
 */
const Core::ParameterFloat RpropDiscriminativeMixtureSetEstimator::paramInitialStepSizeWeights(
    "initial-step-size-weights",
    "step sizes for weights are initialized with this value if no file is given",
    0.001,
    0);

const Core::ParameterFloat RpropDiscriminativeMixtureSetEstimator::paramInitialStepSizeMeans(
    "initial-step-size-means",
    "step sizes for means are initialized with this value if no file is given",
    0.001,
    0);

const Core::ParameterFloat RpropDiscriminativeMixtureSetEstimator::paramInitialStepSizeVariances(
    "initial-step-size-variances",
    "step sizes for variances are initialized with this value if no file is given",
    0.001,
    0);

const Core::ParameterString RpropDiscriminativeMixtureSetEstimator::paramNewStepSizesFilename(
    "new-step-sizes-file",
    "name of new step sizes file");

RpropDiscriminativeMixtureSetEstimator::RpropDiscriminativeMixtureSetEstimator(const Core::Configuration &c) :
    Core::Component(c),
    Precursor(c)
{}

RpropDiscriminativeMixtureSetEstimator::~RpropDiscriminativeMixtureSetEstimator()
{}

RpropDiscriminativeMixtureEstimator* RpropDiscriminativeMixtureSetEstimator::createMixtureEstimator()
{
    return new RpropDiscriminativeMixtureEstimator(mixtureEstimatorConfig_);
}

RpropDiscriminativeGaussDensityEstimator* RpropDiscriminativeMixtureSetEstimator::createDensityEstimator()
{
    return new RpropDiscriminativeGaussDensityEstimator;
}

RpropDiscriminativeGaussDensityEstimator* RpropDiscriminativeMixtureSetEstimator::createDensityEstimator(const GaussDensity&)
{
    return new RpropDiscriminativeGaussDensityEstimator;
}

RpropDiscriminativeMeanEstimator* RpropDiscriminativeMixtureSetEstimator::createMeanEstimator()
{
    return new RpropDiscriminativeMeanEstimator;
}

RpropDiscriminativeMeanEstimator* RpropDiscriminativeMixtureSetEstimator::createMeanEstimator(const Mean& mean)
{
    return new RpropDiscriminativeMeanEstimator(mean.size());
}

RpropDiscriminativeCovarianceEstimator* RpropDiscriminativeMixtureSetEstimator::createCovarianceEstimator()
{
    return new RpropDiscriminativeCovarianceEstimator;
}

RpropDiscriminativeCovarianceEstimator* RpropDiscriminativeMixtureSetEstimator::createCovarianceEstimator(const Covariance& covariance)
{
    return new RpropDiscriminativeCovarianceEstimator(covariance.dimension());
}

bool RpropDiscriminativeMixtureSetEstimator::accumulateMixture(
    Core::BinaryInputStreams &is,
    Core::BinaryOutputStream &os)
{
    return RpropDiscriminativeMixtureEstimator::accumulate(is, os);
}

void RpropDiscriminativeMixtureSetEstimator::loadStepSizes()
{
    Core::Ref<MixtureSet> stepSizes =
	Mm::Module::instance().readMixtureSet(select("old-step-sizes-mixture-set"));
    if(stepSizes) {
	if (!distributeStepSizes(*stepSizes)) {
	    criticalError("step sizes differ in topology");
	}
    } else {
	distributeStepSizes(
	    paramInitialStepSizeWeights(config),
	    paramInitialStepSizeMeans(config),
	    paramInitialStepSizeVariances(config));
    }
}

void RpropDiscriminativeMixtureSetEstimator::storeStepSizes()
{
    MixtureSet stepSizes;
    collectStepSizes(stepSizes);
    Mm::Module::instance().writeMixtureSet(
	paramNewStepSizesFilename(config),
	stepSizes);
}

bool RpropDiscriminativeMixtureSetEstimator::distributeStepSizes(
    const MixtureSet &stepSizes)
{
    if (dimension() != stepSizes.dimension()) {
	return false;
    }
    MixtureSetEstimatorIndexMap indexMaps(*this);
    if (stepSizes.nMixtures() != nMixtures()) {
	return false;
    }
    for (MixtureIndex i = 0; i < stepSizes.nMixtures(); ++ i) {
	mixtureEstimator(i).setStepSizes(stepSizes.mixture(i));
    }
    if (stepSizes.nDensities() != indexMaps.densityMap().size()) {
	return false;
    }
//     for (DensityIndex i = 0; i < stepSizes.nDensities(); ++ i) {
// 	required_cast(RpropDiscriminativeGaussDensityEstimator*,
// 		      indexMaps.densityMap()[i].get())->setStepSizes(
// 			  stepSizes.density(i));
//     }
    if (stepSizes.nMeans() != indexMaps.meanMap().size()) {
	return false;
    }
    for (MeanIndex i = 0; i < stepSizes.nMeans(); ++ i) {
	required_cast(RpropDiscriminativeMeanEstimator*,
		      indexMaps.meanMap()[i].get())->setStepSizes(
			  stepSizes.mean(i));
    }
    if (stepSizes.nCovariances() != indexMaps.covarianceMap().size()) {
	return false;
    }
    for (CovarianceIndex i = 0; i < stepSizes.nCovariances(); ++ i) {
	required_cast(RpropDiscriminativeCovarianceEstimator*,
		      indexMaps.covarianceMap()[i].get())->setStepSizes(
			  stepSizes.covariance(i));
    }
    log("distribute step sizes ...done");
    return true;
}

bool RpropDiscriminativeMixtureSetEstimator::distributeStepSizes(
    Weight stepSizeWeights, Weight stepSizeMeans, Weight stepSizeVariances)
{
    MixtureSetEstimatorIndexMap indexMaps(*this);
    for (MixtureIndex i = 0; i < nMixtures(); ++ i) {
	mixtureEstimator(i).setStepSizes(stepSizeWeights);
    }
//     for (DensityIndex i = 0; i < indexMaps.densityMap().size(); ++ i) {
// 	required_cast(RpropDiscriminativeGaussDensityEstimator*,
// 		      indexMaps.densityMap()[i].get())->setStepSizes(stepSize);
//     }
    for (MeanIndex i = 0; i < indexMaps.meanMap().size(); ++ i) {
	required_cast(RpropDiscriminativeMeanEstimator*,
		      indexMaps.meanMap()[i].get())->setStepSizes(stepSizeMeans);
    }
    for (CovarianceIndex i = 0; i < indexMaps.covarianceMap().size(); ++ i) {
	required_cast(RpropDiscriminativeCovarianceEstimator*,
		      indexMaps.covarianceMap()[i].get())->setStepSizes(stepSizeVariances);
    }
    log("distribute initial step sizes done");
    return true;
}

bool RpropDiscriminativeMixtureSetEstimator::distributeSettings()
{
    MixtureSetEstimatorIndexMap indexMaps(*this);
    {
	const RpropSettings<Weight> settings(select("optimization"));
	for (MixtureIndex i = 0; i < nMixtures(); ++ i) {
	    mixtureEstimator(i).setSettings(settings);
	}
    }
    {
	const RpropSettings<MeanType> settings(select("optimization"));
	for (MeanIndex i = 0; i < indexMaps.meanMap().size(); ++ i) {
	    required_cast(RpropDiscriminativeMeanEstimator*,
			  indexMaps.meanMap()[i].get())->setSettings(settings);
	}
    }
    {
	const RpropSettings<MeanType> settings(select("optimization"));
	for (CovarianceIndex i = 0; i < indexMaps.covarianceMap().size(); ++ i) {
	    required_cast(RpropDiscriminativeCovarianceEstimator*,
			  indexMaps.covarianceMap()[i].get())->setSettings(settings);
	}
    }
    log("distribute rprop settings done");
    return true;
}

void RpropDiscriminativeMixtureSetEstimator::collectStepSizes(
    MixtureSet &stepSizes)
{
    stepSizes.clear();
    stepSizes.setDimension(dimension_);

    MixtureSetEstimatorIndexMap indexMaps(*this);
    for (MixtureIndex i = 0; i < mixtureEstimators_.size(); ++ i) {
	stepSizes.addMixture(i, mixtureEstimator(i).collectStepSizes(indexMaps.densityMap()));
    }
    for (DensityIndex i = 0; i < indexMaps.densityMap().size(); ++ i) {
	stepSizes.addDensity(
	    i,
	    required_cast(
		RpropDiscriminativeGaussDensityEstimator*,
		indexMaps.densityMap()[i].get())->collectStepSizes(
		    indexMaps.meanMap(),
		    indexMaps.covarianceMap()));
    }
    for (MeanIndex i = 0; i < indexMaps.meanMap().size(); ++ i) {
	stepSizes.addMean(
	    i,
	    required_cast(
		RpropDiscriminativeMeanEstimator*,
		indexMaps.meanMap()[i].get())->collectStepSizes());
    }
    for (CovarianceIndex i = 0; i < indexMaps.covarianceMap().size(); ++ i) {
	stepSizes.addCovariance(
	    i,
	    required_cast(
		RpropDiscriminativeCovarianceEstimator*,
		indexMaps.covarianceMap()[i].get())->collectStepSizes());
    }
}

void RpropDiscriminativeMixtureSetEstimator::loadPreviousToPreviousMixtureSet()
{
    Core::Ref<MixtureSet> p2pMixtureSet =
	Mm::Module::instance().readMixtureSet(select("previous-to-previous-mixture-set"));
    if (p2pMixtureSet) {
	if (!distributePreviousToPreviousMixtureSet(*p2pMixtureSet)) {
	    criticalError("previous-to-previous mixture set differs in topology");
	}
    } else {
	criticalError("previous-to-previous mixture set could not be read");
    }
}

bool RpropDiscriminativeMixtureSetEstimator::distributePreviousToPreviousMixtureSet(
    const MixtureSet &p2pMixtureSet)
{
    if (dimension() != p2pMixtureSet.dimension()) {
	return false;
    }
    MixtureSetEstimatorIndexMap indexMaps(*this);
    if (p2pMixtureSet.nMixtures() != nMixtures()) {
	return false;
    }
    for (MixtureIndex i = 0; i < p2pMixtureSet.nMixtures(); ++ i) {
	mixtureEstimator(i).setPreviousToPreviousMixture(p2pMixtureSet.mixture(i));
    }
    if (p2pMixtureSet.nDensities() != indexMaps.densityMap().size()) {
	return false;
    }
    for (DensityIndex i = 0; i < p2pMixtureSet.nDensities(); ++ i) {
	required_cast(RpropDiscriminativeGaussDensityEstimator*,
		      indexMaps.densityMap()[i].get())->setPreviousToPreviousDensity(
			  p2pMixtureSet.density(i));
    }
    if (p2pMixtureSet.nMeans() != indexMaps.meanMap().size()) {
	return false;
    }
    for (MeanIndex i = 0; i < p2pMixtureSet.nMeans(); ++ i) {
	required_cast(RpropDiscriminativeMeanEstimator*,
		      indexMaps.meanMap()[i].get())->setPreviousToPreviousMean(
			  p2pMixtureSet.mean(i));
    }
    if (p2pMixtureSet.nCovariances() != indexMaps.covarianceMap().size()) {
	return false;
    }
    CovarianceToMeanSetMap meanSetMap(
	indexMaps.densityMap().indexToPointerMap());
    for (CovarianceIndex i = 0; i < p2pMixtureSet.nCovariances(); ++ i) {
	required_cast(RpropDiscriminativeCovarianceEstimator*,
		      indexMaps.covarianceMap()[i].get())->setPreviousToPreviousCovariance(
			  p2pMixtureSet.covariance(i));
    }
    log("distribute previous-to-previous mixture set ...done");
    return true;
}

void RpropDiscriminativeMixtureSetEstimator::read(Core::BinaryInputStream &is)
{
    Precursor::read(is);

    Weight observationWeight = 0;
    for (MixtureIndex i = 0; i < mixtureEstimators_.size(); ++ i) {
	observationWeight += Precursor::mixtureEstimator(i).getWeight();
    }
    log("#observation-weight ") << observationWeight;
}

void RpropDiscriminativeMixtureSetEstimator::write(Core::BinaryOutputStream &os)
{
    Precursor::write(os);

    Weight observationWeight = 0;
    for (MixtureIndex i = 0; i < mixtureEstimators_.size(); ++ i) {
	observationWeight += Precursor::mixtureEstimator(i).getWeight();
    }
    log("#observation-weight ") << observationWeight;
}

void RpropDiscriminativeMixtureSetEstimator::load()
{
    Precursor::load();
    loadPreviousToPreviousMixtureSet();
    loadStepSizes();
    distributeSettings();
}

void RpropDiscriminativeMixtureSetEstimator::initialize(
    const MixtureSetEstimatorIndexMap &indexMaps,
    const CovarianceToMeanSetMap &meanSetMap)
{
    Precursor::initialize(indexMaps, meanSetMap);
#if 1
    require(indexMaps.covarianceMap().size() == 1);
    Core::Ref<DiscriminativeCovarianceEstimator> covarianceEstimator(
	required_cast(DiscriminativeCovarianceEstimator*, indexMaps.covarianceMap()[0].get()));
    for (MeanIndex i = 0; i < indexMaps.meanMap().size(); ++ i) {
	required_cast(RpropDiscriminativeMeanEstimator*, indexMaps.meanMap()[i].get())->setCovarianceEstimator(covarianceEstimator);
    }
#endif
}

void RpropDiscriminativeMixtureSetEstimator::finalize(
    MixtureSet &toEstimate,
    const MixtureSetEstimatorIndexMap &indexMaps,
    const CovarianceToMeanSetMap &meanSetMap)
{
    Precursor::finalize(toEstimate, indexMaps, meanSetMap);
    storeStepSizes();
}

/**
 * DiscriminativeMixtureSetEstimatorWithISmoothing: Rprop
 */
RpropDiscriminativeMixtureSetEstimatorWithISmoothing::RpropDiscriminativeMixtureSetEstimatorWithISmoothing(
    const Core::Configuration &c)
    :
    Core::Component(c),
    ISmoothing(c),
    Precursor(c)
{
    ISmoothing::set(this);
}

RpropDiscriminativeMixtureSetEstimatorWithISmoothing::~RpropDiscriminativeMixtureSetEstimatorWithISmoothing()
{}

RpropDiscriminativeMixtureEstimatorWithISmoothing*
RpropDiscriminativeMixtureSetEstimatorWithISmoothing::createMixtureEstimator()
{
    return new RpropDiscriminativeMixtureEstimatorWithISmoothing(mixtureEstimatorConfig_);
}

RpropDiscriminativeGaussDensityEstimatorWithISmoothing*
RpropDiscriminativeMixtureSetEstimatorWithISmoothing::createDensityEstimator()
{
    return new RpropDiscriminativeGaussDensityEstimatorWithISmoothing;
}

RpropDiscriminativeGaussDensityEstimatorWithISmoothing*
RpropDiscriminativeMixtureSetEstimatorWithISmoothing::createDensityEstimator(const GaussDensity&)
{
    return new RpropDiscriminativeGaussDensityEstimatorWithISmoothing;
}

RpropDiscriminativeMeanEstimatorWithISmoothing*
RpropDiscriminativeMixtureSetEstimatorWithISmoothing::createMeanEstimator()
{
    return new RpropDiscriminativeMeanEstimatorWithISmoothing;
}

RpropDiscriminativeMeanEstimatorWithISmoothing*
RpropDiscriminativeMixtureSetEstimatorWithISmoothing::createMeanEstimator(const Mean& mean)
{
    return new RpropDiscriminativeMeanEstimatorWithISmoothing(mean.size());
}

RpropDiscriminativeCovarianceEstimatorWithISmoothing*
RpropDiscriminativeMixtureSetEstimatorWithISmoothing::createCovarianceEstimator()
{
    return new RpropDiscriminativeCovarianceEstimatorWithISmoothing;
}

RpropDiscriminativeCovarianceEstimatorWithISmoothing*
RpropDiscriminativeMixtureSetEstimatorWithISmoothing::createCovarianceEstimator(const Covariance& covariance)
{
    return new RpropDiscriminativeCovarianceEstimatorWithISmoothing(covariance.dimension());
}

void RpropDiscriminativeMixtureSetEstimatorWithISmoothing::load()
{
    Precursor::load();
    loadISmoothingMixtureSet();
}

void RpropDiscriminativeMixtureSetEstimatorWithISmoothing::finalize(
    MixtureSet &toEstimate,
    const MixtureSetEstimatorIndexMap &indexMaps,
    const CovarianceToMeanSetMap &meanSetMap)
{
    Sum iOf = ISmoothing::getObjectiveFunction(toEstimate, indexMaps, meanSetMap);
    log("i-smoothing-objective-function: ") << iOf;
    Precursor::finalize(toEstimate, indexMaps, meanSetMap);
}

void RpropDiscriminativeMixtureSetEstimatorWithISmoothing::setIMixture(
    Core::Ref<AbstractMixtureEstimator> estimator,
    const Mixture *mixture,
    Weight iSmoothing)
{
    RpropDiscriminativeMixtureEstimatorWithISmoothing *_estimator =
	required_cast(RpropDiscriminativeMixtureEstimatorWithISmoothing*, estimator.get());
    _estimator->setIMixture(mixture);
    _estimator->setConstant(iSmoothing);
}

void RpropDiscriminativeMixtureSetEstimatorWithISmoothing::setIDensity(
    Core::Ref<GaussDensityEstimator> estimator,
    const GaussDensity *density)
{
    required_cast(RpropDiscriminativeGaussDensityEstimatorWithISmoothing*,
		  estimator.get())->setIDensity(density);
}

void RpropDiscriminativeMixtureSetEstimatorWithISmoothing::setIMean(
    Core::Ref<AbstractMeanEstimator> estimator,
    const Mean *mean,
    Weight iSmoothing)
{
    RpropDiscriminativeMeanEstimatorWithISmoothing *_estimator =
	required_cast(RpropDiscriminativeMeanEstimatorWithISmoothing*, estimator.get());
    _estimator->setIMean(mean);
    _estimator->setConstant(iSmoothing);
}

void RpropDiscriminativeMixtureSetEstimatorWithISmoothing::setICovariance(
    Core::Ref<AbstractCovarianceEstimator> estimator,
    const Covariance *covariance,
    Weight iSmoothing)
{
    RpropDiscriminativeCovarianceEstimatorWithISmoothing *_estimator =
	required_cast(RpropDiscriminativeCovarianceEstimatorWithISmoothing*, estimator.get());
    _estimator->setICovariance(covariance);
    _estimator->setConstant(iSmoothing);
}

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
#include "EbwDiscriminativeMixtureSetEstimator.hh"
#include "MixtureSetEstimator.hh"
#include "MixtureSet.hh"
#include "Types.hh"
#include "IterationConstants.hh"
#include <Core/Assertions.hh>
#include "EbwDiscriminativeGaussDensityEstimator.hh"

using namespace Mm;

/**
 * EbwDiscriminativeMixtureSetEstimator
 */
EbwDiscriminativeMixtureSetEstimator::EbwDiscriminativeMixtureSetEstimator(const Core::Configuration &c) :
    Core::Component(c),
    Precursor(c)
{}

EbwDiscriminativeMixtureSetEstimator::~EbwDiscriminativeMixtureSetEstimator()
{}

EbwDiscriminativeMixtureEstimator* EbwDiscriminativeMixtureSetEstimator::createMixtureEstimator()
{
    return new EbwDiscriminativeMixtureEstimator(mixtureEstimatorConfig_);
}

EbwDiscriminativeGaussDensityEstimator* EbwDiscriminativeMixtureSetEstimator::createDensityEstimator()
{
    return new EbwDiscriminativeGaussDensityEstimator;
}

EbwDiscriminativeGaussDensityEstimator* EbwDiscriminativeMixtureSetEstimator::createDensityEstimator(const GaussDensity&)
{
    return new EbwDiscriminativeGaussDensityEstimator;
}

EbwDiscriminativeMeanEstimator* EbwDiscriminativeMixtureSetEstimator::createMeanEstimator()
{
    return new EbwDiscriminativeMeanEstimator;
}

EbwDiscriminativeMeanEstimator* EbwDiscriminativeMixtureSetEstimator::createMeanEstimator(const Mean& mean)
{
    return new EbwDiscriminativeMeanEstimator(mean.size());
}

EbwDiscriminativeCovarianceEstimator* EbwDiscriminativeMixtureSetEstimator::createCovarianceEstimator()
{
    return new EbwDiscriminativeCovarianceEstimator;
}

EbwDiscriminativeCovarianceEstimator* EbwDiscriminativeMixtureSetEstimator::createCovarianceEstimator(const Covariance& covariance)
{
    return new EbwDiscriminativeCovarianceEstimator(covariance.dimension());
}

bool EbwDiscriminativeMixtureSetEstimator::accumulateMixture(
    Core::BinaryInputStreams &is,
    Core::BinaryOutputStream &os)
{
    return EbwDiscriminativeMixtureEstimator::accumulate(is, os);
}

void EbwDiscriminativeMixtureSetEstimator::initialize(
    const MixtureSetEstimatorIndexMap &indexMaps,
    const CovarianceToMeanSetMap &meanSetMap)
{
    Precursor::initialize(indexMaps, meanSetMap);
    IterationConstants *ic = IterationConstants::createIterationConstants(select("iteration-constants"));
    ic->set(mixtureEstimators_, meanSetMap);
    delete ic;
}

void EbwDiscriminativeMixtureSetEstimator::read(Core::BinaryInputStream &is)
{
    Precursor::read(is);

    Weight numObservationWeight = 0;
    Weight denObservationWeight = 0;
    for (MixtureIndex i = 0; i < mixtureEstimators_.size(); ++ i) {
	numObservationWeight += Precursor::mixtureEstimator(i).getWeight();
	denObservationWeight += mixtureEstimator(i).getDenWeight();
    }

    log("#numerator-observation-weight ")
	<< numObservationWeight
	<< ", #denominator-observation-weight "
	<< denObservationWeight;
}

void EbwDiscriminativeMixtureSetEstimator::write(Core::BinaryOutputStream &os)
{
    Precursor::write(os);

    Weight numObservationWeight = 0;
    Weight denObservationWeight = 0;
    for (MixtureIndex i = 0; i < mixtureEstimators_.size(); ++ i) {
	numObservationWeight += Precursor::mixtureEstimator(i).getWeight();
	denObservationWeight += mixtureEstimator(i).getDenWeight();
    }

    log("#numerator-observation-weight ")
	<< numObservationWeight
	<< ", #denominator-observation-weight "
	<< denObservationWeight;
}

/**
 * DiscriminativeMixtureSetEstimatorWithISmoothing: EBW
 */
EbwDiscriminativeMixtureSetEstimatorWithISmoothing::EbwDiscriminativeMixtureSetEstimatorWithISmoothing(const Core::Configuration &c) :
    Core::Component(c),
    ISmoothing(c),
    Precursor(c)
{
    ISmoothing::set(this);
}

EbwDiscriminativeMixtureSetEstimatorWithISmoothing::~EbwDiscriminativeMixtureSetEstimatorWithISmoothing()
{}

EbwDiscriminativeMixtureEstimatorWithISmoothing*
EbwDiscriminativeMixtureSetEstimatorWithISmoothing::createMixtureEstimator()
{
    return new EbwDiscriminativeMixtureEstimatorWithISmoothing(mixtureEstimatorConfig_);
}

EbwDiscriminativeGaussDensityEstimatorWithISmoothing*
EbwDiscriminativeMixtureSetEstimatorWithISmoothing::createDensityEstimator()
{
    return new EbwDiscriminativeGaussDensityEstimatorWithISmoothing;
}

EbwDiscriminativeGaussDensityEstimatorWithISmoothing*
EbwDiscriminativeMixtureSetEstimatorWithISmoothing::createDensityEstimator(const GaussDensity&)
{
    return new EbwDiscriminativeGaussDensityEstimatorWithISmoothing;
}

EbwDiscriminativeMeanEstimatorWithISmoothing*
EbwDiscriminativeMixtureSetEstimatorWithISmoothing::createMeanEstimator()
{
    return new EbwDiscriminativeMeanEstimatorWithISmoothing;
}

EbwDiscriminativeMeanEstimatorWithISmoothing*
EbwDiscriminativeMixtureSetEstimatorWithISmoothing::createMeanEstimator(const Mean& mean)
{
    return new EbwDiscriminativeMeanEstimatorWithISmoothing(mean.size());
}

EbwDiscriminativeCovarianceEstimatorWithISmoothing*
EbwDiscriminativeMixtureSetEstimatorWithISmoothing::createCovarianceEstimator()
{
    return new EbwDiscriminativeCovarianceEstimatorWithISmoothing;
}

EbwDiscriminativeCovarianceEstimatorWithISmoothing*
EbwDiscriminativeMixtureSetEstimatorWithISmoothing::createCovarianceEstimator(const Covariance& covariance)
{
    return new EbwDiscriminativeCovarianceEstimatorWithISmoothing(covariance.dimension());
}

void EbwDiscriminativeMixtureSetEstimatorWithISmoothing::load()
{
    Precursor::load();
    loadISmoothingMixtureSet();
}

void EbwDiscriminativeMixtureSetEstimatorWithISmoothing::finalize(
    MixtureSet &toEstimate,
    const MixtureSetEstimatorIndexMap &indexMaps,
    const CovarianceToMeanSetMap &meanSetMap)
{
    Sum iOf = ISmoothing::getObjectiveFunction(toEstimate, indexMaps, meanSetMap);
    log("i-smoothing-objective-function: ") << iOf;
    Precursor::finalize(toEstimate, indexMaps, meanSetMap);
}

void EbwDiscriminativeMixtureSetEstimatorWithISmoothing::setIMixture(
    Core::Ref<AbstractMixtureEstimator> estimator,
    const Mixture *mixture,
    Weight iSmoothing)
{
    EbwDiscriminativeMixtureEstimatorWithISmoothing *_estimator =
	required_cast(EbwDiscriminativeMixtureEstimatorWithISmoothing*, estimator.get());
    _estimator->setIMixture(mixture);
    _estimator->setConstant(iSmoothing);
}

void EbwDiscriminativeMixtureSetEstimatorWithISmoothing::setIDensity(
    Core::Ref<GaussDensityEstimator> estimator,
    const GaussDensity *density)
{
    required_cast(EbwDiscriminativeGaussDensityEstimatorWithISmoothing*, estimator.get())->setIDensity(density);
}

void EbwDiscriminativeMixtureSetEstimatorWithISmoothing::setIMean(
    Core::Ref<AbstractMeanEstimator> estimator,
    const Mean *mean,
    Weight iSmoothing)
{
    EbwDiscriminativeMeanEstimatorWithISmoothing *_estimator =
	required_cast(EbwDiscriminativeMeanEstimatorWithISmoothing*, estimator.get());
    _estimator->setIMean(mean);
    _estimator->setConstant(iSmoothing);
}

void EbwDiscriminativeMixtureSetEstimatorWithISmoothing::setICovariance(
    Core::Ref<AbstractCovarianceEstimator> estimator,
    const Covariance *covariance,
    Weight iSmoothing)
{
    EbwDiscriminativeCovarianceEstimatorWithISmoothing *_estimator =
	required_cast(EbwDiscriminativeCovarianceEstimatorWithISmoothing*, estimator.get());
    _estimator->setICovariance(covariance);
    _estimator->setConstant(iSmoothing);
}

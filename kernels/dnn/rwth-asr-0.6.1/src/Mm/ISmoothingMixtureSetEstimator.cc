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
#include "ISmoothingMixtureSetEstimator.hh"
#include "DiscriminativeMixtureSetEstimator.hh"
#include "MixtureSet.hh"
#include "Module.hh"
#include "AbstractMixtureSetEstimator.hh"

using namespace Mm;

/**
 * ISmoothingMixtureSetEstimator
 */
const Core::ParameterFloat ISmoothingMixtureSetEstimator::paramWeightsConstant(
    "i-smoothing-weights",
    "interpolation constant for i-smoothing of mixture weights",
    0, 0);

const Core::ParameterFloat ISmoothingMixtureSetEstimator::paramMeansConstant(
    "i-smoothing-means",
    "interpolation constant for i-smoothing of means",
    0, 0);

ISmoothingMixtureSetEstimator::ISmoothingMixtureSetEstimator(
    const Core::Configuration &c) :
    Core::Component(c),
    parent_(0),
    weightsConstant_(paramWeightsConstant(c)),
    meansConstant_(paramMeansConstant(c))
{}

void ISmoothingMixtureSetEstimator::set(DiscriminativeMixtureSetEstimator *parent)
{
    parent_ = parent;
}

void ISmoothingMixtureSetEstimator::loadISmoothingMixtureSet()
{
    Core::Ref<MixtureSet> mixtureSet =
	Mm::Module::instance().readMixtureSet(select("i-smoothing-mixture-set"));
    if (mixtureSet) {
	if (!distributeISmoothingMixtureSet(*mixtureSet)) {
	    Core::Application::us()->criticalError("i-smoothing mixture set differs in topology");
	}
    } else {
	Core::Application::us()->criticalError("i-smoothing mixture set could not be read");
    }
}

bool ISmoothingMixtureSetEstimator::distributeISmoothingMixtureSet(
    const MixtureSet &mixtureSet)
{
    verify(parent_);
    if (mixtureSet.dimension() != parent_->dimension()) {
	return false;
    }
    MixtureSetEstimatorIndexMap indexMaps(*parent_);
    if (mixtureSet.nMixtures() != parent_->nMixtures()) {
	return false;
    }
    for (MixtureIndex i = 0; i < mixtureSet.nMixtures(); ++ i) {
	setIMixture((*parent_->getMixtureEstimators())[i], mixtureSet.mixture(i), weightsConstant());
    }
    if (mixtureSet.nDensities() != indexMaps.densityMap().size()) {
	return false;
    }
    for (DensityIndex i = 0; i < mixtureSet.nDensities(); ++ i) {
	setIDensity(indexMaps.densityMap()[i], mixtureSet.density(i));
    }
    if (mixtureSet.nMeans() != indexMaps.meanMap().size()) {
	return false;
    }
    for (MeanIndex i = 0; i < mixtureSet.nMeans(); ++ i) {
	setIMean(indexMaps.meanMap()[i], mixtureSet.mean(i), meansConstant());
    }
    if (mixtureSet.nCovariances() != indexMaps.covarianceMap().size()) {
	return false;
    }
    CovarianceToMeanSetMap meanSetMap(indexMaps.densityMap().indexToPointerMap());
    for (CovarianceIndex i = 0; i < mixtureSet.nCovariances(); ++ i) {
	setICovariance(indexMaps.covarianceMap()[i], mixtureSet.covariance(i), meansConstant());
    }
    log("distribute i-smoothing mixture set ...done");
    return true;
}

/**
 * Estimate i-smoothing portion of objective function w.r.t. @param mixtureSet.
 */
Sum ISmoothingMixtureSetEstimator::getObjectiveFunction(
    const MixtureSet &previousMixtureSet,
    const MixtureSetEstimatorIndexMap &indexMaps,
    const CovarianceToMeanSetMap &meanSetMap)
{
    parent_->distributePreviousMixtureSet(previousMixtureSet);
    Sum iOfW = 0;
    AbstractMixtureSetEstimator::MixtureEstimators &mixtureEstimators =
	*parent_->getMixtureEstimators();
    for (MixtureIndex i = 0; i < parent_->nMixtures(); ++ i) {
	iOfW += getMixtureObjectiveFunction(mixtureEstimators[i]);
    }
    log("weights-objective-function: ") << iOfW;

    Sum iOfM = 0;
    for (CovarianceIndex i = 0; i < indexMaps.covarianceMap().size(); ++ i) {
	iOfM += getCovarianceObjectiveFunction(indexMaps.covarianceMap()[i], meanSetMap);
    }
    log("means-objective-function: ") << iOfM;
    return iOfW + iOfM;
}

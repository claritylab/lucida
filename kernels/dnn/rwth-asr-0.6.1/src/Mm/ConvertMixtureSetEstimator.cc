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
#include "ConvertMixtureSetEstimator.hh"
#include "MixtureSet.hh"
#include "Types.hh"
#include "ConvertGaussDensityEstimator.hh"

using namespace Mm;

/**
 * ConvertMixtureSetEstimator
 */
ConvertMixtureSetEstimator::ConvertMixtureSetEstimator(const Core::Configuration &c) :
    Core::Component(c),
    Precursor(c)
{}

ConvertMixtureEstimator* ConvertMixtureSetEstimator::createMixtureEstimator()
{
    return new ConvertMixtureEstimator;
}

ConvertGaussDensityEstimator* ConvertMixtureSetEstimator::createDensityEstimator()
{
    return new ConvertGaussDensityEstimator;
}

ConvertGaussDensityEstimator* ConvertMixtureSetEstimator::createDensityEstimator(const GaussDensity&)
{
    return new ConvertGaussDensityEstimator;
}

ConvertMeanEstimator* ConvertMixtureSetEstimator::createMeanEstimator()
{
    return new ConvertMeanEstimator;
}

ConvertMeanEstimator* ConvertMixtureSetEstimator::createMeanEstimator(const Mean& mean)
{
    return new ConvertMeanEstimator(mean.size());
}

ConvertCovarianceEstimator* ConvertMixtureSetEstimator::createCovarianceEstimator()
{
    return new ConvertCovarianceEstimator;
}

ConvertCovarianceEstimator* ConvertMixtureSetEstimator::createCovarianceEstimator(const Covariance& covariance)
{
    return new ConvertCovarianceEstimator(covariance.dimension());
}

void ConvertMixtureSetEstimator::setMixtureSet(Core::Ref<const MixtureSet> mixtureSet)
{
    setTopology(mixtureSet);
    MixtureSetEstimatorIndexMap indexMaps(*this);

    verify(nMixtures() == mixtureSet->nMixtures());
    for (MixtureIndex i = 0; i < mixtureSet->nMixtures(); ++ i) {
	mixtureEstimator(i).setMixture(mixtureSet->mixture(i));
    }

    verify(mixtureSet->nDensities() == indexMaps.densityMap().size());
    for (DensityIndex i = 0; i < mixtureSet->nDensities(); ++ i) {
	required_cast(ConvertGaussDensityEstimator*,
		      indexMaps.densityMap()[i].get())->setDensity(
			  mixtureSet->density(i));
    }

    verify(mixtureSet->nMeans() == indexMaps.meanMap().size());
    for (MeanIndex i = 0; i < mixtureSet->nMeans(); ++ i) {
	required_cast(ConvertMeanEstimator*,
		      indexMaps.meanMap()[i].get())->setMean(
			  mixtureSet->mean(i));
    }

    verify(mixtureSet->nCovariances() == indexMaps.covarianceMap().size());
    CovarianceToMeanSetMap meanSetMap(
	indexMaps.densityMap().indexToPointerMap());
    for (CovarianceIndex i = 0; i < mixtureSet->nCovariances(); ++ i) {
	required_cast(ConvertCovarianceEstimator*,
		      indexMaps.covarianceMap()[i].get())->setCovariance(
			  mixtureSet->covariance(i), meanSetMap);
    }
}

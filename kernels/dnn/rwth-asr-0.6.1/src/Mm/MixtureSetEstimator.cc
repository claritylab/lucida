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
#include <Core/Directory.hh>
#include "MixtureSetEstimator.hh"
#include "MixtureSet.hh"

using namespace Mm;

// MixtureSetEstimator
//////////////////////

MixtureSetEstimator::MixtureSetEstimator(const Core::Configuration &c) :
    Core::Component(c),  // Virtually inherited
    Precursor(c)
{}

MixtureEstimator* MixtureSetEstimator::createMixtureEstimator()
{
    return new MixtureEstimator;
}

GaussDensityEstimator* MixtureSetEstimator::createDensityEstimator()
{
    return new GaussDensityEstimator;
}

GaussDensityEstimator* MixtureSetEstimator::createDensityEstimator(const GaussDensity&)
{
    return new GaussDensityEstimator;
}

MeanEstimator* MixtureSetEstimator::createMeanEstimator()
{
    return new MeanEstimator;
}

MeanEstimator* MixtureSetEstimator::createMeanEstimator(const Mean& mean)
{
    return new MeanEstimator(mean.size());
}

CovarianceEstimator* MixtureSetEstimator::createCovarianceEstimator()
{
    return new CovarianceEstimator;
}

CovarianceEstimator* MixtureSetEstimator::createCovarianceEstimator(const Covariance& covariance)
{
    return new CovarianceEstimator(covariance.dimension());
}

bool MixtureSetEstimator::accumulate(
    Core::BinaryInputStreams &is,
    Core::BinaryOutputStream &os)
{
    return Precursor::accumulate(is, os);
}

void MixtureSetEstimator::read(Core::BinaryInputStream &is)
{
    Precursor::read(is);

    Weight observationWeight = 0;
    for (MixtureIndex i = 0; i < mixtureEstimators_.size(); ++ i) {
	observationWeight += mixtureEstimators_[i]->getWeight();
    }

    log("#observation-weight ") << observationWeight;
}

void MixtureSetEstimator::write(Core::BinaryOutputStream &os)
{
    Precursor::write(os);

    Weight observationWeight = 0;
    for(MixtureIndex i = 0; i < mixtureEstimators_.size(); ++ i)
	observationWeight += mixtureEstimators_[i]->getWeight();

    log("#observation-weight ") << observationWeight;
}

void MixtureSetEstimator::write(Core::XmlWriter &os)
{
    os << Core::XmlOpen("mixture-set-estimator")
	+ Core::XmlAttribute("version", version_);

    Precursor::write(os);

    os << Core::XmlClose("mixture-set-estimator");
}

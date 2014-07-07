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
#include "ISmoothingGaussDensityEstimator.hh"
#include "DiscriminativeGaussDensityEstimator.hh"

using namespace Mm;

/**
 * ISmoothingMeanEstimator
 */
ISmoothingMeanEstimator::ISmoothingMeanEstimator() : constant_(0)
{}

void ISmoothingMeanEstimator::setIMean(const Mean *mean)
{
    require(mean);
    verify(iMean_.empty());
    iMean_.resize(mean->size());
    std::copy(mean->begin(), mean->end(), iMean_.begin());
}

/**
 * ISmoothingCovarianceEstimator
 */
ISmoothingCovarianceEstimator::ISmoothingCovarianceEstimator() :
    constant_(0),
    iSum_(0)
{}

ISmoothingCovarianceEstimator::~ISmoothingCovarianceEstimator()
{
    delete iSum_;
}

void ISmoothingCovarianceEstimator::reset()
{
    delete iSum_;
    iSum_ = 0;
}

void ISmoothingCovarianceEstimator::setICovariance(const Covariance *covariance)
{
    require(covariance);
    verify(iCovariance_.empty());
    iCovariance_.resize(covariance->dimension());
    std::copy(covariance->diagonal().begin(), covariance->diagonal().end(), iCovariance_.begin());
}

ISmoothingCovarianceEstimator::Accumulator::SumType
ISmoothingCovarianceEstimator::iSum(
    const CovarianceToMeanSetMap::MeanSet &meanSet, ComponentIndex i) const
{
    if (!iSum_) {
	iSum_ = new std::vector<Accumulator::SumType>(iCovariance().size());
	std::copy(iCovariance().begin(), iCovariance().end(), iSum_->begin());
	std::transform(
	    iSum_->begin(),
	    iSum_->end(),
	    iSum_->begin(),
	    std::bind2nd(std::multiplies<Accumulator::SumType>(), meanSet.size()));
	CovarianceToMeanSetMap::MeanSet::const_iterator m = meanSet.begin();
	for(; m != meanSet.end(); ++ m) {
	    const ISmoothingMeanEstimator* meanEstimator =
		dynamic_cast<const ISmoothingMeanEstimator*>(m->get());
	    std::transform(
		iSum_->begin(),
		iSum_->end(),
		meanEstimator->iMean().begin(),
		iSum_->begin(),
		plusSquare<Accumulator::SumType>());
	}
    }
    return (*iSum_)[i];
}

Sum ISmoothingCovarianceEstimator::getObjectiveFunction(
    const CovarianceToMeanSetMap &meanSetMap)
{
    verify(parent_);
    Sum iOf = 0;
    const CovarianceToMeanSetMap::MeanSet &meanSet = meanSetMap[parent_];
    for (ComponentIndex i = 0; i < parent_->sum().size(); ++ i) {
	const VarianceType prevSigma = parent_->previousCovariance(i);
	iOf += meanSet.size() * std::log(2 * M_PI * prevSigma);
	Sum _iOf = iSum(meanSet, i);
	CovarianceToMeanSetMap::MeanSet::const_iterator m = meanSet.begin();
	for (; m != meanSet.end(); ++ m) {
	    const MeanType prevMean = required_cast(
		const DiscriminativeMeanEstimator*, m->get())->previousMean(i);
	    _iOf += prevMean * (prevMean - 2 * iMean(*m)[i]);
	}
	iOf += _iOf / prevSigma;
    }
    return -0.5 * constant() * iOf;
}

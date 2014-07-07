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
#include "RpropDiscriminativeGaussDensityEstimator.hh"
#include "Utilities.hh"

using namespace Mm;

/**
 * RpropDiscriminativeGaussDensityEstimator
 */
GaussDensity* RpropDiscriminativeGaussDensityEstimator::collectStepSizes(
    const ReferenceIndexMap<AbstractMeanEstimator> &meanMap,
    const ReferenceIndexMap<AbstractCovarianceEstimator> &covarianceMap) const
{
    verify(meanEstimator_ && covarianceEstimator_);
    return new GaussDensity(meanMap[meanEstimator_], covarianceMap[covarianceEstimator_]);
}

/**
 * RpropDiscriminativeMeanEstimator
 */
RpropDiscriminativeMeanEstimator::RpropDiscriminativeMeanEstimator(ComponentIndex dimension) :
    Precursor(dimension)
{}

MeanType RpropDiscriminativeMeanEstimator::gradient(
    ComponentIndex cmp) const
{
    require(covarianceEstimator_);
    return (sum(cmp) - weight() * previousMean(cmp)) / covarianceEstimator_->previousCovariance(cmp);
}

Mean* RpropDiscriminativeMeanEstimator::estimate()
{
    Mean *result = new Mean(previousMean());
    Rprop::apply(*result);
    return result;
}

Mean* RpropDiscriminativeMeanEstimator::collectStepSizes()
{
    Mean *result = new Mean(Rprop::stepSizes());
    return result;
}

/**
 * RpropDiscriminativeMeanEstimatorWithISmoothing
 */
RpropDiscriminativeMeanEstimatorWithISmoothing::RpropDiscriminativeMeanEstimatorWithISmoothing(
    ComponentIndex dimension)
    :
    Precursor(dimension)
{}

/**
 * RpropDiscriminativeCovarianceEstimator
 */
RpropDiscriminativeCovarianceEstimator::RpropDiscriminativeCovarianceEstimator(
    ComponentIndex dimension) :
    Precursor(dimension)
{}

VarianceType RpropDiscriminativeCovarianceEstimator::gradient(
    ComponentIndex cmp) const
{
    const VarianceType sigma2 = previousCovariance(cmp);
    return -0.5 * (weight_ - buffer_[cmp] / sigma2) / sigma2;
}

void RpropDiscriminativeCovarianceEstimator::initialize(
    const CovarianceToMeanSetMap &meanSetMap)
{
    const CovarianceToMeanSetMap::MeanSet &meanSet =
	meanSetMap[Core::Ref<AbstractCovarianceEstimator>(this)];
    buffer_.resize(accumulator_.size());
    for (ComponentIndex cmp = 0; cmp < buffer_.size(); ++ cmp) {
	buffer_[cmp] = sum(meanSet, cmp);
    }
    weight_ = 0;
    for(CovarianceToMeanSetMap::MeanSet::const_iterator m = meanSet.begin(); m != meanSet.end(); ++ m) {
	const Core::Ref<RpropDiscriminativeMeanEstimator> meanEstimator(
	    required_cast(RpropDiscriminativeMeanEstimator*, m->get()));
	const Weight tmpWeight = meanEstimator->weight();
	weight_ += tmpWeight;
	for (ComponentIndex cmp = 0; cmp < buffer_.size(); ++ cmp) {
	    const Sum tmp1 = 2 * meanEstimator->sum(cmp);
	    const Sum tmp2 = meanEstimator->previousMean(cmp) * tmpWeight;
	    buffer_[cmp] += meanEstimator->previousMean(cmp) * (tmp2 - tmp1);
	}
    }
    verify(weight_ != 0);
}

Covariance* RpropDiscriminativeCovarianceEstimator::estimate(
    const CovarianceToMeanSetMap &meanSetMap, VarianceType minimumVariance)
{
    initialize(meanSetMap);
    std::vector<VarianceType> buffer(previousCovariance());
    Rprop::apply(buffer);
    DiagonalCovariance *result = new DiagonalCovariance(buffer);
    applyMinimumVariance(minimumVariance, *result);
    verify(result->isPositiveDefinite());
    return result;
}

Covariance* RpropDiscriminativeCovarianceEstimator::collectStepSizes(
    )
{
    DiagonalCovariance *result = new DiagonalCovariance(Rprop::stepSizes());
    return result;
}

/**
 * RpropDiscriminativeCovarianceEstimatorWithISmoothing
 */
RpropDiscriminativeCovarianceEstimatorWithISmoothing::RpropDiscriminativeCovarianceEstimatorWithISmoothing(ComponentIndex dimension) :
    Precursor(dimension)
{
    ISmoothing::set(Core::Ref<DiscriminativeCovarianceEstimator>(this));
}

void RpropDiscriminativeCovarianceEstimatorWithISmoothing::reset()
{
    Precursor::reset();
    ISmoothing::reset();
}

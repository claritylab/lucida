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
#include "EbwDiscriminativeGaussDensityEstimator.hh"
#include "Utilities.hh"

using namespace Mm;

/**
 * EbwDiscriminativeGaussDensityEstimator
 */
void EbwDiscriminativeGaussDensityEstimator::setIterationConstant(IterationConstant ic)
{
    required_cast(EbwDiscriminativeMeanEstimator*, meanEstimator_.get())->setIterationConstant(ic);
}

/**
 * EbwDiscriminativeMeanEstimator
 */
EbwDiscriminativeMeanEstimator::EbwDiscriminativeMeanEstimator(ComponentIndex dimension) :
    Precursor(dimension),
    denAccumulator_(dimension),
    iterationConstant_(1)
{}

void EbwDiscriminativeMeanEstimator::reset()
{
    Precursor::reset();
    denAccumulator_.reset();
}

void EbwDiscriminativeMeanEstimator::accumulate(const AbstractEstimationAccumulator& other)
{
    Precursor::accumulate(other);
    const EbwDiscriminativeMeanEstimator *_other =
	required_cast(const EbwDiscriminativeMeanEstimator*, &other);
    denAccumulator_.accumulate(_other->denAccumulator_);
}

void EbwDiscriminativeMeanEstimator::accumulateDenominator(
    const std::vector<FeatureType> &v, Weight w)
{
    denAccumulator_.accumulate(v, w);
}

void EbwDiscriminativeMeanEstimator::read(Core::BinaryInputStream &i, u32 version)
{
    Precursor::read(i, version);
    denAccumulator_.read(i, version);
}

void EbwDiscriminativeMeanEstimator::write(Core::BinaryOutputStream &o) const
{
    Precursor::write(o);
    denAccumulator_.write(o);
}

void EbwDiscriminativeMeanEstimator::write(Core::XmlWriter &o) const
{
    Precursor::write(o);
    denAccumulator_.write(o, "denominator-vector-accumulator");
}

bool EbwDiscriminativeMeanEstimator::operator==(const AbstractEstimationAccumulator &other) const
{
    if (!Precursor::operator==(other)) {
	return false;
    }
    const EbwDiscriminativeMeanEstimator *_other =
	required_cast(const EbwDiscriminativeMeanEstimator*, &other);
    return (*this == other) && (denAccumulator_ == _other->denAccumulator_);
}

Mean* EbwDiscriminativeMeanEstimator::estimate()
{
    Weight weight = dtWeight() + iterationConstant();
    if (weight > 0) {
	std::vector<Sum> buffer(accumulator_.size());
	const std::vector<Accumulator::SumType> dSum = dtSum();
	std::transform(dSum.begin(), dSum.end(), previousMean().begin(),
		       buffer.begin(), plusWeighted<Sum>(iterationConstant()));
	Mean *result = new Mean(accumulator_.size());
	std::transform(buffer.begin(), buffer.end(), result->begin(),
		       std::bind2nd(std::divides<Accumulator::SumType>(), weight));
	return result;
    } else {
	Mean *result = new Mean(previousMean());
	return result;
    }
}

std::vector<EbwDiscriminativeMeanEstimator::Accumulator::SumType>
EbwDiscriminativeMeanEstimator::dtSum() const
{
    verify(sum().size() == denSum().size());
    std::vector<Accumulator::SumType> result(sum().size());
    for (u32 i = 0; i < result.size(); ++ i) {
	result[i] = sum()[i] - denSum()[i];
    }
    return result;
}

void EbwDiscriminativeMeanEstimator::setIterationConstant(IterationConstant ic)
{
    verify(!std::isnan(ic));
    verify(!std::isinf(ic));
    verify(ic >= 0);
    iterationConstant_ = ic;
}

/**
 * EbwDiscriminativeMeanEstimatorWithISmoothing
 */
EbwDiscriminativeMeanEstimatorWithISmoothing::EbwDiscriminativeMeanEstimatorWithISmoothing(
    ComponentIndex dimension)
    :
    Precursor(dimension)
{}

std::vector<EbwDiscriminativeMeanEstimatorWithISmoothing::Accumulator::SumType>
EbwDiscriminativeMeanEstimatorWithISmoothing::dtSum() const
{
    verify(sum().size() == denSum().size());
    std::vector<Accumulator::SumType> result(sum().size());
    for (u32 i = 0; i < result.size(); ++ i) {
	result[i] = sum()[i] - denSum()[i] + ISmoothing::constant() * iMean(i);
    }
    return result;
}

/**
 * EbwDiscriminativeCovarianceEstimator
 */
EbwDiscriminativeCovarianceEstimator::EbwDiscriminativeCovarianceEstimator(ComponentIndex dimension) :
    Precursor(dimension),
    denAccumulator_(dimension)
{}

void EbwDiscriminativeCovarianceEstimator::reset()
{
    Precursor::reset();
    denAccumulator_.reset();
}

void EbwDiscriminativeCovarianceEstimator::accumulate(
    const AbstractEstimationAccumulator &other)
{
    Precursor::accumulate(other);
    const EbwDiscriminativeCovarianceEstimator *_other =
	required_cast(const EbwDiscriminativeCovarianceEstimator*, &other);
    denAccumulator_.accumulate(_other->denAccumulator_);
}

void EbwDiscriminativeCovarianceEstimator::accumulateDenominator(
    const std::vector<FeatureType>& v, Weight w)
{
    denAccumulator_.accumulate(v, w);
}

void EbwDiscriminativeCovarianceEstimator::read(Core::BinaryInputStream &i, u32 version)
{
    Precursor::read(i, version);
    denAccumulator_.read(i, version);
}

void EbwDiscriminativeCovarianceEstimator::write(Core::BinaryOutputStream &o) const
{
    Precursor::write(o);
    denAccumulator_.write(o);
}

void EbwDiscriminativeCovarianceEstimator::write(Core::XmlWriter &o) const
{
    Precursor::write(o);
    denAccumulator_.write(o, "denominator-vector-accumulator");
}

bool EbwDiscriminativeCovarianceEstimator::operator==(
    const AbstractEstimationAccumulator &other) const
{
    const EbwDiscriminativeCovarianceEstimator *_other =
	required_cast(const EbwDiscriminativeCovarianceEstimator*, &other);
    return (*this == other) && (denAccumulator_ == _other->denAccumulator_);
}

Covariance* EbwDiscriminativeCovarianceEstimator::estimate(
    const CovarianceToMeanSetMap &meanSetMap, VarianceType minimumVariance)
{
    const ComponentIndex dimension = accumulator_.size();
    const CovarianceToMeanSetMap::MeanSet &meanSet =
	meanSetMap[Core::Ref<AbstractCovarianceEstimator>(this)];
    std::vector<Sum> buffer = dtSum(meanSet);
    Weight weight = 0;
    CovarianceToMeanSetMap::MeanSet::const_iterator m = meanSet.begin();
    for(; m != meanSet.end(); ++ m) {
	const Core::Ref<EbwDiscriminativeMeanEstimator> meanEstimator(
	    required_cast(EbwDiscriminativeMeanEstimator*, m->get()));

	Weight tmpWeight = meanEstimator->dtWeight() + meanEstimator->iterationConstant();
	weight += tmpWeight;

	const Mean *mean = getMean(meanEstimator);
	for (ComponentIndex i = 0; i < dimension; ++ i) {
	    Sum previousSum = previousCovariance(i);
	    previousSum += meanEstimator->previousMean(i) * meanEstimator->previousMean(i);
	    previousSum *= meanEstimator->iterationConstant();
	    Sum newSum = tmpWeight * (*mean)[i] * (*mean)[i];
	    buffer[i] += previousSum - newSum;
	}
	delete mean;
    }
    verify(weight > 0);
    std::transform(buffer.begin(), buffer.end(),
		   buffer.begin(), std::bind2nd(std::divides<Sum>(), weight));
    std::vector<VarianceType> buffer32(dimension);
    std::copy(buffer.begin(), buffer.end(), buffer32.begin());
    DiagonalCovariance *result = new DiagonalCovariance(buffer32);
    verify(result->isPositiveDefinite());
    applyMinimumVariance(minimumVariance, *result);
    return result;
}

std::vector<EbwDiscriminativeCovarianceEstimator::Accumulator::SumType>
EbwDiscriminativeCovarianceEstimator::dtSum(
    const CovarianceToMeanSetMap::MeanSet &) const
{
    verify(sum().size() == denSum().size());
    std::vector<Accumulator::SumType> result(sum().size());
    for (u32 i = 0; i < result.size(); ++ i) {
	result[i] = sum()[i] - denSum()[i];
    }
    return result;
}

/**
 * EbwDiscriminativeCovarianceEstimatorWithISmoothing
 */
EbwDiscriminativeCovarianceEstimatorWithISmoothing::EbwDiscriminativeCovarianceEstimatorWithISmoothing(
    ComponentIndex dimension) :
    Precursor(dimension)
{
    ISmoothing::set(Core::Ref<DiscriminativeCovarianceEstimator>(this));
}

void EbwDiscriminativeCovarianceEstimatorWithISmoothing::reset()
{
    Precursor::reset();
    ISmoothingCovarianceEstimator::reset();
}

std::vector<EbwDiscriminativeCovarianceEstimatorWithISmoothing::Precursor::Accumulator::SumType>
EbwDiscriminativeCovarianceEstimatorWithISmoothing::dtSum(
    const CovarianceToMeanSetMap::MeanSet &meanSet) const
{
    verify(sum().size() == denSum().size());
    std::vector<Precursor::Accumulator::SumType> result(sum().size());
    for (u32 i = 0; i < result.size(); ++ i) {
	result[i] = sum()[i] - denSum()[i] + ISmoothing::constant() * iSum(meanSet, i);
    }
    return result;
}

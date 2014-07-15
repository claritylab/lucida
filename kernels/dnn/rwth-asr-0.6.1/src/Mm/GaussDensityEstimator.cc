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
#include "GaussDensityEstimator.hh"

using namespace Mm;

/*
 * GaussDensityEstimator
 */
void GaussDensityEstimator::accumulate(const FeatureVector &featureVector)
{
    verify_(meanEstimator_ && covarianceEstimator_);
    meanEstimator_->accumulate(featureVector);
    covarianceEstimator_->accumulate(featureVector);
}

void GaussDensityEstimator::accumulate(const FeatureVector &featureVector, Weight weight)
{
    verify_(meanEstimator_ && covarianceEstimator_);
    meanEstimator_->accumulate(featureVector, weight);
    covarianceEstimator_->accumulate(featureVector, weight);
}

void GaussDensityEstimator::reset()
{
    verify(meanEstimator_ && covarianceEstimator_);
    meanEstimator_->reset();
    covarianceEstimator_->reset();
}

GaussDensity* GaussDensityEstimator::estimate(
    const ReferenceIndexMap<AbstractMeanEstimator> &meanMap,
    const ReferenceIndexMap<AbstractCovarianceEstimator> &covarianceMap) const
{
    verify(meanEstimator_ && covarianceEstimator_);
    return new GaussDensity(meanMap[meanEstimator_], covarianceMap[covarianceEstimator_]);
}

void GaussDensityEstimator::read(Core::BinaryInputStream &i,
				 const std::vector<Core::Ref<AbstractMeanEstimator> > &means,
				 const std::vector<Core::Ref<AbstractCovarianceEstimator> > &covariances)
{
    MeanIndex meanIndex; i >> meanIndex;
    meanEstimator_ = means[meanIndex];

    CovarianceIndex covarianceIndex; i >> covarianceIndex;
    covarianceEstimator_ = covariances[covarianceIndex];
}

void GaussDensityEstimator::write(Core::BinaryOutputStream &o,
				  const ReferenceIndexMap<AbstractMeanEstimator> &meanMap,
				  const ReferenceIndexMap<AbstractCovarianceEstimator> &covarianceMap) const
{
    o << (MeanIndex)meanMap[meanEstimator_]
      << (CovarianceIndex)covarianceMap[covarianceEstimator_];
}

void GaussDensityEstimator::write(Core::XmlWriter &o,
				  const ReferenceIndexMap<AbstractMeanEstimator> &meanMap,
				  const ReferenceIndexMap<AbstractCovarianceEstimator> &covarianceMap) const
{
    o << Core::XmlEmpty("density-estimator") +
	Core::XmlAttribute("mean-estimator-index", meanMap[meanEstimator_]) +
	Core::XmlAttribute("covariance-estimator-index", covarianceMap[covarianceEstimator_]);
}

bool GaussDensityEstimator::equalTopology(
    const GaussDensityEstimator &toCompare,
    const ReferenceIndexMap<AbstractMeanEstimator> &meanMap,
    const ReferenceIndexMap<AbstractCovarianceEstimator> &covarianceMap,
    const ReferenceIndexMap<AbstractMeanEstimator> &meanMapToCompare,
    const ReferenceIndexMap<AbstractCovarianceEstimator> &covarianceMapToCompare) const
{
    return (meanMap[meanEstimator_] == meanMapToCompare[toCompare.meanEstimator_] &&
	    covarianceMap[covarianceEstimator_] == covarianceMapToCompare[toCompare.covarianceEstimator_]);
}

bool GaussDensityEstimator::accumulate(
    Core::BinaryInputStreams &is,
    Core::BinaryOutputStream &os)
{
    MeanIndex meanIndex;
    is.front() >> meanIndex;
    for (u32 n = 1; n < is.size(); ++ n) {
	MeanIndex _meanIndex;
	is[n] >> _meanIndex;
	require(_meanIndex == meanIndex);
    }
    os << meanIndex;

    CovarianceIndex covarianceIndex;
    is.front() >> covarianceIndex;
    for (u32 n = 1; n < is.size(); ++ n) {
	CovarianceIndex _covarianceIndex;
	is[n] >> _covarianceIndex;
	require(_covarianceIndex == covarianceIndex);
    }
    os << covarianceIndex;

    return true;
}

/*
 * MeanEstimator
 */
MeanEstimator::MeanEstimator(ComponentIndex dimension) :
    accumulator_(dimension)
{}

void MeanEstimator::reset()
{
    accumulator_.reset();
}

void MeanEstimator::accumulate(const std::vector<FeatureType>& v)
{
    accumulator_.accumulate(v);
}

void MeanEstimator::accumulate(const std::vector<FeatureType>& v, Weight w)
{
    accumulator_.accumulate(v, w);
}

void MeanEstimator::accumulate(const AbstractEstimationAccumulator& other)
{
    const MeanEstimator* pOther;
    pOther = dynamic_cast<const MeanEstimator*>(&other);
    verify(pOther);
    accumulator_.accumulate(pOther->accumulator_);
}

void MeanEstimator::read(Core::BinaryInputStream &i, u32 version)
{
    accumulator_.read(i, version);
}

void MeanEstimator::write(Core::BinaryOutputStream &o) const
{
    accumulator_.write(o);
}

void MeanEstimator::write(Core::XmlWriter &o) const
{
    accumulator_.write(o);
}

bool MeanEstimator::operator==(const AbstractEstimationAccumulator& other) const
{
    const MeanEstimator* pOther;
    pOther = dynamic_cast<const MeanEstimator*>(&other);
    verify(pOther);
    return accumulator_ == pOther->accumulator_;
}

Mean* MeanEstimator::estimate()
{
    if (accumulator_.weight() == 0)
	return new Mean(accumulator_.size());

    Mean* result = new Mean(accumulator_.size());
    std::transform(accumulator_.sum().begin(), accumulator_.sum().end(), result->begin(),
		   std::bind2nd(std::divides<Accumulator::SumType>(), accumulator_.weight()));
    return result;
}

/*
 * CovarianceEstimator
 */
CovarianceEstimator::CovarianceEstimator(ComponentIndex dimension) :
    accumulator_(dimension)
{}

void CovarianceEstimator::reset()
{
    accumulator_.reset();
}

void CovarianceEstimator::accumulate(const std::vector<FeatureType>& v)
{
    accumulator_.accumulate(v);
}

void CovarianceEstimator::accumulate(const std::vector<FeatureType>& v, Weight weight)
{
    accumulator_.accumulate(v, weight);
}

void CovarianceEstimator::accumulate(const AbstractEstimationAccumulator& other)
{
    const CovarianceEstimator* pOther = dynamic_cast<const CovarianceEstimator*>(&other);
    verify(pOther != 0);
    accumulator_.accumulate(pOther->accumulator_);
}

void CovarianceEstimator::read(Core::BinaryInputStream &i, u32 version)
{
    accumulator_.read(i, version);
}

void CovarianceEstimator::write(Core::BinaryOutputStream &o) const
{
    accumulator_.write(o);
}

void CovarianceEstimator::write(Core::XmlWriter &o) const
{
    accumulator_.write(o);
}

bool CovarianceEstimator::operator==(const AbstractEstimationAccumulator& other) const
{
    const CovarianceEstimator* pOther;
    pOther= dynamic_cast<const CovarianceEstimator*>(&other);
    verify(pOther);
    return accumulator_ == pOther->accumulator_;
}

Covariance* CovarianceEstimator::estimate(
    const CovarianceToMeanSetMap &meanSetMap, VarianceType minimumVariance)
{
    if (accumulator_.weight() == 0)
	return new DiagonalCovariance(accumulator_.sum().size());

    const CovarianceToMeanSetMap::MeanSet& meanSet =
	meanSetMap[Core::Ref<AbstractCovarianceEstimator>((CovarianceEstimator *)this)];
    WeighedMeanSquareSum weighedMeanSquareSum(accumulator_.size());

    for(CovarianceToMeanSetMap::MeanSet::const_iterator meanEstimator = meanSet.begin();
	meanEstimator != meanSet.end(); ++ meanEstimator)
    {
	const MeanEstimator* pMeanEstimator = dynamic_cast<const MeanEstimator*>(meanEstimator->get());
	verify(pMeanEstimator);

	if (pMeanEstimator->weight() > 0)
	    weighedMeanSquareSum.accumulate(pMeanEstimator->sum(), pMeanEstimator->weight());
    }
    verify(accumulator_.weight() > 0);
#if 1
    verify(Core::isAlmostEqualUlp(accumulator_.weight(), weighedMeanSquareSum.weight(), s64(1e12)));
#endif
    DiagonalCovariance* result = new DiagonalCovariance(accumulator_.sum().size());

    std::transform(accumulator_.sum().begin(), accumulator_.sum().end(), weighedMeanSquareSum.sum().begin(),
		   result->buffer_.begin(), normalizedMinus<Sum>(accumulator_.weight()));

    applyMinimumVariance(minimumVariance ,*result);
    return result;
}

void CovarianceEstimator::applyMinimumVariance(VarianceType minimumVariance, DiagonalCovariance &result) const
{
    if (minimumVariance == 0) return;

    for(std::vector<VarianceType>::iterator d = result.buffer_.begin();
	d != result.buffer_.end(); ++ d) {
	if (*d < minimumVariance) *d = minimumVariance;
    }
}


/*
 * Covariance2MeanMap
*/


CovarianceToMeanSetMap::CovarianceToMeanSetMap(
    const std::vector<Core::Ref<GaussDensityEstimator > > &densityEstimators)
{
    std::vector<Core::Ref<GaussDensityEstimator> >::const_iterator i = densityEstimators.begin();
    for(; i != densityEstimators.end(); ++ i)
	map_[(*i)->covarianceEstimator_].insert((*i)->meanEstimator_);
}

const CovarianceToMeanSetMap::MeanSet& CovarianceToMeanSetMap::operator[](
    Core::Ref<AbstractCovarianceEstimator> estimator) const
{
    Map::const_iterator result = map_.find(estimator);
    require(result != map_.end());
    return result->second;
}

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
#include "ConvertGaussDensityEstimator.hh"

using namespace Mm;

/**
 * ConvertGaussDensityEstimator
 */
void ConvertGaussDensityEstimator::setDensity(const GaussDensity *density)
{
    verify(meanEstimator_);
    verify(covarianceEstimator_);
}

/**
 * ConvertMeanEstimator
 */
ConvertMeanEstimator::ConvertMeanEstimator(ComponentIndex dimension) :
    Precursor(dimension)
{}

void ConvertMeanEstimator::setMean(const Mean *mean)
{
    require(mean);
    accumulator_.reset();
    accumulator_.accumulate(*mean);
}

/**
 * ConvertCovarianceEstimator
 */
ConvertCovarianceEstimator::ConvertCovarianceEstimator(ComponentIndex dimension) :
    Precursor(dimension)
{}

void ConvertCovarianceEstimator::setCovariance(
    const Covariance *covariance, const CovarianceToMeanSetMap &meanSetMap)
{
    verify(covariance->diagonal().size() == accumulator_.size());
    std::vector<Sum> sum(accumulator_.size(), 0);
    const CovarianceToMeanSetMap::MeanSet &meanSet =
	meanSetMap[Core::Ref<AbstractCovarianceEstimator>(this)];
    Weight weight = 0;
    CovarianceToMeanSetMap::MeanSet::const_iterator meanEstimator = meanSet.begin();
    for(; meanEstimator != meanSet.end(); ++ meanEstimator) {
	verify((*meanEstimator)->weight() == 1);
	const std::vector<Sum> &mean =
	    required_cast(const MeanEstimator*, meanEstimator->get())->sum();
	std::transform(sum.begin(), sum.end(), mean.begin(),
		       sum.begin(), plusSquare<Sum>());
	weight += (*meanEstimator)->weight();
    }
    std::vector<Sum> covariance64(sum.size());
    std::copy(covariance->diagonal().begin(), covariance->diagonal().end(), covariance64.begin());
    std::transform(sum.begin(), sum.end(), covariance64.begin(),
		   sum.begin(), plusWeighted<Sum>(weight));
    accumulator_ = Accumulator(sum, weight);
}

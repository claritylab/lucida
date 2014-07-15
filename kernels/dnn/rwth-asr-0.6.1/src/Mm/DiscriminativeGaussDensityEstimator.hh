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
#ifndef _MM_DISCRIMINATIVE_GAUSS_DENSITY_ESTIMATOR_HH
#define _MM_DISCRIMINATIVE_GAUSS_DENSITY_ESTIMATOR_HH

#include "GaussDensityEstimator.hh"

namespace Mm {

    /**
     * DiscriminativeGaussDensityEstimator
     */
    class DiscriminativeGaussDensityEstimator : public GaussDensityEstimator
    {
    public:
	DiscriminativeGaussDensityEstimator() {}

	void setPreviousDensity(const GaussDensity *) {}
	void accumulateDenominator(const FeatureVector&, Weight weight);
    };

    /**
     * DiscriminativeMeanEstimator
     */
    class DiscriminativeMeanEstimator : public MeanEstimator
    {
	typedef MeanEstimator Precursor;
    protected:
	std::vector<MeanType> previousMean_;
    public:
	DiscriminativeMeanEstimator(ComponentIndex dimension = 0);
	virtual ~DiscriminativeMeanEstimator() {}

	virtual void accumulateDenominator(const std::vector<FeatureType> &, Weight);

	void setPreviousMean(const Mean *);
	const std::vector<MeanType>& previousMean() const {
	    verify(previousMean_.size() == accumulator_.size());
	    return previousMean_;
	}
	MeanType previousMean(ComponentIndex i) const {
	    verify(i < previousMean_.size()); return previousMean_[i];
	}
    };

    /**
     * DiscriminativeCovarianceEstimator
     */
    class DiscriminativeCovarianceEstimator : public CovarianceEstimator
    {
	typedef CovarianceEstimator Precursor;
    private:
	std::vector<VarianceType> previousCovariance_;
    public:
	DiscriminativeCovarianceEstimator(ComponentIndex dimension = 0);

	virtual void accumulateDenominator(const std::vector<FeatureType>&, Weight);

	void setPreviousCovariance(const Covariance *);
	const std::vector<VarianceType>& previousCovariance() const {
	    verify(previousCovariance_.size() == accumulator_.size());
	    return previousCovariance_;
	}
	VarianceType previousCovariance(ComponentIndex i) const {
	    verify(i < previousCovariance_.size());
	    return previousCovariance_[i];
	}
    };

} //namespace Mm

#endif //_MM_DISCRIMINATIVE_MIXTURE_ESTIMATOR_HH

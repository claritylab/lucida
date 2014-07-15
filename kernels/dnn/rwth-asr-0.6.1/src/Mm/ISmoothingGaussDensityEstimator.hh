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
#ifndef _MM_ISMOOTHING_GAUSS_DENSITY_ESTIMATOR_HH
#define _MM_ISMOOTHING_GAUSS_DENSITY_ESTIMATOR_HH

#include "GaussDensity.hh"
#include "GaussDensityEstimator.hh"

namespace Mm {

    class DiscriminativeCovarianceEstimator;

    /**
     * ISmoothingGaussDensityEstimator
     */
    class ISmoothingGaussDensityEstimator
    {
    public:
	ISmoothingGaussDensityEstimator() {}
	void setIDensity(const GaussDensity *) {}
    };

    /**
     * ISmoothingMeanEstimator
     */
    class ISmoothingMeanEstimator
    {
    private:
	std::vector<MeanType> iMean_;
	Weight constant_;
    public:
	ISmoothingMeanEstimator();
	virtual ~ISmoothingMeanEstimator() {}

	void setIMean(const Mean *);
	const std::vector<MeanType>& iMean() const { return iMean_; }
	MeanType iMean(ComponentIndex i) const { return iMean_[i]; }
	void setConstant(Weight constant) { constant_ = constant; }
	Weight constant() const { return constant_; }
    };

    /**
     * ISmoothingCovarianceEstimator
     */
    class ISmoothingCovarianceEstimator
    {
    protected:
	typedef VectorAccumulator<FeatureType, plusSquare<Sum>, plusSquareWeighted<Sum> > Accumulator;
    private:
	Core::Ref<DiscriminativeCovarianceEstimator> parent_;
	std::vector<VarianceType> iCovariance_;
	Weight constant_;
	mutable std::vector<Accumulator::SumType> *iSum_;
    protected:
	virtual const std::vector<MeanType>& iMean(Core::Ref<AbstractMeanEstimator>) const = 0;
    public:
	ISmoothingCovarianceEstimator();
	virtual ~ISmoothingCovarianceEstimator();

	void set(Core::Ref<DiscriminativeCovarianceEstimator> parent) { parent_ = parent; }
	void reset();
	void setICovariance(const Covariance *);
	const std::vector<VarianceType>& iCovariance() const { return iCovariance_; }
	VarianceType iCovariance(const CovarianceToMeanSetMap::MeanSet &, ComponentIndex i) const { return iCovariance_[i]; }
	Accumulator::SumType iSum(const CovarianceToMeanSetMap::MeanSet &, ComponentIndex i) const;
	Sum getObjectiveFunction(const CovarianceToMeanSetMap &);
	void setConstant(Weight constant) { constant_ = constant; }
	Weight constant() const { return constant_; }
    };

} //namespace Mm

#endif //_MM_ISMOOTHING_MIXTURE_ESTIMATOR_HH

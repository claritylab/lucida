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
#ifndef _MM_RPROP_DISCRIMINATIVE_GAUSS_DENSITY_ESTIMATOR_HH
#define _MM_RPROP_DISCRIMINATIVE_GAUSS_DENSITY_ESTIMATOR_HH

#include "DiscriminativeGaussDensityEstimator.hh"
#include "RpropOptimization.hh"
#include "ISmoothingGaussDensityEstimator.hh"

namespace Mm {

    /**
     * RpropDiscriminativeGaussDensityEstimator
     */
    class RpropDiscriminativeGaussDensityEstimator :
	public DiscriminativeGaussDensityEstimator
    {
    public:
	RpropDiscriminativeGaussDensityEstimator() {}

	GaussDensity* collectStepSizes(
	    const ReferenceIndexMap<AbstractMeanEstimator> &,
	    const ReferenceIndexMap<AbstractCovarianceEstimator> &) const;
	void setPreviousToPreviousDensity(const GaussDensity *) {}
    };

    /**
     * RpropDiscriminativeGaussDensityEstimatorWithISmoothing
     */
    class RpropDiscriminativeGaussDensityEstimatorWithISmoothing :
	public RpropDiscriminativeGaussDensityEstimator,
	public ISmoothingGaussDensityEstimator
    {
    public:
	RpropDiscriminativeGaussDensityEstimatorWithISmoothing() {}
    };

    /**
     * RpropDiscriminativeMeanEstimator
     */
    class RpropDiscriminativeMeanEstimator :
	public DiscriminativeMeanEstimator,
	public RpropOptimization<MeanType>
    {
	typedef DiscriminativeMeanEstimator Precursor;
    public:
	friend class RpropDiscriminativeCovarianceEstimator;
    protected:
	typedef RpropOptimization<MeanType> Rprop;
    protected:
	Core::Ref<DiscriminativeCovarianceEstimator> covarianceEstimator_;
    protected:
	virtual MeanType previous(ComponentIndex cmp) const { return previousMean(cmp); }
	virtual MeanType gradient(ComponentIndex) const;
	virtual Accumulator::SumType sum(ComponentIndex cmp) const { return Precursor::sum()[cmp]; }
    public:
	RpropDiscriminativeMeanEstimator(ComponentIndex dimension = 0);

	virtual Mean* estimate();
	void setStepSizes(const Mean *mean) { Rprop::setStepSizes(*mean); }
	void setStepSizes(MeanType stepSize) { Rprop::setStepSizes(accumulator_.size(), stepSize); }
	Mean* collectStepSizes();
	void setPreviousToPreviousMean(const Mean *mean) { Rprop::setPreviousToPrevious(*mean); }
	void setCovarianceEstimator(Core::Ref<DiscriminativeCovarianceEstimator> covarianceEstimator) {
	    covarianceEstimator_ = covarianceEstimator;
	}
    };

    /**
     * RpropDiscriminativeMeanEstimatorWithISmoothing
     */
    class RpropDiscriminativeMeanEstimatorWithISmoothing :
	public RpropDiscriminativeMeanEstimator,
	public ISmoothingMeanEstimator
    {
	typedef RpropDiscriminativeMeanEstimator Precursor;
    protected:
	typedef ISmoothingMeanEstimator ISmoothing;
    public:
	friend class RpropDiscriminativeCovarianceEstimatorWithISmoothing;
    protected:
	virtual Accumulator::SumType sum(ComponentIndex cmp) const {
	    return Precursor::sum(cmp) + ISmoothing::constant() * iMean(cmp);
	}
	virtual Weight weight() const { return Precursor::weight() + ISmoothing::constant(); }
    public:
	RpropDiscriminativeMeanEstimatorWithISmoothing(ComponentIndex dimension = 0);
	virtual ~RpropDiscriminativeMeanEstimatorWithISmoothing() {}
    };

    /**
     * RpropDiscriminativeCovarianceEstimator
     */
    class RpropDiscriminativeCovarianceEstimator :
	public DiscriminativeCovarianceEstimator,
	public RpropOptimization<VarianceType>
    {
	typedef DiscriminativeCovarianceEstimator Precursor;
    protected:
	typedef RpropOptimization<VarianceType> Rprop;
    protected:
	std::vector<Sum> buffer_;
	Weight weight_;
    protected:
	virtual VarianceType previous(ComponentIndex cmp) const { return previousCovariance(cmp); }
	virtual VarianceType gradient(ComponentIndex) const;
	virtual Accumulator::SumType sum(const CovarianceToMeanSetMap::MeanSet &, ComponentIndex cmp) const {
	    return Precursor::sum()[cmp];
	}
	void initialize(const CovarianceToMeanSetMap &);
    public:
	RpropDiscriminativeCovarianceEstimator(ComponentIndex dimension = 0);
	virtual ~RpropDiscriminativeCovarianceEstimator() {}

	Covariance* estimate(const CovarianceToMeanSetMap &, VarianceType minimumVariance);

	void setStepSizes(const Covariance *covariance) { Rprop::setStepSizes(covariance->diagonal()); }
	void setStepSizes(VarianceType stepSize) { Rprop::setStepSizes(accumulator_.size(), stepSize); }
	Covariance* collectStepSizes();
	void setPreviousToPreviousCovariance(const Covariance *covariance) {
	    Rprop::setPreviousToPrevious(covariance->diagonal());
	}
    };

    /**
     * RpropDiscriminativeCovarianceEstimatorWithISmoothing
     */
    class RpropDiscriminativeCovarianceEstimatorWithISmoothing :
	public RpropDiscriminativeCovarianceEstimator,
	public ISmoothingCovarianceEstimator
    {
	typedef RpropDiscriminativeCovarianceEstimator Precursor;
    protected:
	typedef ISmoothingCovarianceEstimator ISmoothing;
    protected:
	virtual Precursor::Accumulator::SumType sum(const CovarianceToMeanSetMap::MeanSet &meanSet, ComponentIndex cmp) const {
	    return Precursor::sum(meanSet, cmp) + ISmoothing::constant() * ISmoothing::iSum(meanSet, cmp);
	}
	virtual Weight weight() const { return Precursor::weight() + ISmoothing::constant(); }
	virtual const std::vector<MeanType>& iMean(Core::Ref<AbstractMeanEstimator> m) const {
	    return required_cast(RpropDiscriminativeMeanEstimatorWithISmoothing*, m.get())->iMean();
	}
    public:
	RpropDiscriminativeCovarianceEstimatorWithISmoothing(ComponentIndex dimension = 0);
	virtual ~RpropDiscriminativeCovarianceEstimatorWithISmoothing() {}

	virtual void reset();
    };

} //namespace Mm

#endif //_MM_RPROP_DISCRIMINATIVE_MIXTURE_ESTIMATOR_HH

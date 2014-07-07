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
#ifndef _MM_EBW_DISCRIMINATIVE_GAUSS_DENSITY_ESTIMATOR_HH
#define _MM_EBW_DISCRIMINATIVE_GAUSS_DENSITY_ESTIMATOR_HH

#include "DiscriminativeGaussDensityEstimator.hh"
#include "ISmoothingGaussDensityEstimator.hh"

namespace Mm {

    /**
     * EbwDiscriminativeGaussDensityEstimator
     */
    class EbwDiscriminativeGaussDensityEstimator : public DiscriminativeGaussDensityEstimator
    {
    public:
	EbwDiscriminativeGaussDensityEstimator() {}

	void setIterationConstant(IterationConstant);
    };

    /**
     * EbwDiscriminativeGaussDensityEstimatorWithISmoothing
     */
    class EbwDiscriminativeGaussDensityEstimatorWithISmoothing :
	public EbwDiscriminativeGaussDensityEstimator,
	public ISmoothingGaussDensityEstimator
    {
    public:
	EbwDiscriminativeGaussDensityEstimatorWithISmoothing() {}
    };

    /**
     * EbwDiscriminativeMeanEstimator
     */
    class EbwDiscriminativeMeanEstimator : public DiscriminativeMeanEstimator
    {
	typedef DiscriminativeMeanEstimator Precursor;
    protected:
	Accumulator denAccumulator_;
	IterationConstant iterationConstant_;
    public:
	EbwDiscriminativeMeanEstimator(ComponentIndex dimension = 0);
	virtual ~EbwDiscriminativeMeanEstimator() {}

	virtual void reset();
	virtual void accumulate(const AbstractEstimationAccumulator &);
	virtual void accumulateDenominator(const std::vector<FeatureType> &, Weight);

	virtual void read(Core::BinaryInputStream &i, u32 version);
	virtual void write(Core::BinaryOutputStream &) const;
	virtual void write(Core::XmlWriter &) const;

	virtual bool operator==(const AbstractEstimationAccumulator &other) const;

	virtual Mean* estimate();

	const std::vector<Accumulator::SumType>& denSum() const { return denAccumulator_.sum(); }
	virtual std::vector<Accumulator::SumType> dtSum() const;
	Weight denWeight() const { return denAccumulator_.weight(); }
	virtual Weight dtWeight() const { return weight() - denWeight(); }
	IterationConstant iterationConstant() const {
	    verify(iterationConstant_ > 0); return iterationConstant_;
	}
	void setIterationConstant(IterationConstant);
    };

    /**
     * EbwDiscriminativeMeanEstimatorWithISmoothing
     */
    class EbwDiscriminativeMeanEstimatorWithISmoothing :
	public EbwDiscriminativeMeanEstimator,
	public ISmoothingMeanEstimator
    {
	typedef EbwDiscriminativeMeanEstimator Precursor;
    protected:
	typedef ISmoothingMeanEstimator ISmoothing;
    public:
	friend class EbwDiscriminativeCovarianceEstimatorWithISmoothing;
    public:
	EbwDiscriminativeMeanEstimatorWithISmoothing(ComponentIndex dimension = 0);

	virtual std::vector<Accumulator::SumType> dtSum() const;
	virtual Weight dtWeight() const { return Precursor::dtWeight() + ISmoothing::constant(); }
    };

    /**
     * EbwDiscriminativeCovarianceEstimator
     */
    class EbwDiscriminativeCovarianceEstimator :
	public DiscriminativeCovarianceEstimator
    {
	typedef DiscriminativeCovarianceEstimator Precursor;
    private:
	Accumulator denAccumulator_;
    protected:
	virtual const Mean* getMean(Core::Ref<EbwDiscriminativeMeanEstimator> e) const { return e->estimate(); }
    public:
	EbwDiscriminativeCovarianceEstimator(ComponentIndex dimension = 0);

	virtual void reset();
	virtual void accumulate(const AbstractEstimationAccumulator &);
	void accumulateDenominator(const std::vector<FeatureType>&, Weight);

	virtual void read(Core::BinaryInputStream &i, u32 version);
	virtual void write(Core::BinaryOutputStream &o) const;
	virtual void write(Core::XmlWriter &o) const;

	virtual bool operator==(const AbstractEstimationAccumulator &other) const;

	virtual Covariance* estimate(const CovarianceToMeanSetMap &, VarianceType minimumVariance);

	const std::vector<Accumulator::SumType>& denSum() const { return denAccumulator_.sum(); }
	virtual std::vector<Accumulator::SumType> dtSum(const CovarianceToMeanSetMap::MeanSet &) const;
	Weight denWeight() const { return denAccumulator_.weight(); }
	virtual Weight dtWeight() const { return weight() - denWeight(); }
    };

    /**
     * EbwDiscriminativeCovarianceEstimatorWithISmoothing
     */
    class EbwDiscriminativeCovarianceEstimatorWithISmoothing :
	public EbwDiscriminativeCovarianceEstimator,
	public ISmoothingCovarianceEstimator
    {
	typedef EbwDiscriminativeCovarianceEstimator Precursor;
    protected:
	typedef ISmoothingCovarianceEstimator ISmoothing;
    protected:
	virtual const std::vector<MeanType>& iMean(Core::Ref<AbstractMeanEstimator> m) const {
	    return required_cast(EbwDiscriminativeMeanEstimatorWithISmoothing*, m.get())->iMean();
	}
	virtual const Mean* getMean(Core::Ref<EbwDiscriminativeMeanEstimator> e) const {
	    return required_cast(EbwDiscriminativeMeanEstimatorWithISmoothing*, e.get())->estimate();
	}
    public:
	EbwDiscriminativeCovarianceEstimatorWithISmoothing(ComponentIndex dimension = 0);
	virtual ~EbwDiscriminativeCovarianceEstimatorWithISmoothing() {}

	virtual void reset();

	virtual std::vector<Precursor::Accumulator::SumType> dtSum(const CovarianceToMeanSetMap::MeanSet &) const;
	virtual Weight dtWeight() const { return dtWeight() + ISmoothing::constant(); }
    };

} //namespace Mm

#endif //_MM_EBW_DISCRIMINATIVE_MIXTURE_ESTIMATOR_HH

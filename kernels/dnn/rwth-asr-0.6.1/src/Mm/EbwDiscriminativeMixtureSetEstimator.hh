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
#ifndef _MM_EBW_DISCRIMINATIVE_MIXTURE_SET_ESTIMATOR_HH
#define _MM_EBW_DISCRIMINATIVE_MIXTURE_SET_ESTIMATOR_HH

#include "DiscriminativeMixtureSetEstimator.hh"
#include "EbwDiscriminativeMixtureEstimator.hh"
#include "ISmoothingMixtureSetEstimator.hh"

namespace Mm {

    /**
     * EbwDiscriminativeMixtureSetEstimator
     */
    class EbwDiscriminativeMixtureSetEstimator :
	public DiscriminativeMixtureSetEstimator
    {
	typedef DiscriminativeMixtureSetEstimator Precursor;
    protected:
	virtual EbwDiscriminativeMixtureEstimator* createMixtureEstimator();
	virtual EbwDiscriminativeGaussDensityEstimator* createDensityEstimator();
	virtual EbwDiscriminativeGaussDensityEstimator* createDensityEstimator(const GaussDensity&);
	virtual EbwDiscriminativeMeanEstimator* createMeanEstimator();
	virtual EbwDiscriminativeMeanEstimator* createMeanEstimator(const Mean&);
	virtual EbwDiscriminativeCovarianceEstimator* createCovarianceEstimator();
	virtual EbwDiscriminativeCovarianceEstimator* createCovarianceEstimator(const Covariance&);

	virtual EbwDiscriminativeMixtureEstimator& mixtureEstimator(MixtureIndex mixture) {
	    return *required_cast(EbwDiscriminativeMixtureEstimator*,
				  mixtureEstimators_[mixture].get());
	}
	virtual bool accumulateMixture(Core::BinaryInputStreams &is, Core::BinaryOutputStream &os);
	virtual void initialize(const MixtureSetEstimatorIndexMap &, const CovarianceToMeanSetMap &);
    public:
	EbwDiscriminativeMixtureSetEstimator(const Core::Configuration &);
	virtual ~EbwDiscriminativeMixtureSetEstimator();

	virtual void read(Core::BinaryInputStream &);
	virtual void write(Core::BinaryOutputStream &);
    };

    /**
     * DiscriminativeMixtureSetEstimatorWithISmoothing: EBW
     */
    class EbwDiscriminativeMixtureSetEstimatorWithISmoothing :
	virtual public ISmoothingMixtureSetEstimator,
	public EbwDiscriminativeMixtureSetEstimator
    {
	typedef EbwDiscriminativeMixtureSetEstimator Precursor;
    protected:
	typedef ISmoothingMixtureSetEstimator ISmoothing;
    protected:
	virtual EbwDiscriminativeMixtureEstimatorWithISmoothing* createMixtureEstimator();
	virtual EbwDiscriminativeGaussDensityEstimatorWithISmoothing* createDensityEstimator();
	virtual EbwDiscriminativeGaussDensityEstimatorWithISmoothing* createDensityEstimator(const GaussDensity&);
	virtual EbwDiscriminativeMeanEstimatorWithISmoothing* createMeanEstimator();
	virtual EbwDiscriminativeMeanEstimatorWithISmoothing* createMeanEstimator(const Mean&);
	virtual EbwDiscriminativeCovarianceEstimatorWithISmoothing* createCovarianceEstimator();
	virtual EbwDiscriminativeCovarianceEstimatorWithISmoothing* createCovarianceEstimator(const Covariance&);

	virtual EbwDiscriminativeMixtureEstimatorWithISmoothing& mixtureEstimator(MixtureIndex mixture) {
	    return *required_cast(EbwDiscriminativeMixtureEstimatorWithISmoothing*,
				  mixtureEstimators_[mixture].get());
	}
	virtual void load();
	virtual void finalize(MixtureSet &, const MixtureSetEstimatorIndexMap &, const CovarianceToMeanSetMap &);
	virtual void setIMixture(Core::Ref<AbstractMixtureEstimator>, const Mixture *, Weight iSmoothing);
	virtual void setIDensity(Core::Ref<GaussDensityEstimator>, const GaussDensity *);
	virtual void setIMean(Core::Ref<AbstractMeanEstimator>, const Mean *, Weight iSmoothing);
	virtual void setICovariance(Core::Ref<AbstractCovarianceEstimator>, const Covariance *, Weight iSmoothing);
	virtual Weight getMixtureObjectiveFunction(Core::Ref<AbstractMixtureEstimator> estimator) {
	    return required_cast(EbwDiscriminativeMixtureEstimatorWithISmoothing*, estimator.get())->getObjectiveFunction();
	}
	virtual Weight getCovarianceObjectiveFunction(Core::Ref<AbstractCovarianceEstimator> estimator, const CovarianceToMeanSetMap &meanSetMap) {
	    return required_cast(EbwDiscriminativeCovarianceEstimatorWithISmoothing*, estimator.get())->getObjectiveFunction(meanSetMap);
	}
    public:
	EbwDiscriminativeMixtureSetEstimatorWithISmoothing(const Core::Configuration &);
	virtual ~EbwDiscriminativeMixtureSetEstimatorWithISmoothing();
    };

} //namespace Mm

#endif //_MM_EBW_DISCRIMINATIVE_MIXTURE_SET_ESTIMATOR_HH

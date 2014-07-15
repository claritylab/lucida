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
#ifndef _MM_RPROP_DISCRIMINATIVE_MIXTURE_SET_ESTIMATOR_HH
#define _MM_RPROP_DISCRIMINATIVE_MIXTURE_SET_ESTIMATOR_HH

#include "DiscriminativeMixtureSetEstimator.hh"
#include "RpropDiscriminativeMixtureEstimator.hh"
#include "ISmoothingMixtureSetEstimator.hh"

namespace Mm {

    /**
     * RpropDiscriminativeMixtureSetEstimator
     */
    class RpropDiscriminativeMixtureSetEstimator :
	public DiscriminativeMixtureSetEstimator
    {
	typedef DiscriminativeMixtureSetEstimator Precursor;
    protected:
	static const Core::ParameterFloat paramInitialStepSizeWeights;
	static const Core::ParameterFloat paramInitialStepSizeMeans;
	static const Core::ParameterFloat paramInitialStepSizeVariances;
	static const Core::ParameterString paramNewStepSizesFilename;
    protected:
	virtual RpropDiscriminativeMixtureEstimator* createMixtureEstimator();
	virtual RpropDiscriminativeGaussDensityEstimator* createDensityEstimator();
	virtual RpropDiscriminativeGaussDensityEstimator* createDensityEstimator(const GaussDensity&);
	virtual RpropDiscriminativeMeanEstimator* createMeanEstimator();
	virtual RpropDiscriminativeMeanEstimator* createMeanEstimator(const Mean&);
	virtual RpropDiscriminativeCovarianceEstimator* createCovarianceEstimator();
	virtual RpropDiscriminativeCovarianceEstimator* createCovarianceEstimator(const Covariance&);

	virtual RpropDiscriminativeMixtureEstimator& mixtureEstimator(MixtureIndex mixture) {
	    return *required_cast(RpropDiscriminativeMixtureEstimator*,
				  mixtureEstimators_[mixture].get());
	}
	virtual bool accumulateMixture(Core::BinaryInputStreams &is, Core::BinaryOutputStream &os);
	virtual const std::string magic() const { return "DTACCR"; }
	virtual void load();
	virtual void initialize(const MixtureSetEstimatorIndexMap &, const CovarianceToMeanSetMap &);
	virtual void finalize(MixtureSet &, const MixtureSetEstimatorIndexMap &, const CovarianceToMeanSetMap &);
	void loadStepSizes();
	void storeStepSizes();
	bool distributeStepSizes(const MixtureSet &stepSizes);
	bool distributeStepSizes(Weight stepSizeWeights, Weight stepSizeMeans, Weight stepSizeVariances);
	bool distributeSettings();
	void collectStepSizes(MixtureSet &stepSizes);
	void loadPreviousToPreviousMixtureSet();
    public:
	RpropDiscriminativeMixtureSetEstimator(const Core::Configuration &);
	virtual ~RpropDiscriminativeMixtureSetEstimator();

	virtual void read(Core::BinaryInputStream &);
	virtual void write(Core::BinaryOutputStream &);
	bool distributePreviousToPreviousMixtureSet(const MixtureSet &);
    };

    /**
     * DiscriminativeMixtureSetEstimatorWithISmoothing: Rprop
     */
    class RpropDiscriminativeMixtureSetEstimatorWithISmoothing :
	virtual public ISmoothingMixtureSetEstimator,
	public RpropDiscriminativeMixtureSetEstimator
    {
	typedef RpropDiscriminativeMixtureSetEstimator Precursor;
    protected:
	typedef ISmoothingMixtureSetEstimator ISmoothing;
    protected:
	virtual RpropDiscriminativeMixtureEstimatorWithISmoothing* createMixtureEstimator();
	virtual RpropDiscriminativeGaussDensityEstimatorWithISmoothing* createDensityEstimator();
	virtual RpropDiscriminativeGaussDensityEstimatorWithISmoothing* createDensityEstimator(const GaussDensity&);
	virtual RpropDiscriminativeMeanEstimatorWithISmoothing* createMeanEstimator();
	virtual RpropDiscriminativeMeanEstimatorWithISmoothing* createMeanEstimator(const Mean&);
	virtual RpropDiscriminativeCovarianceEstimatorWithISmoothing* createCovarianceEstimator();
	virtual RpropDiscriminativeCovarianceEstimatorWithISmoothing* createCovarianceEstimator(const Covariance&);

	virtual RpropDiscriminativeMixtureEstimatorWithISmoothing& mixtureEstimator(MixtureIndex mixture) {
	    return *required_cast(RpropDiscriminativeMixtureEstimatorWithISmoothing*,
				  mixtureEstimators_[mixture].get());
	}
	virtual void setIMixture(Core::Ref<AbstractMixtureEstimator>, const Mixture *, Weight iSmoothing);
	virtual void setIDensity(Core::Ref<GaussDensityEstimator>, const GaussDensity *);
	virtual void setIMean(Core::Ref<AbstractMeanEstimator>, const Mean *, Weight iSmoothing);
	virtual void setICovariance(Core::Ref<AbstractCovarianceEstimator>, const Covariance *, Weight iSmoothing);
	virtual Weight getMixtureObjectiveFunction(Core::Ref<AbstractMixtureEstimator> estimator) {
	    return required_cast(RpropDiscriminativeMixtureEstimatorWithISmoothing*, estimator.get())->getObjectiveFunction();
	}
	virtual Weight getCovarianceObjectiveFunction(Core::Ref<AbstractCovarianceEstimator> estimator, const CovarianceToMeanSetMap &meanSetMap) {
	    return required_cast(RpropDiscriminativeCovarianceEstimatorWithISmoothing*, estimator.get())->getObjectiveFunction(meanSetMap);
	}
	virtual void load();
	virtual void finalize(MixtureSet &, const MixtureSetEstimatorIndexMap &, const CovarianceToMeanSetMap &);
    public:
	RpropDiscriminativeMixtureSetEstimatorWithISmoothing(const Core::Configuration &);
	virtual ~RpropDiscriminativeMixtureSetEstimatorWithISmoothing();
    };

} //namespace Mm

#endif //_MM_RPROP_DISCRIMINATIVE_MIXTURE_SET_ESTIMATOR_HH

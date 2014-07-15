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
#ifndef _MM_ISMOOTHING_MIXTURE_SET_ESTIMATOR_HH
#define _MM_ISMOOTHING_MIXTURE_SET_ESTIMATOR_HH

#include <Core/Component.hh>
#include <Core/Parameter.hh>
#include "Types.hh"
#include "MixtureEstimator.hh"
#include "GaussDensityEstimator.hh"

namespace Mm {

    class DiscriminativeMixtureSetEstimator;
    class MixtureSet;
    class MixtureSetEstimatorIndexMap;
    class CovarianceToMeanSetMap;

    /**
     * ISmoothingMixtureSetEstimator
     */
    class ISmoothingMixtureSetEstimator :
	public virtual Core::Component
    {
    private:
	static const Core::ParameterFloat paramWeightsConstant;
	static const Core::ParameterFloat paramMeansConstant;
    private:
	DiscriminativeMixtureSetEstimator *parent_;
	Weight weightsConstant_;
	Weight meansConstant_;
    protected:
	virtual void setIMixture(Core::Ref<AbstractMixtureEstimator> estimator, const Mixture *mixture, Weight iSmoothing) = 0;
	virtual void setIDensity(Core::Ref<GaussDensityEstimator> estimator, const GaussDensity *density) = 0;
	virtual void setIMean(Core::Ref<AbstractMeanEstimator> estimator, const Mean *mean, Weight iSmoothing) = 0;
	virtual void setICovariance(Core::Ref<AbstractCovarianceEstimator> estimator, const Covariance *covariance, Weight iSmoothing) = 0;
	virtual Weight getMixtureObjectiveFunction(Core::Ref<AbstractMixtureEstimator>) = 0;
	virtual Weight getCovarianceObjectiveFunction(Core::Ref<AbstractCovarianceEstimator>, const CovarianceToMeanSetMap &) = 0;
	bool distributeISmoothingMixtureSet(const MixtureSet &);
    public:
	ISmoothingMixtureSetEstimator(const Core::Configuration &);

	void set(DiscriminativeMixtureSetEstimator *);
	void loadISmoothingMixtureSet();
	Sum getObjectiveFunction(const MixtureSet &, const MixtureSetEstimatorIndexMap &, const CovarianceToMeanSetMap &);
	Weight weightsConstant() const { return weightsConstant_; }
	Weight meansConstant() const { return meansConstant_; }
    };

} //namespace Mm

#endif //_MM_ISMOOTHING_MIXTURE_SET_ESTIMATOR_HH

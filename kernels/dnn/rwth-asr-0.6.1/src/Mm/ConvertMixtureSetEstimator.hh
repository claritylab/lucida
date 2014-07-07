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
#ifndef _MM_CONVERT_MIXTURE_SET_ESTIMATOR_HH
#define _MM_CONVERT_MIXTURE_SET_ESTIMATOR_HH

#include "AbstractMixtureSetEstimator.hh"
#include "MixtureSetEstimator.hh"
#include "ConvertMixtureEstimator.hh"
#include "ConvertGaussDensityEstimator.hh"

namespace Mm {

    /**
     * ConvertMixtureSetEstimator
     */
    class ConvertMixtureSetEstimator : public MixtureSetEstimator
    {
	typedef MixtureSetEstimator Precursor;
    protected:
	virtual ConvertMixtureEstimator* createMixtureEstimator();
	virtual ConvertGaussDensityEstimator* createDensityEstimator();
	virtual ConvertGaussDensityEstimator* createDensityEstimator(const GaussDensity&);
	virtual ConvertMeanEstimator* createMeanEstimator();
	virtual ConvertMeanEstimator* createMeanEstimator(const Mean&);
	virtual ConvertCovarianceEstimator* createCovarianceEstimator();
	virtual ConvertCovarianceEstimator* createCovarianceEstimator(const Covariance&);

	virtual ConvertMixtureEstimator& mixtureEstimator(MixtureIndex mixture) {
	    return *required_cast(ConvertMixtureEstimator*,
				  mixtureEstimators_[mixture].get());
	}
    public:
	ConvertMixtureSetEstimator(const Core::Configuration &);
	virtual ~ConvertMixtureSetEstimator() {}

	void setMixtureSet(Core::Ref<const MixtureSet>);
    };

} //namespace Mm

#endif //_MM_CONVERT_MIXTURE_SET_ESTIMATOR_HH

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
#ifndef _MM_CONVERT_MIXTURE_ESTIMATOR_HH
#define _MM_CONVERT_MIXTURE_ESTIMATOR_HH

#include "MixtureEstimator.hh"
#include "ConvertGaussDensityEstimator.hh"

namespace Mm {

    /**
     *  ConvertMixtureEstimator
     */
    class ConvertMixtureEstimator : public MixtureEstimator
    {
	typedef MixtureEstimator Precursor;
    public:
	ConvertMixtureEstimator() {}
	virtual ~ConvertMixtureEstimator() {}

	void setMixture(const Mixture *);
    };

} //namespace Mm

#endif //_MM_CONVERT_MIXTURE_ESTIMATOR_HH

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
#ifndef _MM_MIXTURE_SET_ESTIMATOR_HH
#define _MM_MIXTURE_SET_ESTIMATOR_HH

#include "AbstractMixtureSetEstimator.hh"
#include "MixtureEstimator.hh"
#include "FeatureScorer.hh"
#include "MixtureSet.hh"
#include "AssigningFeatureScorer.hh"
#include <Core/Component.hh>
#include <Core/Statistics.hh>

namespace Mm {

    /** Maximum Likelihood accumulator and estimator class for mixture sets
     *
     */
    class MixtureSetEstimator : public AbstractMixtureSetEstimator {
	typedef AbstractMixtureSetEstimator Precursor;
	friend class MixtureSetEstimatorIndexMap;
    protected:
	virtual const std::string magic() const { return "MIXSET"; }
    protected:
	virtual MixtureEstimator* createMixtureEstimator();
	virtual GaussDensityEstimator* createDensityEstimator();
	virtual GaussDensityEstimator* createDensityEstimator(const GaussDensity&);
	virtual MeanEstimator* createMeanEstimator();
	virtual MeanEstimator* createMeanEstimator(const Mean&);
	virtual CovarianceEstimator* createCovarianceEstimator();
	virtual CovarianceEstimator* createCovarianceEstimator(const Covariance&);
    public:
	MixtureSetEstimator(const Core::Configuration&);
	virtual ~MixtureSetEstimator() {}

	virtual bool accumulate(Core::BinaryInputStreams &is, Core::BinaryOutputStream &os);

	virtual void read(Core::BinaryInputStream &is);
	virtual void write(Core::BinaryOutputStream &os);
	virtual void write(Core::XmlWriter &os);
    };

} //namespace Mm

#endif //_MM_MIXTURE_SET_ESTIMATOR_HH

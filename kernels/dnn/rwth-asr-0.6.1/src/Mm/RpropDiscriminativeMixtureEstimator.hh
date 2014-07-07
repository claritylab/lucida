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
#ifndef _MM_RPROP_DISCRIMINATIVE_MIXTURE_ESTIMATOR_HH
#define _MM_RPROP_DISCRIMINATIVE_MIXTURE_ESTIMATOR_HH

#include "DiscriminativeMixtureEstimator.hh"
#include "RpropDiscriminativeGaussDensityEstimator.hh"
#include "RpropOptimization.hh"
#include "ISmoothingMixtureEstimator.hh"

namespace Mm {

    /**
     *  discriminative mixture estimator: Rprop
     */
    class RpropDiscriminativeMixtureEstimator :
	public DiscriminativeMixtureEstimator,
	public RpropOptimization<Weight>
    {
	friend class MixtureSetEstimatorIndexMap;
	typedef DiscriminativeMixtureEstimator Precursor;
    protected:
	typedef RpropOptimization<Weight> Rprop;
    protected:
	virtual void removeDensity(DensityIndex indexInMixture);
	virtual Weight previous(DensityIndex dns) const {
	    return logPreviousMixtureWeight(dns);
	}
	virtual Weight gradient(DensityIndex dns) const {
	    return weights_[dns];
	}
	Weight logPreviousMixtureWeight(DensityIndex dns) const {
	    const Weight tmp = previousMixtureWeight(dns);
	    return tmp > 0 ? log(tmp) : Core::Type<f32>::min;
	}
    public:
	RpropDiscriminativeMixtureEstimator(const Core::Configuration &);
	virtual ~RpropDiscriminativeMixtureEstimator() {}

	virtual Mixture* estimate(
	    const ReferenceIndexMap<GaussDensityEstimator> &densityMap, bool normalizeWeights = true);

	void setStepSizes(const Mixture *);
	void setStepSizes(Weight stepSize);
	Mixture* collectStepSizes(const ReferenceIndexMap<GaussDensityEstimator> &);
	void setPreviousToPreviousMixture(const Mixture *);

	static DensityIndex accumulate(Core::BinaryInputStreams &is, Core::BinaryOutputStream &os);
    };

    /**
     *  discriminative mixture estimator with i-smoothing: Rprop
     */
    class RpropDiscriminativeMixtureEstimatorWithISmoothing :
	public RpropDiscriminativeMixtureEstimator,
	public ISmoothingMixtureEstimator
    {
	typedef RpropDiscriminativeMixtureEstimator Precursor;
    protected:
	typedef ISmoothingMixtureEstimator ISmoothing;
    protected:
	virtual void removeDensity(DensityIndex indexInMixture);
	virtual Weight gradient(DensityIndex dns) const {
	    /* Assume that iMixtureWeights are normalized to unity.
	     */
	    return Precursor::gradient(dns) + ISmoothing::constant() * (iMixtureWeight(dns) - previousMixtureWeight(dns));
	}
    public:
	RpropDiscriminativeMixtureEstimatorWithISmoothing(const Core::Configuration &);
	virtual ~RpropDiscriminativeMixtureEstimatorWithISmoothing();

	virtual void clear();
    };

} //namespace Mm

#endif //_MM_RPROP_DISCRIMINATIVE_MIXTURE_ESTIMATOR_HH

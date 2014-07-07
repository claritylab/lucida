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
#ifndef _MM_DISCRIMINATIVE_MIXTURE_ESTIMATOR_HH
#define _MM_DISCRIMINATIVE_MIXTURE_ESTIMATOR_HH

#include "MixtureEstimator.hh"
#include "DiscriminativeGaussDensityEstimator.hh"
#include <Core/Parameter.hh>

namespace Mm {

    /**
     *  discriminative mixture estimator: base class
     */
    class DiscriminativeMixtureEstimator :
	public AbstractMixtureEstimator
    {
	friend class MixtureSetEstimatorIndexMap;
	typedef AbstractMixtureEstimator Precursor;
    private:
	static const Core::ParameterFloat paramMinObservationWeight;
    protected:
	std::vector<Weight> previousMixtureWeights_;
	Weight minimumObservationWeight_;
    protected:
	virtual void removeDensity(DensityIndex indexInMixture);
    public:
	DiscriminativeMixtureEstimator(const Core::Configuration &);
	virtual ~DiscriminativeMixtureEstimator();

	/** Removes densities whose absolute mixture weight has less weight than
	 *  @param minObservationWeight, or whose relative mixture weight is
	 *  less than @param minRelativeWeight. The density with the most
	 *  density(!)-weight is kept by all means.
	 */
	virtual void removeDensitiesWithLowWeight(
	    Weight minObservationWeight, Weight minRelativeWeight = 0);

	virtual void accumulate(const AbstractMixtureEstimator &toAdd);
	virtual void accumulateDenominator(DensityIndex, const FeatureVector &, Weight);
	const std::vector<Weight>& previousMixtureWeights() const {
	    verify(previousMixtureWeights_.size() == nDensities());
	    return previousMixtureWeights_;
	}
	Weight previousMixtureWeight(DensityIndex dns) const {
	    verify(dns < previousMixtureWeights_.size());
	    return previousMixtureWeights_[dns];
	}
	Weight logPreviousMixtureWeight(DensityIndex dns) const;
	void setPreviousMixture(const Mixture *);

	static DensityIndex accumulate(Core::BinaryInputStreams &is, Core::BinaryOutputStream &os);
    };

} //namespace Mm

#endif //_MM_DISCRIMINATIVE_MIXTURE_ESTIMATOR_HH

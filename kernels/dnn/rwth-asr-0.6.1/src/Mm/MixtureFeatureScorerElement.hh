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
#ifndef _MM_MIXTURE_FEATURE_SCORER_ELEMENT_HH
#define _MM_MIXTURE_FEATURE_SCORER_ELEMENT_HH

#include "Mixture.hh"

namespace Mm {

    /** MixtureFeatureScorerElement
     */
    class MixtureFeatureScorerElement : public MixtureTopology {
    private:
	/** - 2 log(c_i)
	 */
	std::vector<Score> minus2LogWeights_;
    public:
	MixtureFeatureScorerElement() {}

	void operator=(const Mixture &mixture);
	void scale(Score scale);
	const std::vector<Score>& minus2LogWeights() const { return minus2LogWeights_; }
    };


    /** Container for precalculated data within a mixture
     *  Precalculated members for each density:
     *    -prepared mean,
     *    -constant precalucated weight
     *    -and convariance index which references the corresponding precalculated feature vector.
     */
    template<class T>
    class QuantizedMixtureFeatureScorerElement {
    public:
	struct Density {
	    /**
	     *  Mmx:
	     *    m_d / sqrt(var_d), where
	     *    m_d is d-th element of the mean of the density,
	     *    var_d d-th diagonal element of covariance matrix of the density
	     *  Alpha:
	     *    Mmx + packed
	     *
	     */
	    std::vector<T> preparedMean_;

	    DensityIndex covarianceIndex_;

	    /** Mmx:
	     *   -2 * log(c) + sum_d (log(2*pi * var_d)), where
	     *    c weight in the mixture
	     *    var_d d-th diagonal element of covariance matrix of the density
	     * Alpha:
	     *   Mmx + sum_d (m_d^2), where
	     *   m_d d-th element of mean vector of the density divided by sqrt(var_d).
	     */
	    s32 constantWeight_;
	};
    private:
	std::vector<Density> densities_;
    public:
	QuantizedMixtureFeatureScorerElement() {}

	void setNumberOfDensities(size_t size) { densities_.resize(size); }
	size_t nDensities() const { return densities_.size(); }

	Density& density(size_t densityInMixture) { return densities_[densityInMixture]; }
	const Density& density(size_t densityInMixture) const { return densities_[densityInMixture]; }
    };

} //namespace Mm

#endif //_MM_MIXTURE_FEATURE_SCORER_ELEMENT_HH

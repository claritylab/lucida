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
#ifndef _MM_MIXTURE_SET_BUILDER_HH
#define _MM_MIXTURE_SET_BUILDER_HH

#include <Core/Configurable.hh>
#include <Core/Parameter.hh>
#include "MixtureSet.hh"

namespace Mm {

    /** MixtureSetTopologyBuilder creates a mixture set with independend densities.
     */
    class MixtureSetBuilder {
    private:
	MixtureIndex nMixtures_;
	size_t nDensitiesPerMixture_;
    protected:
	virtual DensityIndex densityIndex(MixtureIndex mixtureIndex, DensityIndex densityInMixture) {
	    return mixtureIndex * nDensitiesPerMixture_ + densityInMixture;
	}
	virtual MeanIndex meanIndex(MixtureIndex mixtureIndex, DensityIndex densityInMixture) {
	    return densityIndex(mixtureIndex, densityInMixture);
	}
	virtual CovarianceIndex covarianceIndex(MixtureIndex mixtureIndex, DensityIndex densityInMixture) {
	    return densityIndex(mixtureIndex, densityInMixture);
	}
    public:
	MixtureSetBuilder(MixtureIndex nMixtures, size_t nDensitiesPerMixture = 1);
	virtual ~MixtureSetBuilder() {}
	void build(MixtureSet &toBuild, ComponentIndex dimension = 0);
    };

    /** PooledCovarianceTopologyBuilder creates a mixture set with
     *  one pooled covariance matrix.
     */
    class PooledCovarianceBuilder : public MixtureSetBuilder {
    protected:
	virtual CovarianceIndex covarianceIndex(MixtureIndex mixtureIndex, DensityIndex densityIndex) {
	    return 0;
	}
    public:
	PooledCovarianceBuilder(size_t nMixtures, size_t nDensitiesPerMixture = 1) :
	    MixtureSetBuilder(nMixtures, nDensitiesPerMixture) {}
    };

    /** MixtureSpecificCovarianceTopologyBuilder creates a mixture set with
     *  mixture specific covariance matrices.
     */
    class MixtureSpecificCovarianceBuilder : public MixtureSetBuilder {
    protected:
	virtual CovarianceIndex covarianceIndex(MixtureIndex mixtureIndex, DensityIndex densityIndex) {
	    return mixtureIndex;
	}
    public:
	MixtureSpecificCovarianceBuilder(MixtureIndex nMixtures, size_t nDensitiesPerMixture = 1) :
	    MixtureSetBuilder(nMixtures, nDensitiesPerMixture) {}
    };

} // namespace Mm

#endif //_MM_MIXTURE_SET_BUILDER_HH

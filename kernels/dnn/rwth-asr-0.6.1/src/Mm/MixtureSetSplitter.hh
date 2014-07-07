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
#ifndef _MM_MIXTURE_SET_SPLITTER_HH
#define _MM_MIXTURE_SET_SPLITTER_HH

#include <Core/Component.hh>
#include "MixtureSetEstimator.hh"

namespace Mm {

    class MixtureSetSplitter : public virtual Core::Component {
    public:
	typedef Core::Component Precursor;
    public:
	static const Core::ParameterFloat paramMinMeanObservationWeight;
	static const Core::ParameterFloat paramMinCovarianceObservationWeight;
	static const Core::ParameterFloat paramPerturbationWeight;
	static const Core::ParameterBool paramNormalizeMixtureWeights;
    private:
	std::vector<MeanIndex> splittedMeans_;
	Weight minMeanObservationWeight_;
	Weight perturbationWeight_;
	void splitMeans(MixtureSet &mixtureSet, const std::vector<Weight>& weights);
	MeanIndex splitMean(MixtureSet &mixtureSet, MeanIndex index, std::vector<MeanType> &perturbation);

	std::vector<CovarianceIndex> splittedCovariances_;
	Weight minCovarianceObservationWeight_;
	void splitCovariances(MixtureSet &mixtureSet, const std::vector<Weight>& weights);
	CovarianceIndex splitCovariance(MixtureSet &mixtureSet, CovarianceIndex index);

	std::vector<DensityIndex> splittedDensities_;
	void splitDensities(MixtureSet &mixtureSet);

	void splitDensitiesInMixtures(MixtureSet &mixtureSet);

	bool normalizeMixtureWeights_;
    public:
	MixtureSetSplitter(const Core::Configuration &configuration);

	Core::Ref<MixtureSet> split(AbstractMixtureSetEstimator&);
    };

} //namespace Mm

#endif //_MM_MIXTURE_ESTIMATOR_HH

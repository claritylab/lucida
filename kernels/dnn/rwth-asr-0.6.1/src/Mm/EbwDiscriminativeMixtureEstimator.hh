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
#ifndef _MM_EBW_DISCRIMINATIVE_MIXTURE_ESTIMATOR_HH
#define _MM_EBW_DISCRIMINATIVE_MIXTURE_ESTIMATOR_HH

#include "DiscriminativeMixtureEstimator.hh"
#include "EbwDiscriminativeGaussDensityEstimator.hh"
#include "ISmoothingMixtureEstimator.hh"

namespace Mm {

    /**
     *  discriminative mixture estimator: EBW
     */
    class EbwDiscriminativeMixtureEstimator :
	public DiscriminativeMixtureEstimator
    {
	friend class MixtureSetEstimatorIndexMap;
	typedef DiscriminativeMixtureEstimator Precursor;
    private:
	static const Core::ParameterInt paramNumberOfIterations;
    protected:
	std::vector<Weight> denWeights_;
	u32 nIterations_;
    protected:
	virtual void removeDensity(DensityIndex indexInMixture);
	void estimateMixtureWeights(std::vector<Weight> &, bool normalizeWeights = true) const;
	virtual Weight weight(DensityIndex dns) const { return weights_[dns]; }
    public:
	EbwDiscriminativeMixtureEstimator(const Core::Configuration &);
	virtual ~EbwDiscriminativeMixtureEstimator() {}

	virtual void addDensity(Core::Ref<GaussDensityEstimator>);
	virtual void clear();

	virtual void accumulate(const AbstractMixtureEstimator &toAdd);
	virtual void accumulateDenominator(DensityIndex, const FeatureVector &, Weight);
	virtual void reset();
	virtual Mixture* estimate(
	    const ReferenceIndexMap<GaussDensityEstimator> &densityMap, bool normalizeWeights = true);

	virtual void read(Core::BinaryInputStream &,
			  const std::vector<Core::Ref<GaussDensityEstimator> > &,
			  u32 version);
	virtual void write(Core::BinaryOutputStream &,
			   const ReferenceIndexMap<GaussDensityEstimator> &) const;
	virtual void write(Core::XmlWriter &,
			   const ReferenceIndexMap<GaussDensityEstimator> &) const;

	virtual bool equalWeights(const AbstractMixtureEstimator &toCompare) const;

	Weight getDenWeight() const { return std::accumulate(denWeights_.begin(), denWeights_.end(), 0.0); }

	static DensityIndex accumulate(Core::BinaryInputStreams &is, Core::BinaryOutputStream &os);
    };

    /**
     *  discriminative mixture estimator with i-smoothing: EBW
     */
    class EbwDiscriminativeMixtureEstimatorWithISmoothing :
	public EbwDiscriminativeMixtureEstimator,
	public ISmoothingMixtureEstimator
    {
	friend class MixtureSetEstimatorIndexMap;
	typedef EbwDiscriminativeMixtureEstimator Precursor;
    protected:
	typedef ISmoothingMixtureEstimator ISmoothing;
    protected:
	virtual void removeDensity(DensityIndex indexInMixture);
	virtual Weight weight(DensityIndex dns) const {
	    return Precursor::weight(dns) + iMixtureWeight(dns) * ISmoothing::constant();
	}
    public:
	EbwDiscriminativeMixtureEstimatorWithISmoothing(const Core::Configuration &);
	virtual ~EbwDiscriminativeMixtureEstimatorWithISmoothing();

	virtual void clear();
    };

} //namespace Mm

#endif //_MM_EBW_DISCRIMINATIVE_MIXTURE_ESTIMATOR_HH

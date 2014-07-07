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
#ifndef _MM_MIXTURE_ESTIMATOR_HH
#define _MM_MIXTURE_ESTIMATOR_HH

#include <Core/ReferenceCounting.hh>
#include "Mixture.hh"
#include "GaussDensityEstimator.hh"
#include <numeric>

namespace Mm {

    /**
     *  AbstractMixtureEstimator: base class
     */
    class AbstractMixtureEstimator : public Core::ReferenceCounted
    {
    public:
	friend class MixtureSetEstimatorIndexMap;
	typedef std::vector<Core::Ref<GaussDensityEstimator> > DensityEstimators;
    protected:
	DensityEstimators densityEstimators_;
	std::vector<Weight> weights_;
    protected:
	DensityIndex densityIndexWithMaxWeight() const;
	virtual void removeDensity(DensityIndex indexInMixture);
    public:
	AbstractMixtureEstimator() {}
	virtual ~AbstractMixtureEstimator() {}

	virtual void addDensity(Core::Ref<GaussDensityEstimator> densityEstimator);
	virtual void clear();
	/** Removes densities whose mean estimator has weight 0.0.
	 *  If all densities have weight 0.0, the first is kept.
	 */
	void removeDensitiesWithZeroWeight();
	/** Removes densities whose absolute mixture weight has less weight than
	 *  @param minObservationWeight, or whose relative mixture weight is
	 *  less than @param minRelativeWeight. The density with the most
	 *  density(!)-weight is kept by all means.
	 */
	virtual void removeDensitiesWithLowWeight(
	    Weight minObservationWeight, Weight minRelativeWeight = 0);

	void accumulate(DensityIndex, const FeatureVector &);
	void accumulate(DensityIndex, const FeatureVector &, Weight);
	virtual void accumulate(const AbstractMixtureEstimator &toAdd);
	virtual void reset();

	virtual Mixture* estimate(const ReferenceIndexMap<GaussDensityEstimator>& densityMap, bool normalizeWeights = true);
	Weight getWeight() const { return std::accumulate(weights_.begin(), weights_.end(), 0.0); }
	size_t nDensities() const { return densityEstimators_.size(); }
	const DensityEstimators& densityEstimators() const { return densityEstimators_; }
	std::vector<Weight>& weights() { return weights_; }
	const std::vector<Weight>& weights() const { return weights_; }

	void setDensityEstimators(DensityEstimators estimators) {
	    densityEstimators_ = estimators;
	}

	bool checkEventsWithZeroWeight(std::string &message);

	virtual void read(Core::BinaryInputStream&,
			  const std::vector<Core::Ref<GaussDensityEstimator> >&,
			  u32 version);
	virtual void write(Core::BinaryOutputStream&,
			   const ReferenceIndexMap<GaussDensityEstimator>&) const;
	virtual void write(Core::XmlWriter&,
			   const ReferenceIndexMap<GaussDensityEstimator>&) const;

	bool equalTopology(const AbstractMixtureEstimator &toCompare,
			   const ReferenceIndexMap<GaussDensityEstimator> &densityMap,
			   const ReferenceIndexMap<GaussDensityEstimator> &densityMapToCompare) const;
	virtual bool equalWeights(const AbstractMixtureEstimator &toCompare) const {
	    return weights_ == toCompare.weights_;
	}

	void addMixture(const AbstractMixtureEstimator &toAdd);

	static DensityIndex accumulate(Core::BinaryInputStreams &is, Core::BinaryOutputStream &os);
    };

    /**
     *  maximum likelihood MixtureEstimator
     */
    class MixtureEstimator : public AbstractMixtureEstimator
    {
	friend class MixtureSetEstimatorIndexMap;
	typedef AbstractMixtureEstimator Precursor;
    public:
	MixtureEstimator() {}
	virtual ~MixtureEstimator() {}
    };

} //namespace Mm

#endif //_MM_MIXTURE_ESTIMATOR_HH

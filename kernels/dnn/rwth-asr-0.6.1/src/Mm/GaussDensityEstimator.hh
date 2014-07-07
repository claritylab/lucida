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
#ifndef _MM_GAUSS_DENSITY_ESTIMATOR_HH
#define _MM_GAUSS_DENSITY_ESTIMATOR_HH

#include "Types.hh"
#include "Utilities.hh"
#include "PointerIndexMap.hh"
#include "VectorAccumulator.hh"
#include "GaussDensity.hh"
#include "Utilities.hh"
#include <Core/Hash.hh>
#include <Core/ReferenceCounting.hh>
#include <Core/XmlStream.hh>

namespace Mm {

    class AbstractMeanEstimator;
    class AbstractCovarianceEstimator;
    class CovarianceToMeanSetMap;

    /** GaussDensityEstimator
     */
    class GaussDensityEstimator : public Core::ReferenceCounted {
    public:
	friend class MixtureSetEstimatorIndexMap;
	friend class CovarianceToMeanSetMap;
    protected:
	Core::Ref<AbstractMeanEstimator> meanEstimator_;
	Core::Ref<AbstractCovarianceEstimator> covarianceEstimator_;
    public:
	GaussDensityEstimator() {}

	void setMean(Core::Ref<AbstractMeanEstimator> estimator) {
	    meanEstimator_ = estimator;
	}
	const Core::Ref<AbstractMeanEstimator> mean() const { return meanEstimator_; }

	void setCovariance(Core::Ref<AbstractCovarianceEstimator> estimator) {
	    covarianceEstimator_ = estimator;
	}
	const Core::Ref<AbstractCovarianceEstimator> covariance() const { return covarianceEstimator_; }

	void accumulate(const FeatureVector&);
	void accumulate(const FeatureVector&, Weight weight);
	void reset();

	GaussDensity* estimate(const ReferenceIndexMap<AbstractMeanEstimator>&,
			       const ReferenceIndexMap<AbstractCovarianceEstimator>&) const;

	void read(Core::BinaryInputStream&,
		  const std::vector<Core::Ref<AbstractMeanEstimator> >&,
		  const std::vector<Core::Ref<AbstractCovarianceEstimator> >&);
	void write(Core::BinaryOutputStream&,
		   const ReferenceIndexMap<AbstractMeanEstimator>&,
		   const ReferenceIndexMap<AbstractCovarianceEstimator>&) const;
	void write(Core::XmlWriter&,
		   const ReferenceIndexMap<AbstractMeanEstimator>&,
		   const ReferenceIndexMap<AbstractCovarianceEstimator>&) const;

	bool equalTopology(const GaussDensityEstimator &toCompare,
			   const ReferenceIndexMap<AbstractMeanEstimator> &meanMap,
			   const ReferenceIndexMap<AbstractCovarianceEstimator> &covarianceMap,
			   const ReferenceIndexMap<AbstractMeanEstimator> &meanMapToCompare,
			   const ReferenceIndexMap<AbstractCovarianceEstimator> &covarianceMapToCompare) const;

	static bool accumulate(Core::BinaryInputStreams &is, Core::BinaryOutputStream &os);
    };

    /** AbstractEstimationAccumulator
     *
     *	Due to lack of multiple dispatch in C++, the 'accumulate(Self)',
     *	'operator==' and 'operator!=' methods  of derived classes must accept an
     *	argument of type 'AbstractEstimationAccumulator' and dynamically cast it
     *	to its own type.
     */
    class AbstractEstimationAccumulator: public Core::ReferenceCounted {
    public:
	typedef AbstractEstimationAccumulator Self;

	virtual ~AbstractEstimationAccumulator() {}

	virtual void reset() = 0;
	virtual void accumulate(const std::vector<FeatureType>&) = 0;
	virtual void accumulate(const std::vector<FeatureType>&, Weight) = 0;
	virtual void accumulate(const Self&) = 0;

	virtual void read(Core::BinaryInputStream&, u32) = 0;
	virtual void write(Core::BinaryOutputStream&) const = 0;
	virtual void write(Core::XmlWriter&) const = 0;

	virtual bool operator==(const Self&) const = 0;
	bool operator!=(const Self& other) const {
	    return !(*this == other);
	}
    };

    /** AbstractMeanEstimator
     */
    class AbstractMeanEstimator: public AbstractEstimationAccumulator {
    public:
	virtual ~AbstractMeanEstimator() {}

	virtual Mean* estimate() = 0;
	virtual Weight weight() const = 0;
    };

    /** AbstractCovarianceEstimator
     */
    class AbstractCovarianceEstimator: public AbstractEstimationAccumulator {
    protected:
	virtual Covariance* estimate(const CovarianceToMeanSetMap&) { defect(); }
    public:
	virtual ~AbstractCovarianceEstimator() {}

	/**
	 *  Overload this function to perform covariance estimation.
	 *  The @c map contains mean estimator references for each covariance estimator.
	 *  The @c minimumVariance specifies the minimum value in covariance matrix.
	 *    If minimum variance is not supported by a derived estimator class,
	 *    overload the protected estimate function without this additional parameter.
	 */
	virtual Covariance* estimate(const CovarianceToMeanSetMap &map, VarianceType minimumVariance) {
	    return estimate(map);
	}
	virtual Weight weight() const = 0;
    };

    /** MeanEstimator
     */
    class MeanEstimator : public AbstractMeanEstimator {
    public:
	typedef VectorAccumulator<FeatureType, std::plus<Sum>, plusWeighted<Sum> > Accumulator;
    protected:
	Accumulator accumulator_;
    public:
	MeanEstimator(ComponentIndex dimension = 0);
	virtual ~MeanEstimator() {}

	virtual void reset();
	virtual void accumulate(const std::vector<FeatureType>& v);
	virtual void accumulate(const std::vector<FeatureType>& v, Weight weight);
	virtual void accumulate(const AbstractEstimationAccumulator& other);

	virtual void read(Core::BinaryInputStream &i, u32 version);
	virtual void write(Core::BinaryOutputStream &o) const;
	virtual void write(Core::XmlWriter &o) const;

	virtual bool operator==(const AbstractEstimationAccumulator& other) const;

	const std::vector<Accumulator::SumType>& sum() const { return accumulator_.sum(); }
	virtual Weight weight() const { return accumulator_.weight(); }
	virtual Mean* estimate();
    };

    /** CovarianceEstimator
     */
    class CovarianceEstimator : public AbstractCovarianceEstimator {
    public:
	typedef VectorAccumulator<FeatureType, plusSquare<Sum>, plusSquareWeighted<Sum> > Accumulator;
	/** sum_j ( N_j * mean_j^2 ) = sum_j ( acc_j^2 / N_j )  where
	 *    - j goes over means sharing the same covariance
	 *    - mean_j mean vector estimated over acc_j sum of observations
	 *    - N_j number of observation vectors
	 */
	typedef VectorAccumulator<Sum, plusNormalizedSquare<Sum>, plusNormalizedSquare<Sum> > WeighedMeanSquareSum;
    protected:
	Accumulator accumulator_;
    protected:
	void applyMinimumVariance(VarianceType minimumVariance, DiagonalCovariance&) const;
    public:
	CovarianceEstimator(ComponentIndex dimension = 0);
	virtual ~CovarianceEstimator() {}

	virtual void reset();
	virtual void accumulate(const std::vector<FeatureType>& v);
	virtual	void accumulate(const std::vector<FeatureType>& v, Weight weight);
	virtual void accumulate(const AbstractEstimationAccumulator& other);

	virtual void read(Core::BinaryInputStream &i, u32 version);
	virtual void write(Core::BinaryOutputStream &o) const;
	virtual void write(Core::XmlWriter &o) const;

	virtual bool operator==(const AbstractEstimationAccumulator& other) const;

	const std::vector<Accumulator::SumType>& sum() const { return accumulator_.sum(); }
	virtual Weight weight() const { return accumulator_.weight(); }
	virtual Covariance* estimate(const CovarianceToMeanSetMap&,
				     VarianceType minimumVariance);
    };

    /** CovarianceToMeanSetMap
     */
    class CovarianceToMeanSetMap {
    public:
	typedef Core::hash_set<
	Core::Ref<AbstractMeanEstimator>,
	hashReference<AbstractMeanEstimator> > MeanSet;
	typedef Core::hash_map<
	    Core::Ref<AbstractCovarianceEstimator>, MeanSet,
	    hashReference<AbstractCovarianceEstimator> > Map;
	typedef Map::const_iterator const_iterator;
    private:
	Map map_;
    public:
	CovarianceToMeanSetMap(const std::vector<Core::Ref<GaussDensityEstimator> >&);

	const MeanSet& operator[](Core::Ref<AbstractCovarianceEstimator>) const;
	const_iterator begin() const { return map_.begin(); }
	const_iterator end() const { return map_.end(); }
    };

} //namespace Mm

#endif //_MM_GAUSS_DENSITY_ESTIMATOR_HH

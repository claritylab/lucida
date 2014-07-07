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
#ifndef _MM_ABSTRACT_MIXTURE_SET_ESTIMATOR_HH
#define _MM_ABSTRACT_MIXTURE_SET_ESTIMATOR_HH

#include "MixtureEstimator.hh"
#include "FeatureScorer.hh"
#include "MixtureSet.hh"
#include "AssigningFeatureScorer.hh"
#include <Core/Component.hh>
#include <Core/Statistics.hh>
#include <Core/Channel.hh>

namespace Mm {

    /** Base Class for accumulator and estimator class for mixture sets
     *
     *  Override the component creation functions to create concrete density,
     *  mean and covariance objects.
     *
     *  Data structure:
     *    -Graph represenation: objects reference each other by smart-pointers (Core::Reference)
     *    -Root elements of the graph (MixtureEstimator) are stored in a list
     *    Advantage of smart-pointers:
     *      -object types can be extended be deriving from the base types and
     *       without changing (much :-) ) in other parts of this library
     *    Advantages of graph representation:
     *      -easy to remove elements from the structure
     *      -fast access of child elements thus suitable for fast computations
     *    Disadvantages:
     *      -no direct access to set of elements (e.g.: list of all covariances)
     *      -storing and load is not straight forward
     *      To overcome these disadvatages MixtureSetEstimatorIndexMap class converts the graph
     *      representation into a reference-table representation.
     */
    class AbstractMixtureSetEstimator :
		public Core::ReferenceCounted,
		public virtual Core::Component
    {
		typedef Core::Component Precursor;
		friend class MixtureSetEstimatorIndexMap;
		friend class MixtureSetEstimator;
    public:
		typedef std::vector<Core::Ref<AbstractMixtureEstimator> > MixtureEstimators;
    public:
		enum Mode {
			modeViterbi,
			modeBaumWelch
		};
		static const Core::Choice          choiceMode;
		static const Core::ParameterChoice paramMode;
		static const Core::ParameterFloat  paramMinObservationWeight;
		static const Core::ParameterFloat  paramMinRelativeWeight;
		static const Core::ParameterFloat  paramMinVariance;
		static const Core::ParameterBool   paramAllowZeroWeights;
		static const Core::ParameterBool   paramNormalizeMixtureWeights;

    protected:
		Core::Ref<const AssigningFeatureScorer> assigningFeatureScorer_;
		bool viterbi_;
		Weight weightThreshold_;
		Weight minObservationWeight_;
		Weight minRelativeWeight_;
		VarianceType minVariance_;
		bool allowZeroWeights_;
		bool normalizeMixtureWeights_;
		Core::Statistics<f64> scoreStatistics_;
		ProbabilityStatistics weightStats_, dnsPosteriorStats_, finalWeightStats_;
		mutable Core::XmlChannel accumulationChannel_;
		MixtureEstimators mixtureEstimators_;
		ComponentIndex dimension_;
		u32 version_;
    protected:
		void removeDensitiesWithZeroWeight();
		void removeDensitiesWithLowWeight(
										  Weight minObservationWeight, Weight minRelativeWeight);
		DensityIndex densityIndex(MixtureIndex, Core::Ref<const Feature::Vector>);
		void getDensityPosteriorProbabilities(MixtureIndex, Core::Ref<const Feature::Vector>,
											  std::vector<Mm::Weight>&);
		void readHeader(Core::BinaryInputStream &);
		void writeHeader(Core::BinaryOutputStream &);

		virtual bool accumulateDensity(Core::BinaryInputStreams &is, Core::BinaryOutputStream &os);
		virtual bool accumulateMixture(Core::BinaryInputStreams &is, Core::BinaryOutputStream &os);
		virtual const std::string magic() const = 0;
    protected:
		virtual AbstractMixtureEstimator* createMixtureEstimator() = 0;
		virtual GaussDensityEstimator* createDensityEstimator() = 0;
		virtual GaussDensityEstimator* createDensityEstimator(const GaussDensity&) = 0;
		virtual AbstractMeanEstimator* createMeanEstimator() = 0;
		virtual AbstractMeanEstimator* createMeanEstimator(const Mean&) = 0;
		virtual AbstractCovarianceEstimator* createCovarianceEstimator() = 0;
		virtual AbstractCovarianceEstimator* createCovarianceEstimator(const Covariance&) = 0;

		virtual AbstractMixtureEstimator& mixtureEstimator(MixtureIndex mixture) {
			return *mixtureEstimators_[mixture];
		}
    public:
		AbstractMixtureSetEstimator(const Core::Configuration&);
		virtual ~AbstractMixtureSetEstimator() {}

		void setTopology(Core::Ref<const MixtureSet>);
		MixtureIndex nMixtures() const { return mixtureEstimators_.size(); }
		ComponentIndex dimension() const { return dimension_; }

		MixtureEstimators* getMixtureEstimators(){ return &mixtureEstimators_; }

		Weight minObservationWeight() const { return minObservationWeight_; }
		Weight minRelativeWeight() const { return minRelativeWeight_; }

		void checkEventsWithZeroWeight();

		void setAssigningFeatureScorer(
									   Core::Ref<const AssigningFeatureScorer > assigningFeatureScorer) {
			assigningFeatureScorer_ = assigningFeatureScorer;
		}
		void accumulate(MixtureIndex mixtureIndex, Core::Ref<const Feature::Vector>);
		void accumulate(MixtureIndex mixtureIndex, Core::Ref<const Feature::Vector>, Weight);
		virtual bool accumulate(const AbstractMixtureSetEstimator&);
		virtual bool accumulate(Core::BinaryInputStreams &is, Core::BinaryOutputStream &os);
		virtual void reset();

		/**
		 * add new mixture estimators
		 */
		bool addMixtureEstimators(const AbstractMixtureSetEstimator &estimator);

		/**
		 * accumulate a single covariance estimator from all estimators.
		 * update all density estimators to use this covariance.
		 */
		bool createPooledCovarianceEstimator();
		/**
		 * create a copy of a pooled covariance estimator
		 * and set it as mixture specific covariance estimator
		 * in all densities
		 */
		bool createMixtureSpecificCovarianceEstimator();

		Core::Ref<MixtureSet> estimate();
		virtual void estimate(MixtureSet&);
		void estimate(MixtureSet&,
					  std::vector<Weight> &meanObservationWeights,
					  std::vector<Weight> &covarianceObservationWeights);

		virtual void read(Core::BinaryInputStream&) = 0;
		virtual void write(Core::BinaryOutputStream&) = 0;
		virtual void write(Core::XmlWriter&) = 0;

		bool equalTopology(const AbstractMixtureSetEstimator&) const;
		virtual bool operator==(const AbstractMixtureSetEstimator&) const;

		void writeStatistics() const;

		bool map(const std::string &mapping);
		bool changeDensityMixtureAssignment(std::vector< std::vector<DensityIndex> > stateDensityTable);

		void setWeightThreshold(Weight weightThreshold) {
			weightThreshold_ = weightThreshold;
		}
    };

    /** Reference table representation of AbstractMixtureSetEstimator
     *
     *  Density object are collected in densityMap_, mean objects in meanMap_,
     *  and covariance objects in covarianceMap_.
     *  Each map supports following to operations:
     *    -reference->index: map[Core::Ref<..> object] -> size_t index
     *    -index->reference: map[size_t index]->size_t -> Core::Ref<..> object
     */
    class MixtureSetEstimatorIndexMap
    {
    private:
		ReferenceIndexMap<GaussDensityEstimator> densityMap_;
		ReferenceIndexMap<AbstractMeanEstimator> meanMap_;
		ReferenceIndexMap<AbstractCovarianceEstimator> covarianceMap_;
    public:
		MixtureSetEstimatorIndexMap(const AbstractMixtureSetEstimator&);

		const ReferenceIndexMap<GaussDensityEstimator>& densityMap() const { return densityMap_; }
		const ReferenceIndexMap<AbstractMeanEstimator>& meanMap() const { return meanMap_; }
		const ReferenceIndexMap<AbstractCovarianceEstimator>& covarianceMap() const { return covarianceMap_; }
    };


	/** defines a mixture-to-mixture id mapping based on a mapping file */
	class MixtureToMixtureMap :
		public std::vector< Core::hash_set<MixtureIndex> >
	{
	public:
		typedef Core::hash_set<MixtureIndex> MixtureSet;
		typedef std::vector< MixtureSet > MixtureMap;
	protected:
		u32 nOfMappedMixtures_;
	public:
		MixtureSet& mixtureSet(size_t m);
		virtual ~MixtureToMixtureMap(){}
		/** loads a white-space separated mixture-to-mixture id mapping file */
		virtual bool load(const std::string &filename);
		virtual bool save(const std::string &filename) const;
		u32 nMixtures() const { return size() ;}
		u32 nMappedMixtures() const { return nOfMappedMixtures_ ;}
	};


} //namespace Mm

#endif //_MM_ABSTRACT_MIXTURE_SET_ESTIMATOR_HH

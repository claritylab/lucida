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
#ifndef _MM_MODULE_HH
#define _MM_MODULE_HH

#include <Modules.hh>
#include <Core/Directory.hh>
#include <Core/Singleton.hh>
#include "MixtureSetEstimator.hh"
#include "AssigningFeatureScorer.hh"
#include "ScaledFeatureScorer.hh"
//#ifdef MODULE_MM_DT
//#include "DiscriminativeMixtureSetEstimator.hh"
//#endif

namespace Core
{
    class FormatSet;
}

#ifdef MODULE_MM_DT
namespace Mm {
class DiscriminativeMixtureSetEstimator;
}
#endif

namespace Mm {

    class FeatureScorerFactory;

    class Module_
    {
    public:

	/* @todo move the parameters to another place or create methods in Mm::Module_ */
	static const Core::ParameterString paramMixtureSetFilename;
	static const Core::ParameterString paramNewMixtureSetFilename;
	static const Core::ParameterInt paramReducedMixtureSetDimension;
	static const Core::ParameterInt paramReducedMixtureSetDimensionOffset;
	static const Core::ParameterInt paramMixtureSetDimension;

	enum FeatureScorerType {
	    batchFloatDiagonalMaximum,
	    batchPreselectionFloatDiagonalMaximum,
	    batchPreselectionIntDiagonalMaximum,
	    batchIntDiagonalMaximum,
	    batchIntDiagonalMaximumFast,
	    diagonalMaximum,
	    simdDiagonalMaximum,
	    preselectionSimdDiagonalMaximum,
	    diagonalMaximumWeightedFeature,
	    diagonalSum,
	    fastFloatLogLinear,
	    logLinear,
	    simdLogLinear,
	    nearestNeighbor,
	    statePosterior,
	    passThrough,
	    hybridPassThrough,
	    invalidFeatureScorer /* add all valid feature scorers in Mm before this line */
	};
	// static const Core::Choice choiceFeatureScorerType;



	enum CovarianceTyingType {
	    noCovarianceTying,
	    pooledCovariance,
	    mixtureSpecificCovariance
	};

	static const Core::Choice choiceCovarianceTyingType;
	static const Core::ParameterChoice paramCovarianceTyingType;

	enum EstimatorType {
	    maximumLikelihood,
	    discriminative,
	    legacyDiscriminativeWithISmoothing,
	    discriminativeWithISmoothing,
	    rPropDiscriminative,
	    rPropDiscriminativeWithISmoothing,
	};

	static const Core::Choice choiceEstimatorType;
	static const Core::ParameterChoice paramEstimatorType;

    private:
	Core::FormatSet *formats_;
	FeatureScorerFactory *featureScorerFactory_;

    public:
	Core::ParameterChoice paramFeatureScorerType;

    public:
	Module_();
	~Module_();

	/**
	 * Creates an empty mixture set with @param nMixtures of @param dimension.
	 * type of covariance tying is given in @param Configuration (paramCovarianceTyingType)
	 */
	Core::Ref<MixtureSet> createMixtureSet(
		const Core::Configuration&,
		size_t nMixtures, size_t dimension, size_t nDensitiesPerMixture = 1) const;
	/**
	 * Creates an empty mixture set with @param nMixtures of @param dimension.
	 */
	Core::Ref<MixtureSet> createMixtureSet(
		CovarianceTyingType covarianceTying,
		size_t nMixtures, size_t dimension, size_t nDensitiesPerMixture = 1) const;
	/**
	 * create an empty mixture set.
	 * uses paramMixtureSetDimension
	 */
	Core::Ref<MixtureSet> createEmptyMixtureSet(const Core::Configuration&) const;

	/**
	 *  Loads a MixtureSet object from the file @param file.
	 */
	Core::Ref<MixtureSet> readMixtureSet(const std::string &file, const Core::Configuration &c) const;

	/**
	 * Loads a MixtureSet object from the file given in @param Configuration (paramMixtureSetFilename).
	 */
	Core::Ref<MixtureSet> readMixtureSet(const Core::Configuration &c) const;
	void convertMixtureSet(const Core::Configuration&) const;
	/**
	 * Load mixture set from the file given in @param Configuration (paramMixtureSetFilename).
	 * Type of mixture set depends on paramFeatureScorerType
	 */
	Core::Ref<AbstractMixtureSet> readAbstractMixtureSet(const Core::Configuration &configuration) const;
	/**
	 * Load mixture set from file @param filename.
	 * Type of mixture set depends on paramFeatureScorerType
	 */
	Core::Ref<AbstractMixtureSet> readAbstractMixtureSet(const std::string &filename, const Core::Configuration &configuration) const;

	bool writeMixtureSet(const std::string &filename, const AbstractMixtureSet &ms, u32 precision = 6) const;

	/** Creates an empty mixture set estimator. */
	AbstractMixtureSetEstimator* createMixtureSetEstimator(const Core::Configuration&) const;
#ifdef MODULE_MM_DT
	/** Creates an empty discriminative mixture set estimator. */
	DiscriminativeMixtureSetEstimator* createDiscriminativeMixtureSetEstimator(const Core::Configuration&) const;
#endif
	/** Creates an empty mixture set estimator with @param nMixtures of @param dimension. */
	AbstractMixtureSetEstimator* createMixtureSetEstimator(
		const Core::Configuration&,
		size_t nMixtures, size_t dimension, size_t nDensitiesPerMixture = 1) const;

	/** Read a MixtureSetEstimator object from file */
	bool readMixtureSetEstimator(const std::string &filename, AbstractMixtureSetEstimator&);
	/** Saves a MixtureSetEstimator object in the file @param filename. */
	bool writeMixtureSetEstimator(const std::string &filename, AbstractMixtureSetEstimator&);

	/** Creates and initializes a FeatureScorer as configured.
	 *  @return a newly created instance of FeatureScorer or 0 if an error occured.
	 */
	Core::Ref<FeatureScorer> createFeatureScorer(
		const Core::Configuration &c, const Core::Ref<const AbstractMixtureSet> m) const;

	/** Creates and initializes an AssigningFeatureScorer as configured.
	 *  @return a newly created instance of AssigningFeatureScorer or 0 if an error occured.
	 */
	Core::Ref<AssigningFeatureScorer> createAssigningFeatureScorer(
		const Core::Configuration&, const Core::Ref<const AbstractMixtureSet>) const;

	/** Loads a mixture set and creates and initializes a FeatureScorer with it, as configured.
	 *  @return a newly created instance of FeatureScorer or 0 if an error occured.
	 */
	Core::Ref<FeatureScorer> createFeatureScorer(const Core::Configuration& c) const;

	/** Creates a ScaledFeatureScorer by wraping @param featureScorer.
	 *  @return a newly created instance of ScaledFeatureScorer or 0 if an error occured.
	 */
	Core::Ref<ScaledFeatureScorer> createScaledFeatureScorer(
		const Core::Configuration &c, Core::Ref<FeatureScorer> featureScorer) const;
	/** Creates and initializes a ScaledFeatureScorer as configured.
	 *  @return a newly created instance of FeatureScorer or 0 if an error occured.
	 */
	Core::Ref<ScaledFeatureScorer> createScaledFeatureScorer(
		const Core::Configuration &c, const Core::Ref<Mm::AbstractMixtureSet> m) const {
		return createScaledFeatureScorer(c, createFeatureScorer(c, m));
	}
	/** Loads a mixture set and creates and initializes a ScaledFeatureScorer with it, as configured.
	 *  @return a newly created instance of ScaledFeatureScorer or 0 if an error occured.
	 */
	Core::Ref<ScaledFeatureScorer> createScaledFeatureScorer(const Core::Configuration& c) const {
		return createScaledFeatureScorer(c, createFeatureScorer(c));
	}

	/** Set of file format class.
	 */
	Core::FormatSet &formats();

	FeatureScorerFactory* featureScorerFactory() const { return featureScorerFactory_; }
    };

    typedef Core::SingletonHolder<Module_> Module;
}

#endif // _MM_MODULE_HH

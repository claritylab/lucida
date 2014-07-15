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
#include <Modules.hh>
#include "Module.hh"
#include <Core/Application.hh>
#include <Core/FormatSet.hh>
#include "FeatureScorerFactory.hh"
#include "GaussDiagonalMaximumFeatureScorer.hh"
#include "SimdFeatureScorer.hh"
#include "MixtureSetBuilder.hh"
#include "MixtureSet.hh"
#include "MixtureSetLoader.hh"
#include "MixtureSetReader.hh"
#ifdef MODULE_MM_DT
#include "EbwDiscriminativeMixtureSetEstimator.hh"
#include "RpropDiscriminativeMixtureSetEstimator.hh"
#endif
#ifdef MODULE_MM_BATCH
#include "BatchFeatureScorer.hh"
#endif

namespace Mm {

const Core::ParameterString Module_::paramMixtureSetFilename(
    "file", "name of mixture set file");

const Core::ParameterString Module_::paramNewMixtureSetFilename(
    "new-file", "name of new mixture set file");

const Core::ParameterInt Module_::paramReducedMixtureSetDimensionOffset(
    "reduced-mixture-set-dimension-offset",
    "select mixture set dimensions starting at the specified offset", 0, 0);

const Core::ParameterInt Module_::paramReducedMixtureSetDimension(
    "reduced-mixture-set-dimension",
    "reduce dimension of densities if > 0", 0, 0);

const Core::ParameterInt Module_::paramMixtureSetDimension(
    "mixture-set-dimension",
    "dimension of the (empty) mixture set", 0, 0);

const Core::Choice Module_::choiceCovarianceTyingType(
    "none", noCovarianceTying,
    "pooled-covariance", pooledCovariance,
    "mixture-specific-covariance", mixtureSpecificCovariance,
    Core::Choice::endMark());
const Core::ParameterChoice Module_::paramCovarianceTyingType(
    "covariance-tying", &choiceCovarianceTyingType, "type of tying scheme", pooledCovariance);

const Core::Choice Module_::choiceEstimatorType(
    "maximum-likelihood", maximumLikelihood,
    "discriminative", discriminative,
    "legacy-discriminative-with-i-smoothing", legacyDiscriminativeWithISmoothing,
    "discriminative-with-i-smoothing", discriminativeWithISmoothing,
    "rprop-discriminative", rPropDiscriminative,
    "rprop-discriminative-with-i-smoothing", rPropDiscriminativeWithISmoothing,
    Core::Choice::endMark());
const Core::ParameterChoice Module_::paramEstimatorType(
    "estimator-type",
    &choiceEstimatorType,
    "type of mixture set estimator",
    maximumLikelihood);

Module_::Module_() :
    formats_(0),
    featureScorerFactory_(new FeatureScorerFactory),
    paramFeatureScorerType(
	    "feature-scorer-type", &featureScorerFactory_->featureScorerNames(),
	    "type of feature scorer", diagonalMaximum)

{
    // create factory for feature scorer
    featureScorerFactory_->registerFeatureScorer<
	GaussDiagonalMaximumFeatureScorer, MixtureSet, AbstractMixtureSetLoader>(
		diagonalMaximum, "diagonal-maximum");
    featureScorerFactory_->registerFeatureScorer<
	SimdGaussDiagonalMaximumFeatureScorer, MixtureSet, AbstractMixtureSetLoader>(
		simdDiagonalMaximum, "SIMD-diagonal-maximum");

#ifdef MODULE_MM_BATCH
    featureScorerFactory_->registerFeatureScorer<
	BatchFloatFeatureScorer, MixtureSet, AbstractMixtureSetLoader>(
		batchFloatDiagonalMaximum, "batch-diagonal-maximum-float");
    featureScorerFactory_->registerFeatureScorer<
	BatchIntFeatureScorer, MixtureSet, AbstractMixtureSetLoader>(
		batchIntDiagonalMaximum, "batch-diagonal-maximum-int");
    featureScorerFactory_->registerFeatureScorer<
	BatchUnrolledIntFeatureScorer, MixtureSet, AbstractMixtureSetLoader>(
		batchIntDiagonalMaximumFast, "batch-diagonal-maximum-fast");
    featureScorerFactory_->registerFeatureScorer<
	BatchPreselectionFloatFeatureScorer, MixtureSet, AbstractMixtureSetLoader>(
		batchPreselectionFloatDiagonalMaximum, "preselection-batch-float");
    featureScorerFactory_->registerFeatureScorer<
	BatchPreselectionIntFeatureScorer, MixtureSet, AbstractMixtureSetLoader>(
		batchPreselectionIntDiagonalMaximum, "preselection-batch-int");
#endif
}

Module_::~Module_()
{
    delete formats_;
    delete featureScorerFactory_;
}


Core::Ref<MixtureSet> Module_::createMixtureSet(
    const Core::Configuration &configuration,
    size_t nMixtures, size_t dimension, size_t nDensitiesPerMixture) const
{
    return createMixtureSet(static_cast<CovarianceTyingType>( paramCovarianceTyingType(configuration) ),
			    nMixtures, dimension, nDensitiesPerMixture);
}

Core::Ref<MixtureSet> Module_::createMixtureSet(
    CovarianceTyingType covarianceTying,
    size_t nMixtures, size_t dimension, size_t nDensitiesPerMixture) const
{
    Core::Ref<MixtureSet> result;
    MixtureSetBuilder *builder = 0;
    switch(covarianceTying) {
    case pooledCovariance:
	builder = new PooledCovarianceBuilder(nMixtures, nDensitiesPerMixture);
	break;
    case mixtureSpecificCovariance:
	builder = new MixtureSpecificCovarianceBuilder(nMixtures, nDensitiesPerMixture);
	break;
    case noCovarianceTying:
	builder = new MixtureSetBuilder(nMixtures, nDensitiesPerMixture);
	break;
    default:
	Core::Application::us()->error("Covariance tying type not given.");
    }
    if (builder != 0) {
	result = Core::ref(new MixtureSet);
	builder->build(*result, dimension);
	delete builder;
    }
    return result;
}

Core::Ref<MixtureSet> Module_::createEmptyMixtureSet(const Core::Configuration &configuration) const
{
    return createMixtureSet(pooledCovariance, 0, paramMixtureSetDimension(configuration), 0);
}

Core::Ref<MixtureSet> Module_::readMixtureSet(
    const std::string &filename,
    const Core::Configuration &configuration) const
{
    Core::Ref<MixtureSet> result;
    if (filename.empty()) {
	return result;
    }
    Core::Application::us()->log("Loading mixture set from file \"%s\"", filename.c_str());
    result = MixtureSetReader(configuration).read<MixtureSet>(filename);
    if (result)
	Core::Application::us()->log("mixtureset read from \"%s\"", filename.c_str());
    else
	Core::Application::us()->error("failed to read mixtureset from \"%s\"", filename.c_str());

    if (result) {
	ComponentIndex offset = paramReducedMixtureSetDimensionOffset(configuration);
	if(offset > 0) {
	    result->setOffset(offset);
	    Core::Application::us()->log("offset feature space dimension of the mixture set to %d", offset);
	}
	ComponentIndex dim = paramReducedMixtureSetDimension(configuration);

	if (dim > 0) {
	    result->setDimension(dim);
	    Core::Application::us()->log("reduced dimension of mixture set to %d", dim);
	}

	Core::XmlChannel logNormStatisticsChannel(configuration, "log-norm-factor-statistics");
	if (logNormStatisticsChannel.isOpen())
	    result->writeLogNormStatistics(logNormStatisticsChannel);
    }
    return result;
}

Core::Ref<MixtureSet> Module_::readMixtureSet(const Core::Configuration &configuration) const
{
    return readMixtureSet(paramMixtureSetFilename(configuration), configuration);
}


Core::Ref<AbstractMixtureSet> Module_::readAbstractMixtureSet(
    const std::string &filename, const Core::Configuration &configuration) const
{
    FeatureScorerType featureScorer = static_cast<FeatureScorerType>( paramFeatureScorerType(configuration) );
    const AbstractMixtureSetLoader *loader = featureScorerFactory_->getMixtureSetLoader(featureScorer);
    verify(loader);
    return loader->load(filename, configuration);
}


Core::Ref<AbstractMixtureSet> Module_::readAbstractMixtureSet(
    const Core::Configuration &configuration) const
{
    return readAbstractMixtureSet(paramMixtureSetFilename(configuration), configuration);
}

void Module_::convertMixtureSet(const Core::Configuration &configuration) const
{
    /*! @todo this function needs some cleanup! */
    Core::Ref<AbstractMixtureSet> ref = readAbstractMixtureSet(configuration);
    MixtureSet *ms_ptr = dynamic_cast<MixtureSet*>(ref.get());
    verify(ms_ptr);
    Core::Ref<MixtureSet> ms_ref(ms_ptr);

    std::string filename = paramNewMixtureSetFilename(configuration);
    Core::Application::us()->log("Saving mixtureset to file \"%s\"", filename.c_str());
    writeMixtureSet(filename, *ms_ref);
}

AbstractMixtureSetEstimator* Module_::createMixtureSetEstimator(
    const Core::Configuration &configuration) const
{
    AbstractMixtureSetEstimator *result = 0;
    switch (paramEstimatorType(configuration)) {
    case maximumLikelihood:
	result = new MixtureSetEstimator(configuration);
	break;
#ifdef MODULE_MM_DT
    case discriminative:
    case discriminativeWithISmoothing:
    case rPropDiscriminative:
    case rPropDiscriminativeWithISmoothing:
	result = createDiscriminativeMixtureSetEstimator(configuration);
	break;
#endif
    default:
	Core::Application::us()->criticalError("Unknown mixture set estimator type.");
    }
    return result;
}

#ifdef MODULE_MM_DT
DiscriminativeMixtureSetEstimator* Module_::createDiscriminativeMixtureSetEstimator(
    const Core::Configuration &configuration) const
{
    DiscriminativeMixtureSetEstimator *result = 0;
    switch (paramEstimatorType(configuration)) {
    case discriminative:
	result = new EbwDiscriminativeMixtureSetEstimator(configuration);
	break;
    case discriminativeWithISmoothing:
	result = new EbwDiscriminativeMixtureSetEstimatorWithISmoothing(configuration);
	break;
    case rPropDiscriminative:
	result = new RpropDiscriminativeMixtureSetEstimator(configuration);
	break;
    case rPropDiscriminativeWithISmoothing:
	result = new RpropDiscriminativeMixtureSetEstimatorWithISmoothing(configuration);
	break;
    default:
	Core::Application::us()->criticalError("Unknown discriminative mixture set estimator type.");
    }
    return result;
}
#endif

AbstractMixtureSetEstimator* Module_::createMixtureSetEstimator(
    const Core::Configuration &configuration,
    size_t nMixtures, size_t dimension, size_t nDensitiesPerMixture) const
{
    AbstractMixtureSetEstimator *result = 0;
    Core::Ref<MixtureSet> mixtureSet =
	createMixtureSet(configuration, nMixtures, dimension, nDensitiesPerMixture);
    if (mixtureSet) {
	result = createMixtureSetEstimator(configuration);
	result->setTopology(mixtureSet);
    }
    return result;
}

bool Module_::readMixtureSetEstimator(const std::string &filename, AbstractMixtureSetEstimator&e)
{
    MixtureSetReader::MixtureSetEstimatorReader reader(Core::Application::us()->getConfiguration());
    return reader.readMixtureSetEstimator(filename, e);
}


bool Module_::writeMixtureSetEstimator(
    const std::string &filename, AbstractMixtureSetEstimator &estimator)
{
    require(!filename.empty());

    Core::Application::us()->log("Saving mixture set estimator to file \"%s\" ...", filename.c_str());

    std::string backupName = filename + ".bak";
    if (Core::isRegularFile(filename) && rename(filename.c_str(), backupName.c_str()) != 0) {
	Core::Application::us()->error("Could not create backup file \"%s\".", backupName.c_str());
	return false;
    }

    Core::BinaryOutputStream bos(filename);
    if (!bos) {
	Core::Application::us()->error("Failed to open \"%s\" for writing", filename.c_str());
	return false;
    }
    estimator.write(bos);
    if (!bos) {
	Core::Application::us()->error("Failed to write mixture estimator");
	return false;
    }

    if (Core::isRegularFile(backupName) && remove(backupName.c_str()) != 0)
	Core::Application::us()->error("Could not remove backup file \"%s\".", backupName.c_str());

    return true;
}

Core::Ref<FeatureScorer> Module_::createFeatureScorer(
    const Core::Configuration &c,
    const Core::Ref<const AbstractMixtureSet> m) const
{
    FeatureScorer *result = featureScorerFactory_->createFeatureScorer(
	static_cast<FeatureScorerType>(paramFeatureScorerType(c)), c, m);
    if (!result) {
	Core::Application::us()->criticalError("Cannot create feature scorer!");
	return Core::Ref<FeatureScorer>();
    }
    return Core::Ref<FeatureScorer>(result);
}

Core::Ref<AssigningFeatureScorer> Module_::createAssigningFeatureScorer(
    const Core::Configuration &c,
    const Core::Ref<const AbstractMixtureSet> m) const
{
    Core::Ref<AssigningFeatureScorer> result(
	dynamic_cast<AssigningFeatureScorer*>(createFeatureScorer(c, m).get()) );

    ensure(result);
    if (result->hasFatalErrors())
	result.reset();
    return result;
}

Core::Ref<FeatureScorer> Module_::createFeatureScorer(const Core::Configuration &c) const
{
    Core::Ref<AbstractMixtureSet> m = readAbstractMixtureSet(c);
    return createFeatureScorer(c, m);
}

Core::Ref<ScaledFeatureScorer> Module_::createScaledFeatureScorer(
    const Core::Configuration &c, Core::Ref<FeatureScorer> featureScorer) const
{
    return featureScorer ?
	Core::Ref<ScaledFeatureScorer>(new FeatureScorerScaling(c, featureScorer)) :
	Core::Ref<ScaledFeatureScorer>();
}

bool Module_::writeMixtureSet(const std::string &filename, const AbstractMixtureSet &ms, u32 precision) const
{
    bool result = Mm::Module::instance().formats().write(filename, ms, precision);
    if (result) {
	Core::Application::us()->log("mixtureset written to \"") << filename << "\"";
    } else {
	Core::Application::us()->error("failed to write mixtureset to \"") << filename << "\"";
    }
    return result;
}

Core::FormatSet &Module_::formats()
{
    Core::Configuration config(Core::Application::us()->getConfiguration(), "file-format-set");
    if (!formats_) {
	formats_ = new Core::FormatSet(config);
	formats_->registerFormat("bin",
				 new Core::BinaryFormat<AbstractMixtureSet>());
	formats_->registerFormat("ascii",
				 new Core::CompressedPlainTextFormat<AbstractMixtureSet>());
	formats_->registerFormat("_default_",
				 new Core::CompressedPlainTextFormat<AbstractMixtureSet>());
    }
    return *formats_;
}

} // namespace Mm

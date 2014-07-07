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
#include "MixtureSetTrainer.hh"
#include <Mm/AbstractMixtureSetEstimator.hh>
#include <Mm/MixtureSetSplitter.hh>
#include <Mm/Module.hh>

using namespace Speech;

/**
 *  Mixture set trainer: base class
 */
const Core::ParameterString MixtureSetTrainer::paramOldMixtureSetFilename(
    "old-mixture-set-file", "name of mixture set file to start up from");

const Core::ParameterString MixtureSetTrainer::paramNewMixtureSetFilename(
    "new-mixture-set-file", "name of file to save the new mixture set to");

const Core::ParameterBool MixtureSetTrainer::paramSplitFirst(
    "split-first", "Performs splitting before accumulation starts.", false);

const Core::ParameterBool MixtureSetTrainer::paramForceCovarianceTying(
    "force-covariance-tying",
    "set covariance tying independent of the mixture in 'old-mixture-set-file'",
    false);

MixtureSetTrainer::MixtureSetTrainer(const Core::Configuration &configuration) :
    Core::Component(configuration),
    estimator_(0)
{}

MixtureSetTrainer::~MixtureSetTrainer()
{
    if (estimator_) estimator_->writeStatistics();
    clear();
}

// const Core::Ref<Mm::MixtureSet> MixtureSetTrainer::getMixtureSet(
//     size_t nMixtures, size_t dimension, Mm::AbstractMixtureSetEstimator &estimator)
// {
//     const std::string filename = paramOldMixtureSetFilename(config);
//     if (read(filename, estimator)) {
// 	Mm::AbstractMixtureSetEstimator *tmp = estimator_;
// 	estimator_ = &estimator;
// 	checkNumberOfMixture(nMixtures);
// 	checkFeatureDimension(dimension);
// 	const Core::Ref<Mm::MixtureSet> mixtureSet = estimate();
// 	estimator_ = tmp;
// 	return mixtureSet;
//     } else {
// 	error("Failed to read mixture-set from \"%s\".", filename.c_str());
// 	return Core::Ref<Mm::MixtureSet>();
//     }
// }

// const Core::Ref<Mm::MixtureSet> MixtureSetTrainer::getMixtureSet(
//     size_t nMixtures, size_t dimension)
// {
//     Mm::MixtureSetEstimator estimator(config);
//     return getMixtureSet(nMixtures, dimension, estimator);
// }

const Core::Ref<Mm::MixtureSet> MixtureSetTrainer::getMixtureSet(
    size_t nMixtures, size_t dimension)
{
    Core::Ref<Mm::MixtureSet> mixtureSet;
    if (paramSplitFirst(config)) {
	const std::string filename = paramOldMixtureSetFilename(config);
	Mm::MixtureSetEstimator estimator(config);
	if (read(filename, estimator)) {
	    Mm::MixtureSetSplitter splitter(select("splitter"));
	    mixtureSet = splitter.split(estimator);
	} else {
	    error("Failed to read mixture-set from \"%s\".", filename.c_str());
	}
    } else {
	mixtureSet = Mm::Module::instance().readMixtureSet(
	    paramOldMixtureSetFilename(config), select("old-mixture-set"));
	if (!mixtureSet) {
	    error("Failed to read mixture-set.");
	}
    }
    if (mixtureSet) {
	if (mixtureSet->nMixtures() != nMixtures) {
	    error("Number of labels of alignment (%zd) do not match the number of mixtures (%d).",
		  nMixtures, mixtureSet->nMixtures());
	}
	if (mixtureSet->dimension() != dimension) {
	    error("Feature vector size (%zd) do not match the dimension in mixtures-set (%d).",
		  dimension, mixtureSet->dimension());
	}
    }
    return mixtureSet;
}

void MixtureSetTrainer::initializeAccumulation(
    size_t nMixtures,
    size_t dimension,
    Core::Ref<const Mm::AssigningFeatureScorer> assigningFeatureScorer,
    Core::Ref<Mm::MixtureSet> mixtureSet)
{
    clear();
    if (firstRun()) {
	estimator_ = Mm::Module::instance().createMixtureSetEstimator(config, nMixtures, dimension);
	if (!estimator_) {
	    respondToDelayedErrors();
	}
    } else {
	if (!mixtureSet) {
	    mixtureSet = getMixtureSet(nMixtures, dimension);
	}
	if (mixtureSet) {
	    setAssigningFeatureScorer(mixtureSet, assigningFeatureScorer);
	}
	if (!mixtureSet || !estimator_ || !assigningFeatureScorer_) {
	    respondToDelayedErrors();
	}
	if (paramForceCovarianceTying(config)) {
	    checkCovarianceTying();
	}
    }
}

void MixtureSetTrainer::read()
{
    clear();
    read(paramOldMixtureSetFilename(config));
    if (!estimator_) {
	respondToDelayedErrors();
    }
}

bool MixtureSetTrainer::combine(
    const std::vector<std::string> &toCombine)
{
    verify(!estimator_);

    Core::BinaryInputStreams bis(toCombine);
    if (!bis.areOpen()) {
	error("Failed to open <mixture-set-files-to-combine> for reading.");
	return false;
    }

    const std::string newMixtureSetFilename = paramNewMixtureSetFilename(config);
    for (u32 n = 0; n < toCombine.size(); ++ n) {
	if (newMixtureSetFilename == toCombine[n]) {
	    error("<new-mixture-set-file> must be different from <mixture-set-files-to-combine>");
	    return false;
	}
	log("Add \"%s\" ...", toCombine[n].c_str());
    }
    Core::BinaryOutputStream bos(newMixtureSetFilename);
    if (!bos) {
	error("Failed to open \"%s\" for writing", newMixtureSetFilename.c_str());
	return false;
    }

    Mm::AbstractMixtureSetEstimator *estimator = createMixtureSetEstimator();
    bool result = estimator->accumulate(bis, bos);
    if (!result) {
	error("Combination failed.");
    }
    delete estimator;
    return result;

//     verify(estimator_);

//     if (!toCombine.empty()) {
// 	Mm::AbstractMixtureSetEstimator *estimator = createMixtureSetEstimator();
// 	if (read(toCombine, *estimator)) {
// 	    if (estimator_->accumulate(*estimator)) {
// 		delete estimator;
// 		return true;
// 	    }
// 	} else {
// 	    error("Failed to read mixture-set from \"%s\".", toCombine.c_str());
// 	}
// 	delete estimator;
// 	error("Combination failed because mixture set '%s' has different topology.", toCombine.c_str());
//     } else {
// 	error("Filename to combine is empty.");
//     }
//     return false;
}

bool MixtureSetTrainer::combinePartitions(const std::vector<std::string> &toCombine)
{
    typedef Mm::AbstractMixtureSetEstimator::MixtureEstimators MixtureEstimators;
    typedef Mm::AbstractMixtureEstimator::DensityEstimators DensityEstimators;

    verify(!estimator_);
    estimator_ = createMixtureSetEstimator();
    std::vector<std::string>::const_iterator file = toCombine.begin();

    for (; file != toCombine.end(); ++file) {
	Mm::AbstractMixtureSetEstimator *estimator = createMixtureSetEstimator();
	if (read(*file, *estimator)) {
	    if (!estimator_->addMixtureEstimators(*estimator)) {
		error("cannot add mixture estimators");
	    }
	} else {
	    error("Failed to read mixture-set from \"%s\"", file->c_str());
	}
	delete estimator;
    }
    if (Mm::Module_::paramCovarianceTyingType(config) == Mm::Module_::pooledCovariance) {
	estimator_->createPooledCovarianceEstimator();
    }
    return true;
}

void MixtureSetTrainer::write(const std::string &filename) const
{
    if (estimator_) {
	if (filename.empty() || !Mm::Module::instance().writeMixtureSetEstimator(filename, *estimator_))
	    criticalError("Failed to save mixture-set to \"%s\".", filename.c_str());
    } else
	error("Cannot save estimator because it has not been initialized.");
}

const Core::Ref<Mm::MixtureSet> MixtureSetTrainer::estimate() const
{
    verify(estimator_);

    if (paramSplitFirst(config)) {
	Mm::MixtureSetSplitter splitter(select("splitter"));
	return splitter.split(*estimator_);
    } else {
	return estimator_->estimate();
    }
}

void MixtureSetTrainer::setAssigningFeatureScorer(
    const Core::Ref<Mm::MixtureSet> mixtureSet,
    Core::Ref<const Mm::AssigningFeatureScorer> assigningFeatureScorer)
{
    require(mixtureSet);
    verify(!estimator_);
    estimator_ = createMixtureSetEstimator();

    if (!assigningFeatureScorer) {
	assigningFeatureScorer_ = Mm::Module::instance().createAssigningFeatureScorer(config, mixtureSet);
    } else {
	assigningFeatureScorer_ = assigningFeatureScorer;
    }
    if (assigningFeatureScorer_) {
	setAssigningFeatureScorer(assigningFeatureScorer_);
	estimator_->setTopology(mixtureSet);
    } else {
	error("Could not create feature-scorer.");
    }
}

void MixtureSetTrainer::setAssigningFeatureScorer(
    Core::Ref<const Mm::AssigningFeatureScorer> assigningFeatureScorer)
{
    assigningFeatureScorer_ = assigningFeatureScorer;
    estimator_->setAssigningFeatureScorer(assigningFeatureScorer_);
}

// void MixtureSetTrainer::checkNumberOfMixture(size_t nMixture)
// {
//     verify(estimator_);
//     if (nMixture != estimator_->nMixtures()) {
// 	error("Number of labels of alignment (%zd) do not match the number of mixtures (%d).",
// 	      nMixture, estimator_->nMixtures());
//     }
// }

// void MixtureSetTrainer::checkFeatureDimension(size_t dimension)
// {
//     verify(estimator_);
//     if (dimension != estimator_->dimension()) {
// 	error("Feature vector size (%zd) do not match the dimension in mixtures-set (%d).",
// 	      dimension, estimator_->dimension());
//     }
// }

void MixtureSetTrainer::checkCovarianceTying()
{
    Mm::MixtureSetEstimatorIndexMap indexMap(*estimator_);
    u32 nCovariances = indexMap.covarianceMap().size();

    switch(Mm::Module_::paramCovarianceTyingType(config))
    {
    case Mm::Module_::mixtureSpecificCovariance:
	if (nCovariances < estimator_->nMixtures()) {
	    log("creating mixture specific covariance estimators");
	    if (!estimator_->createMixtureSpecificCovarianceEstimator()) {
		error("modification of mixture set topology failed");
	    }
	}
	break;
    case Mm::Module_::pooledCovariance:
	if (nCovariances != 1) {
	    log("creating pooled covariance estimator using %d covariance estimators", nCovariances);
	    if (!estimator_->createPooledCovarianceEstimator()) {
		error("modification of mixture set topology failed");
	    }
	}
	break;
    default:
	warning("covariance tying not checked");
	break;
    }
}


void MixtureSetTrainer::clear()
{
    assigningFeatureScorer_.reset();
    delete estimator_; estimator_ = 0;
}

bool MixtureSetTrainer::read(
    const std::string &filename,
    Mm::AbstractMixtureSetEstimator &estimator)
{
    return (!filename.empty() && Mm::Module::instance().readMixtureSetEstimator(filename, estimator));
}

void MixtureSetTrainer::read(const std::string &filename)
{
    verify(!estimator_);
    estimator_ = createMixtureSetEstimator();
    if (!read(filename, *estimator_)) {
	error("Failed to read mixture-set from \"%s\".", filename.c_str());
	delete estimator_;
	estimator_ = 0;
    }
}

bool MixtureSetTrainer::map(const std::string &mappingFilename)
{
    verify(estimator_);
    if (!mappingFilename.empty()) {
	return estimator_->map(mappingFilename);
    } else {
	error("Filename to map is empty.");
    }
    return false;
}

/**
 *  Maximum likelihood mixture set trainer
 */
MlMixtureSetTrainer::MlMixtureSetTrainer(const Core::Configuration &c) :
    Core::Component(c),
    Precursor(c)
{}

MlMixtureSetTrainer::~MlMixtureSetTrainer()
{}

Mm::MixtureSetEstimator* MlMixtureSetTrainer::createMixtureSetEstimator() const
{
    return new Mm::MixtureSetEstimator(config);
}

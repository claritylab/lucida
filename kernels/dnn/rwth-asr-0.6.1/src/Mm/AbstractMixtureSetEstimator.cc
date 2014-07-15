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
#include <Core/StringUtilities.hh>
#include <Core/Directory.hh>
#include "AbstractMixtureSetEstimator.hh"
#include "MixtureSet.hh"

using namespace Mm;

// AbstractMixtureSetEstimator
//////////////////////

const Core::ParameterFloat AbstractMixtureSetEstimator::paramMinObservationWeight(
    "minimum-observation-weight",
    "densities with means of less observation weight get removed",
    5, 0);

const Core::ParameterFloat AbstractMixtureSetEstimator::paramMinRelativeWeight(
    "minimum-relative-weight",
    "mixture densities with less relative weight get removed",
    0, 0);

const Core::ParameterFloat AbstractMixtureSetEstimator::paramMinVariance(
    "minimum-variance",
    "variances lower will be rounded up to minimum-variance",
    0, 0);

const Core::Choice AbstractMixtureSetEstimator::choiceMode(
    "viterbi", modeViterbi,
    "baum-welch", modeBaumWelch,
    Core::Choice::endMark());

const Core::ParameterChoice AbstractMixtureSetEstimator::paramMode(
    "mode", &choiceMode,
    "describes how posterior probabilities of the mixture densities are comuted",
    modeViterbi);

const Core::ParameterBool AbstractMixtureSetEstimator::paramAllowZeroWeights(
    "allow-zero-weights",
    "allow mixtures with zero observations, needed for mixture specific training",
    false);

const Core::ParameterBool AbstractMixtureSetEstimator::paramNormalizeMixtureWeights(
    "normalize-mixture-weights",
    "mixture weights are normalized",
    true);

AbstractMixtureSetEstimator::AbstractMixtureSetEstimator(const Core::Configuration &c) :
    Precursor(c),
    viterbi_(paramMode(c) == modeViterbi),
    weightThreshold_(Core::Type<f32>::epsilon),
    minObservationWeight_(paramMinObservationWeight(c)),
    minRelativeWeight_(paramMinRelativeWeight(c)),
    minVariance_(paramMinVariance(c)),
    allowZeroWeights_(paramAllowZeroWeights(c)),
    normalizeMixtureWeights_(paramNormalizeMixtureWeights(c)),
    scoreStatistics_("score-statistics"),
    weightStats_("accumulation-weights"),
    dnsPosteriorStats_("density-posteriors"),
    finalWeightStats_("final-accumulation-weights"),
    accumulationChannel_(c, "dump-accumulation-statistics"),
    dimension_(0),
    version_(2)
{}

void AbstractMixtureSetEstimator::setTopology(Core::Ref<const MixtureSet> topology)
{
    require(topology);

    dimension_ = topology->dimension();

    // Create Estimator Objects
    mixtureEstimators_.resize(topology->nMixtures());
    for (MixtureIndex m = 0; m < mixtureEstimators_.size(); ++ m)
	mixtureEstimators_[m] = Core::Ref<AbstractMixtureEstimator>(createMixtureEstimator());

    std::vector<Core::Ref<GaussDensityEstimator> > densityEstimators(topology->nDensities());
    for (DensityIndex i = 0; i < densityEstimators.size(); ++ i)
	densityEstimators[i] = Core::ref(createDensityEstimator(*topology->density(i)));

    std::vector<Core::Ref<AbstractMeanEstimator> > meanEstimators(topology->nMeans());
    for (MeanIndex i = 0; i < meanEstimators.size(); ++ i)
	meanEstimators[i] = Core::ref(createMeanEstimator(*topology->mean(i)));

    std::vector<Core::Ref<AbstractCovarianceEstimator> > covarianceEstimators(topology->nCovariances());
    for (CovarianceIndex i = 0; i < covarianceEstimators.size(); ++ i)
	covarianceEstimators[i] = Core::ref(createCovarianceEstimator(*topology->covariance(i)));

    // Create Connections
    for (MixtureIndex m = 0; m < mixtureEstimators_.size(); ++ m) {
	mixtureEstimators_[m]->clear();
	const Mixture* mixture = topology->mixture(m);
	for (DensityIndex dns = 0; dns < mixture->nDensities(); ++ dns) {
	    Core::Ref<GaussDensityEstimator> densityEstimator =
		densityEstimators[mixture->densityIndex(dns)];
	    const GaussDensity* density = topology->density(mixture->densityIndex(dns));

	    densityEstimator->setMean(meanEstimators[density->meanIndex()]);
	    densityEstimator->setCovariance(covarianceEstimators[density->covarianceIndex()]);

	    mixtureEstimators_[m]->addDensity(densityEstimator);
	}
    }
}

void AbstractMixtureSetEstimator::accumulate(
    MixtureIndex mixtureIndex, Core::Ref<const Feature::Vector> featureVector)
{
    if (viterbi_) {
	DensityIndex index = densityIndex(mixtureIndex, featureVector);
	mixtureEstimators_[mixtureIndex]->accumulate(index, *featureVector);
    } else {
	accumulate(mixtureIndex, featureVector, 1.0);
    }
}

void AbstractMixtureSetEstimator::accumulate(
    MixtureIndex mixtureIndex, Core::Ref<const Feature::Vector> featureVector, Weight weight)
{
    if (viterbi_) {
	DensityIndex index = densityIndex(mixtureIndex, featureVector);
	mixtureEstimators_[mixtureIndex]->accumulate(index, *featureVector, weight);
    } else {
	std::vector<Mm::Weight> dnsPosteriors;
	getDensityPosteriorProbabilities(mixtureIndex, featureVector, dnsPosteriors);
	weightStats_ += weight;
	Mm::Weight dnsWeight, finalWeight;
	for (DensityIndex index = 0; index < dnsPosteriors.size(); ++index) {
	    dnsWeight = dnsPosteriors[index];
	    finalWeight = weight * dnsWeight;
	    if (finalWeight > weightThreshold_) {
		mixtureEstimators_[mixtureIndex]->accumulate(index, *featureVector, finalWeight);
		dnsPosteriorStats_ += dnsWeight;
		finalWeightStats_ += finalWeight;
	    }
	}
    }
}

bool AbstractMixtureSetEstimator::accumulate(const AbstractMixtureSetEstimator &toAdd)
{
    if (!equalTopology(toAdd))
	return false;

    MixtureSetEstimatorIndexMap indexMaps(*this);
    MixtureSetEstimatorIndexMap indexMapsToAdd(toAdd);

    verify(mixtureEstimators_.size() == toAdd.mixtureEstimators_.size());
    for (MixtureIndex i = 0; i < mixtureEstimators_.size(); ++ i)
	mixtureEstimators_[i]->accumulate(*toAdd.mixtureEstimators_[i]);

    verify(indexMaps.meanMap().size() == indexMapsToAdd.meanMap().size());
    for (MeanIndex i = 0; i < indexMaps.meanMap().size(); ++ i)
	indexMaps.meanMap()[i]->accumulate(*indexMapsToAdd.meanMap()[i]);

    verify(indexMaps.covarianceMap().size() == indexMapsToAdd.covarianceMap().size());
    for (CovarianceIndex i = 0; i < indexMaps.covarianceMap().size(); ++ i)
	indexMaps.covarianceMap()[i]->accumulate(*indexMapsToAdd.covarianceMap()[i]);

    return true;
}

bool AbstractMixtureSetEstimator::accumulate(
    Core::BinaryInputStreams &is,
    Core::BinaryOutputStream &os)
{
    u32 version = version_;
    for (u32 n = 0; n < is.size(); ++ n) {
	char _magic[8];
	is[n].read(_magic, 8);
	if (strcmp(_magic, magic().c_str()) != 0) {
	    criticalError("Mixture set estimator file with magic \"")
		<< _magic << "\" could not be read, \""
		<< magic() << "\" expected.";
	    return false;
	}
	u32 _version; is[n] >> _version;
	log("version-") << n << ": " << _version;
	if (n == 0) {
	    version = _version;
	}
	if (_version != version) {
	    criticalError("Different versions.");
	    return false;
	}
    }
    os.write(magic().c_str(), 8);
    os << (u32)version_;

    Mm::ComponentIndex dimension;
    is.front() >> dimension;
    for (u32 n = 1; n < is.size(); ++ n) {
	Mm::ComponentIndex _dimension;
	is[n] >> _dimension;
	if (_dimension != dimension) {
	    criticalError("Mismatch in dimension.");
	    return false;
	}
    }
    os << dimension;

    u32 nMeans;
    is.front() >> nMeans;
    for (u32 n = 1; n < is.size(); ++ n) {
	u32 _nMeans;
	is[n] >> _nMeans;
	if (_nMeans != nMeans) {
	    criticalError("Mismatch in number of means.");
	    return false;
	}
    }
    os << (u32)nMeans;
    Core::Ref<AbstractMeanEstimator> meanEstimator(createMeanEstimator());
    Core::Ref<AbstractMeanEstimator> _meanEstimator(createMeanEstimator());
    for (MeanIndex i = 0; i < nMeans; ++ i) {
	meanEstimator->read(is.front(), version);
	for (u32 n = 1; n < is.size(); ++ n) {
	    _meanEstimator->read(is[n], version);
	    meanEstimator->accumulate(*_meanEstimator);
	}
	meanEstimator->write(os);
    }

    u32 nCovariances;
    is.front() >> nCovariances;
    for (u32 n = 1; n < is.size(); ++ n) {
	u32 _nCovariances;
	is[n] >> _nCovariances;
	if (_nCovariances != nCovariances) {
	    criticalError("Mismatch in number of covariances.");
	    return false;
	}
    }
    os << (u32)nCovariances;
    Core::Ref<AbstractCovarianceEstimator> covarianceEstimator(createCovarianceEstimator());
    Core::Ref<AbstractCovarianceEstimator> _covarianceEstimator(createCovarianceEstimator());
    for (CovarianceIndex i = 0; i < nCovariances; ++ i) {
	covarianceEstimator->read(is.front(), version);
	for (u32 n = 1; n < is.size(); ++ n) {
	    _covarianceEstimator->read(is[n], version);
	    covarianceEstimator->accumulate(*_covarianceEstimator);
	}
	covarianceEstimator->write(os);
    }

    u32 nDensities;
    is.front() >> nDensities;
    for (u32 n = 1; n < is.size(); ++ n) {
	u32 _nDensities;
	is[n] >> _nDensities;
	if (_nDensities != nDensities) {
	    criticalError("Mismatch in number of densities.");
	    return false;
	}
    }
    os << (u32)nDensities;
    for (DensityIndex i = 0; i < nDensities; ++ i) {
	if (!accumulateDensity(is, os)) {
	    warning("Density accumulation failed.");
	    return false;
	}
    }

    u32 nMixtures;
    is.front() >> nMixtures;
    for (u32 n = 1; n < is.size(); ++ n) {
	u32 _nMixtures;
	is[n] >> _nMixtures;
	if (_nMixtures != nMixtures) {
	    criticalError("Mismatch in number of mixtures.");
	    return false;
	}
    }
    os << (u32)nMixtures;
    for (MixtureIndex i = 0; i < nMixtures; ++ i) {
	if (!accumulateMixture(is, os)) {
	    warning() << "Accumulation failed for mixture " << i;
	    if(!allowZeroWeights_) // Can happen for mixtures which have no densities
	    return false;
	}
    }
    return true;
}

void AbstractMixtureSetEstimator::reset()
{
    for (u32 mixture = 0; mixture < mixtureEstimators_.size(); ++ mixture)
	mixtureEstimators_[mixture]->reset();

    scoreStatistics_.clear();
}

Core::Ref<MixtureSet> AbstractMixtureSetEstimator::estimate()
{
    Core::Ref<MixtureSet> result(new MixtureSet);
    estimate(*result);
    return result;
}

void AbstractMixtureSetEstimator::estimate(MixtureSet &toEstimate)
{

    if (!allowZeroWeights_) {
	checkEventsWithZeroWeight();
    }

    /* Store mean estimators (with non-zero weight) necessary for covariance estimation since
     * some may get removed in the next step. */
    CovarianceToMeanSetMap meanSetMap(
	MixtureSetEstimatorIndexMap(*this).densityMap().indexToPointerMap());

    removeDensitiesWithLowWeight(minObservationWeight(), minRelativeWeight());

    toEstimate.clear();
    toEstimate.setDimension(dimension_);

    MixtureSetEstimatorIndexMap indexMaps(*this);
    for (MixtureIndex i = 0; i < mixtureEstimators_.size(); ++ i)
	toEstimate.addMixture(i, mixtureEstimators_[i]->estimate(indexMaps.densityMap(), normalizeMixtureWeights_));

    for (DensityIndex i = 0; i < indexMaps.densityMap().size(); ++ i)
	toEstimate.addDensity(i, indexMaps.densityMap()[i]->estimate(indexMaps.meanMap(),
								     indexMaps.covarianceMap()));

    for (MeanIndex i = 0; i < indexMaps.meanMap().size(); ++ i)
	toEstimate.addMean(i, indexMaps.meanMap()[i]->estimate());

    for (CovarianceIndex i = 0; i < indexMaps.covarianceMap().size(); ++ i) {
	toEstimate.addCovariance(i, indexMaps.covarianceMap()[i]->estimate(
				     meanSetMap,
				     minVariance_));
    }

    log("Mixture set estimated:\n") << "#densities " << toEstimate.nDensities();
}

void AbstractMixtureSetEstimator::estimate(MixtureSet &toEstimate,
					   std::vector<Weight> &meanObservationWeights,
					   std::vector<Weight> &covarianceObservationWeights)
{
    estimate(toEstimate);

    MixtureSetEstimatorIndexMap indexMaps(*this);

    meanObservationWeights.resize(indexMaps.meanMap().size());
    for (MeanIndex i = 0; i < meanObservationWeights.size(); ++ i)
	meanObservationWeights[i] = indexMaps.meanMap()[i]->weight();

    covarianceObservationWeights.resize(indexMaps.covarianceMap().size());
    for (CovarianceIndex i = 0; i < covarianceObservationWeights.size(); ++ i)
	covarianceObservationWeights[i] = indexMaps.covarianceMap()[i]->weight();
}

void AbstractMixtureSetEstimator::removeDensitiesWithZeroWeight()
{
    for (MixtureIndex mixture = 0; mixture < mixtureEstimators_.size(); ++ mixture) {
	mixtureEstimators_[mixture]->removeDensitiesWithZeroWeight();
    }
}

void AbstractMixtureSetEstimator::removeDensitiesWithLowWeight(
    Weight minObservationWeight, Weight minRelativeWeight)
{
    for (MixtureIndex mixture = 0; mixture < mixtureEstimators_.size(); ++ mixture) {
	mixtureEstimators_[mixture]->removeDensitiesWithLowWeight(
	    minObservationWeight, minRelativeWeight);
    }
}

DensityIndex AbstractMixtureSetEstimator::densityIndex(
    MixtureIndex mixtureIndex, Core::Ref<const Feature::Vector> featureVector)
{
    verify_(mixtureIndex < mixtureEstimators_.size());

    DensityIndex densityIndex = 0;
    if (assigningFeatureScorer_) {
	Core::Ref<const Feature> feature(new Feature(featureVector));
	AssigningFeatureScorer::AssigningScorer scorer =
	    assigningFeatureScorer_->getAssigningScorer(feature);
	densityIndex = scorer->bestDensity(mixtureIndex);
	scoreStatistics_ += scorer->score(mixtureIndex);
    } else
	verify_(mixtureEstimators_[mixtureIndex]->nDensities() == 1);
    return densityIndex;
}

void AbstractMixtureSetEstimator::getDensityPosteriorProbabilities(
    MixtureIndex mixtureIndex, Core::Ref<const Feature::Vector> featureVector,
    std::vector<Mm::Weight> &posteriors)
{
    verify_(mixtureIndex < mixtureEstimators_.size());

    if (assigningFeatureScorer_) {
	Core::Ref<const Feature> feature(new Feature(featureVector));
	AssigningFeatureScorer::AssigningScorer scorer =
	    assigningFeatureScorer_->getAssigningScorer(feature);
	scorer->getDensityPosteriorProbabilities(mixtureIndex, posteriors);
	scoreStatistics_ += scorer->score(mixtureIndex);
    } else {
	verify_(mixtureEstimators_[mixtureIndex]->nDensities() == 1);
	posteriors.clear();
	posteriors.push_back(1.0);
    }
}

void AbstractMixtureSetEstimator::readHeader(Core::BinaryInputStream &is)
{
    char _magic[8];
    is.read(_magic, 8);
    if (strcmp(_magic, magic().c_str()) != 0) {
	criticalError("Mixture set estimator file with magic \"")
	    << _magic << "\" could not be read, \""
	    << magic() << "\" expected.";
    }
    is >> version_;
    log("version: ") << version_;
}

void AbstractMixtureSetEstimator::writeHeader(Core::BinaryOutputStream &os)
{
    os.write(magic().c_str(), 8);
    os << (u32) version_;
    log() << "version " << version_;
}

void AbstractMixtureSetEstimator::checkEventsWithZeroWeight()
{
    for (MixtureIndex mixture = 0; mixture < mixtureEstimators_.size(); ++ mixture) {
	if (mixtureEstimators_[mixture]->getWeight() == 0)
	    criticalError("Mixture ") << mixture << " has zero weight.";

	std::string message = Core::form("In mixture %d: ", mixture);
	if (!mixtureEstimators_[mixture]->checkEventsWithZeroWeight(message))
	    warning("%s", message.c_str());
    }
}

void AbstractMixtureSetEstimator::read(Core::BinaryInputStream &is)
{
    readHeader(is);

    is >> dimension_;

    u32 nMeans; is >> nMeans;
    std::vector<Core::Ref<AbstractMeanEstimator> > meanEstimators(nMeans);
    for (MeanIndex i = 0; i < meanEstimators.size(); ++ i) {
	meanEstimators[i] = Core::ref(createMeanEstimator());
	meanEstimators[i]->read(is, version_);
    }

    u32 nCovariances; is >> nCovariances;
    std::vector<Core::Ref<AbstractCovarianceEstimator> > covarianceEstimators(nCovariances);
    for (CovarianceIndex i = 0; i < covarianceEstimators.size(); ++ i) {
	covarianceEstimators[i] = Core::ref(createCovarianceEstimator());
	covarianceEstimators[i]->read(is, version_);
    }

    u32 nDensities; is >> nDensities;
    std::vector<Core::Ref<GaussDensityEstimator> > densityEstimators(nDensities);
    for (DensityIndex i = 0; i < densityEstimators.size(); ++ i) {
	densityEstimators[i] = Core::ref(createDensityEstimator());
	densityEstimators[i]->read(is, meanEstimators, covarianceEstimators);
    }

    u32 nMixtures; is >> nMixtures;
    mixtureEstimators_.resize(nMixtures);
    for (MixtureIndex i = 0; i < mixtureEstimators_.size(); ++ i) {
	mixtureEstimators_[i] = Core::ref(createMixtureEstimator());
	mixtureEstimators_[i]->read(is, densityEstimators, version_);
    }
    if (is.fail()) {
	error("error reading mixture set estimator");
    }
    log("Mixture set estimator read:\n")
	<< "    #mixtures " << nMixtures
	<< ", #densities " << nDensities << ",\n"
	<< "    #means " << nMeans
	<< ", #covariances " << nCovariances << ",\n"
	<< "    " << "dimension " << dimension_;
}

void AbstractMixtureSetEstimator::write(Core::BinaryOutputStream &os)
{
    writeHeader(os);

    os << dimension_;
    MixtureSetEstimatorIndexMap indexMaps(*this);

    os << (u32)indexMaps.meanMap().size();
    for(MeanIndex i = 0; i < indexMaps.meanMap().size(); ++ i)
	indexMaps.meanMap()[i]->write(os);

    os << (u32)indexMaps.covarianceMap().size();
    for(CovarianceIndex i = 0; i < indexMaps.covarianceMap().size(); ++ i)
	indexMaps.covarianceMap()[i]->write(os);

    os << (u32)indexMaps.densityMap().size();
    for(DensityIndex i = 0; i < indexMaps.densityMap().size(); ++ i)
	indexMaps.densityMap()[i]->write(os, indexMaps.meanMap(), indexMaps.covarianceMap());

    os << (u32)mixtureEstimators_.size();
    for(MixtureIndex i = 0; i < mixtureEstimators_.size(); ++ i)
	mixtureEstimators_[i]->write(os, indexMaps.densityMap());

    log("Mixture set estimator saved:\n")
	<< "    #mixtures " << mixtureEstimators_.size()
	<< ", #densities " << indexMaps.densityMap().size() << ",\n"
	<< "    #means " << indexMaps.meanMap().size()
	<< ", #covariances " << indexMaps.covarianceMap().size() << ",\n"
	<< "    dimension " << dimension_;
}

void AbstractMixtureSetEstimator::write(Core::XmlWriter &os)
{
    MixtureSetEstimatorIndexMap indexMaps(*this);

    os << Core::XmlOpen("mixture-estimators") +
	Core::XmlAttribute("size", mixtureEstimators_.size());
    for(MixtureIndex i = 0; i < mixtureEstimators_.size(); ++ i)
	mixtureEstimators_[i]->write(os, indexMaps.densityMap());
    os << Core::XmlClose("mixture-estimators");

    os << Core::XmlOpen("density-estimators") +
	Core::XmlAttribute("size", indexMaps.densityMap().size());
    for(DensityIndex i = 0; i < indexMaps.densityMap().size(); ++ i)
	indexMaps.densityMap()[i]->write(os, indexMaps.meanMap(), indexMaps.covarianceMap());
    os << Core::XmlClose("density-estimators");

    os << Core::XmlOpen("mean-estimators") +
	Core::XmlAttribute("size", indexMaps.meanMap().size());
    for(MeanIndex i = 0; i < indexMaps.meanMap().size(); ++ i)
	indexMaps.meanMap()[i]->write(os);
    os << Core::XmlClose("mean-estimators");

    os << Core::XmlOpen("covariance-estimators") +
	Core::XmlAttribute("size", indexMaps.covarianceMap().size());
    for(CovarianceIndex i = 0; i < indexMaps.covarianceMap().size(); ++ i)
	indexMaps.covarianceMap()[i]->write(os);
    os << Core::XmlClose("covariance-estimators");
}

bool AbstractMixtureSetEstimator::equalTopology(const AbstractMixtureSetEstimator &toCompare) const
{
    if (dimension_ != toCompare.dimension_)
	return false;

    MixtureSetEstimatorIndexMap indexMaps(*this);
    MixtureSetEstimatorIndexMap indexMapsToCompare(toCompare);

    if (mixtureEstimators_.size() != toCompare.mixtureEstimators_.size())
	return false;
    for (MixtureIndex i = 0; i < mixtureEstimators_.size(); ++ i) {
	if (!mixtureEstimators_[i]->equalTopology(
		*toCompare.mixtureEstimators_[i],
		indexMaps.densityMap(), indexMapsToCompare.densityMap()))
	    return false;
    }

    if (indexMaps.densityMap().size() != indexMapsToCompare.densityMap().size())
	return false;
    for (DensityIndex i = 0; i < indexMaps.densityMap().size(); ++ i) {
	if (!indexMaps.densityMap()[i]->equalTopology(
		*indexMapsToCompare.densityMap()[i],
		indexMaps.meanMap(), indexMaps.covarianceMap(),
		indexMapsToCompare.meanMap(), indexMapsToCompare.covarianceMap()))
	    return false;
    }

    if (indexMaps.meanMap().size() != indexMapsToCompare.meanMap().size())
	return false;

    if (indexMaps.covarianceMap().size() != indexMapsToCompare.covarianceMap().size())
	return false;

    return true;
}

bool AbstractMixtureSetEstimator::operator==(const AbstractMixtureSetEstimator &toCompare) const
{
    if (!equalTopology(toCompare))
	return false;

    MixtureSetEstimatorIndexMap indexMaps(*this);
    MixtureSetEstimatorIndexMap indexMapsToCompare(toCompare);

    verify(mixtureEstimators_.size() == toCompare.mixtureEstimators_.size());
    for (MixtureIndex i = 0; i < mixtureEstimators_.size(); ++ i) {
	if (!mixtureEstimators_[i]->equalWeights(*toCompare.mixtureEstimators_[i]))
	    return false;
    }

    verify(indexMaps.meanMap().size() == indexMapsToCompare.meanMap().size());
    for (MeanIndex i = 0; i < indexMaps.meanMap().size(); ++ i) {
	if ((*indexMaps.meanMap()[i]) != (*indexMapsToCompare.meanMap()[i]))
	    return false;
    }

    verify(indexMaps.covarianceMap().size() == indexMapsToCompare.covarianceMap().size());
    for (CovarianceIndex i = 0; i < indexMaps.covarianceMap().size(); ++ i) {
	if ((*indexMaps.covarianceMap()[i]) != (*indexMapsToCompare.covarianceMap()[i]))
	    return false;
    }

    return true;
}

void AbstractMixtureSetEstimator::writeStatistics() const
{
    if (accumulationChannel_.isOpen()) {
	scoreStatistics_.write(accumulationChannel_);
	weightStats_.writeXml(accumulationChannel_);
	dnsPosteriorStats_.writeXml(accumulationChannel_);
	finalWeightStats_.writeXml(accumulationChannel_);
    }
}

bool AbstractMixtureSetEstimator::map(const std::string &mappingFilename)
{
    MixtureToMixtureMap mapping;
    if (!mapping.load(mappingFilename)) {
	error("map could not be loaded");
    }
    MixtureEstimators toMap = mixtureEstimators_;
    mixtureEstimators_.resize(mapping.size());
    MixtureIndex nMixtures = 0;
    for (MixtureIndex i = 0; i < mapping.size(); ++ i) {
	require(!mapping[i].empty());
	nMixtures += mapping[i].size();
	MixtureToMixtureMap::MixtureSet::const_iterator m = mapping[i].begin();
	for (; m != mapping[i].end(); ++ m) {
	    if (m == mapping[i].begin()) {
		//		mixtureEstimators_[i] = toMap[*m];
		mixtureEstimators_[i] = Core::Ref<AbstractMixtureEstimator>(new AbstractMixtureEstimator(*toMap[*m]));
	    } else {
		mixtureEstimators_[i]->addMixture(*toMap[*m]);
	    }
	}
    }
    log("to-map-size: ") << toMap.size() << ", n-mixtures: " << nMixtures;
    return true;
}

bool AbstractMixtureSetEstimator::changeDensityMixtureAssignment(std::vector< std::vector<DensityIndex> > stateDensityTable)
{

    MixtureSetEstimatorIndexMap indexMaps(*this);


    bool normalizeWeights_ = true;
    std::vector<std::vector<MixtureIndex> > densityStateTable(indexMaps.densityMap().size());
    if(normalizeWeights_)
	for(MixtureIndex m = 0; m < stateDensityTable.size(); ++m)
	    for(std::vector<DensityIndex>::const_iterator di = stateDensityTable[m].begin(); di != stateDensityTable[m].end(); ++di)
		densityStateTable[(*di)].push_back(m);


    mixtureEstimators_.resize(stateDensityTable.size());
    for (MixtureIndex m = 0; m < mixtureEstimators_.size(); ++ m) {
	mixtureEstimators_[m] = Core::Ref<AbstractMixtureEstimator>(createMixtureEstimator());

	std::vector<Core::Ref<GaussDensityEstimator> > estimators;
	std::vector<Weight> weights;
	for (u32 i = 0; i < stateDensityTable[m].size(); ++ i) {
	    DensityIndex d = stateDensityTable[m][i];
	    verify(d < indexMaps.densityMap().size());
	    estimators.push_back(indexMaps.densityMap()[d]);
	    Weight densityWeight = indexMaps.densityMap()[d]->mean()->weight();
	    if(normalizeWeights_)
		densityWeight = densityWeight/densityStateTable[d].size();
	    weights.push_back(densityWeight);
	}

	mixtureEstimators_[m]->setDensityEstimators(estimators);
	mixtureEstimators_[m]->weights() = weights;
    }
    log("changed density mixture/state assignment");
    return true;
}

bool AbstractMixtureSetEstimator::accumulateDensity(
    Core::BinaryInputStreams &is,
    Core::BinaryOutputStream &os)
{
    return GaussDensityEstimator::accumulate(is, os);
}

bool AbstractMixtureSetEstimator::accumulateMixture(
    Core::BinaryInputStreams &is,
    Core::BinaryOutputStream &os)
{
    return AbstractMixtureEstimator::accumulate(is, os);
}

bool AbstractMixtureSetEstimator::addMixtureEstimators(const AbstractMixtureSetEstimator &estimator)
{
    const MixtureEstimators &mixturesToAdd = estimator.mixtureEstimators_;
    if (mixtureEstimators_.size() == 0) {
	dimension_ = estimator.dimension_;
	mixtureEstimators_.resize(mixturesToAdd.size());
	for (Mm::MixtureIndex m = 0; m < mixtureEstimators_.size(); ++m)
	    mixtureEstimators_[m] = Core::Ref<Mm::AbstractMixtureEstimator>(new Mm::MixtureEstimator);
    }
    for (Mm::MixtureIndex m = 0; m < mixtureEstimators_.size(); ++m) {
	Core::Ref<Mm::AbstractMixtureEstimator> mixtureToAdd = mixturesToAdd[m];
	Core::Ref<Mm::AbstractMixtureEstimator> mixture = mixtureEstimators_[m];

	Mm::Weight weight = mixtureToAdd->getWeight();
	if (weight > 0) {
	    if(mixture->getWeight() > 0) {
		error("existing mixture has weight > 0");
		return false;
	    } else {
		mixture->setDensityEstimators( mixtureToAdd->densityEstimators() );
		mixture->weights().resize( mixtureToAdd->weights().size() );
		std::copy(mixtureToAdd->weights().begin(), mixtureToAdd->weights().end(), mixture->weights().begin());
	    }
	}
    }
    return true;
}

bool AbstractMixtureSetEstimator::createPooledCovarianceEstimator()
{
    MixtureSetEstimatorIndexMap indexMaps(*this);
    /*! @todo diagonal covariance should not be hardcoded! */
    Covariance *cov = new DiagonalCovariance(dimension_);

    Core::Ref<AbstractCovarianceEstimator> covarianceEstimator( createCovarianceEstimator(*cov) );
    for(CovarianceIndex i = 0; i < indexMaps.covarianceMap().size(); ++ i)
	covarianceEstimator->accumulate(*indexMaps.covarianceMap()[i]);
    for(DensityIndex i = 0; i < indexMaps.densityMap().size(); ++ i)
	indexMaps.densityMap()[i]->setCovariance(covarianceEstimator);
    delete cov;
    return true;
}

bool Mm::AbstractMixtureSetEstimator::createMixtureSpecificCovarianceEstimator()
{
    /*! @todo diagonal covariance should not be hardcoded! */
    Covariance *cov = new DiagonalCovariance(dimension_);

    for (MixtureIndex m = 0; m < mixtureEstimators_.size(); ++m) {
	Core::Ref<AbstractCovarianceEstimator> covariance( createCovarianceEstimator(*cov) );
	MixtureEstimator::DensityEstimators::const_iterator density = mixtureEstimators_[m]->densityEstimators().begin();
	for (; density != mixtureEstimators_[m]->densityEstimators().end(); ++density) {
	    (*density)->setCovariance(covariance);
	}
    }
    delete cov;
    return true;
}


// MixtureToMixtureMap
//////////////////////////////

MixtureToMixtureMap::MixtureSet& MixtureToMixtureMap::mixtureSet(size_t m) {
	if (m >= size()) {
	    resize(m + 1);
	}
	return operator[](m);
}

bool MixtureToMixtureMap::load(const std::string &filename) {
	Core::Application::us()->log("mixture-to-mixture mapping: loading mapping from file \"%s\" ...", filename.c_str());
	if (filename.empty()) return false;
	std::ifstream is(filename.c_str());
	if (!is) return false;

	std::string line;
	MixtureToMixtureMap mappedMixtures_;
	while (Core::getline(is, line) != EOF) {
	    std::vector<std::string> tokens = Core::split(line, " ");
	    verify(tokens.size() > 1);
	    MixtureIndex m, front;
	    Core::str2unsigned(tokens.front(), front);
	    MixtureSet &set = mixtureSet(front);
	    for (u32 i = 1; i < tokens.size(); ++ i) {
			Core::str2unsigned(tokens[i], m);
			set.insert(m); //forward mapping
			(mappedMixtures_.mixtureSet(m)).insert(front); //reverse mapping
			//Core::Application::us()->log("mixture-to-mixture mapping:") << front << " to "  << m;
	    }
	}
	nOfMappedMixtures_ = mappedMixtures_.nMixtures();
	Core::Application::us()->log("mixture-to-mixture mapping: mapped %d mixtures to %d mixtures.", nMixtures(), nOfMappedMixtures_);
	return true;
}

bool MixtureToMixtureMap::save(const std::string &filename) const {
	Core::Application::us()->log("mixture-to-mixture mapping: saving mapping to file \"%s\" ...", filename.c_str());
	if (filename.empty()) return false;
	std::ofstream os(filename.c_str());
	if (!os) return false;

	for (u32 m = 0; m < size(); m++){
	    if (!(*this)[m].empty()){
		os << m;
		for (MixtureSet::const_iterator it = (*this)[m].begin() ; it != (*this)[m].end(); ++it)
		    os << " " << *it;
		os << std::endl;
	    }
	}
	os.close();
	return true;
}


// MixtureSetEstimatorIndexMap
//////////////////////////////

MixtureSetEstimatorIndexMap::MixtureSetEstimatorIndexMap(const AbstractMixtureSetEstimator &estimator)
{
    for (u32 m = 0; m < estimator.mixtureEstimators_.size(); ++ m) {
	const MixtureEstimator::DensityEstimators &densityEstimators =
	    estimator.mixtureEstimators_[m]->densityEstimators_;

	for (u32 dns = 0; dns < densityEstimators.size(); ++ dns) {
	    Core::Ref<GaussDensityEstimator> densityEstimator = densityEstimators[dns];

	    meanMap_.add(densityEstimator->meanEstimator_);
	    covarianceMap_.add(densityEstimator->covarianceEstimator_);
	    densityMap_.add(densityEstimator);
	}
    }
}

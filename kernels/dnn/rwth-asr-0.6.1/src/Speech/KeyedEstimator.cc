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
#include "KeyedEstimator.hh"
#include <Core/Directory.hh>
#include <Core/MapParser.hh>
#include <Mm/MixtureSet.hh>
#include <Mm/Module.hh>
#include <Am/ClassicAcousticModel.hh>

using namespace Speech;


const Core::ParameterFloat KeyedEstimator::paramCombinationWeight(
    "combination-weight", "combination weight", 1.0);

const Core::ParameterInt KeyedEstimator::paramFeatureStream(
    "feature-stream", "stream number of stream to accumulate", 0);

const Core::ParameterString KeyedEstimator::paramCorpusKeyMap(
    "corpus-key-map",
    "path of corpus-key-map file");


KeyedEstimator::KeyedEstimator(const Core::Configuration &c, Operation op) :
    Component(c),
    Precursor(c, Am::AcousticModel::noEmissions),
    corpusKey_(new Bliss::CorpusKey(select("corpus-key"))),
    featureDimension_(0),
    modelDimension_(0),
    featureStream_(paramFeatureStream(config)),
    operation_(op),
    currentAccumulator_(0),
    accumulatorCache_(select("accumulator-cache"),
	    op != estimate ? Core::reuseObjectCacheMode : Core::createObjectCacheMode)
{
    if (op == combines) {
	Core::Configuration additionConfiguration(select("addition"), "accumulator-cache");
	accumulatorCacheToAdd_ = new AccumulatorCache(
		additionConfiguration,
		Core::reuseObjectCacheMode);
	log("Opening combination archive...");
    }
    loadMixtureSet();
    loadCorpusKeyMap();
}

KeyedEstimator::~KeyedEstimator()
{ }

void KeyedEstimator::loadMixtureSet()
{
    mixtureSet_ = Core::Ref<Mm::MixtureSet>(dynamic_cast<Mm::MixtureSet*>(acousticModel()->mixtureSet().get()));
    require(mixtureSet_);
    modelDimension_= mixtureSet_->dimension();
}

void KeyedEstimator::createAssigningFeatureScorer()
{
    assigningFeatureScorer_ = Mm::Module::instance().createAssigningFeatureScorer(config, acousticModel()->mixtureSet());
}

void KeyedEstimator::loadCorpusKeyMap()
{
    std::string mapFile = paramCorpusKeyMap(config);
    if (mapFile != "") {
	log("Corpus key map file: ") << mapFile;
	Core::XmlMapDocument<Core::StringHashMap<std::string> > parser(
		config, corpusKeyMap_, "coprus-key-map", "map-item", "key", "value");
	parser.parseFile(mapFile.c_str());
    }
}

void KeyedEstimator::signOn(CorpusVisitor &corpusVisitor)
{
    corpusVisitor.signOn(corpusKey_);
    Precursor::signOn(corpusVisitor);
}

void KeyedEstimator::setFeatureDescription(const Mm::FeatureDescription &description)
{
    description[featureStream_].getValue(Mm::FeatureDescription::nameDimension, featureDimension_);

    std::string key;
    corpusKey_->resolve(key);
    Core::StringHashMap<std::string>::const_iterator i = corpusKeyMap_.find(key);
    if (i != corpusKeyMap_.end())
	key = i->second;

    currentAccumulator_ = accumulatorCache_.findForWriteAccess(key);

    if (currentAccumulator_ == 0) {
	if (featureDimension_ == 0)
	    warning("Input vector size is 0");

	if (featureDimension_ < modelDimension_)
	    warning("Model dimension larger than feature dimension");

	createAccumulator(key);

	if (!accumulatorCache_.insert(key, currentAccumulator_))
	    defect();
    }

    verify(currentAccumulator_);
    createAssigningFeatureScorer();
}

void KeyedEstimator::processAlignedFeature(
	Core::Ref<const Feature> f, Am::AllophoneStateIndex i)
{
    Mm::MixtureIndex mixture= acousticModel()->emissionIndex(i);
    Mm::DensityIndex density= mixtureSet_->mixture(mixture)->densityIndex(
	assigningFeatureScorer_->getAssigningScorer(f)->bestDensity(mixture));
    (*currentAccumulator_)->accumulate((*f)[featureStream_], density, mixture, mixtureSet_);
}

void KeyedEstimator::processAlignedFeature(
	Core::Ref<const Feature> f, Am::AllophoneStateIndex i, Mm::Weight w)
{
    Mm::MixtureIndex mixture= acousticModel()->emissionIndex(i);
    Mm::DensityIndex density= mixtureSet_->mixture(mixture)->densityIndex(
	assigningFeatureScorer_->getAssigningScorer(f)->bestDensity(mixture));
    (*currentAccumulator_)->accumulate(
	    (*f)[featureStream_], density, mixture, mixtureSet_, w);
}

void KeyedEstimator::combine()
{
    verify(operation_ == combines);

    std::set<std::string> keysToAdd= accumulatorCacheToAdd_->keys();
    if (keysToAdd.size() == 0)
	warning("Accumulator to add is empty!");

    for (std::set<std::string>::iterator key= keysToAdd.begin();
	    key!=keysToAdd.end(); ++key) {

	const Accumulator* accumulatorToAdd =
		accumulatorCacheToAdd_->findForReadAccess(*key);
	verify(accumulatorToAdd);

	currentAccumulator_ = accumulatorCache_.findForWriteAccess(*key);
	if (currentAccumulator_ == 0) {
	    featureDimension_= (*accumulatorToAdd)->featureDimension();
	    createAccumulator(*key);
	    if (!accumulatorCache_.insert(*key, currentAccumulator_))
		defect();
	}
	verify(currentAccumulator_);

	log("Combining accumulator '") << *key <<
	    "' using weight '" << paramCombinationWeight(config) << "'";
	(*currentAccumulator_)->combine(**accumulatorToAdd, paramCombinationWeight(config));
    }
}

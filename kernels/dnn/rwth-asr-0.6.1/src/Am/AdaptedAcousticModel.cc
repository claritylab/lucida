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
#include "AdaptedAcousticModel.hh"
#include <Core/Choice.hh>
#include <Core/MapParser.hh>
#include <Mm/Module.hh>

using namespace Am;

const Core::ParameterString AdaptedAcousticModel::paramCorpusKeyMap(
    "corpus-key-map",
    "path of corpus-key-map file");

const Core::ParameterBool AdaptedAcousticModel::paramReuseAdaptors(
    "reuse-adaptors",
    "reuse adaptors instead of estimating them new",
    false);

AdaptedAcousticModel::AdaptedAcousticModel(const Core::Configuration &configuration,
					   Bliss::LexiconRef lexiconRef) :
    Core::Component(configuration),
    Precursor(config, lexiconRef),
    adaptationConfiguration_(config, "adaptation"),
    adaptorCache_(Core::Configuration(adaptationConfiguration_, "adaptor-cache"),
				  paramReuseAdaptors(adaptationConfiguration_) ? Core::reuseObjectCacheMode : Core::createObjectCacheMode),
    adaptorEstimatorCache_(Core::Configuration(adaptationConfiguration_, "accumulator-cache"),
						   Core::reuseObjectCacheMode),
    useCorpusKey_(false),
    corpusKey_(new Bliss::CorpusKey(Core::Configuration(adaptationConfiguration_, "corpus-key")))
{
    loadCorpusKeyMap();

    adaptationTree_= Core::ref(new Am::AdaptationTree(
	    Core::Configuration(adaptationConfiguration_, "mllr-tree"), stateModel(), silence() ));

    Core::IoRef<Mm::AdaptorEstimator>::registerClass<Mm::FullAdaptorViterbiEstimator>(
	    adaptationConfiguration_, adaptationTree_);
    Core::IoRef<Mm::AdaptorEstimator>::registerClass<Mm::ShiftAdaptorViterbiEstimator>(
	    adaptationConfiguration_, adaptationTree_);


    Core::IoRef<Mm::Adaptor>::registerClass<Mm::FullAdaptor>(adaptationConfiguration_);
    Core::IoRef<Mm::Adaptor>::registerClass<Mm::ShiftAdaptor>(adaptationConfiguration_);
}

AdaptedAcousticModel::~AdaptedAcousticModel() {}

Core::Ref<Mm::AbstractMixtureSet> AdaptedAcousticModel::mixtureSet()
{
	checkIfModelNeedsUpdate();
	verify(adaptMixtureSet_);
	return Core::Ref<Mm::AbstractMixtureSet>(adaptMixtureSet_.get());
}

Core::Ref<const Mm::ScaledFeatureScorer> AdaptedAcousticModel::featureScorer()
{
    checkIfModelNeedsUpdate();
    return ClassicAcousticModel::featureScorer();
}

void AdaptedAcousticModel::loadCorpusKeyMap()
{
    std::string mapFile = paramCorpusKeyMap(adaptationConfiguration_);
    if (mapFile != "") {
	log("Corpus key map file: ") << mapFile;
	Core::XmlMapDocument<Core::StringHashMap<std::string> > parser(
		adaptationConfiguration_, corpusKeyMap_, "coprus-key-map", "map-item", "key", "value");
	parser.parseFile(mapFile.c_str());
    }
}

void AdaptedAcousticModel::signOn(Speech::CorpusVisitor &corpusVisitor)
{
    corpusVisitor.signOn(corpusKey_);
    useCorpusKey_ = true;
}

void AdaptedAcousticModel::checkIfModelNeedsUpdate()
{
    if (useCorpusKey_) {
	    std::string key;
	    corpusKey_->resolve(key);
	    setKey(key);
   }
}

bool AdaptedAcousticModel::setKey(const std::string &key)
{
    if (key == currentKey_)
	return true;
    currentKey_ = key;

    std::string mappedKey(key);
    Core::StringHashMap<std::string>::const_iterator i = corpusKeyMap_.find(key);
    if (i != corpusKeyMap_.end())
	mappedKey = i->second;

    if (mappedKey == currentMappedKey_)
	return true;
    currentMappedKey_ = mappedKey;

    Core::Ref<Mm::MixtureSet> mixtureSet = Core::Ref<Mm::MixtureSet>(dynamic_cast<Mm::MixtureSet*>(Precursor::mixtureSet().get()));
    if (!mixtureSet) {
	error("Could not get mixture set from precursor.");
	return false;
    }

    // Do the adaptation
    log("Adaptation key: ") << currentMappedKey_;

    if (!adaptorCache_.findForReadAccess(currentMappedKey_)) {
	if (!adaptorEstimatorCache_.findForReadAccess(currentMappedKey_)) {
	    log("No adaptation data seen for key ") << currentMappedKey_ <<
		"using unadapted references!\n";
	}
	else {
	    adaptorCache_.insert(currentMappedKey_, new Core::IoRef<Mm::Adaptor>(
	    (*adaptorEstimatorCache_.findForReadAccess(currentMappedKey_))->adaptor()));

	    (*adaptorCache_.findForReadAccess(currentMappedKey_))->adaptMixtureSet(mixtureSet);
	}
    }
    else {
	(*adaptorCache_.findForReadAccess(currentMappedKey_))->adaptMixtureSet(mixtureSet);
    }
    mixtureSet_.reset();
    adaptMixtureSet_ = mixtureSet;

    Core::Ref<Mm::ScaledFeatureScorer> featureScorer =
	Mm::Module::instance().createScaledFeatureScorer(
	    select("mixture-set"),
	    Core::Ref<Mm::AbstractMixtureSet>(mixtureSet));
    if (!featureScorer) {
	error("Could not create feature scorer.");
	return false;
    }

    return setFeatureScorer(featureScorer);
}


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
#include "FeatureShiftAdaptor.hh"
#include <Am/ClassicAcousticModel.hh>
#include <Core/MapParser.hh>
#include "ModelCombination.hh"
#include "ModelCombination.hh"

using namespace Speech;

const Core::ParameterString FeatureShiftAdaptor::paramCorpusKeyMap(
    "corpus-key-map",
    "path of corpus-key-map file");

const Core::ParameterString FeatureShiftAdaptor::paramKey(
    "key",
    "key for adaptor selection");

const Core::ParameterBool FeatureShiftAdaptor::paramReuseAdaptors(
    "reuse-adaptors",
    "reuse adaptors instead of estimating them new",
    false);

FeatureShiftAdaptor::FeatureShiftAdaptor(const Core::Configuration &c) :
    Core::Component(c),
    Flow::Node(c),
    featureIndex_(0),
    adaptorCache_(select("adaptor-cache"), paramReuseAdaptors(c)
		? Core::reuseObjectCacheMode : Core::createObjectCacheMode),
    adaptorEstimatorCache_(select("accumulator-cache"), Core::reuseObjectCacheMode)
{
    addInputs(2);
    addOutputs(1);
    loadCorpusKeyMap();

    ModelCombination modelCombination(select("model-combination"),
	    ModelCombination::useAcousticModel);
    modelCombination.load();
    acousticModel_ = modelCombination.acousticModel();
    adaptationTree_= Core::ref(new Am::AdaptationTree(select("mllr-tree"),
	    (dynamic_cast<const Am::ClassicAcousticModel& >(*acousticModel_)).stateModel(),
	    acousticModel_->silence()));

    Core::IoRef<Mm::AdaptorEstimator>::registerClass<Mm::ShiftAdaptorViterbiEstimator>(
	    config, adaptationTree_);

    Core::IoRef<Mm::Adaptor>::registerClass<Mm::ShiftAdaptor>(config);
}

void FeatureShiftAdaptor::updateAlignment(const Flow::Timestamp &timestamp)
{
    while (!alignment_ || !alignment_->contains(timestamp)) {
	featureIndex_ = 0;
	if (!getData(1, alignment_)) {
	    criticalError("In data stream, no object contained the interval [%f..%f].",
			  timestamp.startTime(), timestamp.endTime());
	}
	// log("Alignment range: [") << alignment_->startTime()
	//    << ", " << alignment_->endTime() << "]";
    }
}

bool FeatureShiftAdaptor::configure()
{
    alignment_.reset();

    Core::Ref<Flow::Attributes> attributesSelection(new Flow::Attributes);
    getInputAttributes(1, *attributesSelection);
    if (!configureDatatype(attributesSelection, Flow::DataAdaptor<Alignment>::type()))
	return false;

    return putOutputAttributes(0, getInputAttributes(0));

}

bool FeatureShiftAdaptor::work(Flow::PortId p)
{
    Flow::DataPtr< Flow::Vector<Mm::FeatureType> > in;
    while (getData(0, in)) {
	updateAlignment(*in);
	Alignment &alignment = alignment_->data();
	if (featureIndex_ >= alignment.size())
	    criticalError("Input stream (%d) is longer than alignment (%zd).", featureIndex_ + 1, alignment.size());
	Mm::MixtureIndex mixture = acousticModel_->emissionIndex(
		alignment[featureIndex_++].emission);

	Math::Vector<Mm::FeatureType> inVec(Flow::Vector<Mm::FeatureType>( (*in.get()) ) );
	Flow::Vector<Mm::FeatureType> *out = new Flow::Vector<Mm::FeatureType>( inVec
		- (static_cast<Mm::ShiftAdaptor*>(currentAdaptor_.get())
		    -> shiftVector(mixture)));
	out->setStartTime(in->startTime());
	out->setEndTime(in->endTime());
	putData(0, out);
    }
    return putData(0, in.get());
}

void FeatureShiftAdaptor::loadCorpusKeyMap()
{
    std::string mapFile = paramCorpusKeyMap(config);
    if (mapFile != "") {
	log("Corpus key map file: ") << mapFile;
	Core::XmlMapDocument<Core::StringHashMap<std::string> > parser(
		config, corpusKeyMap_, "coprus-key-map", "map-item", "key", "value");
	parser.parseFile(mapFile.c_str());
    }
}

bool FeatureShiftAdaptor::setKey(const std::string &key)
{
    if (key == currentKey_ || key == "")
	return true;
    currentKey_ = key;

    std::string mappedKey(key);
    Core::StringHashMap<std::string>::const_iterator i = corpusKeyMap_.find(key);
    if (i != corpusKeyMap_.end())
	mappedKey = i->second;

    if (mappedKey == currentMappedKey_)
	return true;
    currentMappedKey_ = mappedKey;

    // Do the adaptation
    log("Adaptation key: ") << currentMappedKey_;

    if (!adaptorCache_.findForReadAccess(currentMappedKey_)) {
	if (!adaptorEstimatorCache_.findForReadAccess(currentMappedKey_)) {

	    criticalError("No adaptation data seen for key ") << currentMappedKey_ <<
		". Exiting!\n";
	}
	else {
	    adaptorCache_.insert(currentMappedKey_, new Core::IoRef<Mm::Adaptor>(
	    (*adaptorEstimatorCache_.findForReadAccess(currentMappedKey_))->adaptor()));

	    currentAdaptor_= (*adaptorCache_.findForReadAccess(currentMappedKey_));
	}
    }
    else {
	currentAdaptor_= (*adaptorCache_.findForReadAccess(currentMappedKey_));
    }
    return true;
}

bool FeatureShiftAdaptor::setParameter(const std::string &name, const std::string &value) {

    if (paramKey.match(name))
	setKey(paramKey(value));
    else
	return false;

    return true;
}

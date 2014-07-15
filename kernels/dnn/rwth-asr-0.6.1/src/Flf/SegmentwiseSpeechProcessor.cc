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
#include <Am/Module.hh>
#include <Lm/Module.hh>
#include <Speech/Module.hh>
#include <SegmentwiseSpeechProcessor.hh>


namespace Flf {

// -------------------------------------------------------------------------
AcousticModelRef getAm(const Core::Configuration &config) {
    return Am::Module::instance().createAcousticModel(config, Bliss::LexiconRef(Lexicon::us()));
}

ScaledLanguageModelRef getLm(const Core::Configuration &config) {
    return Lm::Module::instance().createScaledLanguageModel(config, Bliss::LexiconRef(Lexicon::us()));
}

ModelCombinationRef getModelCombination(const Core::Configuration &config, AcousticModelRef acousticModel, ScaledLanguageModelRef languageModel) {
    return ModelCombinationRef(new Speech::ModelCombination(config, Bliss::LexiconRef(Lexicon::us()), acousticModel, languageModel));
}
// -------------------------------------------------------------------------


// -------------------------------------------------------------------------
SegmentwiseFeatureExtractor::SegmentwiseFeatureExtractor(const Core::Configuration &config, DataSourceRef dataSource) :
    Core::Component(config), dataSource_(dataSource),
    statisticsChannel_(config, "statistics") {
    dataSource_->respondToDelayedErrors();
    nRecordings_ = nSegments_ = 0;
}

SegmentwiseFeatureExtractor::~SegmentwiseFeatureExtractor() {}

void SegmentwiseFeatureExtractor::enterRecording(const Bliss::Recording *recording) {
    if (recording->fullName() == lastRecordingName_)
	return;
    std::string inputFile;
    if (!recording->audio().empty()) {
	inputFile = recording->audio();
	dataSource_->setParameter("input-audio-file", recording->audio());
	dataSource_->setParameter("input-audio-name", recording->name());
    }
    if (!recording->video().empty()) {
	inputFile = recording->video();
	dataSource_->setParameter("input-video-file", recording->video());
    }
    require(!inputFile.empty());
    dataSource_->setParameter("input-file", inputFile);
    dataSource_->setParameter("recording-index", Core::form("%d", u32(nRecordings_)));
    lastRecordingName_ = recording->fullName();
    ++nRecordings_;
}

void SegmentwiseFeatureExtractor::enterSegment(const Bliss::SpeechSegment *segment) {
    enterRecording(segment->recording());
    dataSource_->setParameter("id", segment->fullName());
    dataSource_->setParameter("segment-index", Core::form("%d", u32(nSegments_)));
    dataSource_->setParameter("segment-type", std::string(Bliss::Segment::typeId[segment->type()]));
    dataSource_->setParameter("acoustic-condition", segment->condition() ? segment->condition()->name() : "");
    dataSource_->setParameter("start-time", Core::form("%g", segment->start()));
    dataSource_->setParameter("end-time", Core::form("%g", segment->end()));
    dataSource_->setParameter("track", Core::form("%d", segment->track()));
    // disassemble segment fullname: .../segment-1/segment-0 and corpus-0/corpus-1/...
    dataSource_->setParameter("segment", segment->fullName());
    dataSource_->setParameter("segment-0", segment->name());
    u32 segmentLevel = 1;
    Bliss::CorpusSection *corpusSection = segment->parent();
    while(corpusSection) {
	dataSource_->setParameter("corpus-" + Core::form("%d", corpusSection->level()),
		corpusSection->name());
	dataSource_->setParameter("segment-" + Core::form("%d", segmentLevel),
		corpusSection->name());
	corpusSection = corpusSection->parent();
	segmentLevel ++;
    }
    if (segment->speaker()) {
	dataSource_->setParameter("speaker", segment->speaker()->name());
	dataSource_->setParameter("gender", Bliss::Speaker::genderId[segment->speaker()->gender()]);
    }
    dataSource_->setParameter("orthography", segment->orth());
    lastSegmentName_ = segment->fullName();
    ++nSegments_;
}

void SegmentwiseFeatureExtractor::leaveSegment(const Bliss::SpeechSegment *segment) {
    if (statisticsChannel_.isOpen()) {
	const std::vector<size_t> &nFrames(dataSource_->nFrames());
	for(size_t i = nFrames_.size(); i < nFrames.size(); ++i) {
	    portNames_.push_back(dataSource_->outputName(i));
	    nFrames_.push_back(0);
	}
	statisticsChannel_ << Core::XmlOpen("statistics");
	for(size_t i = 0; i < nFrames_.size(); ++i) {
	    nFrames_[i] += nFrames[i];
	    statisticsChannel_ << Core::XmlEmpty("frames")
	    + Core::XmlAttribute("port", portNames_[i])
	    + Core::XmlAttribute("number", nFrames[i]);
	}
	statisticsChannel_ << Core::XmlClose("statistics");
    }
    dataSource_->setParameter("speaker", "");
    dataSource_->setParameter("gender", "");
    dataSource_->setParameter("orthography", "");
}

void SegmentwiseFeatureExtractor::reset() {
    if (statisticsChannel_.isOpen()) {
	statisticsChannel_ << Core::XmlOpen("statistics");
	statisticsChannel_ << Core::XmlEmpty("recordings") + Core::XmlAttribute("number", nRecordings_);
	statisticsChannel_ << Core::XmlEmpty("segments") + Core::XmlAttribute("number", nSegments_);
	for(size_t i = 0; i < nFrames_.size(); ++i) {
	    statisticsChannel_ << Core::XmlEmpty("frames")
	    + Core::XmlAttribute("port", portNames_[i])
	    + Core::XmlAttribute("number", nFrames_[i]);
	}
	statisticsChannel_ << Core::XmlClose("statistics");
	portNames_.clear();
	nFrames_.clear();
    }
    nRecordings_ = nSegments_ = 0;
    lastRecordingName_.clear();
    lastSegmentName_.clear();
}
// -------------------------------------------------------------------------


// -------------------------------------------------------------------------
SegmentwiseSpeechProcessor::SegmentwiseSpeechProcessor(const Core::Configuration &config, ModelCombinationRef modelCombination) {
    Core::Configuration featureExtractionConfig(config, "feature-extraction");
    DataSourceRef dataSource = DataSourceRef(Speech::Module::instance().createDataSource(featureExtractionConfig));
    featureExtractor_ = SegmentwiseFeatureExtractorRef(new SegmentwiseFeatureExtractor(featureExtractionConfig, dataSource));
    modelAdaptor_ = SegmentwiseModelAdaptorRef(new SegmentwiseModelAdaptor(modelCombination));
}

SegmentwiseSpeechProcessor::SegmentwiseSpeechProcessor(SegmentwiseFeatureExtractorRef featureExtractor, SegmentwiseModelAdaptorRef modelAdaptor) :
    featureExtractor_(featureExtractor), modelAdaptor_(modelAdaptor) {}

SegmentwiseSpeechProcessor::~SegmentwiseSpeechProcessor() {}

void SegmentwiseSpeechProcessor::processSegment(const Bliss::SpeechSegment *segment) {
    modelAdaptor_->enterSegment(segment);
    featureExtractor_->enterSegment(segment);
    DataSourceRef dataSource = featureExtractor_->extractor();
    FeatureList features;
    FeatureRef feature;
    dataSource->initialize(const_cast<Bliss::SpeechSegment*>(segment));
    if (dataSource->getData(feature)) {
	// try to check the dimension only once for each segment
	AcousticModelRef acousticModel = modelAdaptor_->modelCombination()->acousticModel();
	if (acousticModel) {
	    Mm::FeatureDescription *description = feature->getDescription(*featureExtractor_);
	    if (!acousticModel->isCompatible(*description))
		acousticModel->respondToDelayedErrors();
	    delete description;
	}
	features.push_back(feature);
	while (dataSource->getData(feature))
	    features.push_back(feature);
    }
    dataSource->finalize();
    featureExtractor_->leaveSegment(segment);
    process(features);
    modelAdaptor_->leaveSegment(segment);
}

void SegmentwiseSpeechProcessor::reset() {
    featureExtractor_->reset();
    if (modelAdaptor_)
	modelAdaptor_->reset();
}
// -------------------------------------------------------------------------

} // namespace

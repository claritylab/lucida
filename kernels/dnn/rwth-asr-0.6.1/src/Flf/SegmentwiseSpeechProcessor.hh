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
#ifndef _FLF_SEGMENTWISE_SPEECH_PROCESSOR_HH
#define _FLF_SEGMENTWISE_SPEECH_PROCESSOR_HH

#include <Am/AcousticModel.hh>
#include <Bliss/CorpusDescription.hh>
#include <Bliss/Lexicon.hh>
#include <Core/ReferenceCounting.hh>
#include <Speech/DataSource.hh>
#include <Speech/ModelCombination.hh>
#include "Lexicon.hh"


namespace Flf {

    /**
     * Segmentwise model adaptation and feature extraction
     **/


    // -------------------------------------------------------------------------
    /**
     * Basic models
     **/
    typedef Core::Ref<Am::AcousticModel> AcousticModelRef;
    typedef Core::Ref<Lm::LanguageModel> LanguageModelRef;
    typedef Core::Ref<Lm::ScaledLanguageModel> ScaledLanguageModelRef;
    typedef Core::Ref<Speech::ModelCombination> ModelCombinationRef;

    AcousticModelRef getAm(const Core::Configuration &config);
    ScaledLanguageModelRef getLm(const Core::Configuration &config);
    ModelCombinationRef getModelCombination(const Core::Configuration &config, AcousticModelRef acousticModel, ScaledLanguageModelRef languageModel = ScaledLanguageModelRef());

    /**
     * Adpat basic models on segment
     **/
    class SegmentwiseModelAdaptor : public Core::ReferenceCounted {
    private:
	ModelCombinationRef modelCombination_;
	AcousticModelRef acousticModel_;

    public:
	SegmentwiseModelAdaptor(ModelCombinationRef modelCombination) :
	    modelCombination_(modelCombination), acousticModel_(modelCombination->acousticModel()) {}

	ModelCombinationRef modelCombination() { return modelCombination_; }

	void enterSegment(const Bliss::SpeechSegment *segment) {
	    if (acousticModel_)
		acousticModel_->setKey(segment->fullName());
	}
	void leaveSegment(const Bliss::SpeechSegment *segment) {}
	void reset() {}
    };
    typedef Core::Ref<SegmentwiseModelAdaptor> SegmentwiseModelAdaptorRef;
    // -------------------------------------------------------------------------



    // -------------------------------------------------------------------------
    /**
     * Feature extraction
     **/
    typedef Core::Ref<Speech::DataSource> DataSourceRef;
    typedef Core::Ref<Speech::Feature> FeatureRef;
    typedef std::vector<FeatureRef> FeatureList;

    /**
     * Adpat feature extraction on segment
     **/
    class SegmentwiseFeatureExtractor : public virtual Core::Component, public Core::ReferenceCounted {
    private:
	DataSourceRef dataSource_;
	Core::XmlChannel statisticsChannel_;

	std::vector<std::string> portNames_;
	std::vector<size_t> nFrames_;

	u32 nRecordings_;
	u32 nSegments_;
	std::string lastRecordingName_;
	std::string lastSegmentName_;

    protected:
	void enterRecording(const Bliss::Recording *recording);

    public:
	SegmentwiseFeatureExtractor(const Core::Configuration &config, DataSourceRef dataSource);
	virtual ~SegmentwiseFeatureExtractor();

	DataSourceRef extractor() { return dataSource_; }

	void enterSegment(const Bliss::SpeechSegment *segment);
	void leaveSegment(const Bliss::SpeechSegment *segment);
	void reset();
    };
    typedef Core::Ref<SegmentwiseFeatureExtractor> SegmentwiseFeatureExtractorRef;
    // -------------------------------------------------------------------------



    // -------------------------------------------------------------------------
    /**
     * 1) adapt feature extraction and models on segment
     * 2) process all features in segment, verify consistence with acoustic model -> call processFeature for each extracted feature
     **/
    class SegmentwiseSpeechProcessor {
    private:
	SegmentwiseFeatureExtractorRef featureExtractor_;
	SegmentwiseModelAdaptorRef modelAdaptor_;
    protected:
	virtual void process(const FeatureList &) = 0;
    public:
	SegmentwiseSpeechProcessor(const Core::Configuration &config, ModelCombinationRef = ModelCombinationRef());
	SegmentwiseSpeechProcessor(SegmentwiseFeatureExtractorRef featureExtractor, SegmentwiseModelAdaptorRef modelAdaptor);
	virtual ~SegmentwiseSpeechProcessor();

	void processSegment(const Bliss::SpeechSegment *segment);
	void reset();
    };
    // -------------------------------------------------------------------------

} // namespace

#endif // _FLF_SEGMENTWISE_SPEECH_PROCESSOR_HH

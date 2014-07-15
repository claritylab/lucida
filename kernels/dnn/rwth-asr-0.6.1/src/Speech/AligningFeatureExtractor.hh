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
#ifndef _SPEECH_ALIGNING_FEATURE_EXTRACTOR_HH
#define _SPEECH_ALIGNING_FEATURE_EXTRACTOR_HH

#include "DataExtractor.hh"
#include "AlignedFeatureProcessor.hh"
#include "Alignment.hh"
#include <Flow/DataAdaptor.hh>

namespace Speech {

    /** AligningFeatureExtractor
     */
    class AligningFeatureExtractor : public FeatureExtractor
    {
	typedef FeatureExtractor Precursor;
    public:
	static const Core::ParameterString paramAlignmentPortName;
	static const Core::ParameterBool paramEnforceWeightedProcessing;
	static const Core::ParameterString paramAlignment2PortName;
    protected:
	AlignedFeatureProcessor &alignedFeatureProcessor_;
	Flow::PortId alignmentPortId_;
	TimeframeIndex currentFeatureId_;
	Alignment::const_iterator currentAlignmentItem_;
	bool processWeighted_;
	Flow::DataPtr<Flow::DataAdaptor<Alignment> > alignmentRef_;
	/** Fast access to the alignment object. */
	const Alignment *alignment_;

	Flow::PortId alignment2PortId_;
	Alignment::const_iterator currentAlignment2Item_;
	Flow::DataPtr<Flow::DataAdaptor<Alignment> > alignment2Ref_;
	/** Fast access to the alignment-2 object. */
	const Alignment *alignment2_;
    protected:
	virtual void setFeatureDescription(const Mm::FeatureDescription &description);
	virtual void processFeature(Core::Ref<const Feature> f);
	void unaryProcessFeature(Core::Ref<const Feature> f);
	void binaryProcessFeature(Core::Ref<const Feature> f);
	bool initializeAlignment();
    public:
	AligningFeatureExtractor(const Core::Configuration&, AlignedFeatureProcessor&);
	~AligningFeatureExtractor();

	virtual void signOn(CorpusVisitor&);

	virtual void enterCorpus(Bliss::Corpus*);
	virtual void leaveCorpus(Bliss::Corpus*);
	virtual void enterSegment(Bliss::Segment*);
	virtual void leaveSegment(Bliss::Segment*);
	virtual void enterSpeechSegment(Bliss::SpeechSegment*);
	virtual void leaveSpeechSegment(Bliss::SpeechSegment*);
	virtual void processSegment(Bliss::Segment*);
    };

} // namespace Speech

#endif // _SPEECH_ALIGNING_FEATURE_EXTRACTOR_HH

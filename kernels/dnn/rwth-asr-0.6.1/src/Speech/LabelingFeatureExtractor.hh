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
#ifndef _SPEECH_LABELING_FEATURE_EXTRACTOR_HH
#define _SPEECH_LABELING_FEATURE_EXTRACTOR_HH

#include "DataExtractor.hh"
#include "LabeledFeatureProcessor.hh"
#include <Core/Hash.hh>
#include <Flow/Timestamp.hh>

namespace Speech {

    /** Assigns labels to feature vectors
     *  Extracts features and corresponding string labels from the underlying data source network.
     *    -Label is assigned to a feature vector if label.startTime() <= feature.startTime and
     *     label.endTime() >= feature.endTime.
     *    -Feature extraction is interrupted if no label could be found for a feature vector.
     *
     *  The processAlignedFeature function of the LabeledFeatureProcessor objects is called
     *  with each feature and with the corresponding label index. Where the label-index is
     *  the position of the label (starting with zero) in the list of lables.
     */
    class LabelingFeatureExtractor : public FeatureExtractor
    {
	typedef FeatureExtractor Precursor;
    public:
	typedef LabeledFeatureProcessor::LabelIndex LabelIndex;
    public:
	static const Core::ParameterString paramLabels;
	static const Core::ParameterString paramLabelPortName;
    private:
	LabeledFeatureProcessor &labeledFeatureProcessor_;
	Flow::PortId labelsPort_;
	Core::StringHashMap<LabelIndex> labelToIndexMap_;
	Flow::Timestamp currentTimestamp_;
	LabelIndex currentLabelIndex_;
    private:
	void setLabels(const std::string&);
	void setLabelPortName(const std::string&);
	void reset();
    protected:
	virtual void setFeatureDescription(const Mm::FeatureDescription &);
	virtual void processFeature(Core::Ref<const Feature>);
    public:
	LabelingFeatureExtractor(const Core::Configuration&, LabeledFeatureProcessor&);
	~LabelingFeatureExtractor();

	virtual void signOn(CorpusVisitor &corpusVisitor);

	virtual void enterSegment(Bliss::Segment *segment);
	virtual void leaveSegment(Bliss::Segment *segment);
	virtual void enterSpeechSegment(Bliss::SpeechSegment *);
	virtual void leaveSpeechSegment(Bliss::SpeechSegment *);
    };

} // namespace Speech

#endif // _SPEECH_LABELING_FEATURE_EXTRACTOR_HH

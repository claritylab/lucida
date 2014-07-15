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
#include "LabelingFeatureExtractor.hh"
#include <Core/VectorParser.hh>
#include <Flow/DataAdaptor.hh>

using namespace Speech;


const Core::ParameterString LabelingFeatureExtractor::paramLabels(
    "labels", "name of file containing the list of labels");

const Core::ParameterString LabelingFeatureExtractor::paramLabelPortName(
    "label-port-name", "network port name of the labels", "labels");


LabelingFeatureExtractor::LabelingFeatureExtractor(
    const Core::Configuration &c, LabeledFeatureProcessor &labeledFeatureProcessor) :
    Component(c),
    Precursor(c),
    labeledFeatureProcessor_(labeledFeatureProcessor),
    labelsPort_(Flow::IllegalPortId)
{
    setLabels(paramLabels(c));
    setLabelPortName(paramLabelPortName(c));
    reset();
    labeledFeatureProcessor_.setDataSource(dataSource());
}

LabelingFeatureExtractor::~LabelingFeatureExtractor()
{}

void LabelingFeatureExtractor::setLabels(const std::string &fileName)
{
    std::vector<std::string> labels;
    if (!fileName.empty()) {
	Core::XmlVectorDocument<std::string> parser(config, labels);
	parser.parseFile(fileName.c_str());

	for(LabelIndex i = 0; i < labels.size(); ++ i)
	    labelToIndexMap_[labels[i]] = i;

	labeledFeatureProcessor_.setLabels(labels);
    } else
	criticalError("Labels file is not given.");
}

void LabelingFeatureExtractor::setLabelPortName(const std::string &portName)
{
    if (!portName.empty()) {
	labelsPort_ = dataSource()->getOutput(portName);
	if (labelsPort_ == Flow::IllegalPortId)
	    criticalError("Flow network does not have an output named \"%s\"", portName.c_str());
    } else
	criticalError("Label-port-name is not given.");
}

void LabelingFeatureExtractor::reset()
{
    currentTimestamp_.setStartTime(Core::Type<Flow::Time>::min);
    currentTimestamp_.setEndTime(currentTimestamp_.startTime());
    currentLabelIndex_ = Core::Type<LabelIndex>::max;
}

void LabelingFeatureExtractor::processFeature(Core::Ref<const Feature> feature)
{
    while(!currentTimestamp_.contains(feature->timestamp())) {
	Flow::DataPtr<Flow::String> in;
	if (dataSource()->getData(labelsPort_, in)) {
	    Core::StringHashMap<LabelIndex>::iterator index = labelToIndexMap_.find((*in)());
	    if (index != labelToIndexMap_.end()) {
		currentLabelIndex_ = index->second;
		currentTimestamp_ = *in;
	    } else
		criticalError("Unknown label recivied '%s'.", (*in)().c_str());
	} else {
	    criticalError("No label found for interval [%f..%f].",
			  feature->timestamp().startTime(), feature->timestamp().endTime());
	}
    }
    labeledFeatureProcessor_.processLabeledFeature(feature, currentLabelIndex_);
}

void LabelingFeatureExtractor::signOn(CorpusVisitor &corpusVisitor) {
    labeledFeatureProcessor_.signOn(corpusVisitor);
    Precursor::signOn(corpusVisitor);
}

void LabelingFeatureExtractor::enterSegment(Bliss::Segment *segment) {
    Precursor::enterSegment(segment);
    reset();
    labeledFeatureProcessor_.enterSegment(segment);
}

void LabelingFeatureExtractor::leaveSegment(Bliss::Segment *segment) {
    labeledFeatureProcessor_.leaveSegment(segment);
    Precursor::leaveSegment(segment);
}

void LabelingFeatureExtractor::enterSpeechSegment(Bliss::SpeechSegment *segment) {
    Precursor::enterSpeechSegment(segment);
    labeledFeatureProcessor_.enterSpeechSegment(segment);
}

void LabelingFeatureExtractor::leaveSpeechSegment(Bliss::SpeechSegment *segment) {
    labeledFeatureProcessor_.leaveSpeechSegment(segment);
    Precursor::leaveSpeechSegment(segment);
}

void LabelingFeatureExtractor::setFeatureDescription(const Mm::FeatureDescription &description) {
    labeledFeatureProcessor_.setFeatureDescription(description);
}

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
#include "AligningFeatureExtractor.hh"

using namespace Speech;

const Core::ParameterString AligningFeatureExtractor::paramAlignmentPortName(
    "alignment-port-name",
    "name of the main data source port",
    "alignments");

const Core::ParameterBool AligningFeatureExtractor::paramEnforceWeightedProcessing(
    "enforce-weighted-processing",
    "enforce weighted processing even for weights=1 etc.",
    false);

const Core::ParameterString AligningFeatureExtractor::paramAlignment2PortName(
    "alignment-2-port-name",
    "name of the second data source port",
    "alignments-2");

AligningFeatureExtractor::AligningFeatureExtractor(
    const Core::Configuration &c,
    AlignedFeatureProcessor &alignedFeatureProcessor)
    :
    Core::Component(c),
    Precursor(c),
    alignedFeatureProcessor_(alignedFeatureProcessor),
    alignmentPortId_(Flow::IllegalPortId),
    processWeighted_(paramEnforceWeightedProcessing(config)),
    alignment_(0),
    alignment2PortId_(Flow::IllegalPortId),
    alignment2_(0)
{
    alignedFeatureProcessor_.setDataSource(dataSource());

    const std::string alignmentPortName(paramAlignmentPortName(c));
    alignmentPortId_ = dataSource()->getOutput(alignmentPortName);
    if (alignmentPortId_ == Flow::IllegalPortId)
	criticalError("Flow network does not have an output named \"%s\"", alignmentPortName.c_str());

    const std::string alignment2PortName(paramAlignment2PortName(c));
    alignment2PortId_ = dataSource()->getOutput(alignment2PortName);
}

AligningFeatureExtractor::~AligningFeatureExtractor()
{}

void AligningFeatureExtractor::signOn(CorpusVisitor &corpusVisitor)
{
    alignedFeatureProcessor_.signOn(corpusVisitor);
    Precursor::signOn(corpusVisitor);
}

void AligningFeatureExtractor::enterCorpus(Bliss::Corpus *corpus)
{
    Precursor::enterCorpus(corpus);
    alignedFeatureProcessor_.enterCorpus(corpus);
}

void AligningFeatureExtractor::leaveCorpus(Bliss::Corpus *corpus)
{
    alignedFeatureProcessor_.leaveCorpus(corpus);
    Precursor::leaveCorpus(corpus);
}

void AligningFeatureExtractor::enterSegment(Bliss::Segment *segment)
{
    Precursor::enterSegment(segment);
    alignedFeatureProcessor_.enterSegment(segment);
}

void AligningFeatureExtractor::leaveSegment(Bliss::Segment *segment)
{
    alignedFeatureProcessor_.leaveSegment(segment);
    Precursor::leaveSegment(segment);
}

void AligningFeatureExtractor::enterSpeechSegment(Bliss::SpeechSegment *segment)
{
    Precursor::enterSpeechSegment(segment);
    alignedFeatureProcessor_.enterSpeechSegment(segment);
}

void AligningFeatureExtractor::leaveSpeechSegment(Bliss::SpeechSegment *segment)
{
    alignedFeatureProcessor_.leaveSpeechSegment(segment);
    Precursor::leaveSpeechSegment(segment);
}

void AligningFeatureExtractor::processSegment(Bliss::Segment *segment)
{
    verify(alignmentPortId_ != Flow::IllegalPortId);
    if (initializeAlignment()) {
	Precursor::processSegment(segment);
    }else{
	log() << "alignment failed: " << segment->name();
    }
}

void AligningFeatureExtractor::setFeatureDescription(const Mm::FeatureDescription &description)
{
    alignedFeatureProcessor_.setFeatureDescription(description);
}

void AligningFeatureExtractor::processFeature(Core::Ref<const Feature> f)
{
    verify(alignment_ and !alignment_->empty());
    if (!processWeighted_) {
	if (currentAlignmentItem_ != alignment_->end()) {
	    if(currentAlignmentItem_->time > currentFeatureId_)
	    {
		warning("Alignment item further than current timeframe (alignment=%zd, current=%zd), skipping!", (size_t)currentAlignmentItem_->time, (size_t)currentFeatureId_);
		while(currentFeatureId_ < currentAlignmentItem_->time)
		  ++currentFeatureId_;
	    }
	    alignedFeatureProcessor_.processAlignedFeature(f, currentAlignmentItem_->emission);
	    ++currentAlignmentItem_;
	} else {
	    warning("Alignment (size=%zd) shorter than the feature stream.", alignment_->size());
	}
    } else {
	if (!alignment2_) {
	    unaryProcessFeature(f);
	} else {
	    binaryProcessFeature(f);
	}
    }
    ++currentFeatureId_;
}

void AligningFeatureExtractor::unaryProcessFeature(Core::Ref<const Feature> f)
{
    verify(processWeighted_);
    while (currentAlignmentItem_ != alignment_->end() && currentAlignmentItem_->time == currentFeatureId_) {
	verify(currentAlignmentItem_ == alignment_->begin() ? true :
	       (currentAlignmentItem_-1)->time <= currentAlignmentItem_->time);
	alignedFeatureProcessor_.processAlignedFeature(f, currentAlignmentItem_->emission, currentAlignmentItem_->weight);
	++currentAlignmentItem_;
    }
}

void AligningFeatureExtractor::binaryProcessFeature(Core::Ref<const Feature> f)
{
    verify(alignment2_);
    verify(!alignment2_->empty());
    verify(processWeighted_);
    while (currentAlignmentItem_ != alignment_->end() && currentAlignmentItem_->time == currentFeatureId_) {
	verify(currentAlignmentItem_ == alignment_->begin() ? true :
	       (currentAlignmentItem_-1)->time <= currentAlignmentItem_->time);
	verify(currentAlignmentItem_->time == currentAlignment2Item_->time);
	verify(currentAlignmentItem_->emission == currentAlignment2Item_->emission);
	alignedFeatureProcessor_.processAlignedFeature(
	    f,
	    currentAlignmentItem_->emission,
	    currentAlignmentItem_->weight,
	    currentAlignment2Item_->weight);
	++currentAlignmentItem_;
	++currentAlignment2Item_;
    }
}

bool AligningFeatureExtractor::initializeAlignment()
{
    alignment_ = 0;
    alignment2_ = 0;

    if (!dataSource()->getData(alignmentPortId_, alignmentRef_)) {
	error("Failed to extract alignment.");
	return false;
    }
    alignment_ = &alignmentRef_->data();
    currentFeatureId_ = 0;
    currentAlignmentItem_ = alignment_->begin();
    if (alignment_->empty()) {
	warning("Segment has been discarded because of empty alignment.");
	return false;
    }
    processWeighted_ = (processWeighted_ || alignment_->hasWeights() || (currentAlignmentItem_->time != 0));

    if (alignment2PortId_ != Flow::IllegalPortId) {
	if (!dataSource()->getData(alignment2PortId_, alignment2Ref_)) {
	    error("Failed to extract alignment.");
	    return false;
	}
	alignment2_ = &alignment2Ref_->data();
	currentAlignment2Item_ = alignment2_->begin();
	processWeighted_ = true;
	if (alignment2_->empty()) {
	    warning("Segment has been discarded because of empty alignment.");
	    return false;
	}
	if (alignment_->size() != alignment2_->size()) {
	    error("Mismatch in size of alignments.");
	    return false;
	}
    }

    return true;
}

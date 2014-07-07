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
#include "CorpusDescription.hh"
#include "CorpusParser.hh"
#include "SegmentOrdering.hh"

#include <Core/Application.hh>
#include <Core/CompressedStream.hh>
#include <Core/Hash.hh>
#include <Core/Parameter.hh>
#include <Core/ProgressIndicator.hh>
#include <Core/StringUtilities.hh>
#include <Core/TextStream.hh>

using namespace Bliss;

// ========================================================================
// class NamedCorpusEntity

// initialization of static members:
const char *const NamedCorpusEntity::anonymous = "ANONYMOUS";

NamedCorpusEntity::NamedCorpusEntity(ParentEntity *_parent) :
    parent_(_parent),
    name_(anonymous)
{}

std::string NamedCorpusEntity::fullName() const {
    if (parent())
	return parent()->fullName() + "/" + name() ;
    else
	return name() ;
}

// ========================================================================
// class Speaker

const char *Speaker::genderId[] = {
  "unknown",
  "male",
  "female"
};

Speaker::Speaker(ParentEntity *_parent) :
    NamedCorpusEntity(_parent),
    gender_(unknown)
{}

// ========================================================================
// class AcousticCondition

AcousticCondition::AcousticCondition(ParentEntity *_parent) :
    NamedCorpusEntity(_parent)
{}

// ========================================================================
// class CorpusSection

CorpusSection::CorpusSection(CorpusSection *_parent) :
    ParentEntity(_parent),
    level_((_parent) ? (_parent->level() + 1) : 0),
    defaultCondition_(0),
    defaultSpeaker_(0)
{}

const Speaker *CorpusSection::defaultSpeaker() const {
    if (defaultSpeaker_) {
	return defaultSpeaker_ ;
    } else if (parent()) {
	return parent()->defaultSpeaker() ;
    } else {
	return 0 ;
    }
}

const AcousticCondition *CorpusSection::defaultCondition() const {
    if (defaultCondition_) {
	return defaultCondition_ ;
    } else if (parent()) {
	return parent()->defaultCondition() ;
    } else {
	return 0 ;
    }
}

// ========================================================================
// class Corpus
Corpus::Corpus(Corpus *parentCorpus) :
    CorpusSection(parentCorpus)
{}


// ========================================================================
// class Recording

Recording::Recording(Corpus *corpus) :
    CorpusSection(corpus),
    duration_(0)
{
    require(corpus);
}

// ========================================================================
// class Segment

const char *Segment::typeId[] = {
    "speech",
    "other"
};

Segment::Segment(Type _type, Recording *_recording) :
    ParentEntity(_recording),
    recording_(_recording),
    type_(_type),
    start_(0), end_(0), track_(0),
    condition_(0)
{
    require(_recording);
}

void Segment::accept(SegmentVisitor *v) {
    v->visitSegment(this);
}

// ========================================================================
// class SpeechSegment

SpeechSegment::SpeechSegment(Recording *_recording) :
    Segment(typeSpeech, _recording),
    speaker_(0)
{}

void SpeechSegment::accept(SegmentVisitor *v) {
    v->visitSpeechSegment(this);
}

// ========================================================================
// class CorpusDescription

const Core::ParameterString CorpusDescription::paramFilename(
    "file",
    "file name",
    "");
const Core::ParameterString CorpusDescription::paramEncoding(
    "encoding",
    "encoding",
    "utf-8");
const Core::ParameterInt CorpusDescription::paramPartition(
    "partition",
    "divide corpus into partitions with (approximately) equal number of segments",
    0, 0);
const Core::ParameterInt CorpusDescription::paramPartitionSelection(
    "select-partition",
    "select a partition of the corpus",
    0, 0);
const Core::ParameterInt CorpusDescription::paramSkipFirstSegments(
    "skip-first-segments",
    "skip the first N segments (counted after partitioning)",
    0, 0);
const Core::ParameterStringVector CorpusDescription::paramSegmentsToSkip(
    "segments-to-skip",
    "skip the segments in this list");
const Core::ParameterBool CorpusDescription::paramRecordingBasedPartition(
    "recording-based-partition",
    "create corpus partitions based on recordings instead of segments",
    false);
const Core::ParameterBool CorpusDescription::paramProgressReportingSegmentOrth(
    "report-segment-orth",
    "output also segment orth in progress report",
    false);
const Core::ParameterString CorpusDescription::paramSegmentOrder(
    "segment-order",
    "file defining the order of processed segments", "");
const Core::ParameterBool CorpusDescription::paramSegmentOrderLookupName(
    "segment-order-look-up-short-name",
    "Look up using full or short name (segment only)", false);


// ---------------------------------------------------------------------------
class CorpusDescription::SegmentPartitionVisitorAdaptor :
    public CorpusVisitor
{
private:
    u32 segmentIndex_, recordingIndex_, nPartitions_, selectedPartition_, nSkippedSegments_;
    Core::StringHashSet segmentsToSkip_; // blacklist
    Core::StringHashSet segmentsToKeep_; // whitelist
    Recording *currentRecording_;
    bool isVisitorInCurrentRecording_;
    bool recordingBasedPartitions_;
    CorpusVisitor *visitor_;

    bool shouldVisit(Segment *s) {
	bool isSelectedPartition = false;
	if (recordingBasedPartitions_)
	    isSelectedPartition = ((recordingIndex_ % nPartitions_) == selectedPartition_);
	else
	    isSelectedPartition = ((segmentIndex_ % nPartitions_) == selectedPartition_);
	bool hasSkippedEnoughSegments = ((segmentIndex_ / nPartitions_) >= nSkippedSegments_);
	++segmentIndex_;
	if (isSelectedPartition
	    && hasSkippedEnoughSegments
	    && (segmentsToSkip_.empty() || (segmentsToSkip_.find(s->fullName()) == segmentsToSkip_.end()))
	    && (segmentsToKeep_.empty() || (segmentsToKeep_.find(s->fullName()) != segmentsToKeep_.end()) || (segmentsToKeep_.find(s->name()) != segmentsToKeep_.end()))) {
	    if (!isVisitorInCurrentRecording_) {
		visitor_->enterRecording(currentRecording_);
		isVisitorInCurrentRecording_ = true;
	    }
	    return true;
	} else
	    return false;
    }
public:
    SegmentPartitionVisitorAdaptor() :
	visitor_(0)
    {
	nPartitions_       = 1;
	selectedPartition_ = 0;
	recordingBasedPartitions_ = false;
	nSkippedSegments_  = 0;
    }
    void loadSegmentList(const std::string &filename, const std::string &encoding);
    const Core::StringHashSet& segmentsToKeep() const { return segmentsToKeep_; }
    void setPartitioning(u32 nPartitions, u32 selectedPartition, bool recordingBased = false) {
	require(selectedPartition < nPartitions);
	nPartitions_ = nPartitions;
	selectedPartition_ = selectedPartition;
	recordingBasedPartitions_ = recordingBased;
    }
    void setSkippedSegments(u32 nSkippedSegments) {
	nSkippedSegments_ = nSkippedSegments;
    }
    void setSegmentsToSkip(const Core::StringHashSet &segmentsToSkip) {
	segmentsToSkip_ = segmentsToSkip;
    }
    void setVisitor(CorpusVisitor *v) { visitor_ = v; }
    virtual void visitSegment(Segment *s) {
	if (shouldVisit(s)) visitor_->visitSegment(s);
    }
    virtual void visitSpeechSegment(SpeechSegment *s) {
	if (shouldVisit(s)) visitor_->visitSpeechSegment(s);
    }
    virtual void enterRecording(Recording *r) {
	currentRecording_ = r;
	++recordingIndex_;
	isVisitorInCurrentRecording_ = false;
    }
    virtual void leaveRecording(Recording *r) {
	if (isVisitorInCurrentRecording_)
	    visitor_->leaveRecording(r);
	currentRecording_ = 0;
    }
    virtual void enterCorpus(Corpus *c) {
	if (!c->level()) {
	    segmentIndex_ = 0;
	    recordingIndex_ = 0;
	}
	visitor_->enterCorpus(c);
    }
    virtual void leaveCorpus(Corpus *c) {
	visitor_->leaveCorpus(c);
    }
};

void CorpusDescription::SegmentPartitionVisitorAdaptor::loadSegmentList(const std::string &filename, const std::string &encoding)
{
    if (!filename.empty()) {
	Core::CompressedInputStream *cis = new Core::CompressedInputStream(filename.c_str());
	Core::TextInputStream is(cis);
	is.setEncoding(encoding);
	if (!is)
	    Core::Application::us()->criticalError("Failed to open segment list file \"%s\".", filename.c_str());
	std::string s;
	while (Core::getline(is, s) != EOF) {
	    if ((s.size() == 0) || (s.at(0) == '#'))
		continue;
	    Core::stripWhitespace(s);
	    segmentsToKeep_.insert(s);
	}
    }
}

// ---------------------------------------------------------------------------
ProgressReportingVisitorAdaptor::ProgressReportingVisitorAdaptor(Core::XmlChannel &ch, bool reportOrth) :
    visitor_(0), channel_(ch), reportSegmentOrth_(reportOrth) {}

void ProgressReportingVisitorAdaptor::enterCorpus(Corpus *c) {
    channel_ << Core::XmlOpen((c->level()) ? "subcorpus" : "corpus")
	+ Core::XmlAttribute("name", c->name())
	+ Core::XmlAttribute("full-name", c->fullName());
    visitor_->enterCorpus(c);
}

void ProgressReportingVisitorAdaptor::leaveCorpus(Corpus *c) {
    visitor_->leaveCorpus(c);
    channel_ << Core::XmlClose((c->level()) ? "subcorpus" : "corpus");
}

void ProgressReportingVisitorAdaptor::enterRecording(Recording *r) {
    Core::XmlOpen open("recording");
    open + Core::XmlAttribute("name", r->name());
    open + Core::XmlAttribute("full-name", r->fullName());
    if (!r->audio().empty())
	open + Core::XmlAttribute("audio", r->audio());
    if (!r->video().empty())
	open + Core::XmlAttribute("video", r->video());
    channel_ << open;
    visitor_->enterRecording(r);
}

void ProgressReportingVisitorAdaptor::leaveRecording(Bliss::Recording *r) {
    visitor_->leaveRecording(r);
    channel_ << Core::XmlClose("recording");
}

void ProgressReportingVisitorAdaptor::openSegment(Segment *s) {
    channel_ << Core::XmlOpen("segment")
	+ Core::XmlAttribute("name", s->name())
	+ Core::XmlAttribute("full-name", s->fullName())
	+ Core::XmlAttribute("track", s->track())
	+ Core::XmlAttribute("start", s->start())
	+ Core::XmlAttribute("end", s->end());
    if (s->condition()) {
	channel_ << Core::XmlEmpty("condition")
	    + Core::XmlAttribute("name", s->condition()->name());
    }
}

void ProgressReportingVisitorAdaptor::closeSegment(Segment *s) {
    channel_ << Core::XmlClose("segment");
}


void ProgressReportingVisitorAdaptor::visitSegment(Segment *s) {
    openSegment(s);
    visitor_->visitSegment(s);
    closeSegment(s);
}
void ProgressReportingVisitorAdaptor::visitSpeechSegment(SpeechSegment *s) {
    openSegment(s);
    if (s->speaker()) {
	channel_ << Core::XmlEmpty("speaker")
	    + Core::XmlAttribute("name", s->speaker()->name())
	    + Core::XmlAttribute("gender", Bliss::Speaker::genderId[s->speaker()->gender()]);
    }
    if ((s->orth() != "") && (reportSegmentOrth_)) {
	channel_ << Core::XmlOpen("orth")
		 << s->orth()
		 << Core::XmlClose("orth");
    }
    visitor_->visitSpeechSegment(s);
    closeSegment(s);
}


// ---------------------------------------------------------------------------
const Core::Choice CorpusDescription::progressIndicationChoice(
    "none",   noProgress,
    "local",  localProgress,
    "global", globalProgress,
    Core::Choice::endMark());

const Core::ParameterChoice CorpusDescription::paramProgressIndication(
    "progress-indication",
    &progressIndicationChoice,
    "how to display progress in processing the corpus",
    noProgress);

class CorpusDescription::SegmentCountingVisitor :
    public CorpusVisitor
{
private:
    u32 nSegments_;
public:
    void reset() { nSegments_ = 0; }
    u32 nSegments() const { return nSegments_; }
    virtual void visitSegment(Segment*) { ++nSegments_; }
};

class CorpusDescription::ProgressIndicationVisitorAdaptor :
    public CorpusVisitor
{
private:
    u32 nSegments_;
    CorpusVisitor *visitor_;
    Core::ProgressIndicator pi_;
public:
    ProgressIndicationVisitorAdaptor() :
	nSegments_(0), visitor_(0),
	pi_("traversing corpus", "segments") {}
    void setVisitor(CorpusVisitor *v) { visitor_ = v; }
    void setTotal(u32 n) { nSegments_ = n; }
    virtual void visitSegment(Segment *s) {
	visitor_->visitSegment(s);
	pi_.notify();
    }
    virtual void visitSpeechSegment(SpeechSegment *s) {
	visitor_->visitSpeechSegment(s);
	pi_.notify();
    }
    virtual void enterRecording(Recording *r) { visitor_->enterRecording(r); }
    virtual void leaveRecording(Recording *r) { visitor_->leaveRecording(r); }
    virtual void enterCorpus(Corpus *c) {
	pi_.setTask(c->fullName());
	if (!c->level()) pi_.start(nSegments_);
	visitor_->enterCorpus(c);
    }
    virtual void leaveCorpus(Corpus *c) {
	if (!c->level()) pi_.finish();
	visitor_->leaveCorpus(c);
    }
};

// ---------------------------------------------------------------------------
CorpusDescription::CorpusDescription(const Core::Configuration &c) :
    Component(c),
    selector_(0),
    progressChannel_(c, "progress"),
    reporter_(0),
    indicator_(0),
    ordering_(0)
{
    filename_ = paramFilename(config);

    s32 partitioning = paramPartition(config);
    u32 skipFirstSegments = paramSkipFirstSegments(config);
    const std::vector<std::string> segmentFullNames = paramSegmentsToSkip(config);
    std::string segmentsFilename = paramFilename(select("segments"));
    std::string segmentOrder = paramSegmentOrder(config);
    if (partitioning || skipFirstSegments || !segmentFullNames.empty() || !segmentsFilename.empty()) {
	selector_ = new SegmentPartitionVisitorAdaptor;
    }

    if (partitioning) {
	s32 selectedPartition = paramPartitionSelection(config);
	bool recordingBasedPartition = paramRecordingBasedPartition(config);
	if (selectedPartition == partitioning)
	    selectedPartition = 0; // This convention is useful for SGE array jobs
	else if (selectedPartition > partitioning)
	    error("Invalid partition %d (should be 0 - %d).", selectedPartition, partitioning);
	selector_->setPartitioning(partitioning, selectedPartition, recordingBasedPartition);
    }
    if (skipFirstSegments) {
	selector_->setSkippedSegments(skipFirstSegments);
    }
    if (!segmentFullNames.empty()) {
	Core::StringHashSet segmentsToSkip;
	for (u32 i = 0; i < segmentFullNames.size(); ++ i) {
	    segmentsToSkip.insert(segmentFullNames[i]);
	}
	selector_->setSegmentsToSkip(segmentsToSkip);
    }
    if (!segmentsFilename.empty()) {
	selector_->loadSegmentList(segmentsFilename, paramEncoding(select("segments")));
	if (selector_->segmentsToKeep().empty())
	    error("Discard segment whitelist, because file is empty or does not exist: ") << segmentsFilename;
	else
	    log("Use a segment whitelist with %d entries, keep only listed segments.", u32(selector_->segmentsToKeep().size()));
    }
    if (!segmentOrder.empty()) {
	ordering_ = new SegmentOrderingVisitor( paramSegmentOrderLookupName(config) );
	log("Using segment order list '%s'", segmentOrder.c_str());
	ordering_->setSegmentList(segmentOrder);
    }

    progressIndicationMode_ = ProgressIndcationMode(paramProgressIndication(config));
    u32 nSegments = 0;
    switch (progressIndicationMode_) {
    case globalProgress: {
	SegmentCountingVisitor *counter = new SegmentCountingVisitor();
	counter->reset();
	// Note: It is necessary that selector_ and ordering_ are set before
	// and reporter_ after this call to accept()
	SegmentOrderingVisitor *ordering = ordering_ ? new SegmentOrderingVisitor(*ordering_) : 0;
	accept(counter);
	delete ordering_;
	ordering_ = ordering;
	nSegments = counter->nSegments();
	delete counter;
    } // fall through
    case localProgress:
	indicator_ = new ProgressIndicationVisitorAdaptor();
	indicator_->setTotal(nSegments);
	break;
    case noProgress: break;
    default: defect();
    }

    if (progressChannel_.isOpen()) {
	reporter_ = new ProgressReportingVisitorAdaptor(progressChannel_, paramProgressReportingSegmentOrth(config));
    }
}

CorpusDescription::~CorpusDescription() {
    delete ordering_;
    delete selector_;
    delete reporter_;
    delete indicator_;
}

void CorpusDescription::accept(CorpusVisitor *visitor) {
    CorpusDescriptionParser parser(config);
    if (indicator_) {
	indicator_->setVisitor(visitor);
	visitor = indicator_;
    }
    if (reporter_) {
	reporter_->setVisitor(visitor);
	visitor = reporter_;
    }
    if (selector_) {
	selector_->setVisitor(visitor);
	visitor = selector_;
    }
    if (ordering_) {
	ordering_->setVisitor(visitor);
	visitor = ordering_;
    }
    parser.accept(file(), visitor);
}

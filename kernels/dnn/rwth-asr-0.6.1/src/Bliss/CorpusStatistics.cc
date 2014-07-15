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
#include "CorpusStatistics.hh"
#include "Lexicon.hh"
#include "Orthography.hh"
#include <Core/Extensions.hh>
#include <Core/Utility.hh>

using namespace Bliss;

// ===========================================================================
void CorpusSizeStatisticsVisitor::reset() {
    nRecordings_                = 0;
    nSpeechSegments_            = 0;
    nOtherSegments_             = 0;
    totalGrossDuration_         = 0.0;
    totalNetDuration_           = 0.0;
    nNetTimeFrames_             = 0;
}

void CorpusSizeStatisticsVisitor::enterRecording(Recording *recording) {
    nRecordings_ += 1;
    totalGrossDuration_ += recording->duration();
    // check presence of audio file
}

void CorpusSizeStatisticsVisitor::visitSegment(Segment *segment) {
    nOtherSegments_ += 1;
    totalNetDuration_ += segment->end() - segment->start();
    nNetTimeFrames_ += 0;  // number of time frames in segment
}

void CorpusSizeStatisticsVisitor::visitSpeechSegment(SpeechSegment *segment) {
    nSpeechSegments_ += 1;
    totalNetDuration_ += segment->end() - segment->start();
    nNetTimeFrames_ += 0;  // number of time frames in segment
}

void CorpusSizeStatisticsVisitor::writeReport(Core::XmlWriter &os) const {
    os << Core::XmlOpen("size-statistics")
       << "number of recordings                     \t: " << nRecordings_ << "\n"
       << "number of speech segments                \t: " << nSpeechSegments_ << "\n"
       << "number of other segments                 \t: " << nOtherSegments_ << "\n"
       << "gross duration (all recordings)          \t: " << totalGrossDuration_ << "s" << "\n"
       << "net duration (all segments)              \t: " << totalNetDuration_ << "s" << "\n"
       << "net number of time frames (all segments) \t: " << nNetTimeFrames_ << "\n"
       << Core::XmlClose("size-statistics");
}

// ===========================================================================
// class CorpusSpeakerStatisticsVisitor

void CorpusSpeakerStatisticsVisitor::reset() {
    speakerStatistics_.clear();
    noSpeaker_ = SpeakerStatistics();
    for (int g = 0; g < Speaker::nGenders; ++g) {
	genderSpeakers_[g].clear();
	genderStatistics_[g] = SpeakerStatistics();
    }
}

void CorpusSpeakerStatisticsVisitor::accu(
    SpeakerStatistics &stat,
    const SpeechSegment *segment)
{
    stat.nSegments += 1;
    stat.totalDuration += segment->end() - segment->start();
}

void CorpusSpeakerStatisticsVisitor::visitSpeechSegment(SpeechSegment *segment) {
    const Speaker *s = segment->speaker();
    if (s) {
	accu(speakerStatistics_[s->fullName()], segment);
	accu(genderStatistics_[s->gender()], segment);
	genderSpeakers_[s->gender()].insert(s->fullName());
    } else {
	accu(noSpeaker_, segment);
    }
}

void CorpusSpeakerStatisticsVisitor::writeSpeakerStatistics(
    const SpeakerStatistics &ss, Core::XmlWriter &os) const
{
    os << ss.nSegments << " segments"
       << " (" << ss.totalDuration << "s)"
       << "\n";
}

void CorpusSpeakerStatisticsVisitor::writeReport(Core::XmlWriter &os) const {
    os << Core::XmlOpen("speaker-statistics");
    os << "number of speakers used                 \t: " << u32(speakerStatistics_.size()) << "\n";
    for (int g = 0; g < Speaker::nGenders; ++g) {
	os << u32(genderSpeakers_[g].size()) << " "
	   << Speaker::genderId[g] << " speakers account for ";
	writeSpeakerStatistics(genderStatistics_[g], os);
    }
    for (SpeakerStatisticsMap::const_iterator i = speakerStatistics_.begin(); i != speakerStatistics_.end(); ++i) {
	os << "speaker " << i->first << " accounts for ";
	writeSpeakerStatistics(i->second, os);
    }
    os << "no speaker specified for ";
    writeSpeakerStatistics(noSpeaker_, os);
    os << Core::XmlClose("speaker-statistics");
}

// ===========================================================================
// class CorpusConditionStatisticsVisitor

void CorpusConditionStatisticsVisitor::reset() {
    conditionStatistics_.clear();
    noCondition_ = ConditionStatistics();
}

void CorpusConditionStatisticsVisitor::accu(
    ConditionStatistics &stat,
    const Segment *segment)
{
    stat.nSegments += 1;
    stat.totalDuration += segment->end() - segment->start();
}

void CorpusConditionStatisticsVisitor::visitSegment(Segment *segment) {
    const AcousticCondition *c = segment->condition();
    if (c) {
	accu(conditionStatistics_[c->fullName()], segment);
    } else {
	accu(noCondition_, segment);
    }
}

void CorpusConditionStatisticsVisitor::writeConditionStatistics(
    const ConditionStatistics &cs, Core::XmlWriter &os) const
{
    os << cs.nSegments << " segments"
       << " (" << cs.totalDuration << "s)"
       << "\n";
}

void CorpusConditionStatisticsVisitor::writeReport(Core::XmlWriter &os) const {
    os << Core::XmlOpen("condition-statistics");
    os << "number of conditions used                 \t: " << u32(conditionStatistics_.size()) << "\n";
    for (ConditionStatisticsMap::const_iterator i = conditionStatistics_.begin(); i != conditionStatistics_.end(); ++i) {
	os << "condition " << i->first << " accounts for ";
	writeConditionStatistics(i->second, os);
    }
    os << "no condition specified for ";
    writeConditionStatistics(noCondition_, os);
    os << Core::XmlClose("condition-statistics");
}

// ===========================================================================
class CorpusLexicalStatisticsVisitor::Internal :
    public OrthographicParser::Handler
{
public:
    typedef Core::hash_map<const Lemma*, u32, Core::conversion<const Lemma*, size_t> > LemmaMap;
    LemmaMap lemmaCounts_;
    typedef Core::StringHashMap<u32> UnmatchedOrthMap;
    UnmatchedOrthMap unmatchedOrthCounts_;
    u32 nOrthMatchPositions_;
    u32 nUnmatchedOrth_;
    u32 nLemmas_,
	nNonLoopLemmas_,
	nPronunciations_,
	nNonLoopPronunciations_,
	nEvaluationSequences_,
	nNonLoopEvaluationSequences_;
    f64 nEvaluationTokens_,
	nNonLoopEvaluationTokens_,
	nPhonemes_;
    Core::Channel drawChannel_;

    Internal(const Core::Configuration&);
    void reset();
    void writeReport(Core::XmlWriter&) const;
    bool isNonWordLemma(const Lemma*) const;

    virtual void initialize(const OrthographicParser*);
    virtual OrthographicParser::Node newNode();
    virtual void newEdge(OrthographicParser::Node from, OrthographicParser::Node to, const Lemma*);
    virtual void newUnmatchableEdge(OrthographicParser::Node from, OrthographicParser::Node to, const std::string&);
    virtual void finalize(OrthographicParser::Node intial, OrthographicParser::Node final);
};

CorpusLexicalStatisticsVisitor::CorpusLexicalStatisticsVisitor(
    const Core::Configuration &c,
    LexiconRef _lexicon) :
    CorpusStatisticsVisitor(c),
    lexicon_(_lexicon),
    orthographicParser_(0),
    internal_(0)
{
    orthographicParser_ = new OrthographicParser(config, lexicon_);
    internal_ = new Internal(config);
}

CorpusLexicalStatisticsVisitor::~CorpusLexicalStatisticsVisitor() {
    delete internal_;
    delete orthographicParser_;
}

CorpusLexicalStatisticsVisitor::Internal::Internal(const Core::Configuration &c) :
    drawChannel_(c, "dot")
{}

void CorpusLexicalStatisticsVisitor::Internal::reset() {
    nOrthMatchPositions_         = 0;
    nUnmatchedOrth_              = 0;
    nLemmas_                     = 0;
    nNonLoopLemmas_              = 0;
    nPronunciations_             = 0;
    nNonLoopPronunciations_      = 0;
    nPhonemes_                   = 0;
    nEvaluationSequences_        = 0;
    nEvaluationTokens_           = 0;
    nNonLoopEvaluationSequences_ = 0;
    nNonLoopEvaluationTokens_    = 0;
    lemmaCounts_.clear();
    unmatchedOrthCounts_.clear();
}

bool CorpusLexicalStatisticsVisitor::Internal::isNonWordLemma(
    const Lemma *lemma) const
{
    if (lemma->nEvaluationTokenSequences() == 1) {
	if ((*lemma->evaluationTokenSequences().first).isEpsilon()) {
	    return true;
	}
    }
    return false;
}

void CorpusLexicalStatisticsVisitor::reset() {
    internal_->reset();
}

void CorpusLexicalStatisticsVisitor::Internal::initialize(const OrthographicParser *parser) {
    OrthographicParser::Handler::initialize(parser);

    drawChannel_
	<< "ranksep = 1.0;" << std::endl
	<< "rankdir = LR;" << std::endl
	<< "center = 1;" << std::endl
	<< "orientation = Landscape" << std::endl
	<< "node [fontname=\"Helvetica\"]" << std::endl
	<< "edge [fontname=\"Helvetica\"]" << std::endl;
}

OrthographicParser::Node CorpusLexicalStatisticsVisitor::Internal::newNode() {
    nOrthMatchPositions_ += 1;

    return (void*) size_t(nOrthMatchPositions_);
}

void CorpusLexicalStatisticsVisitor::Internal::newEdge(
    OrthographicParser::Node from, OrthographicParser:: Node to,
    const Lemma *lemma)
{
    if (lemma) {
	lemmaCounts_[lemma] += 1;
	nLemmas_ += 1;
	nPronunciations_ += lemma->nPronunciations();
	nEvaluationSequences_ += lemma->nEvaluationTokenSequences();
	u32 nEvaluationTokensInLemma_ = 0;
	Lemma::EvaluationTokenSequenceIterator e, e_end;
	for (Core::tie(e, e_end) = lemma->evaluationTokenSequences(); e != e_end; ++e)
	    nEvaluationTokensInLemma_ += e->length();
	nEvaluationTokens_ += f64(nEvaluationTokensInLemma_) / f64(lemma->nEvaluationTokenSequences());
	if (from != to) {
	    nNonLoopLemmas_ += 1;
	    nNonLoopPronunciations_ += lemma->nPronunciations();
	    nNonLoopEvaluationSequences_ += lemma->nEvaluationTokenSequences();
	    nNonLoopEvaluationTokens_ += f64(nEvaluationTokensInLemma_) / f64(lemma->nEvaluationTokenSequences());
	}

	if (lemma->nPronunciations()) {
	    Lemma::PronunciationIterator pron, pron_end;
	    u32 nPhonemesTotal = 0;
	    for (Core::tie(pron, pron_end) = lemma->pronunciations(); pron != pron_end;	++pron)
		nPhonemesTotal += pron->pronunciation()->length();
	    nPhonemes_ += f64(nPhonemesTotal) / f64(lemma->nPronunciations());
	}
    }

    if (drawChannel_.isOpen()) {
	drawChannel_ << "n" << from << " -> " << "n" << to;
	if (lemma)
	    drawChannel_ << " [label=\"" << lemma->name() << "\"]";
	drawChannel_ << std::endl;
    }
}

void CorpusLexicalStatisticsVisitor::Internal::newUnmatchableEdge(
    OrthographicParser::Node from, OrthographicParser::Node to,
    const std::string &orth)
{
    unmatchedOrthCounts_[orth] += 1;
    nUnmatchedOrth_ += 1;
    newEdge(from, to, 0);
}

void CorpusLexicalStatisticsVisitor::Internal::finalize(
    OrthographicParser::Node intial, OrthographicParser::Node final)
{
    nOrthMatchPositions_ -= 1; // don't count final node

}

void CorpusLexicalStatisticsVisitor::visitSpeechSegment(SpeechSegment *segment) {
    internal_->drawChannel_
	<< "digraph \"" << segment->fullName() << "\" {" << std::endl;

    orthographicParser_->parse(segment->orth(), *internal_);

    internal_->drawChannel_
	<< "}" << std::endl;
}

void CorpusLexicalStatisticsVisitor::Internal::writeReport(Core::XmlWriter &os) const {
    os << "number of orthographic match positions         \t: " << nOrthMatchPositions_         << "\n"
       << "number of orthographic match failures          \t: " << nUnmatchedOrth_              << "\n"
       << "number of matched lemmas                       \t: " << nLemmas_                     << "\n"
       << "number of non-loop matched lemmas              \t: " << nNonLoopLemmas_              << "\n"
       << "number of matched pronunciations               \t: " << nPronunciations_             << "\n"
       << "number of non-loop matched pronunciations      \t: " << nNonLoopPronunciations_      << "\n"
       << "number of matched evaluation token sequences   \t: " << nEvaluationSequences_        << "\n"
       << "number of non-loop matched eval token sequences\t: " << nNonLoopEvaluationSequences_ << "\n"
       << "avg. number of matched evaluation tokens       \t: " << nEvaluationTokens_           << "\n"
       << "avg. number of non-loop matched eval tokens    \t: " << nNonLoopEvaluationTokens_    << "\n"
       << "avg. number of phonemes (average of variants)  \t: " << nPhonemes_                   << "\n";
}

void CorpusLexicalStatisticsVisitor::writeReport(Core::XmlWriter &os) const {
    os << Core::XmlOpen("lexical-statistics");

    internal_->writeReport(os);

    os << Core::XmlComment("occurances of known words");
    f64 nWordMatchPositions = 0;
    Lexicon::LemmaIterator l, l_end;
    for (Core::tie(l, l_end) = lexicon_->lemmas(); l != l_end; ++l) {
	const Lemma *lemma(*l);
	if (internal_->lemmaCounts_.find(lemma) == internal_->lemmaCounts_.end())
	    continue;
	const u32 count = const_cast<Internal::LemmaMap&>(internal_->lemmaCounts_)[lemma];
	os << Core::XmlOpen("lemma")
	    + Core::XmlAttribute("name", lemma->name())
	    + Core::XmlAttribute("count", count)
	   << Core::XmlFull("orth", lemma->preferredOrthographicForm())
	   << Core::XmlClose("lemma");
	if (!internal_->isNonWordLemma(lemma)) {
	    nWordMatchPositions += count;
	}
    }

    os << (Core::XmlOpen("lemma") + Core::XmlAttribute("special", "unknown"))
       << Core::XmlComment("occurances of unknown words");
    f64 nWordFailurePositions = 0;
    typedef std::vector<std::string> WordList;
    WordList list;
    std::transform(internal_->unmatchedOrthCounts_.begin(),
		   internal_->unmatchedOrthCounts_.end(),
		   std::back_insert_iterator<WordList>(list),
		   Core::select1st<Internal::UnmatchedOrthMap::value_type>());
    std::sort(list.begin(), list.end());
    for (std::vector<std::string>::const_iterator i = list.begin(); i != list.end(); ++i) {
	const u32 count = const_cast<Internal::UnmatchedOrthMap&>(internal_->unmatchedOrthCounts_)[*i];
	os << Core::XmlFull("orth", *i)
	    + Core::XmlAttribute("count", count);
	nWordFailurePositions += count;
    }
    os << Core::XmlClose("lemma");

    os << Core::XmlFull(
	"out-of-vocabulary-rate",
	(nWordFailurePositions / (nWordMatchPositions + nWordFailurePositions)) * 100);

    os << Core::XmlClose("lexical-statistics");
}

// ===========================================================================
CompositeCorpusStatisticsVisitor::CompositeCorpusStatisticsVisitor(
    const Core::Configuration &c) :
    CorpusStatisticsVisitor(c)
{}

void CompositeCorpusStatisticsVisitor::add(CorpusStatisticsVisitor *c) {
    components_.push_back(c);
}

void CompositeCorpusStatisticsVisitor::reset() {
    for (ComponentList::iterator i = components_.begin(); i != components_.end(); ++i)
	(*i)->reset();
}

void CompositeCorpusStatisticsVisitor::enterRecording(Recording *r) {
    for (ComponentList::iterator i = components_.begin(); i != components_.end(); ++i)
	(*i)->enterRecording(r);
}

void CompositeCorpusStatisticsVisitor::visitSegment(Segment *s) {
    for (ComponentList::iterator i = components_.begin(); i != components_.end(); ++i)
	(*i)->visitSegment(s);
}

void CompositeCorpusStatisticsVisitor::visitSpeechSegment(SpeechSegment *s) {
    for (ComponentList::iterator i = components_.begin(); i != components_.end(); ++i)
	(*i)->visitSpeechSegment(s);
}

void CompositeCorpusStatisticsVisitor::leaveRecording(Recording *r) {
    for (ComponentList::iterator i = components_.begin(); i != components_.end(); ++i)
	(*i)->leaveRecording(r);
}

void CompositeCorpusStatisticsVisitor::writeReport(Core::XmlWriter &os) const {
    for (ComponentList::const_iterator i = components_.begin(); i != components_.end(); ++i)
	(*i)->writeReport(os);
}

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
// $Id: CorpusStatistics.cc 7257 2009-07-10 12:17:00Z rybach $

#include <Core/Utility.hh>
#include <Fsa/Basic.hh>
#include <Fsa/Best.hh>
#include <Fsa/Cache.hh>
#include <Fsa/Linear.hh>
#include <Fsa/Output.hh>
#include <Fsa/Sort.hh>
#include <Lm/Compose.hh>
#include <cmath>

#include "Module.hh"
#include "CorpusStatistics.hh"

using namespace Lm;


const f32 ln2 = .69314718055994530942;

static const u32 ReportLanguageModel = 1;
static const u32 ReportClassModel    = 2;

struct CorpusStatisticsVisitor::SentenceStatistics
{
    u32 nLemmas;
    u32 nSyntacticTokens;
    f32 avgEvaluationTokens;
    u32 nUnkLemmas;
    u32 nUnkSyntacticTokens;
    f32 avgUnkEvaluationTokens;
    f64 score;

    SentenceStatistics() :
	nLemmas(0),
	nSyntacticTokens(0),
	avgEvaluationTokens(0.0),
	nUnkLemmas(0),
	nUnkSyntacticTokens(0),
	avgUnkEvaluationTokens(0.0),
	score(0.0) {}

    f32 lemmaPerplexity() const
	{ return (nLemmas)             ? exp(score / f32(nLemmas))           : Core::Type<f32>::max; }
    f32 lemmaCrossEntropy() const
	{ return (nLemmas)             ? score / f32(nLemmas) / ln2          : Core::Type<f32>::max; }
    f32 syntPerplexity() const
	{ return (nSyntacticTokens)    ? exp(score / f32(nSyntacticTokens))  : Core::Type<f32>::max; }
    f32 syntCrossEntropy() const
	{ return (nSyntacticTokens)    ? score / f32(nSyntacticTokens) / ln2 : Core::Type<f32>::max; }
    f32 evalPerplexity() const
	{ return (avgEvaluationTokens) ? exp(score / avgEvaluationTokens)    : Core::Type<f32>::max; }
    f32 evalCrossEntropy() const
	{ return (avgEvaluationTokens) ? score / avgEvaluationTokens / ln2   : Core::Type<f32>::max; }
};


struct CorpusStatisticsVisitor::TextStatistics :
    public CorpusStatisticsVisitor::SentenceStatistics
{
    u32 nSentences;

    TextStatistics() :
	SentenceStatistics(),
	nSentences(0) {}

    void operator+= (const SentenceStatistics &stats) {
	++nSentences;
	nLemmas += stats.nLemmas;
	nSyntacticTokens += stats.nSyntacticTokens;
	avgEvaluationTokens += stats.avgEvaluationTokens;
	nUnkLemmas += stats.nUnkLemmas;
	nUnkSyntacticTokens += stats.nUnkSyntacticTokens;
	avgUnkEvaluationTokens += stats.avgUnkEvaluationTokens;
	score += stats.score;
    }
};

const Core::ParameterBool CorpusStatisticsVisitor::paramIgnoreUnknowns(
    "ignore-unknowns",
    "ignore lemmas containing unknown syntactic token(s)",
    true);

const Core::ParameterBool CorpusStatisticsVisitor::paramUseClassEmissionProbabilities(
    "use-class-emission-probabilities",
    "use class emission probabilities",
    true);

CorpusStatisticsVisitor::CorpusStatisticsVisitor(
    const Core::Configuration &c, Bliss::LexiconRef l) :
    Bliss::CorpusStatisticsVisitor(c),
    lexicon_(l),
    sentenceChannel_(c, "sentence"),
    orthographicParser_(c, l)
{
    lm_ = Lm::Module::instance().createLanguageModel(config, lexicon_);
    if (!lm_)
	criticalError("failed to initialize language model");
    lmStats_ = cmStats_ = 0;
    syntaxEmissionScale_ = 1.0;
    ignoreUnk_ = paramIgnoreUnknowns(config);
    what_ = (paramUseClassEmissionProbabilities(config)) ?
	ReportLanguageModel : ReportClassModel;
    if (what_ & ReportClassModel) {
	classLm_ = dynamic_cast<Lm::ClassLm*>(lm_.get());
	if (classLm_) {
	    log("class lm found");
	    classMapping_ = classLm_->classMapping();
	    classEmissionScale_ = classLm_->classEmissionScale();
	    Core::XmlChannel ch(select("classes"), "dump");
	    if (ch.isOpen()) classLm_->classMapping()->writeClasses(ch);
	} else
	    classEmissionScale_ = 0.0;
    } else {
	classLm_ = 0;
	classEmissionScale_ = 0.0;
    }
    reset();
}

CorpusStatisticsVisitor::~CorpusStatisticsVisitor() {
    delete lmStats_;
    delete cmStats_;
}

void CorpusStatisticsVisitor::reset() {
    delete lmStats_;
    lmStats_ = new TextStatistics;
    delete cmStats_;
    cmStats_ = new TextStatistics;
}

/**
 * @page Perplexity On computing perplexity / cross entropy
 *
 Definitions:
 cross-entropy in bit
 H_p(T)  =        - \frac{1}{W} \log_2 p(T)
 perplexity
 PP_p(T) =  \exp( - \frac{1}{W} \log   p(T) )

 where $T$ is the text and $W$ is the length of the text measured in
 words.  If the $s_1, \ldots, s_K$ are the sentences of the text, then

 p(T) = \prod_{k=1}^K p(s_k)

 * A recurring issue with perplexity is whether or not to count the
 * sentence end.  The option chosen here, is to treat the sentence end
 * as a genuine event.  I.e. the term p(sentence-end | final words) is
 * included in $p(s)$, and $W$ is the number of "ordinary" words plus
 * one for each sentences.  (This is consistent with well-known
 * publications such as [Chen & Goodman 1999].)
 *
 * Calculating the perplexity is less straight forward than you might
 * think, due to the concept of language model phrases.  The idea is
 * that multiple words are treated as a single event by the language
 * model.  In Bliss such a thing is represented as an ortographic form
 * containing one (or more) blank character(s).  Now let us consider
 * the following example: first the Bliss lexicon:

 <lemma>
 <orth>going</orth>
 <phon>g @U I N</phon>
 </lemma>
 <lemma>
 <orth>to</orth>
 <phon>t u:</phon>
 </lemma>
 <lemma>
 <orth>going to</orth>
 <phon>g O n @</phon>
 <synt><tok>going_to</tok></synt>
 <eval><tok>going</tok><tok>to</tok></eval>
 </lemma>
 <lemma>
 <orth>New York</orth>
 <phon>n u: j O: k</phon>
 <synt><tok>[CITY]</tok></synt>
 </lemma>
 <lemma>
 <orth>[breath]</orth>
 <synt/>
 <eval/>
 </lemma>

 * Now what about the sentence "I'am going to [breath] visit New York".  We
 * observe that the phrase "going to" allows for two parsings, namely
 * as one or as two lemmata, while "New York" can only be read as a
 * single token.  (Let's assume that "New" with captial N does not
 * occur in the lexicon.)  So there are two possible syntactic token
 * sequences:

 I'am  going  to  visit  [CITY]  </s>
 I'am  going_to   visit  [CITY]  </s>

 * The first question when computing perplexity is: What is the
 * log-likelihood LL = ln(p(W))?  The solution implemented below is:
 * Use the most likely token sequence, i.e. LL = max_W ln(p(W)).
 *
 * The second question is what is the number of words $W$?  We might
 * use the length of the most likely token sequence, but this would
 * bias the result in favour of long phrases.  We could count the
 * number of blanks (plus one), but this would violate an important
 * design principle of Bliss and moreover you can argue that "New
 * York" should be counted as a single word unless "New" and "York"
 * individually also have the status of a word, which is not true for
 * "New".  Non-lexical events like '[breath]" pose another
 * complication: Some system setups model them as language model
 * events others don't.  A common formal characterisitic property is
 * that they do not count in evaluation.  The solution to these issues
 * as implemented below is: Take the most likely lemma sequence,
 * according to the language model, and count the number of evaluation
 * tokens (plus one for the sentence end).  In this example the result
 * is six.  Addtionally we provide numbers on the basis of syntactic
 * tokens.
 */
CorpusStatisticsVisitor::SentenceStatistics CorpusStatisticsVisitor::buildSentenceStatistics(
    Fsa::ConstAutomatonRef sentence,
    Lm::Score syntaxEmissionScale,
    Lm::Score classEmissionScale) {

    SentenceStatistics stats;

    // determine best lemma sequence (in case the orthography was ambigious)
    if (classLm_)
	classLm_->setClassEmissionScale(classEmissionScale);
    sentence = Lm::compose(sentence, lm_, 1.0, syntaxEmissionScale);
    sentence = Fsa::firstbest(sentence);
    if (Fsa::isEmpty(sentence))
	return stats;

    // convert to sequence of lemmas
    std::vector<Fsa::LabelId> labels;
    Fsa::getLinearInput(sentence, labels);
    Core::Ref<const Bliss::LemmaAlphabet> lemmaAlphabet = ref(dynamic_cast<const Bliss::LemmaAlphabet*>(sentence->getInputAlphabet().get()));
    std::vector<const Bliss::Lemma*> lemmas;
    for (std::vector<Fsa::LabelId>::const_iterator li = labels.begin() ; li != labels.end() ; ++li)
	lemmas.push_back(lemmaAlphabet->lemma(*li));

    // process LM events
    History h = lm_->startHistory();
    for (std::vector<const Bliss::Lemma*>::const_iterator ll = lemmas.begin() ; ll != lemmas.end() ; ++ll) {
	const Bliss::SyntacticTokenSequence syntacticTokens((*ll)->syntacticTokenSequence());
	u32 nSyntacticTokens = syntacticTokens.length();
	u32 nEvaluationTokens = 0;
	u32 nEvaluationTokenSequences = 0;
	Bliss::Lemma::EvaluationTokenSequenceIterator e, e_end;
	for (Core::tie(e, e_end) = (*ll)->evaluationTokenSequences(); e != e_end; ++e) {
	    nEvaluationTokens += e->length();
	    ++nEvaluationTokenSequences;
	}
	f32 avgEvaluationTokens = nEvaluationTokenSequences ?
	    f32(nEvaluationTokens) / f32(nEvaluationTokenSequences) : 0;
	bool hasAnythingToReport = (nSyntacticTokens > 0) || (nEvaluationTokens > 0);

	if (sentenceChannel_.isOpen() && hasAnythingToReport)
	    sentenceChannel_
		<< (Core::XmlOpen("lemma")
		    +  Core::XmlAttribute("name", (*ll)->name().str()));
	Lm::Score lemmaScore = 0.0;
	bool isUnk = false;
	for (u32 ti = 0; ti < syntacticTokens.length(); ++ti) {
	    const Bliss::SyntacticToken *st = syntacticTokens[ti];
	    Lm::Score lmScore = lm_->score(h, st);
	    if (lmScore != Core::Type<Lm::Score>::max) {
		lmScore += syntaxEmissionScale * st->classEmissionScore();
		if (classMapping_)
		    lmScore += classEmissionScale * classMapping_->classEmissionScore(st);
	    } else
		isUnk = true;
	    if (sentenceChannel_.isOpen()) {
		sentenceChannel_
		    << Core::XmlOpen("event")
		    << Core::XmlFull("token", st->symbol());
		if (classMapping_) sentenceChannel_
		    << Core::XmlFull("class", classMapping_->classToken(st)->symbol());
		sentenceChannel_
		    << Core::XmlFull("history", lm_->formatHistory(h))
		    << Core::XmlFull("lm-score", lmScore)
		    << Core::XmlClose("event");
	    }
	    lemmaScore += lmScore;
	    h = lm_->extendedHistory(h, st);
	}
	if (isUnk) {
	    ++stats.nUnkLemmas;
	    stats.nUnkSyntacticTokens += nSyntacticTokens;
	    stats.avgUnkEvaluationTokens += avgEvaluationTokens;
	}
	if (!(isUnk && ignoreUnk_)) {
	    ++stats.nLemmas;
	    stats.nSyntacticTokens += nSyntacticTokens;
	    stats.avgEvaluationTokens += avgEvaluationTokens;
	    stats.score += lemmaScore;
	}
	if (sentenceChannel_.isOpen() && hasAnythingToReport)
	    sentenceChannel_
		<< Core::XmlFull("words", avgEvaluationTokens)
		//		<< Core::XmlFull("score", stats.score)
		<< Core::XmlClose("lemma");
    }
    Lm::Score exitScore = lm_->sentenceEndScore(h);
    stats.avgEvaluationTokens += 1.0;
    stats.score += exitScore;
    if (sentenceChannel_.isOpen()) {
	sentenceChannel_
	    << Core::XmlOpen("sentence-end")
	    << Core::XmlOpen("event");
	if (lm_->sentenceEndToken()) {
	    sentenceChannel_
		<< Core::XmlFull("token", lm_->sentenceEndToken()->symbol());
	    if (classMapping_) sentenceChannel_
		<< Core::XmlFull("class", lm_->sentenceEndToken()->symbol());
	} else
	    sentenceChannel_
		<< Core::XmlFull("token", "n/a");
	sentenceChannel_
	    << Core::XmlFull("history", lm_->formatHistory(h))
	    << Core::XmlFull("sentence-end-score", exitScore)
	    << Core::XmlClose("event")
	    << Core::XmlFull("words", 1)
	    //	    << Core::XmlFull("score", stats.score)
	    << Core::XmlClose("sentence-end");
    }
    if (sentenceChannel_.isOpen()) {
	sentenceChannel_
	    << (Core::XmlFull("log-likelihood", -stats.score)
		+ Core::XmlAttribute("base", "e"))
	    <<  Core::XmlFull("words", stats.avgEvaluationTokens)
	    << (Core::XmlFull("perplexity", stats.evalPerplexity())
		+ Core::XmlAttribute("base", "word"))
	    << (Core::XmlFull("cross-entropy", stats.evalCrossEntropy())
		+ Core::XmlAttribute("base", "word")
		+ Core::XmlAttribute("unit", "bit"))
	    <<  Core::XmlFull("events", stats.nSyntacticTokens)
	    << (Core::XmlFull("perplexity", stats.syntPerplexity())
		+ Core::XmlAttribute("base", "event"))
	    << (Core::XmlFull("cross-entropy", stats.syntCrossEntropy())
		+ Core::XmlAttribute("type", "event")
		+ Core::XmlAttribute("unit", "bit"));
    }

    // check against FSA result
    if (!ignoreUnk_) {
	Fsa::Weight scoreFsa = Fsa::getLinearWeight(sentence);
	if (!Core::isAlmostEqualUlp(f32(stats.score), f32(scoreFsa), 10))
	    warning("Lm/Fsa discrepancy: sentence log-likelihood %g != %g",
		    f32(stats.score), f32(scoreFsa));
    }

    return stats;
}

void CorpusStatisticsVisitor::visitSpeechSegment(Bliss::SpeechSegment *segment) {
    sentenceChannel_
	<< Core::XmlOpen("orth") + Core::XmlAttribute("source", "reference")
	<< segment->orth()
	<< Core::XmlClose("orth");

    Fsa::ConstAutomatonRef sentence = orthographicParser_.createLemmaAcceptor(segment->orth());

    if (what_ & ReportLanguageModel) {
	if (sentenceChannel_.isOpen())
	    sentenceChannel_
		<< (Core::XmlOpen("sentence-statistics") + Core::XmlAttribute("model", "language model"));
	SentenceStatistics lmStats = buildSentenceStatistics(sentence, syntaxEmissionScale_, classEmissionScale_);
	if (lmStats.nLemmas > 0) *lmStats_ += lmStats;
	else error("failed to determine best lemma sequence for orthography \"%s\"",
		   segment->orth().c_str());
	if (sentenceChannel_.isOpen())
	    sentenceChannel_ << Core::XmlClose("sentence-statistics");
    }
    if (what_ & ReportClassModel) {
	if (sentenceChannel_.isOpen())
	    sentenceChannel_
		<< (Core::XmlOpen("sentence-statistics") + Core::XmlAttribute("model", (classLm_ ? "class model" : "syntax model")));
	SentenceStatistics cmStats = buildSentenceStatistics(sentence, 0.0, 0.0);
	if (cmStats.nLemmas > 0) *cmStats_ += cmStats;
	else error("failed to determine best lemma sequence for orthography \"%s\"",
		   segment->orth().c_str());
	if (sentenceChannel_.isOpen())
	    sentenceChannel_ << Core::XmlClose("sentence-statistics");
    }
}

void CorpusStatisticsVisitor::writeReport(Core::XmlWriter &xml, TextStatistics &stats) const {
    xml << (Core::XmlFull("log-likelihood", -stats.score)
	    + Core::XmlAttribute("base", "e")
	    + Core::XmlAttribute("ignore-unknowns", (ignoreUnk_ ? "yes" : "no")))
	<<  Core::XmlFull("sentences", stats.nSentences)
	<<  Core::XmlFull("words", stats.avgEvaluationTokens)
	<<  Core::XmlFull("unk-words", stats.avgUnkEvaluationTokens)
	<< (Core::XmlFull("perplexity", stats.evalPerplexity())
	    + Core::XmlAttribute("base", "word"))
	<< (Core::XmlFull("cross-entropy", stats.evalCrossEntropy())
	    + Core::XmlAttribute("base", "word")
	    + Core::XmlAttribute("unit", "bit"))
	<<  Core::XmlFull("events", stats.nSyntacticTokens)
	<<  Core::XmlFull("unk-events", stats.nUnkSyntacticTokens)
	<< (Core::XmlFull("perplexity", stats.syntPerplexity())
	    + Core::XmlAttribute("base", "event"))
	<< (Core::XmlFull("cross-entropy", stats.syntCrossEntropy())
	    + Core::XmlAttribute("base", "event")
	    + Core::XmlAttribute("unit", "bit"));
}

void CorpusStatisticsVisitor::writeReport(Core::XmlWriter &xml) const {
    if (what_ & ReportLanguageModel) {
	xml << (Core::XmlOpen("statistics") + Core::XmlAttribute("model", "language model"));
	writeReport(xml, *lmStats_);
	xml << Core::XmlClose("statistics");
    }
    if (what_ & ReportClassModel) {
	xml << (Core::XmlOpen("sentence-statistics") + Core::XmlAttribute("model", (classLm_ ? "class model" : "syntax model")));
	writeReport(xml, *cmStats_);
	xml << Core::XmlClose("statistics");
    }
}

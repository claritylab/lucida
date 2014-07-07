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
// $Id: Evaluation.cc 9621 2014-05-13 17:35:55Z golik $

#include "Evaluation.hh"
#include <Bliss/Orthography.hh>
#include <Bliss/EditDistance.hh>
#include <Bliss/Fsa.hh>
#include <Bliss/Unknown.hh>
#include <Core/Assertions.hh>
#include <Fsa/Arithmetic.hh>
#include <Fsa/Basic.hh>
#include <Fsa/Best.hh>
#include <Fsa/Cache.hh>
#include <Fsa/Compose.hh>
#include <Fsa/Determinize.hh>
#include <Fsa/Minimize.hh>
#include <Fsa/Output.hh>
#include <Fsa/Project.hh>
#include <Fsa/Rational.hh>
#include <Fsa/RemoveEpsilons.hh>
#include <Fsa/Static.hh>

using namespace Bliss;


const Core::ParameterBool Evaluator::paramWordErrors(
    "word-errors",
    "compute edit-distance on word level",
    true);
const Core::ParameterBool Evaluator::paramLetterErrors(
    "letter-errors",
    "compute edit-distance on letter level",
    false);
const Core::ParameterBool Evaluator::paramFilterLettersByEvalTokens(
    "filter-letters-by-eval-tokens",
    "when computing letter-errors, ignore letters for lemmas which have no eval token",
    false);
const Core::ParameterBool Evaluator::paramPhonemeErrors(
    "phoneme-errors",
    "compute edit-distance on phoneme level",
    false);


class Evaluator::LetterAcceptorBuilder : public OrthographicParser::Handler {
    typedef OrthographicParser::Handler Precursor;
private:
    Core::Ref<Fsa::StaticAutomaton> product_;
public:
    LetterAcceptorBuilder() {}
    ~LetterAcceptorBuilder() {}
    Fsa::ConstAutomatonRef product() const { return product_; }

    virtual void initialize(const OrthographicParser *parser) {
	Precursor::initialize(parser);
	product_ = ref(new Fsa::StaticAutomaton);
	product_->setType(Fsa::TypeAcceptor);
	product_->setSemiring(Fsa::TropicalSemiring);
	product_->setInputAlphabet(parser_->lexicon()->letterAlphabet());
    }

    virtual OrthographicParser::Node newNode() {
	return product_->newState();
    }

    void buildString(Fsa::State *source, Fsa::State *target, const std::string &orth) {
	Fsa::Weight one = Fsa::Weight(product_->semiring()->one());
	const char *cc, *nc;
	Fsa::State *state = source;
	for (cc = nc = orth.c_str(); *cc; cc=nc) {
	    do ++nc; while (*nc && utf8::byteType(*nc) == utf8::multiByteTail);
	    std::string letter(cc, nc);
	    Fsa::State *next = (*nc) ? product_->newState() : target;
	    state->newArc(next->id(), one, parser_->lexicon()->letter(letter)->id());
	    state = next;
	}
	if (state != target) // when orth has length zero
	    source->newArc(target->id(), one, Fsa::Epsilon);
    }

    virtual void newEdge(
	OrthographicParser::Node from,
	OrthographicParser::Node to,
	const Lemma *lemma)
    {
	Fsa::State *source = static_cast<Fsa::State*>(from);
	Fsa::State *target = static_cast<Fsa::State*>(to);
	if (lemma) {
#if 1
	    buildString(source, target, lemma->preferredOrthographicForm());
#endif
	} else {
	    source->newArc(target->id(), product_->semiring()->one(), Fsa::Epsilon);
	}
    }

    void newUnmatchableEdge(
	OrthographicParser::Node from, OrthographicParser::Node to,
	const std::string &orth)
    {
	Fsa::State *source = static_cast<Fsa::State*>(from);
	Fsa::State *target = static_cast<Fsa::State*>(to);
	buildString(source, target, orth);
    }

    void finalize(
	OrthographicParser::Node initial,
	OrthographicParser::Node final)
    {
	product_->setInitialStateId(static_cast<Fsa::State*>(initial)->id());
	product_->setStateFinal(static_cast<Fsa::State*>(final));
	product_->normalize();
    }

};


Evaluator::Evaluator(
    const Core::Configuration &c,
    Core::Ref<const Lexicon> l) :
    Core::Component(c),
    lexicon_(l),
    orthParser_(),
    letterAcceptorBuilder_(0),
    editDistance_(0),
    graphStatisticsChannel_(config, "graph-statistics"),
    graphDumpChannel_(config, "dump-graphs")
{
    shallComputeWordErrors_    = paramWordErrors(config);
    shallComputeLetterErrors_  = paramLetterErrors(config);
    shallFilterLettersByEvalTokens_ = paramFilterLettersByEvalTokens(config);
    shallComputePhonemeErrors_ = paramPhonemeErrors(config);

    orthParser_ = Core::ref(new Bliss::OrthographicParser(config, lexicon_));
    lemmaPronToLemma_ = Fsa::multiply(lexicon_->createLemmaPronunciationToLemmaTransducer(), Fsa::Weight(0.0));
    lemmaToSynt_ = Fsa::multiply(lexicon_->createLemmaToSyntacticTokenTransducer(), Fsa::Weight(0.0));
    lemmaToEval_ = Fsa::multiply(lexicon_->createLemmaToEvaluationTokenTransducer(), Fsa::Weight(0.0));
    //    lemmaToPreferredEval_ = Fsa::multiply(lexicon_->createLemmaToPreferredEvaluationTokenSequenceTransducer(), Fsa::Weight(0.0));

    if (shallComputeLetterErrors_ || shallComputePhonemeErrors_) {
	lemmaToLemmaConfusion_ = Fsa::composeMatching(lemmaToEval_, Fsa::invert(lemmaToEval_));
	lemmaToLemmaConfusion_ = Fsa::cache(lemmaToLemmaConfusion_);
    }

    if (shallComputeLetterErrors_) {
	letterAcceptorBuilder_ = new LetterAcceptorBuilder;
	lemmaToLetter_ = lexicon_->createLemmaToOrthographyTransducer(false, shallFilterLettersByEvalTokens_);
    }

    if (shallComputePhonemeErrors_) {
	Fsa::ConstAutomatonRef phonToPron =
	    lexicon_->createPhonemeToLemmaPronunciationTransducer();
	lemmaPronToPhoneme_ = Fsa::cache(Fsa::invert(phonToPron));
	lemmaToPhoneme_ = Fsa::invert(
	    Fsa::removeDisambiguationSymbols(
		Fsa::fuse(
		    Fsa::composeSequencing(
			phonToPron,
			lemmaPronToLemma_),
		    createAnyPhonemeToUnknownTransducer(lexicon_))));
	lemmaToPhoneme_ = Fsa::cache(lemmaToPhoneme_);
    }

    editDistance_ = new EditDistance(select("edit-distance"));
}

Evaluator::~Evaluator() {
    delete letterAcceptorBuilder_;
    delete editDistance_;
}

void Evaluator::setReferenceTranscription(const std::string &referenceTranscription) {
    correct_.lemma = orthParser_->createLemmaAcceptor(referenceTranscription);

    graphStatisticsChannel_ << Core::XmlOpen("reference") + Core::XmlAttribute("type", "lemma");
    if (graphStatisticsChannel_.isOpen())
	Fsa::info(correct_.lemma, graphStatisticsChannel_);
    if (graphDumpChannel_.isOpen())
	Fsa::drawDot(correct_.lemma, graphDumpChannel_);
    graphStatisticsChannel_ << Core::XmlClose("reference");

    if (shallComputeWordErrors_) {
	correct_.eval = Fsa::staticCopy(
	    Fsa::minimize(
		Fsa::determinize(
		    Fsa::projectOutput(
			Fsa::composeMatching(
			    correct_.lemma,
			    lemmaToEval_)))));

	graphStatisticsChannel_ << Core::XmlOpen("reference") + Core::XmlAttribute("type", "eval");
	if (graphStatisticsChannel_.isOpen())
	    Fsa::info(correct_.eval, graphStatisticsChannel_);
	if (graphDumpChannel_.isOpen())
	    Fsa::drawDot(correct_.eval, graphDumpChannel_);
	graphStatisticsChannel_ << Core::XmlClose("reference");
    } else {
	correct_.eval.reset();
    }

    if (shallComputeLetterErrors_) {
	orthParser_->parse(referenceTranscription, *letterAcceptorBuilder_);
	if(shallFilterLettersByEvalTokens_)
	{
		correct_.orth =
		    Fsa::staticCopy(Fsa::trim(Fsa::projectOutput(
			Fsa::composeMatching(
			    correct_.lemma,
			    lemmaToLetter_))));
	}else{
	correct_.orth = Fsa::staticCopy(
	    Fsa::minimize(
		Fsa::determinize(
		    Fsa::removeEpsilons(
			letterAcceptorBuilder_->product()))));
	}

	graphStatisticsChannel_ << Core::XmlOpen("reference") + Core::XmlAttribute("type", "orth");
	if (graphStatisticsChannel_.isOpen())
	    Fsa::info(correct_.orth, graphStatisticsChannel_);
	if (graphDumpChannel_.isOpen())
	    Fsa::drawDot(correct_.orth, graphDumpChannel_);
	graphStatisticsChannel_ << Core::XmlClose("reference");
    } else {
	correct_.orth.reset();
    }

    if (shallComputePhonemeErrors_) {
	Fsa::ConstAutomatonRef confusedLemma = Fsa::minimize(
	    Fsa::determinize(
		Fsa::removeEpsilons(
		    Fsa::projectOutput(
			Fsa::composeMatching(
			    correct_.lemma,
			    lemmaToLemmaConfusion_)))));
	correct_.phon = Fsa::staticCopy(
	    Fsa::minimize(
		Fsa::determinize(
		    Fsa::projectOutput(
			Fsa::composeMatching(
			    confusedLemma,
			    lemmaToPhoneme_)))));

	graphStatisticsChannel_ << Core::XmlOpen("reference") + Core::XmlAttribute("type", "phon");
	if (graphStatisticsChannel_.isOpen())
	    Fsa::info(correct_.phon, graphStatisticsChannel_);
	if (graphDumpChannel_.isOpen())
	    Fsa::drawDot(correct_.phon, graphDumpChannel_);
	graphStatisticsChannel_ << Core::XmlClose("reference");

	if (Fsa::isEmpty(correct_.phon)) {
	    warning("The set of correct phoneme sequences is empty.  Phoneme errors cannot be computed.");
	    correct_.phon.reset();
	}
    } else {
	correct_.phon.reset();
    }
}


/**
 * Compute and report density of the lattice in terms of arcs per
 * reference word.
 *
 * Warning: The word lattice density as defined here is a bit
 * "skewed": The denominator counts evaluation tokens, while the
 * numerator counts lemma pronuncations.  (Actually it is not even
 * guatanteed that @c nWordArcs corresponds to lemma pronuncations,
 * since the interface of this function is rather fexible.)  However,
 * this may be justified since eval tokens corrspond to "words that
 * matter" and the number of arcs in the lattice to "search and
 * storage effort.  Therefore the resulting quotient is a figure of
 * efficiency of the lattice.  Another issue is that the number of
 * eval tokens can be ambiguous (depending on the lexicon).  This is
 * addressed by using the sequence which yields the fewest errors.
 */
void Evaluator::reportLatticeDensity(
    Fsa::ConstAutomatonRef lattice,
    const ErrorStatistic &latticeWordErrors)
{
    Core::XmlWriter &os(clog());

    u32 nCorrectEvalTokens = latticeWordErrors.nLeftTokens();
    os << Core::XmlFull("correct-words", nCorrectEvalTokens);

    Fsa::AutomatonCounts countsLattice = Fsa::count(lattice);
    u32 nArcs = countsLattice.nArcs_ - countsLattice.nIEps_;
    os << Core::XmlFull("word-arcs", nArcs);

    f32 wordLatticeDensity = f32(nArcs) / f32(nCorrectEvalTokens);
    os << Core::XmlFull("word-lattice-density", wordLatticeDensity);
}

u32 Evaluator::evaluateWords(
    Fsa::ConstAutomatonRef lemmaOrPronOrSyntOrEval,
    const std::string &name)
{
    require(!Fsa::isEmpty(lemmaOrPronOrSyntOrEval));
    require(lemmaOrPronOrSyntOrEval->getOutputAlphabet() == lexicon_->lemmaPronunciationAlphabet() ||
	    lemmaOrPronOrSyntOrEval->getOutputAlphabet() == lexicon_->lemmaAlphabet() ||
	    lemmaOrPronOrSyntOrEval->getOutputAlphabet() == lexicon_->syntacticTokenAlphabet() ||
	    lemmaOrPronOrSyntOrEval->getOutputAlphabet() == lexicon_->evaluationTokenAlphabet());

    Fsa::ConstAutomatonRef lemma, lemmaPron;
    if (lemmaOrPronOrSyntOrEval->getOutputAlphabet() == lexicon_->lemmaPronunciationAlphabet()) {
	lemmaPron = lemmaOrPronOrSyntOrEval;
	lemma = Fsa::cache(Fsa::composeMatching(lemmaPron, lemmaPronToLemma_));
    } else if (lemmaOrPronOrSyntOrEval->getOutputAlphabet() == lexicon_->lemmaAlphabet()) {
	lemma = lemmaOrPronOrSyntOrEval;
    } else if (lemmaOrPronOrSyntOrEval->getOutputAlphabet() == lexicon_->syntacticTokenAlphabet()) {
	lemma = Fsa::cache(Fsa::composeMatching(lemmaOrPronOrSyntOrEval, Fsa::invert(lemmaToSynt_)));
    } else if (lemmaOrPronOrSyntOrEval->getOutputAlphabet() == lexicon_->evaluationTokenAlphabet()) {
	lemma = Fsa::cache(Fsa::composeMatching(lemmaOrPronOrSyntOrEval, Fsa::invert(lemmaToEval_)));
    }

    u32 result = 0;

    if (shallComputeWordErrors_) {
	Fsa::ConstAutomatonRef eval =
	    Fsa::projectOutput(
		Fsa::composeMatching(
		    lemma,
		    lemmaToEval_));
	eval = Fsa::staticCopy(Fsa::trim(eval));
	ErrorStatistic errors = evaluateMetric(
	    correct_.eval, eval, "eval", name);
	reportLatticeDensity(Fsa::projectOutput(lemma), errors);
	result = errors.nErrors();
    }

    if (shallComputeLetterErrors_) {
	Fsa::ConstAutomatonRef orth =
	    Fsa::projectOutput(
		Fsa::composeMatching(
		    lemma,
		    lemmaToLetter_));
	orth = Fsa::staticCopy(Fsa::trim(orth));
	evaluateMetric(correct_.orth, orth, "orth", name);
    }

    if (shallComputePhonemeErrors_ && correct_.phon && lemmaPron) {
	Fsa::ConstAutomatonRef phon = Fsa::removeDisambiguationSymbols(
	    Fsa::projectOutput(
		Fsa::composeMatching(
		    lemmaPron,
		    lemmaPronToPhoneme_)));
	phon = Fsa::staticCopy(Fsa::trim(phon));
	evaluateMetric(correct_.phon, phon, "phon", name);
    }

    return result;
}

u32 Evaluator::evaluatePhonemes(
    Fsa::ConstAutomatonRef phon,
    const std::string &name)
{
    require(phon->getOutputAlphabet() == lexicon_->phonemeInventory()->phonemeAlphabet());
    require(!Fsa::isEmpty(phon));

    u32 result = 0;

    if (shallComputePhonemeErrors_ && correct_.phon) {
	ErrorStatistic errors = evaluateMetric(
	    correct_.phon,
	    Fsa::staticCopy(Fsa::trim(phon)),
	    "phon", name);
	reportLatticeDensity(phon, errors);
	result = errors.nErrors();
    }

    return result;
}

u32 Evaluator::evaluate(
    Fsa::ConstAutomatonRef aa,
    const std::string &name)
{
    require(!Fsa::isEmpty(aa));
    if (aa->getOutputAlphabet() == lexicon_->lemmaPronunciationAlphabet())
	return evaluateWords(aa, name);
    if (aa->getOutputAlphabet() == lexicon_->lemmaAlphabet())
	return evaluateWords(aa, name);
    if (aa->getOutputAlphabet() == lexicon_->phonemeInventory()->phonemeAlphabet())
	return evaluatePhonemes(aa, name);
    if (aa->getOutputAlphabet() == lexicon_->syntacticTokenAlphabet())
	return evaluateWords(aa, name);
    if (aa->getOutputAlphabet() == lexicon_->evaluationTokenAlphabet())
	return evaluateWords(aa, name);
    require(aa->getOutputAlphabet() != aa->getOutputAlphabet());
    return 0;
}

ErrorStatistic Evaluator::evaluateMetric(
    Fsa::ConstAutomatonRef correct,
    Fsa::ConstAutomatonRef candidate,
    const std::string &type,
    const std::string &name)
{
    Core::XmlWriter &os(clog());
    os << Core::XmlOpen("evaluation")
	+ Core::XmlAttribute("name", name)
	+ Core::XmlAttribute("type", type);

    if (graphStatisticsChannel_.isOpen()) {
	graphStatisticsChannel_ << Core::XmlOpen("candidate");
	Fsa::info(candidate, graphStatisticsChannel_);
	graphStatisticsChannel_ << Core::XmlClose("candidate");
    }

    EditDistance::Alignment al = editDistance_->newAlignment();
    editDistance_->align(correct, candidate, al);
    ErrorStatistic errors(al);

    // report alignment
    al.write(os);

    // report alignment statistics
    os << (Core::XmlOpen("statistic")
	   + Core::XmlAttribute("type", "alignment"))
       << Core::XmlFull("cost", al.cost)
       << Core::XmlFull("edit-operations", errors.nErrors())
       << (Core::XmlFull("score", al.score)
	   + Core::XmlAttribute("source", "best"))
       << (Core::XmlFull("count", errors.nRightTokens())
	   + Core::XmlAttribute("event", "token")
	   + Core::XmlAttribute("source", "best"))
       << (Core::XmlFull("count", errors.nLeftTokens())
	   + Core::XmlAttribute("event", "token")
	   + Core::XmlAttribute("source", "reference"));
#if 0 // information of interest ?
    Fsa::AutomatonCounts countsCandidate = Fsa::count(candidate);
    os << (Core::XmlFull("lattice-arcs", countsCandidate.nArcs_)
	   + Core::XmlAttribute("source", "hypothesis"))
       << (Core::XmlFull("lattice-density", f32(countsCandidate.nArcs_) / f32(errors.nLeftTokens()))
	   + Core::XmlAttribute("source", "hypothesis"));
#endif
    os << Core::XmlClose("statistic");

    // report best sequence
    os << Core::XmlOpen(type)
	+ Core::XmlAttribute("source", "best sequence");
    for (EditDistance::Alignment::const_iterator ai = al.begin(); ai != al.end(); ++ai)
	if (ai->b)
	    os << ai->b->symbol() << Core::XmlBlank();
    os << Core::XmlClose(type);

    // report number of errors
    errors.write(os);

    os << Core::XmlClose("evaluation");

    return errors;
}

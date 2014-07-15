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
#include "WordLatticeExtractor.hh"
#include <Bliss/Lexicon.hh>
#include <Bliss/Orthography.hh>
#include <Fsa/Cache.hh>
#include <Fsa/Compose.hh>
#include <Fsa/Linear.hh>
#include <Fsa/Project.hh>
#include <Lattice/Basic.hh>
#include <Lattice/Merge.hh>
#include <Lattice/Rational.hh>
#include <Lattice/Static.hh>
#include <Lm/Module.hh>
#include "LatticeExtractor.hh"
#include "DataExtractor.hh"

using namespace Speech;

/**
 * WordLatticeUnion
 */
const Core::ParameterString WordLatticeUnion::paramFsaPrefix(
    "fsa-prefix",
    "prefix of automaton in archive",
    Lattice::WordLattice::acousticFsa);

WordLatticeUnion::WordLatticeUnion(const Core::Configuration &c) :
    Core::Component(c),
    Precursor(c),
    prefix_(paramFsaPrefix(config)),
    numeratorArchiveReader_(0)
{}

WordLatticeUnion::~WordLatticeUnion()
{
    delete numeratorArchiveReader_;
}

void WordLatticeUnion::initialize(Bliss::LexiconRef lexicon)
{
    Precursor::initialize(lexicon);

    verify(!numeratorArchiveReader_);
    if (!Lattice::Archive::paramPath(select("numerator-lattice-archive")).empty()) {
	numeratorArchiveReader_ = Lattice::Archive::openForReading(
	    select("numerator-lattice-archive"), lexicon);
	if (!numeratorArchiveReader_ or numeratorArchiveReader_->hasFatalErrors()) {
	    delete numeratorArchiveReader_; numeratorArchiveReader_ = 0;
	    error("failed to open lattice archive");
	    return;
	}
    }
}

void WordLatticeUnion::processWordLattice(
    Lattice::ConstWordLatticeRef denominator, Bliss::SpeechSegment *s)
{
    if (denominator and denominator->nParts() == 1) {
	Lattice::ConstWordLatticeRef numerator;
	if (numeratorArchiveReader_) {
	    numerator = numeratorArchiveReader_->get(s->fullName(), prefix());
	}
	if (numerator) {
	    denominator = Lattice::unite(denominator, numerator);
	} else {
	    warning(
		"no union for '%s' because there is no numerator lattice",
		s->fullName().c_str());
	}
	Precursor::processWordLattice(denominator, s);
    } else {
	warning("no union for '%s' because of empty denominator lattice",
		s->fullName().c_str());
    }
}

/**
 * BaseWordLatticeMerger
 */
const Core::ParameterBool BaseWordLatticeMerger::paramMergeOnlyIfSpokenNotInLattice(
    "merge-only-if-spoken-not-in-lattice",
    "merge denominator and numerator lattice only if spoken sentence is not in denominator lattice",
    true);

BaseWordLatticeMerger::BaseWordLatticeMerger(const Core::Configuration &c) :
    Core::Component(c),
    Precursor(c),
    mergeOnlyIfSpokenNotInLattice_(paramMergeOnlyIfSpokenNotInLattice(config)),
    orthToLemma_(0)
{}

BaseWordLatticeMerger::~BaseWordLatticeMerger()
{
    delete orthToLemma_;
}

bool BaseWordLatticeMerger::needsMerging(
    Lattice::ConstWordLatticeRef lattice, Bliss::SpeechSegment *s) const
{
    if (mergeOnlyIfSpokenNotInLattice_) {
	verify(orthToLemma_);
	Lattice::ConstWordLatticeRef spokenFromLattice =
	    Lattice::extractNumerator(
		s->orth(), lattice, orthToLemma_, lemmaPronToLemma_,
		lemmaToLemmaConfusion_);
	if (spokenFromLattice and
	    spokenFromLattice->mainPart() and
	    !Fsa::isEmpty(spokenFromLattice->mainPart())) {
	    return false;
	}
    }
    return true;
}

void BaseWordLatticeMerger::initialize(Bliss::LexiconRef lexicon)
{
    Precursor::initialize(lexicon);

    if (mergeOnlyIfSpokenNotInLattice_) {
	verify(!orthToLemma_);
	orthToLemma_ = new Bliss::OrthographicParser(select("orthographic-parser"), lexicon);
	lemmaPronToLemma_ = lexicon->createLemmaPronunciationToLemmaTransducer();

	Fsa::ConstAutomatonRef lemmaToEval = lexicon->createLemmaToEvaluationTokenTransducer();
	lemmaToLemmaConfusion_ = Fsa::composeMatching(lemmaToEval, Fsa::invert(lemmaToEval));
	lemmaToLemmaConfusion_ = Fsa::cache(lemmaToLemmaConfusion_);
    }
}

/**
 * WordLatticeMerger
 */
WordLatticeMerger::WordLatticeMerger(const Core::Configuration &c) :
    Core::Component(c),
    Precursor(c)
{}

void WordLatticeMerger::initialize(Bliss::LexiconRef lexicon)
{
    Precursor::initialize(lexicon);

    verify(!languageModel_);
    languageModel_ = Lm::Module::instance().createLanguageModel(select("lm"), lexicon);
    if (!languageModel_) criticalError("failed to initialize language model");
}

void WordLatticeMerger::processWordLattice(
    Lattice::ConstWordLatticeRef denominator,
    Bliss::SpeechSegment *s)
{
    if (denominator and denominator->nParts() == 1) {
	if (needsMerging(denominator, s)) {
	    Lattice::ConstWordLatticeRef numerator;
	    if (numeratorArchiveReader_) {
		numerator = numeratorArchiveReader_->get(s->fullName(), prefix());
	    }
	    if (numerator) {
		log("merging of reference required");
		denominator = Lattice::merge(denominator, numerator, languageModel_);
	    } else {
		warning(
		    "no merge for '%s' because there is no numerator lattice",
		    s->fullName().c_str());
	    }
	}
	LatticeSetProcessor::processWordLattice(denominator, s);
    } else {
	warning("no merge for '%s' because of empty denominator lattice",
		s->fullName().c_str());
    }
}

/**
 * SpokenAndCompetingListProcessor
 */
const Core::ParameterInt SpokenAndCompetingListProcessor::paramNumberOfHypotheses(
    "number-of-hypotheses",
    "target number of hypotheses after merge",
    -1);

SpokenAndCompetingListProcessor::SpokenAndCompetingListProcessor(
    const Core::Configuration &c)
    :
    Core::Component(c),
    Precursor(c),
    numberOfHypotheses_((u32)paramNumberOfHypotheses(c))
{
    mergeOnlyIfSpokenNotInLattice_ = true;
}

void SpokenAndCompetingListProcessor::processWordLattice(
    Lattice::ConstWordLatticeRef l, Bliss::SpeechSegment *s)
{
    if (l) {
	Core::Vector<Lattice::ConstWordLatticeRef> hypotheses;

	// add spoken hypothesis from archive
	require(numeratorArchiveReader_);
	hypotheses.push_back(
	    numeratorArchiveReader_->get(
		s->fullName(),
		prefix()));
	if (!Fsa::isLinear(hypotheses.back()->mainPart())) {
	    criticalError("numerator must be linear");
	}

	// add competing hypotheses from @input l
	Fsa::ConstStateRef lSp =
	    l->mainPart()->getState(l->mainPart()->initialStateId());
	for (Fsa::State::const_iterator hIt = lSp->begin();
	     (hypotheses.size() < numberOfHypotheses_) and (hIt != lSp->end());
	     ++ hIt) {
	    Lattice::ConstWordLatticeRef h = Lattice::partial(l, hIt->target(), hIt->weight());
	    if (needsMerging(h, s)) {
		hypotheses.push_back(h);
	    }
	}

	// combine hypotheses in single lattice
	for (u32 n = 0; n < hypotheses.size(); ++ n) {
	    hypotheses[n] =
		Lattice::staticCopy(
		    Lattice::normalize(
			hypotheses[n]));
	}
	Lattice::ConstWordLatticeRef result = Lattice::unite(hypotheses);

	// pass result to next node
	LatticeSetProcessor::processWordLattice(result, s);
    } else {
	warning("no move for '%s' because of empty n-best list",
		s->fullName().c_str());
    }
}

/**
 * NumeratorFromDenominatorExtractor
 */
NumeratorFromDenominatorExtractor::NumeratorFromDenominatorExtractor(const Core::Configuration &c) :
    Core::Component(c),
    Precursor(c),
    orthToLemma_(0)
{}

NumeratorFromDenominatorExtractor::~NumeratorFromDenominatorExtractor()
{
    delete orthToLemma_;
}

void NumeratorFromDenominatorExtractor::initialize(Bliss::LexiconRef lexicon)
{
    Precursor::initialize(lexicon);

    verify(!orthToLemma_);
    orthToLemma_ = new Bliss::OrthographicParser(select("orthographic-parser"), lexicon);
    lemmaPronToLemma_ = lexicon->createLemmaPronunciationToLemmaTransducer();

    Fsa::ConstAutomatonRef lemmaToEval = lexicon->createLemmaToEvaluationTokenTransducer();
    lemmaToLemmaConfusion_ = Fsa::composeMatching(lemmaToEval, Fsa::invert(lemmaToEval));
    lemmaToLemmaConfusion_ = Fsa::cache(lemmaToLemmaConfusion_);
}

void NumeratorFromDenominatorExtractor::processWordLattice(
    Lattice::ConstWordLatticeRef denominator, Bliss::SpeechSegment *s)
{
    Lattice::ConstWordLatticeRef numeratorFromDenominator =
	Lattice::extractNumerator(s->orth(), denominator,
				  orthToLemma_, lemmaPronToLemma_,
				  lemmaToLemmaConfusion_);
    Precursor::processWordLattice(numeratorFromDenominator, s);
}

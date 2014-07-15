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
#include "AccuracyFsaBuilder.hh"
#include <Bliss/Evaluation.hh>
#include <Bliss/Lexicon.hh>
#include <Bliss/Orthography.hh>
#include <Fsa/Arithmetic.hh>
#include <Fsa/Basic.hh>
#include <Fsa/Cache.hh>
#include <Fsa/Compose.hh>
#include <Fsa/Project.hh>
#include <Lattice/Archive.hh>
#include <Lattice/Arithmetic.hh>
#include <Lattice/Basic.hh>
#include <Lattice/Merge.hh>
#include <Lattice/Posterior.hh>
#include <Am/Utilities.hh>
#include <Core/StringUtilities.hh>

using namespace Speech;


/**
 * TimeAlignmentBasedMetricLatticeBuilder
 */
const Core::ParameterStringVector TimeAlignmentBasedMetricLatticeBuilder::paramShortPausesLemmata(
    "short-pauses-lemmata",
    "names of lemmata that are treated in a special way");

Core::Choice TimeAlignmentBasedMetricLatticeBuilder::choiceTokenType(
    "none", noneType,
    "lemma-pronunciation", lemmaPronunciationType,
    "lemma", lemmaType,
    "phone", phoneType,
    "state", stateType,
    Core::Choice::endMark());

Core::ParameterChoice TimeAlignmentBasedMetricLatticeBuilder::paramTokenType(
    "token-type",
    &choiceTokenType,
    "specify the tokens for the calculation of the metric",
    noneType);

TimeAlignmentBasedMetricLatticeBuilder::TimeAlignmentBasedMetricLatticeBuilder(
    const Core::Configuration &c, Bliss::LexiconRef lexicon)
    :
    Precursor(c),
    tokenType_(noneType)
{}

TimeAlignmentBasedMetricLatticeBuilder::~TimeAlignmentBasedMetricLatticeBuilder()
{}

void TimeAlignmentBasedMetricLatticeBuilder::setReference(
    Lattice::ConstWordLatticeRef reference)
{
    reference_.reset();
    if (reference && !Fsa::isEmpty(reference->mainPart())) {
	reference_ = reference;
    }
}

void TimeAlignmentBasedMetricLatticeBuilder::initializeShortPauses(Bliss::LexiconRef lexicon)
{
    verify(shortPauses_.empty());
    verify(tokenType_ != noneType);
    std::vector<std::string> shortPausesLemmata = paramShortPausesLemmata(config);
    for (std::vector<std::string>::const_iterator it = shortPausesLemmata.begin();
	 it != shortPausesLemmata.end(); ++ it) {
	std::string name(*it);
	Core::normalizeWhitespace(name);
	log("Append short pause lemma \"") << name << "\"";
	const Bliss::Lemma *lemma = lexicon->lemma(name);
	if (!lemma) {
	    error("Unknown lemma \"") << name << "\"";
	    continue;
	}
	switch (tokenType_) {
	case lemmaType: {
	    shortPauses_.insert(lemma->id());
	} break;
	case lemmaPronunciationType:
	case phoneType: {
	    if (lemma->nPronunciations() == 0) {
		error("Lemma \"") << lemma->name() << "\" does not have any pronunciation.";
	    }
	    Bliss::Lemma::PronunciationIterator p, p_end;
	    for (Core::tie(p, p_end) = lemma->pronunciations(); p != p_end; ++ p) {
		if (tokenType_ == lemmaPronunciationType) {
		    shortPauses_.insert(p->id());
		} else { // phoneType
		    const Bliss::Pronunciation &pron = *(p->pronunciation());
		    for (u32 i = 0; i < pron.length(); ++ i) {
			shortPauses_.insert(pron[i]);
		    }
		}
	    }
	} break;
	default: {
	    criticalError("unknown token type");
	} break;
	}
    }
}

/**
 * OrthographyTimeAlignmentBasedMetricLatticeBuilder
 */
OrthographyTimeAlignmentBasedMetricLatticeBuilder::OrthographyTimeAlignmentBasedMetricLatticeBuilder(
    const Core::Configuration &c, Bliss::LexiconRef lexicon)
    :
    Precursor(c, lexicon)
{
    orthToLemma_ = new Bliss::OrthographicParser(select("orthographic-parser"), lexicon);
    lemmaPronToLemma_ = lexicon->createLemmaPronunciationToLemmaTransducer();

    Fsa::ConstAutomatonRef lemmaToEval = lexicon->createLemmaToEvaluationTokenTransducer();
    lemmaToLemmaConfusion_ = Fsa::composeMatching(lemmaToEval, Fsa::invert(lemmaToEval));
    lemmaToLemmaConfusion_ = Fsa::cache(lemmaToLemmaConfusion_);

    respondToDelayedErrors();
}

OrthographyTimeAlignmentBasedMetricLatticeBuilder::~OrthographyTimeAlignmentBasedMetricLatticeBuilder()
{
    delete orthToLemma_;
}

OrthographyTimeAlignmentBasedMetricLatticeBuilder::Functor
OrthographyTimeAlignmentBasedMetricLatticeBuilder::createFunctor(
    const std::string &id,
    const std::string &orth,
    Lattice::ConstWordLatticeRef lattice)
{
    setReference(
	Lattice::extractNumerator(
	    orth,
	    lattice,
	    orthToLemma_,
	    lemmaPronToLemma_,
	    lemmaToLemmaConfusion_));
    return Functor(*this, id, lattice);
}

/**
 * ArchiveApproximateWordAccuracyLatticeBuilder
 */
const Core::ParameterString ArchiveTimeAlignmentBasedMetricLatticeBuilder::paramName(
    "name",
    "name of reference fsa",
    Lattice::WordLattice::acousticFsa);

ArchiveTimeAlignmentBasedMetricLatticeBuilder::ArchiveTimeAlignmentBasedMetricLatticeBuilder(
    const Core::Configuration &c, Bliss::LexiconRef lexicon)
    :
    Precursor(c, lexicon),
    name_(paramName(config))
{
    numeratorArchiveReader_ = Lattice::Archive::openForReading(
	select("numerator-lattice-archive"), lexicon);
    if (!numeratorArchiveReader_ || numeratorArchiveReader_->hasFatalErrors()) {
	delete numeratorArchiveReader_; numeratorArchiveReader_ = 0;
	error("failed to open numerator lattice archive");
    }

    respondToDelayedErrors();
}

ArchiveTimeAlignmentBasedMetricLatticeBuilder::~ArchiveTimeAlignmentBasedMetricLatticeBuilder()
{
    delete numeratorArchiveReader_;
}

ArchiveTimeAlignmentBasedMetricLatticeBuilder::Functor
ArchiveTimeAlignmentBasedMetricLatticeBuilder::createFunctor(
    const std::string &id,
    const std::string &segmentId,
    Lattice::ConstWordLatticeRef lattice)
{
    verify(numeratorArchiveReader_);
    setReference(numeratorArchiveReader_->get(segmentId, name()));
    return Functor(*this, id, lattice);
}

/**
 * OrthographyApproximateWordAccuracyLatticeBuilder
 */
OrthographyApproximateWordAccuracyLatticeBuilder::OrthographyApproximateWordAccuracyLatticeBuilder(
    const Core::Configuration &c, Core::Ref<const Bliss::Lexicon> lexicon)
    :
    Precursor(c, lexicon)
{
    tokenType_ = (TokenType)paramTokenType(config);
    if (tokenType_ != lemmaPronunciationType && tokenType_ != lemmaType) {
	criticalError("Invalid token type");
    }
    initializeShortPauses(lexicon);
}

Fsa::ConstAutomatonRef OrthographyApproximateWordAccuracyLatticeBuilder::build(
    Lattice::ConstWordLatticeRef lattice)
{
    if (reference_) {
	Lattice::ConstWordLatticeRef result =
	    Lattice::getApproximateWordAccuracy(
		lattice, reference_, shortPauses_, tokenType_ == lemmaType);
	return result->mainPart();
    } else {
	warning("Approximate word accuracies cannot be calculated because reference is empty.");
	return Fsa::ConstAutomatonRef();
    }
}

/**
 * ArchiveApproximateWordAccuracyLatticeBuilder
 */
ArchiveApproximateWordAccuracyLatticeBuilder::ArchiveApproximateWordAccuracyLatticeBuilder(
    const Core::Configuration &c, Core::Ref<const Bliss::Lexicon> lexicon)
    :
    Precursor(c, lexicon)
{
    tokenType_ = (TokenType)paramTokenType(config);
    if (tokenType_ != lemmaPronunciationType && tokenType_ != lemmaType) {
	criticalError("Invalid token type");
    }
    initializeShortPauses(lexicon);
}

Fsa::ConstAutomatonRef ArchiveApproximateWordAccuracyLatticeBuilder::build(
    Lattice::ConstWordLatticeRef lattice)
{
    if (reference_) {
	Lattice::ConstWordLatticeRef result =
	    Lattice::getApproximateWordAccuracy(
		lattice, reference_, shortPauses_, tokenType_ == lemmaType);
	return result->mainPart();
    } else {
	warning("Approximate word accuracies cannot be calculated because reference is empty.");
	return Fsa::ConstAutomatonRef();
    }
}

/**
 * OrthographyApproximatePhoneAccuracyLatticeBuilder
 */
OrthographyApproximatePhoneAccuracyLatticeBuilder::OrthographyApproximatePhoneAccuracyLatticeBuilder(
    const Core::Configuration &c, Core::Ref<const Bliss::Lexicon> lexicon)
    :
    Precursor(c, lexicon)
{
    tokenType_ = phoneType;
    initializeShortPauses(lexicon);
}

OrthographyApproximatePhoneAccuracyLatticeBuilder::Functor
OrthographyApproximatePhoneAccuracyLatticeBuilder::createFunctor(
    const std::string &id,
    const std::string &orth,
    Lattice::ConstWordLatticeRef lattice,
    Core::Ref<PhonemeSequenceAlignmentGenerator> alignmentGenerator)
{
    alignmentGenerator_ = alignmentGenerator;
    return Precursor::createFunctor(id, orth, lattice);
}

Fsa::ConstAutomatonRef OrthographyApproximatePhoneAccuracyLatticeBuilder::build(
    Lattice::ConstWordLatticeRef lattice)
{
    if (reference_) {
	Lattice::ConstWordLatticeRef result =
	    Lattice::getApproximatePhoneAccuracy(
		lattice, reference_, shortPauses_, alignmentGenerator_);
	return result->mainPart();
    } else {
	warning("Approximate phone accuracies cannot be calculated because reference is empty.");
	return Fsa::ConstAutomatonRef();
    }
}

/**
 * ArchiveApproximatePhoneAccuracyLatticeBuilder
 */
ArchiveApproximatePhoneAccuracyLatticeBuilder::ArchiveApproximatePhoneAccuracyLatticeBuilder(
    const Core::Configuration &c, Core::Ref<const Bliss::Lexicon> lexicon)
    :
    Precursor(c, lexicon)
{
    tokenType_ = phoneType;
    initializeShortPauses(lexicon);
}

ArchiveApproximatePhoneAccuracyLatticeBuilder::Functor
ArchiveApproximatePhoneAccuracyLatticeBuilder::createFunctor(
    const std::string &id,
    const std::string &segmentId,
    Lattice::ConstWordLatticeRef lattice,
    Core::Ref<PhonemeSequenceAlignmentGenerator> alignmentGenerator)
{
    alignmentGenerator_ = alignmentGenerator;
    return Precursor::createFunctor(id, segmentId, lattice);
}

Fsa::ConstAutomatonRef ArchiveApproximatePhoneAccuracyLatticeBuilder::build(
    Lattice::ConstWordLatticeRef lattice)
{
    if (reference_) {
	Lattice::ConstWordLatticeRef result =
	    Lattice::getApproximatePhoneAccuracy(
		lattice, reference_, shortPauses_, alignmentGenerator_);
	return result->mainPart();
    } else {
	warning("Approximate phone accuracies cannot be calculated because reference is empty.");
	return Fsa::ConstAutomatonRef();
    }
}


/**
 * ApproximatePhoneAccuracyLatticeBuilder
 */
ApproximatePhoneAccuracyLatticeBuilder::ApproximatePhoneAccuracyLatticeBuilder(
    const Core::Configuration &c, Bliss::LexiconRef lexicon)
    :
    Precursor(c, lexicon)
{
    tokenType_ = phoneType;
    initializeShortPauses(lexicon);
}

ApproximatePhoneAccuracyLatticeBuilder::Functor
ApproximatePhoneAccuracyLatticeBuilder::createFunctor(
    const std::string &id,
    Lattice::ConstWordLatticeRef reference,
    Lattice::ConstWordLatticeRef lattice,
    AlignmentGeneratorRef alignmentGenerator)
{
    setReference(reference);
    alignmentGenerator_ = alignmentGenerator;
    return Functor(*this, id, lattice);
}

Fsa::ConstAutomatonRef ApproximatePhoneAccuracyLatticeBuilder::build(
    Lattice::ConstWordLatticeRef lattice)
{
    if (reference_) {
	Lattice::ConstWordLatticeRef result =
	    Lattice::getApproximatePhoneAccuracy(
		lattice, reference_, shortPauses_, alignmentGenerator_);
	return result->mainPart();
    } else {
	warning("Approximate phone accuracies cannot be calculated because reference is empty.");
	return Fsa::ConstAutomatonRef();
    }
}

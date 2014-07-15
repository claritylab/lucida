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
#include "LatticeExtractor.hh"
#include "LatticeExtractorAutomaton.hh"
#include "ModelCombination.hh"
#include <Lattice/Archive.hh>
#include <Lattice/Basic.hh>
#include <Lattice/Lattice.hh>
#include <Fsa/Arithmetic.hh>
#include <Fsa/Best.hh>
#include <Fsa/Cache.hh>
#include <Fsa/Compose.hh>
#include <Fsa/Determinize.hh>
#include <Fsa/Dfs.hh>
#include <Fsa/Minimize.hh>
#include <Fsa/Project.hh>
#include <Fsa/Rational.hh>
#include <Fsa/RemoveEpsilons.hh>
#include <Core/Vector.hh>
#include <Lattice/Static.hh>
#include "DataExtractor.hh"
#include "Alignment.hh"
#include "AllophoneStateGraphBuilder.hh"
#include <Am/Utilities.hh>
#include <Core/Hash.hh>
#include "AccuracyFsaBuilder.hh"
#include <Bliss/Orthography.hh>
#include <Am/Module.hh>
#include "Module.hh"

using namespace Speech;
using namespace LatticeExtratorInternal;

/*
 * LatticeExtractor
 */
const Core::ParameterString LatticeExtractor::paramLevel(
    "level", "level of lattice-states (word/state)", "word");

LatticeExtractor::LatticeExtractor(const Core::Configuration &c) :
    Core::Component(c),
    level_(paramLevel(c))
{}

Lattice::ConstWordLatticeRef LatticeExtractor::extract(
    Lattice::ConstWordLatticeRef lattice, Bliss::SpeechSegment *)
{
    Lattice::WordLattice *result = new Lattice::WordLattice;
    result->setWordBoundaries(lattice->wordBoundaries());
    result->setFsa(lattice->part(name()), name());
    return Lattice::ConstWordLatticeRef(result);
}

/*
 * LatticeRescorer: base class
 */
Lattice::ConstWordLatticeRef LatticeRescorer::extract(
    Lattice::ConstWordLatticeRef lattice,
    Bliss::SpeechSegment *segment)
{
    Lattice::ConstWordLatticeRef rescored = work(lattice, segment);
    if (!rescored) {
	return Lattice::ConstWordLatticeRef();
    }
    if (rescored->mainPart()->hasProperty(Fsa::PropertyStorage) and
	rescored->mainPart()->hasProperty(Fsa::PropertyCached)) {
	return rescored;
    } else {
	/**
	 * Static copy (efficiency, and otherwise: automata produce correct
	 * results only if traversed with dfs).
	 */
	return Lattice::staticCopy(rescored, name());
    }
}



/*
 * LatticeRescorer
 */
LatticeRescorer::LatticeRescorer(const Core::Configuration &c) :
    Precursor(c)
{}

/*
 * LatticeRescorer: acoustic base class
 */
AcousticLatticeRescorerBase::AcousticLatticeRescorerBase(const Core::Configuration &c) :
    Precursor(c)
{}

/*
 * LatticeRescorer: acoustic base class for exact match rescoring
 */
AcousticLatticeRescorer::AcousticLatticeRescorer(const Core::Configuration &c) :
    Precursor(c)
{}

Lattice::ConstWordLatticeRef AcousticLatticeRescorer::work(
    Lattice::ConstWordLatticeRef lattice,
    Bliss::SpeechSegment *segment)
{
    alignmentGenerator_->setSpeechSegment(segment);
    Lattice::WordLattice *result = new Lattice::WordLattice;
    result->setWordBoundaries(lattice->wordBoundaries());
    result->setFsa(lattice->part(name()), name());
    return Lattice::ConstWordLatticeRef(result);
}

void AcousticLatticeRescorer::signOn(Speech::CorpusVisitor &corpusVisitor)
{
    if (acousticModel_) {
	acousticModel_->signOn(corpusVisitor);
    }
}



/*
 * AlignmentLatticeRescorerAutomaton
 */
class AlignmentLatticeRescorerAutomaton : public CachedLatticeRescorerAutomaton
{
    typedef CachedLatticeRescorerAutomaton Precursor;
    typedef Core::Ref<PhonemeSequenceAlignmentGenerator> AlignmentGeneratorRef;
private:
    AlignmentGeneratorRef alignmentGenerator_;
protected:
    virtual Fsa::Weight score(Fsa::StateId s, const Fsa::Arc &a) const;
public:
    AlignmentLatticeRescorerAutomaton(Lattice::ConstWordLatticeRef,
				      AlignmentGeneratorRef);
    virtual ~AlignmentLatticeRescorerAutomaton() {}
    virtual std::string describe() const {
	return Core::form("acoustic-rescore(%s)", fsa_->describe().c_str());
    }
    Fsa::Weight _score(const Bliss::Coarticulated<Bliss::LemmaPronunciation> &,
		       TimeframeIndex begtime, TimeframeIndex endtime) const;
};

AlignmentLatticeRescorerAutomaton::AlignmentLatticeRescorerAutomaton(
    Lattice::ConstWordLatticeRef lattice,
    AlignmentGeneratorRef alignmentGenerator)
    :
    Precursor(lattice),
    alignmentGenerator_(alignmentGenerator)
{
    require(alignmentGenerator_);
}

Fsa::Weight AlignmentLatticeRescorerAutomaton::score(Fsa::StateId s, const Fsa::Arc &a) const
{
    const Bliss::LemmaPronunciationAlphabet *alphabet =
	required_cast(const Bliss::LemmaPronunciationAlphabet*, fsa_->getInputAlphabet().get());
    const Bliss::LemmaPronunciation *pronunciation = alphabet->lemmaPronunciation(a.input());
    const TimeframeIndex begtime = wordBoundaries_->time(s);
    if (pronunciation && begtime != InvalidTimeframeIndex) {
	// fsa_->getState(a.target())->id(): guarantee that wordBoundary at a.target() exists
	Bliss::Coarticulated<Bliss::LemmaPronunciation> coarticulatedPronunciation(
	    *pronunciation, wordBoundaries_->transit(s).final,
	    wordBoundaries_->transit(fsa_->getState(a.target())->id()).initial);
	const TimeframeIndex endtime = wordBoundaries_->time(fsa_->getState(a.target())->id());
	return _score(coarticulatedPronunciation, begtime, endtime);
    } else {
	return fsa_->semiring()->one();
    }
}

Fsa::Weight AlignmentLatticeRescorerAutomaton::_score(
    const Bliss::Coarticulated<Bliss::LemmaPronunciation> &coarticulatedPronunciation,
    TimeframeIndex begtime, TimeframeIndex endtime) const
{
    if (begtime < endtime) {
	return Fsa::Weight(alignmentGenerator_->alignmentScore(coarticulatedPronunciation, begtime, endtime));
    } else {
	Core::Application::us()->warning("score 0 assigned to arc with begin time ")
	    << begtime << " , end time " << endtime <<
	    " and label id " << coarticulatedPronunciation.object().id();
	return fsa_->semiring()->one();
    }
}

/*
 * LatticeRescorer: "alignment model"
 */
AlignmentLatticeRescorer::AlignmentLatticeRescorer(
    const Core::Configuration &c)
    :
    Precursor(c)
{}

Lattice::ConstWordLatticeRef AlignmentLatticeRescorer::work(
    Lattice::ConstWordLatticeRef lattice, Bliss::SpeechSegment *segment)
{
    alignmentGenerator_->setSpeechSegment(segment);
    AlignmentLatticeRescorerAutomaton *f =
	new AlignmentLatticeRescorerAutomaton(lattice, alignmentGenerator_);
    Lattice::WordLattice *result = new Lattice::WordLattice;
    result->setWordBoundaries(lattice->wordBoundaries());
    result->setFsa(Fsa::ConstAutomatonRef(f), Lattice::WordLattice::acousticFsa);
    return Lattice::ConstWordLatticeRef(result);
}



/*
 * LmLatticeRescorerAutomaton
 */
class LmLatticeRescorerAutomaton : public LatticeRescorerAutomaton
{
    typedef Core::Ref<const Lm::ScaledLanguageModel> ConstLanguageModelRef;
    typedef Lm::History History;
    typedef Core::Vector<History> Histories;
private:
    ConstLanguageModelRef languageModel_;
    mutable Histories histories_;
    const Bliss::LemmaPronunciationAlphabet *alphabet_;
    f32 pronunciationScale_;
private:
    virtual Fsa::Weight score(Fsa::StateId s, const Fsa::Arc &a) const;
public:
    LmLatticeRescorerAutomaton(Lattice::ConstWordLatticeRef,
			       Core::Ref<const Lm::ScaledLanguageModel>,
			       f32 pronunciationScale = 0);
    virtual ~LmLatticeRescorerAutomaton() {}

    virtual std::string describe() const {
	return Core::form("lm-rescore(%s)", fsa_->describe().c_str());
    }
};

LmLatticeRescorerAutomaton::LmLatticeRescorerAutomaton(
    Lattice::ConstWordLatticeRef lattice,
    Core::Ref<const Lm::ScaledLanguageModel> languageModel,
    f32 pronunciationScale)
    :
    LatticeRescorerAutomaton(lattice),
    languageModel_(languageModel),
    alphabet_(required_cast(const Bliss::LemmaPronunciationAlphabet*, fsa_->getInputAlphabet().get())),
    pronunciationScale_(pronunciationScale)
{}

Fsa::Weight LmLatticeRescorerAutomaton::score(Fsa::StateId s, const Fsa::Arc &a) const
{
    if (s == fsa_->initialStateId()) {
	histories_.grow(s);
	histories_[s] = languageModel_->startHistory();
    }
    require(histories_[s].isValid());
    Lm::History hist = histories_[s];
    Lm::Score score = 0;
    const Bliss::LemmaPronunciation *lp = alphabet_->lemmaPronunciation(a.input());
    if (lp) {
	Lm::addLemmaPronunciationScore(
	    languageModel_, lp, pronunciationScale_, languageModel_->scale(), hist, score);
    }
    /*! \warning sentence end score has to be added manually */
    if (fsa_->getState(a.target())->isFinal()) {
	score += languageModel_->sentenceEndScore(hist);
	hist = languageModel_->startHistory();
    }

    histories_.grow(a.target());
    if (!histories_[a.target()].isValid()) {
	histories_[a.target()] = hist;
    }
    if (!(hist == histories_[a.target()])) {
	languageModel_->error() <<
	    "Mismatch between lattice and language model: "                  \
	    "ambiguous history at state '" << a.target() << "' ('" <<
	    languageModel_->formatHistory(hist) << "' vs. '" <<
	    languageModel_->formatHistory(histories_[a.target()]) << "').\n" \
	    "Possible causes: 1) lattice is time-conditioned,\n"	     \
	    "2) lattice has been generated by using another language model.";
    }
    return Fsa::Weight(std::isinf(score) ? Core::Type<f32>::max : score);
}

/*
 * LatticeRescorer: lm
 */
LmLatticeRescorer::LmLatticeRescorer(
    const Core::Configuration &c, bool initialize)
    :
    Precursor(c)
{
    if (initialize) {
	ModelCombination modelCombination(select("model-combination"),
					  ModelCombination::useLanguageModel);
	modelCombination.load();
	languageModel_ = Core::Ref<const Lm::ScaledLanguageModel>(
	    modelCombination.languageModel().get());
    }
}

Lattice::ConstWordLatticeRef LmLatticeRescorer::work(
    Lattice::ConstWordLatticeRef lattice, Bliss::SpeechSegment *)
{
    LmLatticeRescorerAutomaton *f =
	new LmLatticeRescorerAutomaton(lattice, languageModel_);
    Lattice::WordLattice *result = new Lattice::WordLattice;
    result->setWordBoundaries(f->wordBoundaries());
    result->setFsa(Fsa::ConstAutomatonRef(f), Lattice::WordLattice::acousticFsa);
    return Lattice::ConstWordLatticeRef(result);
}

/*
 * LatticeRescorer: combined lm = lm + pronunciation
 */
CombinedLmLatticeRescorer::CombinedLmLatticeRescorer(
    const Core::Configuration &c)
    :
    Precursor(c, false)
{
    ModelCombination modelCombination(
	select("model-combination"), ModelCombination::useLanguageModel);
    modelCombination.load();
    languageModel_ = Core::Ref<const Lm::ScaledLanguageModel>(
	modelCombination.languageModel().get());
    pronunciationScale_ = modelCombination.pronunciationScale();
}

Lattice::ConstWordLatticeRef CombinedLmLatticeRescorer::work(
    Lattice::ConstWordLatticeRef lattice, Bliss::SpeechSegment *)
{
    Fsa::ConstAutomatonRef lm = Fsa::ConstAutomatonRef(
	new LmLatticeRescorerAutomaton(
	    lattice, languageModel_, pronunciationScale_));
    Lattice::WordLattice *result = new Lattice::WordLattice;
    result->setWordBoundaries(lattice->wordBoundaries());
    result->setFsa(lm, Lattice::WordLattice::acousticFsa);
    return Lattice::ConstWordLatticeRef(result);
}

/*
 * LatticeReader
 */
const Core::ParameterString LatticeReader::paramFsaPrefix(
    "fsa-prefix",
    "prefix of automaton in archive");

LatticeReader::LatticeReader(
    const Core::Configuration &c, Bliss::LexiconRef lexicon) :
    Precursor(c),
    archiveReader_(0)
{
    fsaPrefix_ = paramFsaPrefix(c);
    if (fsaPrefix_.empty()) {
	fsaPrefix_ = Lattice::WordLattice::mainFsa;
    }

    archiveReader_ = Lattice::Archive::openForReading(
	select("lattice-archive"), lexicon);
    if (!archiveReader_ || archiveReader_->hasFatalErrors()) {
	delete archiveReader_; archiveReader_ = 0;
	error("failed to open lattice archive");
    }
}

Lattice::ConstWordLatticeRef LatticeReader::extract(
    Lattice::ConstWordLatticeRef lattice, Bliss::SpeechSegment *segment)
{
    verify(archiveReader_);
    Lattice::ConstWordLatticeRef result =
	archiveReader_->get(segment->fullName(), fsaPrefix_);
    if (result && result->nParts() == 1) {
	if (result->wordBoundaries() and (*result->wordBoundaries()) != (*lattice->wordBoundaries())) {
	    error("Mismatch between the word boundaries of the given lattice and the read lattice.");
	}
    } else {
	error("Failed to retrieve lattice '%s' for segment '%s'.",
	      fsaPrefix_.c_str(), segment->fullName().c_str());
	result.reset();
    }
    return result;
}



/*
 * LatticeRescorer: distance
 */
Core::Choice DistanceLatticeRescorer::choiceDistanceType(
    "approximate-word-accuracy", approximateWordAccuracy,
    "approximate-phone-accuracy", approximatePhoneAccuracy,
    "approximate-phone-accuracy-mask", approximatePhoneAccuracyMask,
    "levenshtein-on-list", levenshteinOnList,
    "word-accuracy", wordAccuracy,
    "phoneme-accuracy", phonemeAccuracy,
    "frame-word-accuracy", frameWordAccuracy,
    "frame-phone-accuracy", framePhoneAccuracy,
    "frame-state-accuracy", frameStateAccuracy,
    "smoothed-frame-state-accuracy", smoothedFrameStateAccuracy,
    "soft-frame-phone-accuracy", softFramePhoneAccuracy,
    Core::Choice::endMark());

Core::ParameterChoice DistanceLatticeRescorer::paramDistanceType(
    "distance-type",
    &choiceDistanceType,
    "distance measure to apply",
    approximateWordAccuracy);

Core::Choice DistanceLatticeRescorer::choiceSpokenSource(
    "orthography", orthography,
    "archive", archive,
    Core::Choice::endMark());

Core::ParameterChoice DistanceLatticeRescorer::paramSpokenSource(
    "spoken-source",
    &choiceSpokenSource,
    "where to get the correct hypotheses from",
    orthography);

DistanceLatticeRescorer::DistanceLatticeRescorer(
    const Core::Configuration &c) :
    Precursor(c)
{}

LatticeRescorer* DistanceLatticeRescorer::createDistanceLatticeRescorer(
    const Core::Configuration &config, Bliss::LexiconRef lexicon)
{
    /*! @todo: move ? */
    return Speech::Module::instance().createDistanceLatticeRescorer(
	config, lexicon);
}

/*
 * ApproximateDistanceLatticeRescorer: approximate
 */
ApproximateDistanceLatticeRescorer::ApproximateDistanceLatticeRescorer(
    const Core::Configuration &c, Bliss::LexiconRef lexicon)
    :
    Precursor(c)
{}

Lattice::ConstWordLatticeRef ApproximateDistanceLatticeRescorer::work(
    Lattice::ConstWordLatticeRef lattice, Bliss::SpeechSegment *segment)
{
    Fsa::ConstAutomatonRef fsa = getDistanceFsa(lattice, segment);
    if (fsa) {
	Core::Ref<Lattice::WordLattice> result(new Lattice::WordLattice);
	result->setWordBoundaries(lattice->wordBoundaries());
	result->setFsa(fsa, Lattice::WordLattice::acousticFsa);
	return Lattice::ConstWordLatticeRef(result);
    }
    return Lattice::ConstWordLatticeRef();
}

/*
 * DistanceLatticeRescorer: approximate word accuracy
 */
ArchiveApproximateWordAccuracyLatticeRescorer::ArchiveApproximateWordAccuracyLatticeRescorer(
    const Core::Configuration &c, Bliss::LexiconRef lexicon)
    :
    Precursor(c, lexicon),
    builder_(0)
{
    builder_ = new ArchiveApproximateWordAccuracyLatticeBuilder(
	select("approximate-word-accuracy-lattice-builder"), lexicon);
}

ArchiveApproximateWordAccuracyLatticeRescorer::~ArchiveApproximateWordAccuracyLatticeRescorer()
{
    delete builder_;
}

Fsa::ConstAutomatonRef ArchiveApproximateWordAccuracyLatticeRescorer::getDistanceFsa(
    Lattice::ConstWordLatticeRef lattice, Bliss::SpeechSegment *segment)
{
    verify(builder_);
    return builder_->createFunctor(
	segment->fullName(),
	segment->fullName(),
	lattice).build();
}

OrthographyApproximateWordAccuracyLatticeRescorer::OrthographyApproximateWordAccuracyLatticeRescorer(
    const Core::Configuration &c, Bliss::LexiconRef lexicon)
    :
    Precursor(c, lexicon),
    builder_(0)
{
    builder_ = new OrthographyApproximateWordAccuracyLatticeBuilder(
	select("approximate-word-accuracy-lattice-builder"), lexicon);
}

OrthographyApproximateWordAccuracyLatticeRescorer::~OrthographyApproximateWordAccuracyLatticeRescorer()
{
    delete builder_;
}

Fsa::ConstAutomatonRef OrthographyApproximateWordAccuracyLatticeRescorer::getDistanceFsa(
    Lattice::ConstWordLatticeRef lattice, Bliss::SpeechSegment *segment)
{
    verify(builder_);
    return builder_->createFunctor(
	segment->fullName(),
	segment->orth(),
	lattice).build();
}

/*
 * DistanceLatticeRescorer: approximate phone accuracy
 */
ApproximatePhoneAccuracyLatticeRescorer::ApproximatePhoneAccuracyLatticeRescorer(
    const Core::Configuration &c, Bliss::LexiconRef lexicon)
    :
    Precursor(c, lexicon)
{}

ApproximatePhoneAccuracyLatticeRescorer::~ApproximatePhoneAccuracyLatticeRescorer()
{}

ArchiveApproximatePhoneAccuracyLatticeRescorer::ArchiveApproximatePhoneAccuracyLatticeRescorer(
    const Core::Configuration &c, Bliss::LexiconRef lexicon)
    :
    Precursor(c, lexicon),
    builder_(0)
{
    builder_ = new ArchiveApproximatePhoneAccuracyLatticeBuilder(
	select("approximate-phone-accuracy-lattice-builder"), lexicon);
}

ArchiveApproximatePhoneAccuracyLatticeRescorer::~ArchiveApproximatePhoneAccuracyLatticeRescorer()
{
    delete builder_;
}

Fsa::ConstAutomatonRef ArchiveApproximatePhoneAccuracyLatticeRescorer::getDistanceFsa(
    Lattice::ConstWordLatticeRef lattice, Bliss::SpeechSegment *segment)
{
    verify(builder_);
    alignmentGenerator_->setSpeechSegment(segment);
    return builder_->createFunctor(
	segment->fullName(),
	segment->fullName(),
	lattice,
	alignmentGenerator_).build();
}

OrthographyApproximatePhoneAccuracyLatticeRescorer::OrthographyApproximatePhoneAccuracyLatticeRescorer(
    const Core::Configuration &c, Bliss::LexiconRef lexicon)
    :
    Precursor(c, lexicon),
    builder_(0)
{
    builder_ = new OrthographyApproximatePhoneAccuracyLatticeBuilder(
	select("approximate-phone-accuracy-lattice-builder"), lexicon);
}

OrthographyApproximatePhoneAccuracyLatticeRescorer::~OrthographyApproximatePhoneAccuracyLatticeRescorer()
{
    delete builder_;
}

Fsa::ConstAutomatonRef OrthographyApproximatePhoneAccuracyLatticeRescorer::getDistanceFsa(
    Lattice::ConstWordLatticeRef lattice, Bliss::SpeechSegment *segment)
{
    verify(builder_);
    alignmentGenerator_->setSpeechSegment(segment);
    return builder_->createFunctor(
	segment->fullName(),
	segment->orth(),
	lattice,
	alignmentGenerator_).build();
}

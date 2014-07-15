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
#include "ModelCombination.hh"
#include "Recognizer.hh"
#include <Search/LatticeHandler.hh>
#include <Search/Module.hh>
#include <Speech/Module.hh>
#include <Bliss/CorpusDescription.hh>
#include <Bliss/Evaluation.hh>
#include <Bliss/Orthography.hh>
#include <Core/Archive.hh>
#include <Core/Types.hh>
#include <Fsa/Arithmetic.hh>
#include <Fsa/Basic.hh>
#include <Fsa/Cache.hh>
#include <Fsa/Compose.hh>
#include <Fsa/Determinize.hh>
#include <Fsa/Minimize.hh>
#include <Fsa/Project.hh>
#include <Fsa/RemoveEpsilons.hh>
#include <Lm/LanguageModel.hh>
#include <Lm/Module.hh>
#include <Lattice/LatticeAdaptor.hh>
#include <Mm/Types.hh>

using namespace Speech;

// ===========================================================================
// class Recognizer

const Core::Choice Recognizer::searchTypeChoice_(
    "word-conditioned-tree-search",          Search::WordConditionedTreeSearchType,
    "advanced-tree-search",                  Search::AdvancedTreeSearch,
    "teaching-word-conditioned-tree-search", Search::TeachingWordConditionedTreeSearchType,
    "fsa-search",                            Search::FsaSearchType,
    "expanding-fsa-search",                  Search::ExpandingFsaSearchType,
    "linear-search",                         Search::LinearSearchType,
    "linear-signlanguage-search",            Search::LinearSignLanguageSearchType,
    "teaching-linear-search",                Search::TeachingLinearSearchType,
    Core::Choice::endMark());

const Core::ParameterChoice Recognizer::paramSearch(
    "search-type", &searchTypeChoice_,
    "select search algorithm",
    Search::WordConditionedTreeSearchType);

Recognizer::Recognizer(const Core::Configuration &c) :
    Core::Component(c), recognizer_(0)
{}

Recognizer::~Recognizer()
{
    delete recognizer_; recognizer_ = 0;
}

void Recognizer::createRecognizer()
{
    delete recognizer_; recognizer_ = 0;
    recognizer_ = Search::Module::instance().createRecognizer(static_cast<Search::SearchType>(paramSearch(config)),
							      select("recognizer"));
}

void Recognizer::initializeRecognizer(Am::AcousticModel::Mode acousticModelMode)
{
    createRecognizer();
    ModelCombination modelCombination(
	select("model-combination"), recognizer_->modelCombinationNeeded(), acousticModelMode);
    modelCombination.load();
    recognizer_->setModelCombination(modelCombination);
    recognizer_->init();
    lexicon_ = modelCombination.lexicon();
    acousticModel_ = modelCombination.acousticModel();
}

void Recognizer::initializeRecognizer(const Speech::ModelCombination &modelCombination)
{
    createRecognizer();
    recognizer_->setModelCombination(modelCombination);
    recognizer_->init();
    lexicon_ = modelCombination.lexicon();
    acousticModel_ = modelCombination.acousticModel();
}

// ===========================================================================
// class OfflineRecognizer

const Core::ParameterBool OfflineRecognizer::paramStoreLattices(
    "store-lattices",
    "store word lattices in archive",
    false);
const Core::ParameterBool OfflineRecognizer::paramStoreTracebacks(
    "store-tracebacks",
    "store recognition tracebacks in archive",
    false);
const Core::ParameterBool  OfflineRecognizer::paramTimeConditionedLattice(
    "time-conditioned-lattice",
    "produce time-conditioned lattice (instead of LM conditioned lattice)",
    false);
const Core::ParameterString OfflineRecognizer::paramLayerName(
    "layer-name",
    "name to distinguish results of differently parameterized passes over same corpus",
    "",
    "Analog tool keeps the results of different layers apart");
const Core::ParameterFloat OfflineRecognizer::paramPartialResultInterval(
    "partial-result-interval",
    "time between attempt to produce partial recognition results (in seconds)",
    Core::Type<f32>::max, 0.0);
const Core::ParameterBool OfflineRecognizer::paramEvaluteResult(
    "evaluate-result",
    "evaluate recognition results",
    true);
const Core::ParameterBool OfflineRecognizer::paramNoDependencyCheck(
	"no-dependency-check",
	"do not check any dependencies",
	false
	);

OfflineRecognizer::OfflineRecognizer(const Core::Configuration &c,
				     Am::AcousticModel::Mode acousticModelMode) :
    Core::Component(c),
    Precursor(c),
    Recognizer(c),
    partialResultInterval_(Core::Type<f32>::max),
    lastPartialResult_(0),
    shouldEvaluateResult_(true),
    shouldStoreLattice_(false),
    latticeHandler_(0),
    tracebackArchiveWriter_(0),
    tracebackChannel_(config, "traceback"),
	noDependencyCheck_(paramNoDependencyCheck(c))
{
    initializeRecognizer(acousticModelMode);
    partialResultInterval_ = paramPartialResultInterval(config);
    shouldEvaluateResult_ = paramEvaluteResult(config);
    shouldStoreLattice_ = paramStoreLattices(config);
    latticeHandler_ = Search::Module::instance().createLatticeHandler(select("lattice-archive"));
    latticeHandler_->setLexicon(lexicon_);
    timeConditionedLattice_ = paramTimeConditionedLattice(config);
    if (paramStoreTracebacks(config)) {
	log("opening traceback archive");
	tracebackArchiveWriter_ = Lattice::Archive::openForWriting(select("traceback-archive"), lexicon_);
	if (tracebackArchiveWriter_->hasFatalErrors()) {
	    delete tracebackArchiveWriter_; tracebackArchiveWriter_ = 0;
	}
    }
    evaluator_ = new Bliss::Evaluator(select("evaluation"), lexicon_);
    layerName_ = paramLayerName(c);
}

OfflineRecognizer::~OfflineRecognizer() {
    delete latticeHandler_;
    delete tracebackArchiveWriter_;
    delete evaluator_;
}

void OfflineRecognizer::signOn(CorpusVisitor &corpusVisitor)
{
    // @todo: create a virtual initialization method and check
    // recognizer there. cannot use this in the constructor as it is used
    // by all derived classes.
    if (recognizer_->lookAheadLength() > 0) {
	error("cannot use a recognizer with acoustic look-ahead.  "
	      "use DelayedRecognizer instead (recognition-mode=delayed)");
    }

    Precursor::signOn(corpusVisitor);
    acousticModel_->signOn(corpusVisitor);
}

void OfflineRecognizer::enterSpeechSegment(Bliss::SpeechSegment *s) {
    recognizer_->resetStatistics();
    recognizer_->setSegment(s->fullName());
    recognizer_->restart();

    Precursor::enterSpeechSegment(s);
    Core::XmlWriter &os(clog());
    if (!layerName_.empty())
	os << Core::XmlOpen("layer") + Core::XmlAttribute("name", layerName_);
    if (s->orth().size()) {
	os << Core::XmlOpen("orth") + Core::XmlAttribute("source", "reference")
	       << s->orth()
	       << Core::XmlClose("orth");
    }
    traceback_.clear();
    acousticModel_->featureScorer()->reset();
}

void OfflineRecognizer::processResultAndLogStatistics(Bliss::SpeechSegment *s) {
    processResult(s);
    recognizer_->logStatistics();
}

void OfflineRecognizer::leaveSegment(Bliss::Segment* s) {
    Precursor::leaveSegment(s);
}

void OfflineRecognizer::leaveSpeechSegment(Bliss::SpeechSegment *s) {
    Core::Ref<const Mm::ScaledFeatureScorer> scorer = acousticModel_->featureScorer();
    if (scorer->isBuffered()) {
	while (!scorer->bufferEmpty()) {
	    recognizer_->feed(scorer->flush());
	}
    }
    finishSegment(s);
}

void OfflineRecognizer::finishSegment(Bliss::SpeechSegment *segment)
{
    processResult(segment);
    recognizer_->logStatistics();
    if (!layerName_.empty())
	clog() << Core::XmlClose("layer");
    Precursor::leaveSpeechSegment(segment);
}

void OfflineRecognizer::processResult(Bliss::SpeechSegment *s) {
    Traceback remainingTraceback;
    recognizer_->getCurrentBestSentence(remainingTraceback);
    addPartialToTraceback(remainingTraceback);

    Core::XmlWriter &os(clog());
    os << Core::XmlOpen("traceback");
    traceback_.write(os, lexicon_->phonemeInventory());
    os << Core::XmlClose("traceback");
    os << Core::XmlOpen("orth") + Core::XmlAttribute("source", "recognized");
    for (u32 i = 0; i < traceback_.size(); ++i)
	if (traceback_[i].pronunciation)
	    os << traceback_[i].pronunciation->lemma()->preferredOrthographicForm()
	       << Core::XmlBlank();
    os << Core::XmlClose("orth");
    if (tracebackChannel_.isOpen()) {
	logTraceback(traceback_);
	featureTimes_.clear();
    }

    /*! \todo Should stop timer here: evaluation may take some time we don't want to count it. */

    Core::Ref<const Search::LatticeAdaptor> lattice = recognizer_->getCurrentWordLattice();
    if (lattice && !lattice->empty()) {
	if (timeConditionedLattice_) {
	    lattice = Core::ref(new Lattice::WordLatticeAdaptor(
		    Lattice::timeConditionedWordLattice(lattice->wordLattice(latticeHandler_))));
	}
	if (shouldStoreLattice_) {
	    if (!lattice->write(s->fullName(), latticeHandler_))
		error("cannot write lattice '%s'", s->fullName().c_str());
	}
    }
    if (tracebackArchiveWriter_) {
	tracebackArchiveWriter_->store(s->fullName(), traceback_.wordLattice(lexicon_));
    }


    if (shouldEvaluateResult_) {
	evaluator_->setReferenceTranscription(s->orth());
	evaluator_->evaluate(
			     traceback_.lemmaPronunciationAcceptor(lexicon_),
			     "single best");
	if (lattice && !lattice->empty()) {
	    Lattice::ConstWordLatticeRef wl = lattice->wordLattice(latticeHandler_);
	    if (wl->nParts() > 0)
		evaluator_->evaluate(wl->part(0), "lattice");
	}
    }
}

void OfflineRecognizer::addPartialToTraceback(Recognizer::Traceback &partialTraceback) {
    if(!traceback_.empty()
       && traceback_.back().time == partialTraceback.front().time)
	partialTraceback.erase(partialTraceback.begin());
    traceback_.insert(traceback_.end(), partialTraceback.begin(), partialTraceback.end());
}


void OfflineRecognizer::processFeature(Core::Ref<const Feature> f)
{
    Core::Ref<const Mm::ScaledFeatureScorer> scorer = acousticModel_->featureScorer();

    if (scorer->isBuffered() && !scorer->bufferFilled()) {
	scorer->addFeature(f);
    } else {
	recognizer_->feed(scorer->getScorer(f));
    }
    processFeatureTimestamp(f->timestamp());
}

void OfflineRecognizer::processFeatureTimestamp(const Flow::Timestamp &timestamp)
{
    if (tracebackChannel_.isOpen())
	featureTimes_.push_back(timestamp);

    if (partialResultInterval_ < Core::Type<f32>::max) {
	if (timestamp.startTime() - lastPartialResult_ > partialResultInterval_) {
	    Traceback partialTraceback;
	    recognizer_->getPartialSentence(partialTraceback);
	    addPartialToTraceback(partialTraceback);
	    lastPartialResult_ = timestamp.startTime();
	}
    }
}

void OfflineRecognizer::setFeatureDescription(const Mm::FeatureDescription &description)
{
    if (!noDependencyCheck_) {
	if (!acousticModel_->isCompatible(description))
	    acousticModel_->respondToDelayedErrors();
    }
    Precursor::setFeatureDescription(description);
}

void OfflineRecognizer::logTraceback(const Recognizer::Traceback &traceback)
{
    tracebackChannel_ << Core::XmlOpen("traceback") + Core::XmlAttribute("type", "xml");
    u32 previousIndex = traceback.begin()->time;
    Search::SearchAlgorithm::ScoreVector previousScore(0.0, 0.0);
    for (std::vector<Search::SearchAlgorithm::TracebackItem>::const_iterator tbi = traceback.begin(); tbi != traceback.end(); ++tbi) {
	if (tbi->pronunciation) {
	    tracebackChannel_ << Core::XmlOpen("item") + Core::XmlAttribute("type", "pronunciation")
			      << Core::XmlFull("orth", tbi->pronunciation->lemma()->preferredOrthographicForm())
			      << Core::XmlFull("phon", tbi->pronunciation->pronunciation()->format(lexicon_->phonemeInventory()))
			      << Core::XmlFull("score", f32(tbi->score.acoustic - previousScore.acoustic))
		+ Core::XmlAttribute("type", "acoustic")
			      << Core::XmlFull("score", f32(tbi->score.lm - previousScore.lm))
		+ Core::XmlAttribute("type", "language");
	    if(previousIndex < tbi->time)
		tracebackChannel_ << Core::XmlEmpty("samples")
		    + Core::XmlAttribute("start", f32(featureTimes_[previousIndex].startTime())) +
		    Core::XmlAttribute("end", f32(featureTimes_[tbi->time - 1].endTime()))
				  << Core::XmlEmpty("features") + Core::XmlAttribute("start", previousIndex) +
		    Core::XmlAttribute("end", tbi->time - 1);
	    tracebackChannel_ << Core::XmlClose("item");
	}
	previousScore = tbi->score;
	previousIndex = tbi->time;
    }
    tracebackChannel_ << Core::XmlClose("traceback");
}

// ===========================================================================
// class ConstrainedOfflineRecognizer
const Core::ParameterBool ConstrainedOfflineRecognizer::paramUseLanguageModel(
    "use-language-model", "factor to scale fsa scores with in case the lm is not used", true);

const Core::ParameterFloat ConstrainedOfflineRecognizer::paramScale(
    "scale", "factor to scale fsa scores with", 0.0);

const Core::ParameterString ConstrainedOfflineRecognizer::paramFsaPrefix(
    "fsa-prefix", "prefix of fsas in archive", Lattice::WordLattice::lmFsa);

ConstrainedOfflineRecognizer::ConstrainedOfflineRecognizer(
    const Core::Configuration &c,
    Am::AcousticModel::Mode acousticModelMode)
    :
    Core::Component(c),
    Precursor(c, acousticModelMode),
    latticeArchiveReader_(0),
    scale_(Fsa::Weight((f32)paramScale(config))),
    fsaPrefix_(paramFsaPrefix(config))
{
    log("opening lattice archive");
    latticeArchiveReader_ = Lattice::Archive::openForReading(select("constrained-lattice-archive"), lexicon_);
    if (latticeArchiveReader_->hasFatalErrors()) {
	delete latticeArchiveReader_; latticeArchiveReader_ = 0;
    }
    log("opening lattice archive done");

    lemmaPronunciationToLemmaTransducer_ =
	Fsa::cache(
	    Fsa::multiply(
		lexicon_->createLemmaPronunciationToLemmaTransducer(),
		Fsa::Weight(f32(0))));

    lemmaToSyntacticTokenTransducer_ =
	Fsa::cache(
	    Fsa::multiply(
		lexicon_->createLemmaToSyntacticTokenTransducer(),
		Fsa::Weight(f32(0))));

    if (paramUseLanguageModel(config)) {
	Core::Ref<Lm::ScaledLanguageModel> lm =
	    Lm::Module::instance().createScaledLanguageModel(select("lm"), lexicon_);
	lmFsa_ = lm->getFsa();
    }
}

ConstrainedOfflineRecognizer::~ConstrainedOfflineRecognizer() {
    delete latticeArchiveReader_;
}

void ConstrainedOfflineRecognizer::enterSpeechSegment(Bliss::SpeechSegment *s) {
    verify(latticeArchiveReader_);
    Fsa::ConstAutomatonRef f = latticeArchiveReader_->get(s->fullName(), fsaPrefix_)->mainPart();

    // map to syntactic tokens
    if (f->getInputAlphabet() == lemmaPronunciationToLemmaTransducer_->getInputAlphabet()) {
	f = Fsa::composeMatching(f, lemmaPronunciationToLemmaTransducer_);
    }
    require(f->getOutputAlphabet() == lemmaToSyntacticTokenTransducer_->getInputAlphabet());
    f = Fsa::projectOutput(Fsa::composeMatching(f, lemmaToSyntacticTokenTransducer_));

    // restrict search space to word sequences in the automaton f
    Fsa::ConstAutomatonRef g = Fsa::multiply(f, scale_);
    /*
     * not yet checked: is new implementation more efficient than the old minimize implementation,
     * i.e., Fsa::determinize(Fsa::transpose(Fsa::determinize(Fsa::transpose(g))))?
     */
    g = Fsa::minimize(Fsa::determinize(Fsa::removeEpsilons(g)));
    if (lmFsa_) {
	g = Fsa::composeMatching(g, lmFsa_);
    }

    verify(recognizer_);
    recognizer_->setGrammar(Fsa::staticCopy(g));

    Precursor::enterSpeechSegment(s);
}

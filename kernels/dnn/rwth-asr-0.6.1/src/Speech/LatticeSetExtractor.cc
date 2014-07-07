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
#include "LatticeSetExtractor.hh"
#include <Modules.hh>
#include <sys/time.h>
#include <Core/Utility.hh>
#include <Math/CudaDataStructure.hh>

#if 1
#include "SegmentwiseFeatureExtractor.hh"
#include "PhonemeSequenceAlignmentGenerator.hh"
#endif



using namespace Speech;

/**
 *  LatticeSetExtractor
 */
const Core::ParameterStringVector LatticeSetExtractor::paramAcousticExtractors(
    "acoustic-rescorers",
    "set of lattice extractors, type=acoustic",
    ",");


const Core::ParameterStringVector LatticeSetExtractor::paramEmissionExtractors(
    "emission-rescorers",
    "set of lattice extractors, type=emission",
    ",");

const Core::ParameterStringVector LatticeSetExtractor::paramNnEmissionExtractors(
    "nn-emission-rescorers",
    "set of lattice extractors, type=nn-emission",
    ",");

const Core::ParameterStringVector LatticeSetExtractor::paramTdpExtractors(
    "tdp-rescorers",
    "set of lattice extractors, type=tdp",
    ",");

const Core::ParameterStringVector LatticeSetExtractor::paramPronunciationExtractors(
    "pronunciation-rescorers",
    "set of lattice extractors, type=pronunciation",
    ",");

const Core::ParameterStringVector LatticeSetExtractor::paramLmExtractors(
    "lm-rescorers",
    "set of lattice extractors, type=lm",
    ",");

const Core::ParameterStringVector LatticeSetExtractor::paramCombinedLmExtractors(
    "combined-lm-rescorers",
    "set of lattice extractors, type=combined-lm",
    ",");

const Core::ParameterStringVector LatticeSetExtractor::paramRestorers(
    "restorers",
    "set of lattice extractors, type=restorer",
    ",");

const Core::ParameterStringVector LatticeSetExtractor::paramReaders(
    "readers",
    "set of lattice extractors, type=reader",
    ",");

const Core::ParameterStringVector LatticeSetExtractor::paramDistanceExtractors(
    "distance-rescorers",
    "set of lattice extractors, type=distance",
    ",");

const Core::ParameterStringVector LatticeSetExtractor::paramPosteriorExtractors(
    "posterior-rescorers",
    "set of lattice extractors, type=posterior",
    ",");

const Core::ParameterStringVector LatticeSetExtractor::paramPassExtractors(
    "pass-extractors",
    "set of lattice extractors, type=pass",
    ",");

LatticeSetExtractor::LatticeSetExtractor(const Core::Configuration &c) :
    Precursor(c)
{}

/**
 *  LatticeSetGenerator
 */
Core::Choice LatticeSetGenerator::choiceSearchType(
    "exact-match", exactMatch,
    "full-search", fullSearch,
    Core::Choice::endMark());

const Core::ParameterChoice LatticeSetGenerator::paramSearchType(
    "search-type",
    &choiceSearchType,
    "choose between exact match (word boundaries are given) and full search",
    exactMatch);

const Core::ParameterBool LatticeSetGenerator::paramShareAcousticModel(
    "share-acoustic-model",
    "if alignment generator and rescorer have the same acoustic model, they can share it",
    false);

const Core::ParameterBool LatticeSetGenerator::paramLoadAcoustics(
    "load-acoustics",
    "load acoustics (e.g. alignment generator), used for pass rescorer only",
    false);

LatticeSetGenerator::LatticeSetGenerator(const Core::Configuration &c) :
    Core::Component(c),
    Precursor(c),
    LatticeSetExtractor(c)
{}

LatticeSetGenerator::~LatticeSetGenerator()
{
    for (LatticeExtractors::iterator it = extractors_.begin();
	 it != extractors_.end(); ++ it) {
	delete *it;
    }
}

Core::Ref<SegmentwiseFeatureExtractor> LatticeSetGenerator::segmentwiseFeatureExtractor()
{
    if (!segmentwiseFeatureExtractor_) {
#if 1
	segmentwiseFeatureExtractor_ =
	    Core::ref(new SegmentwiseFeatureExtractor(select("segmentwise-feature-extraction")));
#endif
    }
    return segmentwiseFeatureExtractor_;
}

LatticeSetGenerator::AlignmentGeneratorRef LatticeSetGenerator::alignmentGenerator()
{
    if (!alignmentGenerator_) {
#if 1
	alignmentGenerator_ =
	    Core::ref(new PhonemeSequenceAlignmentGenerator(select("segmentwise-alignment")));
#endif
	alignmentGenerator_->setSegmentwiseFeatureExtractor(segmentwiseFeatureExtractor());
    }
    return alignmentGenerator_;
}

void LatticeSetGenerator::initializeExtractors()
{
    verify(extractors_.empty());

    appendAcousticRescorers();
    appendLmRescorers();
    appendCombinedLmRescorers();
    appendReaders();
    appendDistanceRescorers();
    appendPassRescorers();
    timeRescorers_.resize(extractors_.size(), 0);
}

void LatticeSetGenerator::appendAcousticRescorers()
{
    std::vector<std::string> acousticNames = paramAcousticExtractors(config);
    for (std::vector<std::string>::iterator name = acousticNames.begin();
	 name != acousticNames.end(); ++ name) {
	switch (paramSearchType(select(*name))) {
	case exactMatch: {
	    AcousticLatticeRescorer *rescorer;
	    if (paramShareAcousticModel(config)) {
		rescorer = new AlignmentLatticeRescorer(select(*name));
	    }
	    rescorer->setAlignmentGenerator(alignmentGenerator());
	    extractors_.push_back(rescorer);
	    log("\"%s\" appended (acoustic-rescorer, exact match)", name->c_str());
	} break;
	}
    }
}

void LatticeSetGenerator::appendEmissionRescorers()
{
#if 1
    criticalError("%s requires MODULE_SPEECH_DT_ADVANCED", __PRETTY_FUNCTION__);
#endif
}

void LatticeSetGenerator::appendNnEmissionRescorers()
{
#if 1
    criticalError("%s requires MODULE_NN_SEQUENCE_TRAINING", __PRETTY_FUNCTION__);
#endif
}


void LatticeSetGenerator::appendTdpRescorers()
{
#if 1
    criticalError("%s requires MODULE_SPEECH_DT_ADVANCED", __PRETTY_FUNCTION__);
#endif
}

void LatticeSetGenerator::appendPronunciationRescorers()
{
#if 1
    criticalError("%s requires MODULE_SPEECH_DT_ADVANCED", __PRETTY_FUNCTION__);
#endif
}


void LatticeSetGenerator::appendLmRescorers()
{
    std::vector<std::string> lmNames = paramLmExtractors(config);
    for (std::vector<std::string>::const_iterator name = lmNames.begin();
	 name != lmNames.end(); ++ name) {
	LmLatticeRescorer *rescorer =
	    new LmLatticeRescorer(select(*name));
	extractors_.push_back(rescorer);
	log("\"%s\" appended (lm-rescorer)", name->c_str());
    }
}

void LatticeSetGenerator::appendCombinedLmRescorers()
{
    std::vector<std::string> combinedLmNames =
	paramCombinedLmExtractors(config);
    for (std::vector<std::string>::iterator name = combinedLmNames.begin();
	 name != combinedLmNames.end(); ++ name) {
	CombinedLmLatticeRescorer *rescorer =
	    new CombinedLmLatticeRescorer(select(*name));
	extractors_.push_back(rescorer);
	log("\"%s\" appended (combined-lm-rescorer)", name->c_str());
    }
}

void LatticeSetGenerator::appendRestorers()
{
#if 1
    criticalError("%s requires MODULE_SPEECH_DT_ADVANCED", __PRETTY_FUNCTION__);
#endif
}


void LatticeSetGenerator::appendReaders()
{
    std::vector<std::string> readerNames = paramReaders(config);
    for (std::vector<std::string>::const_iterator name = readerNames.begin();
	 name != readerNames.end(); ++ name) {
	LatticeReader *reader =
	    new LatticeReader(select(*name), lexicon_);
	reader->respondToDelayedErrors();
	extractors_.push_back(reader);
	log("\"%s\" appended (reader)", name->c_str());
    }
}

void LatticeSetGenerator::appendDistanceRescorers()
{
    std::vector<std::string> distanceRescorersNames = paramDistanceExtractors(config);
    for (std::vector<std::string>::const_iterator name = distanceRescorersNames.begin();
	 name != distanceRescorersNames.end(); ++ name) {
	LatticeRescorer *rescorer =
	    DistanceLatticeRescorer::createDistanceLatticeRescorer(
		select(*name), lexicon_);
	if (!rescorer) {
	    error("Unknown distance type for rescorer.");
	}
	if (dynamic_cast<ApproximatePhoneAccuracyLatticeRescorer*>(rescorer)) {
	    dynamic_cast<ApproximatePhoneAccuracyLatticeRescorer*>(
		rescorer)->setAlignmentGenerator(alignmentGenerator());
	}
	rescorer->respondToDelayedErrors();
	extractors_.push_back(rescorer);
	log("\"%s\" appended (distance-rescorer)", name->c_str());
    }
}

void LatticeSetGenerator::appendPosteriorRescorers()
{
#if 1
    criticalError("%s requires MODULE_SPEECH_DT_ADVANCED", __PRETTY_FUNCTION__);
#endif
}


void LatticeSetGenerator::appendPassRescorers()
{
    std::vector<std::string> passNames = paramPassExtractors(config);
    for (std::vector<std::string>::const_iterator name = passNames.begin();
	 name != passNames.end(); ++ name) {
	LatticeExtractor *pass = 0;
	    pass = new LatticeExtractor(select(*name));

	pass->respondToDelayedErrors();
	extractors_.push_back(pass);
	log("\"%s\" appended (pass)", name->c_str());
    }
}

void LatticeSetGenerator::signOn(CorpusVisitor &corpusVisitor)
{
    if (segmentwiseFeatureExtractor_) {
	segmentwiseFeatureExtractor_->signOn(corpusVisitor);
	segmentwiseFeatureExtractor_->respondToDelayedErrors();
    }
    for (LatticeExtractors::iterator it = extractors_.begin(); it != extractors_.end(); ++ it) {
	(*it)->signOn(corpusVisitor);
    }
    if (alignmentGenerator_) {
	alignmentGenerator_->signOn(corpusVisitor);
    }
    Precursor::signOn(corpusVisitor);
}

void LatticeSetGenerator::processWordLattice(
    Lattice::ConstWordLatticeRef lattice, Bliss::SpeechSegment *s)
{
    if (!segmentwiseFeatureExtractor_ || segmentwiseFeatureExtractor_->valid()) {
	timeval start, end;
	int i = 0;
	TIMER_START(start)
	Core::Ref<Lattice::WordLattice> rescored(new Lattice::WordLattice);
	for (LatticeExtractors::iterator it = extractors_.begin(); it != extractors_.end(); ++ it, ++i) {
	    timeval startRescorer, endRescorer;
	    TIMER_START(startRescorer)
	    Lattice::ConstWordLatticeRef r = (*it)->extract(lattice, s);
	    if (!r) continue;
	    if (!rescored->wordBoundaries()) {
		rescored->setWordBoundaries(r->wordBoundaries());
	    }
	    rescored->setFsa(r->mainPart(), (*it)->name());
	    TIMER_GPU_STOP(startRescorer, endRescorer, true, timeRescorers_[i])
	}
	TIMER_GPU_STOP(start, end, true, timeProcessSegment_)
	Precursor::processWordLattice(rescored, s);
    } else {
	warning("invalid segmentwise feature extractor");
	Precursor::processWordLattice(Lattice::ConstWordLatticeRef(), s);
    }
}

void LatticeSetGenerator::logComputationTime() const {
    int i = 0;
    for (LatticeExtractors::const_iterator it = extractors_.begin(); it != extractors_.end(); ++ it, ++i) {
	log("time for rescorer \"") << (*it)->name() << "\" : " << timeRescorers_[i];
    }
    LatticeSetProcessor::logComputationTime();
}

void LatticeSetGenerator::initialize(Bliss::LexiconRef lexicon)
{
    Precursor::initialize(lexicon);
    lexicon_ = lexicon;
    if (!lexicon_) {
	error("Could not initialize lexicon.");
    }

    initializeExtractors();

    if (segmentwiseFeatureExtractor_) {
	setSegmentwiseFeatureExtractor(segmentwiseFeatureExtractor_);
    }
    if (alignmentGenerator_) {
	setAlignmentGenerator(alignmentGenerator_);
    }
}

void LatticeSetGenerator::setSegmentwiseFeatureExtractor(
    Core::Ref<SegmentwiseFeatureExtractor> segmentwiseFeatureExtractor)
{
    if (!segmentwiseFeatureExtractor_) {
	segmentwiseFeatureExtractor_ = segmentwiseFeatureExtractor;
    }
    Precursor::setSegmentwiseFeatureExtractor(segmentwiseFeatureExtractor);
}

void LatticeSetGenerator::setAlignmentGenerator(
    Core::Ref<PhonemeSequenceAlignmentGenerator> alignmentGenerator)
{
    if (!alignmentGenerator_) {
	alignmentGenerator_ = alignmentGenerator;
    }
    Precursor::setAlignmentGenerator(alignmentGenerator);
}

void LatticeSetGenerator::leaveCorpus(Bliss::Corpus *corpus){
    LatticeSetProcessor::leaveCorpus(corpus);
    if (corpus->level() == 0) {
	for (LatticeExtractors::iterator extr = extractors_.begin(); extr != extractors_.end(); extr++)
	    (*extr)->finalize();
    }
}

/**
 *  LatticeSetReader
 */
LatticeSetReader::LatticeSetReader(const Core::Configuration &c) :
    Core::Component(c),
    Precursor(c),
    LatticeSetExtractor(c),
    archiveReader_(0)
{}

LatticeSetReader::~LatticeSetReader()
{
    delete archiveReader_;
}

void LatticeSetReader::initializeReaders()
{
    appendAcousticReaders();
    appendLmReaders();
    appendCombinedLmReaders();
    appendReaders();
    appendPassReaders();
}

void LatticeSetReader::appendAcousticReaders()
{
    std::vector<std::string> acousticNames = paramAcousticExtractors(config);
    for (std::vector<std::string>::iterator name = acousticNames.begin();
	 name != acousticNames.end(); ++ name) {
	readers_.push_back(*name);
	log("\"%s\" appended (acoustic-reader)", name->c_str());
    }
}

void LatticeSetReader::appendEmissionReaders()
{
#if 1
    criticalError("%s requires MODULE_SPEECH_DT_ADVANCED", __PRETTY_FUNCTION__);
#endif

}

void LatticeSetReader::appendTdpReaders()
{
#if 1
    criticalError("%s requires MODULE_SPEECH_DT_ADVANCED", __PRETTY_FUNCTION__);
#endif
}

void LatticeSetReader::appendPronunciationReaders()
{
#if 1
    criticalError("%s requires MODULE_SPEECH_DT_ADVANCED", __PRETTY_FUNCTION__);
#endif
}


void LatticeSetReader::appendLmReaders()
{
    std::vector<std::string> lmNames = paramLmExtractors(config);
    for (std::vector<std::string>::iterator name = lmNames.begin();
	 name != lmNames.end(); ++ name) {
	readers_.push_back(*name);
	log("\"%s\" appended (lm-reader)", name->c_str());
    }
}

void LatticeSetReader::appendCombinedLmReaders()
{
    std::vector<std::string> combinedLmNames =
	paramCombinedLmExtractors(config);
    for (std::vector<std::string>::iterator name = combinedLmNames.begin();
	 name != combinedLmNames.end(); ++ name) {
	readers_.push_back(*name);
	log("\"%s\" appended (combined-lm-reader)", name->c_str());
    }
}

void LatticeSetReader::appendReaders()
{
    std::vector<std::string> readerNames = paramReaders(config);
    for (std::vector<std::string>::const_iterator name = readerNames.begin();
	 name != readerNames.end(); ++ name) {
	readers_.push_back(*name);
	log("\"%s\" appended (reader)", name->c_str());
    }
}

void LatticeSetReader::appendPassReaders()
{
    std::vector<std::string> passNames = paramPassExtractors(config);
    for (std::vector<std::string>::const_iterator name = passNames.begin();
	 name != passNames.end(); ++ name) {
	readers_.push_back(*name);
	log("\"%s\" appended (pass)", name->c_str());
    }
}

void LatticeSetReader::leaveSpeechSegment(Bliss::SpeechSegment *s)
{
    require(archiveReader_);
    timeval start, end;
    TIMER_START(start)
    Lattice::ConstWordLatticeRef lattice = archiveReader_->get(s->fullName(), readers_);
    TIMER_GPU_STOP(start, end, true, timeProcessSegment_)
    if (lattice && lattice->nParts() == readers_.size()) {
	processWordLattice(lattice, s);
    } else {
	log("skip this segment because not all lattice parts could be read");
    }
    Precursor::leaveSpeechSegment(s);
}

void LatticeSetReader::initialize(Bliss::LexiconRef lexicon)
{
    Precursor::initialize(lexicon);
    initializeReaders();

    verify(!archiveReader_);
    archiveReader_ = Lattice::Archive::openForReading(
	select("lattice-archive"), lexicon);
    if (!archiveReader_ || archiveReader_->hasFatalErrors()) {
	delete archiveReader_; archiveReader_ = 0;
	error("failed to open lattice archive");
	return;
    }
}

/**
 *  LatticeSetWriter
 */
LatticeSetWriter::LatticeSetWriter(const Core::Configuration &c) :
    Core::Component(c),
    Precursor(c),
    archiveWriter_(0)
{}

LatticeSetWriter::~LatticeSetWriter()
{
    delete archiveWriter_;
}

void LatticeSetWriter::processWordLattice(
    Lattice::ConstWordLatticeRef lattice, Bliss::SpeechSegment *s)
{
    verify(archiveWriter_);
    archiveWriter_->store(s->fullName(), lattice);
    Precursor::processWordLattice(lattice, s);
}

void LatticeSetWriter::initialize(Bliss::LexiconRef lexicon)
{
    Precursor::initialize(lexicon);

    verify(!archiveWriter_);
    archiveWriter_ = Lattice::Archive::openForWriting(
	select("lattice-archive"), lexicon);
    if (!archiveWriter_ || archiveWriter_->hasFatalErrors()) {
	delete archiveWriter_; archiveWriter_ = 0;
	error("failed to open lattice archive");
	return;
    }

}

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
#include <Modules.hh>
#include <Bliss/Fsa.hh>
#include <Core/Directory.hh>
#include <Core/TextStream.hh>
#include <Fsa/Cache.hh>
#include <Fsa/Project.hh>
#include <Fsa/Rational.hh>
#include <Fsa/Semiring.hh>
#include <Fsa/Sort.hh>
#include <Fsa/Static.hh>

#include "Archive.hh"
#ifdef MODULE_LATTICE_HTK
#include "HtkReader.hh"
#include "HtkWriter.hh"
#endif



using namespace Lattice;

const char Archive::latticeConfigFilename[] = "default.config";
const char Archive::wordBoundarySuffix[] = ".binwb.gz";

namespace Lattice {
    enum LatticeFormat {
	formatFsa,
	formatHtk,
	formatLuna,
	formatLin,
	formatSourceFile,
	formatWolfgang
    };
}

namespace Lattice {
    class FsaArchiveReader;
    class FsaArchiveWriter;
}

#ifdef MODULE_LATTICE_HTK
namespace Lattice {
    class HtkArchiveReader;
    class HtkArchiveWriter;
}
#endif // MODULE_LATTICE_HTK


const Core::Choice Archive::formatChoice(
    "fsa",      formatFsa,
    "htk",      formatHtk,
    "luna",		formatLuna,
    "lin",		formatLin,
    "source",	formatSourceFile,
    "wolfgang", formatWolfgang,
    Core::Choice::endMark());

const Core::ParameterChoice Archive::paramFormat(
    "type",
    &Archive::formatChoice,
    "format of word lattices",
    formatFsa);
const Core::ParameterString Archive::paramPath(
    "path",
    "path to word lattices");

namespace Lattice {
    enum Alphabet {
	unknownAlphabet,
	lemmaPronunciationAlphabet,
	lemmaPronunciationAlphabetAcceptor,
	lemmaAlphabet,
	syntacticTokenAlphabet,
	evaluationTokenAlphabet,
	noLexiconCheck
    };
}

const Core::Choice Archive::alphabetChoice(
    "unknown", unknownAlphabet,
    "lemma-pronunciation", lemmaPronunciationAlphabet,
    "lemma-pronunciation-acceptor", lemmaPronunciationAlphabetAcceptor,
    "lemma", lemmaAlphabet,
    "syntactic-token", syntacticTokenAlphabet,
    "evaluation-token", evaluationTokenAlphabet,
    "no-lexicon-check", noLexiconCheck,
    Core::Choice::endMark());

const Core::ParameterChoice Archive::paramAlphabet(
    "alphabet",
    &Archive::alphabetChoice,
    "alphabet of word lattices",
    lemmaPronunciationAlphabet);

Archive::Archive(
    const Core::Configuration &config,
    const std::string &pathname,
    Bliss::LexiconRef lexicon)
    :
    Core::Component(config),
    lexicon_(lexicon),
    pathname_(pathname) {}

// generic interface =========================================================
const Core::ParameterFloat ArchiveReader::paramAmScale(
    "am-scale",
    "acoustic model scale", 1.0);
const Core::ParameterFloat ArchiveReader::paramLmScale(
    "lm-scale",
    "language model scale", 0.0);
const Core::ParameterFloat ArchiveReader::paramPenaltyScale(
    "penalty-scale",
    "penalty scale", 1.0);
const Core::ParameterFloat ArchiveReader::paramPronunciationScale(
    "pronunciation-scale",
    "pronunciation scale", 0.0);

ArchiveReader::ArchiveReader(
    const Core::Configuration &config,
    const std::string &pathname,
    Bliss::LexiconRef lexicon)
    :
    Core::Component(config),
    Precursor(config, pathname, lexicon) {}

void ArchiveReader::loadLatticeConfiguration()
{
    if (hasFile(latticeConfigFilename)) {
	if (latticeConfig_.setFromFile(Core::joinPaths(pathname_, latticeConfigFilename))) {
	    log("read config file from archive");
	    latticeConfig_.setSelection("lattice-archive");
	} else {
	    error("failed to read config file \"%s\" from archive", latticeConfigFilename);
	}
    } else {
	warning("no default config file \"%s\" in archive, use main config only", latticeConfigFilename);
    }
}

ArchiveWriter::ArchiveWriter(
    const Core::Configuration &config,
    const std::string &pathname,
    Bliss::LexiconRef lexicon)
    :
    Core::Component(config),
    Precursor(config, pathname, lexicon) {}

// native lattice format =====================================================

// writer --------------------------------------------------------------------
/*
  Parameters needed for restoring the search space:
  .lm-scale
*/

class Lattice::FsaArchiveWriter :
    public ArchiveWriter {
private:
    typedef FsaArchiveWriter Self;
    typedef ArchiveWriter Precursor;

private:
    Fsa::ArchiveWriter *fsaArchiveWriter_;

private:
    std::string filename(const std::string &id, const std::string &name) const;

public:
    FsaArchiveWriter(const Core::Configuration &config,const std::string &pathname,Bliss::LexiconRef lexicon);
    virtual ~FsaArchiveWriter();

    virtual void store(const std::string &id, ConstWordLatticeRef wordLattice);
};

Lattice::FsaArchiveWriter::FsaArchiveWriter(
    const Core::Configuration &config,
    const std::string &pathname,
    Bliss::LexiconRef lexicon)
    :
    Core::Component(config),
    Precursor(config, pathname, lexicon),
    fsaArchiveWriter_(0)
{
//  storeLatticeConfiguration();
}

FsaArchiveWriter::~FsaArchiveWriter()
{
    delete fsaArchiveWriter_;
}

std::string FsaArchiveWriter::filename(const std::string &id, const std::string &name) const
{
    if (name == WordLattice::acousticFsa || name == WordLattice::mainFsa || name.empty())
	return id + suffix();
    return id + "-" + name + suffix();
}

bool writeWordBoundariesBinary(
    Core::Ref<const WordBoundaries> wordBoundaries,
    Core::BinaryOutputStream &bo)
{
    return wordBoundaries->writeBinary(bo);
}

void FsaArchiveWriter::store(
    const std::string &id, ConstWordLatticeRef wordLattice)
{
    // when the first lattice needs to be saved, we create a Fsa::ArchiveWriter
    if (!fsaArchiveWriter_ and wordLattice->nParts() > 0) {
	Fsa::ConstAutomatonRef fsa = wordLattice->part(0);
	if (fsa->type() == Fsa::TypeAcceptor) {
	    fsaArchiveWriter_ =
		new Fsa::ArchiveWriter(
		    config,
		    pathname_,
		    fsa->getInputAlphabet());
	} else {
	    fsaArchiveWriter_ =
		new Fsa::ArchiveWriter(
		    config,
		    pathname_,
		    fsa->getInputAlphabet(),
		    fsa->getOutputAlphabet());
	}
    }

    for (size_t i = 0; i < wordLattice->nParts(); ++i) {
	Fsa::ConstAutomatonRef part = wordLattice->part(i);
	if (part)
	    fsaArchiveWriter_->store(
		filename(id, wordLattice->name(i)),
		part);
    }

    if (wordLattice->wordBoundaries()) {
	Core::ArchiveWriter o(*fsaArchiveWriter_->archive(), id + wordBoundarySuffix, true);
	if (!o) {
	    error("Failed to write word boundaries \"%s\"", id.c_str());
	}
	Core::BinaryOutputStream bo(o);
	if (!writeWordBoundariesBinary(wordLattice->wordBoundaries(), bo)) {
	    error("Failed to write word boundaries \"%s\"", id.c_str());
	}
    }
}

// reader --------------------------------------------------------------------
/*
  Parameters needed for restoring the search space:
  .lm-scale
*/
class Lattice::FsaArchiveReader :
    public ArchiveReader {
private:
    typedef FsaArchiveReader Self;
    typedef ArchiveReader Precursor;

private:
    std::string filename(const std::string &id, const std::string &name) const;

private:
    Fsa::ArchiveReader *fsaArchiveReader_;
    static const Core::ParameterString paramAmFileSuffix;

public:
    FsaArchiveReader(
	const Core::Configuration &config,
	const std::string &pathname,
	Bliss::LexiconRef lexicon);
    virtual ~FsaArchiveReader();

    bool hasFile(const std::string &file) const { return fsaArchiveReader_->hasFile(file); }
    const_iterator files() const { return fsaArchiveReader_->files(); }

    virtual ConstWordLatticeRef get(
	const std::string &id, const std::vector<std::string> &names);
};

const Core::ParameterString FsaArchiveReader::paramAmFileSuffix(
    "am-file-suffix",
    "", "");

FsaArchiveReader::FsaArchiveReader(
    const Core::Configuration &config,
    const std::string &pathname,
    Bliss::LexiconRef lexicon) :
    Core::Component(config),
    Precursor(config, pathname, lexicon),
    fsaArchiveReader_(0)
{
    fsaArchiveReader_ = new Fsa::ArchiveReader(config, pathname);
    if (!fsaArchiveReader_->archive()) {
	error("Failed to open FSA archive '%s'.", pathname.c_str());
	return;
    }
    switch (paramAlphabet(config)) {
    case lemmaPronunciationAlphabetAcceptor:
	fsaArchiveReader_->mapOutput(lexicon_->lemmaPronunciationAlphabet());
    case lemmaPronunciationAlphabet:
	fsaArchiveReader_->mapInput(lexicon_->lemmaPronunciationAlphabet());
	break;
    case lemmaAlphabet:
	fsaArchiveReader_->mapInput(lexicon_->lemmaAlphabet());
	break;
    case syntacticTokenAlphabet:
	fsaArchiveReader_->mapInput(lexicon_->syntacticTokenAlphabet());
	break;
    case evaluationTokenAlphabet:
	fsaArchiveReader_->mapInput(lexicon_->evaluationTokenAlphabet());
	break;
    case noLexiconCheck:
    // do nothing
	break;
    default:
	criticalError("Unknown alphabet.");
    }

    loadLatticeConfiguration();
    if (amScale() != 1.0)
	error("cannot set am-scale to %f; am-scale for fsa lattices is always 1.0",
	      amScale());
    if (penaltyScale() != amScale())
	error("cannot set penalty-scale to %f; penalty-scale for fsa lattices is always equal to am-scale, which is %f",
	      penaltyScale(), amScale());
    if (lmScale() != 0.0)
	error("cannot set lm-scale to %f; lm-scale for fsa lattices is always 0.0",
	      lmScale());
    if (pronunciationScale() != 0.0)
	error("cannot set pronunciation-scale to %f; pronunciation-scale for fsa lattices is always 0.0",
	      pronunciationScale());
}

FsaArchiveReader::~FsaArchiveReader()
{
    delete fsaArchiveReader_;
}

std::string FsaArchiveReader::filename(const std::string &id, const std::string &name) const
{
    if (name == WordLattice::acousticFsa || name == WordLattice::mainFsa || name.empty()) {
    if(name == WordLattice::acousticFsa) {
      std::string amSuffix = paramAmFileSuffix(config);
      if(!amSuffix.empty()) {
	return id + "-" + amSuffix + suffix();
      }
    }
	return id + suffix();
    }else
	return id + "-" + name + suffix();
}

bool readWordBoundariesBinary(Core::Ref<WordBoundaries> wordBoundaries,
			      Core::BinaryInputStream &bi)
{
    verify(wordBoundaries);
    return wordBoundaries->readBinary(bi);
}

ConstWordLatticeRef FsaArchiveReader::get(
    const std::string &id, const std::vector<std::string> &names)
{
    Core::Ref<WordLattice> wordLattice;
    Core::Ref<WordBoundaries> wordBoundaries;
    std::string wordBoundariesName(id + wordBoundarySuffix);
    if (fsaArchiveReader_->archive()->hasFile(wordBoundariesName)) {
	Core::ArchiveReader i(*fsaArchiveReader_->archive(), wordBoundariesName);
	if (i) {
	    Core::BinaryInputStream bi(i);
	    wordBoundaries = Core::ref(new WordBoundaries);
	    if (!readWordBoundariesBinary(wordBoundaries, bi)) {
		error("Failed to read word boundaries \"%s\"", id.c_str());
		wordBoundaries.reset();
	    }
	} else {
	    error("Failed to read word boundaries \"%s\"", id.c_str());
	}
    } else {
	error("Could not find word boundaries \"%s\"", id.c_str());
    }

    wordLattice = Core::ref(new WordLattice);
    wordLattice->setWordBoundaries(wordBoundaries);
    for (size_t i = 0; i < names.size(); ++ i) {
	wordLattice->setFsa(fsaArchiveReader_->get(filename(id, names[i])), names[i]);
	if (!wordLattice->part(names[i])) {
	    error("Failed to read part '%s'.", names[i].c_str());
	    wordLattice.reset();
	    return wordLattice;
	}
    }
    return wordLattice;
}

#ifdef MODULE_LATTICE_HTK
// HTK lattices ==============================================================
// writer --------------------------------------------------------------------
class Lattice::HtkArchiveWriter :
    public ArchiveWriter
{
    typedef ArchiveWriter Precursor;
private:
    Core::Archive *archive_;
    HtkWriter htkWriter_;

public:
    HtkArchiveWriter(
	const Core::Configuration &config,
	const std::string &pathname,
	Bliss::LexiconRef lexicon)
	:
	Core::Component(config),
	Precursor(config, pathname, lexicon),
	htkWriter_(lexicon) {
	archive_ = Core::Archive::create(config, pathname, Core::Archive::AccessModeWrite);
	warning("operability of HtkArchiveWriter is not fully tested");
	if (!archive_) error("Failed to open lattice archive for writing");
    }

    virtual ~HtkArchiveWriter() {
	delete archive_;
    }

    virtual void store(const std::string &id, ConstWordLatticeRef wordLattice) {
	require(wordLattice->hasPart(WordLattice::acousticFsa) &&
		wordLattice->hasPart(WordLattice::lmFsa));
	Core::ArchiveWriter os(*archive_, id + suffix(), true);
	htkWriter_.write(id, wordLattice, os);
	if (!os)
	    error("failed to write word lattice \"%s\"in HTK format", id.c_str());
    }
};

#endif // MODULE_LATTICE_HTK


#ifdef MODULE_LATTICE_HTK

// reader --------------------------------------------------------------------

class Lattice::HtkArchiveReader :
    public ArchiveReader {
private:
    typedef HtkArchiveReader Self;
    typedef ArchiveReader Precursor;

    static const Core::ParameterString paramEncoding;
    static const Core::ParameterBool paramCapitalizeTranscriptions;
    static const Core::ParameterString paramSuffix;
    static const Core::ParameterFloat paramWordPenalty;
    static const Core::ParameterFloat paramSilencePenalty;

private:
    Core::Archive *archive_;
    HtkReader *htkReader_;
    std::string encoding_;
    bool capitalize_;

public:
    HtkArchiveReader(
	const Core::Configuration &config,
	const std::string &pathname,
	Bliss::LexiconRef lexicon);
    ~HtkArchiveReader();

    const std::string & encoding() { return encoding_; }
    bool isCapitalized() { return capitalize_; }

    bool hasFile(const std::string &file) const { require(archive_); return archive_->hasFile(file); }
    const_iterator files() const { require(archive_); return archive_->files(); }

    virtual ConstWordLatticeRef get(const std::string &id, const std::vector<std::string> &names);
};

const Core::ParameterString HtkArchiveReader::paramEncoding(
    "encoding",
    "encoding of htk lattices in archive", Core::UnicodeInputConverter::defaultEncoding);
const Core::ParameterBool HtkArchiveReader::paramCapitalizeTranscriptions(
    "capitalize-transcriptions",
    "convert all words to upper case: yes/no", false);
const Core::ParameterString HtkArchiveReader::paramSuffix(
    "suffix",
    "suffix of htk lattices in archive", ".lat.gz");
const Core::ParameterFloat HtkArchiveReader::paramWordPenalty(
    "word-penalty",
    "word penalty", 0.0);
const Core::ParameterFloat HtkArchiveReader::paramSilencePenalty(
    "silence-penalty",
    "silence penalty", 0.0);

Fsa::ConstAutomatonRef createEpsilonToSentenceBoundariesTransducer(
    Bliss::LexiconRef lexicon,
    Fsa::ConstSemiringRef semiring)
{
    Fsa::StaticAutomaton *epsilonToSentenceBoundaries = new Fsa::StaticAutomaton(Fsa::TypeTransducer);
    epsilonToSentenceBoundaries->setSemiring(semiring);
    epsilonToSentenceBoundaries->setInputAlphabet(lexicon->lemmaPronunciationAlphabet());
    epsilonToSentenceBoundaries->setOutputAlphabet(lexicon->lemmaAlphabet());
    Fsa::State *initialState = new Fsa::State(0);
    epsilonToSentenceBoundaries->setState(initialState);
    epsilonToSentenceBoundaries->setInitialStateId(0);
    Fsa::State *finalState = new Fsa::State(1);
    finalState->setFinal(Fsa::Weight(0.0));
    epsilonToSentenceBoundaries->setState(finalState);
    const Bliss::Lemma *lemma;
    if ((lemma = lexicon->specialLemma("sentence-begin")))
	initialState->newArc(1, Fsa::Weight(0.0), Fsa::Epsilon, lexicon->lemmaAlphabet()->index(lemma));
    if ((lemma = lexicon->specialLemma("sentence-end")))
	initialState->newArc(1, Fsa::Weight(0.0), Fsa::Epsilon, lexicon->lemmaAlphabet()->index(lemma));
    if ((lemma = lexicon->specialLemma("sentence-boundary")))
	initialState->newArc(1, Fsa::Weight(0.0), Fsa::Epsilon, lexicon->lemmaAlphabet()->index(lemma));
    if (!initialState->hasArcs()) {
	epsilonToSentenceBoundaries->clear();
	initialState = new Fsa::State(0);
	initialState->setFinal(Fsa::Weight(0.0));
	epsilonToSentenceBoundaries->setState(initialState);
	epsilonToSentenceBoundaries->setInitialStateId(0);
    }
    return Fsa::ConstAutomatonRef(epsilonToSentenceBoundaries);
}

HtkArchiveReader::HtkArchiveReader(
    const Core::Configuration &config,
    const std::string &pathname,
    Bliss::LexiconRef lexicon) :
    Core::Component(config),
    Precursor(config, pathname, lexicon)
{
    archive_ = Core::Archive::create(config, pathname, Core::Archive::AccessModeRead);
    if (!archive_) {
	error("failed to open htk archive \"%s\"", pathname.c_str());
	return;
    }
    log("open htk archive \"%s\"", pathname.c_str());
    warning("operability of HtkArchiveReader is not fully tested");

    loadLatticeConfiguration();
    setSuffix(paramSuffix(config, paramSuffix(latticeConfig_)));
    encoding_ = paramEncoding(config, paramEncoding(latticeConfig_));
    capitalize_ = paramCapitalizeTranscriptions(config, paramCapitalizeTranscriptions(latticeConfig_));

    htkReader_ = new HtkReader(config, lexicon_, amScale(), lmScale(), penaltyScale(), pronunciationScale(), false);
    htkReader_->setCapitalize(capitalize_);
    htkReader_->setWordPenalty(
	paramWordPenalty(config, paramWordPenalty(latticeConfig_)));
    htkReader_->setSilencePenalty(
	paramSilencePenalty(config, paramSilencePenalty(latticeConfig_)));
}

HtkArchiveReader::~HtkArchiveReader()
{
    delete htkReader_;
    delete archive_;
}

ConstWordLatticeRef HtkArchiveReader::get(
    const std::string &id, const std::vector<std::string> &names)
{
    require(names.size() == 1 && names.front() == WordLattice::totalFsa);
    require(archive_);
    std::string filename = id + suffix();
    Core::ArchiveReader *isPtr = new Core::ArchiveReader(*archive_, filename);
    if (!isPtr->isOpen()) {
	error("Cannot open \"%s\" for reading", filename.c_str());
	return ConstWordLatticeRef();
    }
    Core::TextInputStream tis(static_cast<std::istream *>(isPtr));
    tis.setEncoding(encoding_);
    return htkReader_->read(tis);
}

#endif // MODULE_LATTICE_HTK


// factory methods ===========================================================

Lattice::ArchiveReader *Archive::openForReading(
    const Core::Configuration &config,
    Bliss::LexiconRef lexicon)
{
    std::string pathname = paramPath(config);
    if (pathname.empty()) return 0;

    ArchiveReader *archiveReader = 0;
    switch (paramFormat(config)) {
    case formatFsa:
	archiveReader = new FsaArchiveReader(config, pathname, lexicon);
	archiveReader->setSuffix(".binfsa.gz");
	break;
#ifdef MODULE_LATTICE_HTK
    case formatHtk:
	archiveReader = new HtkArchiveReader(config, pathname, lexicon);
	// suffix is read from config
	break;
#endif
    default: defect();
    }

    if (archiveReader->hasFatalErrors()) {
	delete archiveReader; archiveReader = 0;
    }

    return archiveReader;
}

Lattice::ArchiveWriter *Archive::openForWriting(
    const Core::Configuration &config,
    Bliss::LexiconRef lexicon)
{
    std::string pathname = paramPath(config);
    if (pathname.empty()) return 0;

    ArchiveWriter *archiveWriter = 0;
    switch (paramFormat(config)) {
    case formatFsa:
	archiveWriter = new FsaArchiveWriter(config, pathname, lexicon);
	archiveWriter->setSuffix(".binfsa.gz");
	break;
#ifdef MODULE_LATTICE_HTK
    case formatHtk:
	archiveWriter = new HtkArchiveWriter(config, pathname, lexicon);
	archiveWriter->setSuffix(".lat.gz");
	break;
#endif


    default: defect();
    }

    if (archiveWriter->hasFatalErrors()) {
	delete archiveWriter; archiveWriter = 0;
    }

    return archiveWriter;
}

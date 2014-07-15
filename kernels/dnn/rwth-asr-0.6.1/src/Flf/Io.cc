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
#include <sstream>
#include <Core/Application.hh>
#include <Core/Choice.hh>
#include <Core/Directory.hh>
#include <Core/Hash.hh>
#include <Core/StringUtilities.hh>
#include <Core/Vector.hh>
#include <Fsa/Basic.hh>
#include <Fsa/Cache.hh>
#include <Fsa/Input.hh>

#include "FlfCore/Utility.hh"
#include "Convert.hh"
#include "Copy.hh"
#include "Draw.hh"
#include "Io.hh"
#include "HtkSlfIo.hh"
#include "FlfIo.hh"
#include "Lexicon.hh"

namespace Flf {

    // -------------------------------------------------------------------------
    std::pair<std::string, bool> stripExtension(const std::string &filename) {
	std::string name(filename);
	bool gzipped(false);
	if (name.find(".gz", name.size() - 3) != std::string::npos) {
	    name.erase(name.size() - 3);
	    gzipped = true;
	}
	std::string::size_type i;
	if ((i = name.rfind(".")) != std::string::npos)
	    name.erase(i);
	return std::make_pair(name, gzipped);
    }
    // -------------------------------------------------------------------------


    // -------------------------------------------------------------------------
    InputStream::InputStream(const std::string &path, Core::Archive *archive) :
	ar(0), cis(0), is(0) {
	std::string filename = Core::normalizePath(path);
	if (archive) {
	    ar = new Core::ArchiveReader(*archive, filename);
	    if (!(*ar) || !ar->isOpen()) {
		Core::Application::us()->
		    warning("Could not open archive entry \"%s\".", filename.c_str());
		delete ar;
		ar = 0;
	    } else
		is = static_cast<std::istream*>(ar);
	} else {
	    cis = new Core::CompressedInputStream(filename);
	    if (!(*cis)) {
		Core::Application::us()->
		    warning("Could not open file \"%s\".", filename.c_str());
		delete cis;
		cis = 0;
	    } else
		is = static_cast<std::istream*>(cis);
	}
    }

    InputStream::~InputStream() {
	delete ar;
	delete cis;
    }

    std::istream * InputStream::steal() {
	//require(is);
	if (!is) return 0;
	std::istream *_is = is;
	ar = 0; cis = 0; is = 0;
	return _is;
    }
    // -------------------------------------------------------------------------


    // -------------------------------------------------------------------------
    OutputStream::OutputStream(const std::string &path, Core::Archive *archive) :
	aw(0), cos(0), os(0) {
	std::string filename = Core::normalizePath(path);
	if (archive) {
	    aw = new Core::ArchiveWriter(*archive, filename, true);
	    if (!(*aw)) {
		Core::Application::us()->
		    error("Could not open archive entry \"%s\"", filename.c_str());
		delete aw;
		aw = 0;
	    } else
		os = static_cast<std::ostream*>(aw);
	} else {
	    cos = new Core::CompressedOutputStream(filename);
	    if (!(*cos)) {
		Core::Application::us()->
		    error("Could not open file \"%s\"", filename.c_str());
		delete cos;
		cos = 0;
	    } else
		os = static_cast<std::ostream*>(cos);
	}
    }

    OutputStream::~OutputStream() {
	delete aw;
	delete cos;
    }

    std::ostream * OutputStream::steal() {
	// require(os);
	if (!os) return 0;
	std::ostream *_os = os;
	aw = 0; cos = 0; os = 0;
	return _os;
    }
    // -------------------------------------------------------------------------


    // -------------------------------------------------------------------------
    namespace {
	struct BinaryHeader {
	    static const char *magic;
	    static const size_t magicSize = 8;
	    static const u32 version = 3; // version 4 only supported by FLF
	};
	const char *BinaryHeader::magic = "LATWRDBN";
    }

    bool readBinary(StaticBoundaries *b, std::istream &is) {
	Core::BinaryInputStream bis(is);
	char magic[BinaryHeader::magicSize];
	u32 version;
	bis.read(magic, BinaryHeader::magicSize);
	if (!bis) return false;
	if (strncmp(magic, BinaryHeader::magic, BinaryHeader::magicSize) != 0) {
	    // backward compatibility: old format without any header
#if defined(BISANI)
	    version = 1;
#else
	    version = 2;
#endif
	    bis.seek( - BinaryHeader::magicSize, std::ios::cur);
	} else {
	    if (!(bis >> version)) return false;
	}
	u32 size;
	if (!(bis >> size)) return false;
	b->resize(size);
	switch (version) {
	case 1: {
	    for (StaticBoundaries::iterator it = b->begin(); (it != b->end()) && (bis); ++it) {
		u32 time, transitId;
		if (!(bis >> time >> transitId)) return false;
		it->setTime(time);
	    }
	} break;
	case 2:
	case 3: {
	    for (StaticBoundaries::iterator it = b->begin(); (it != b->end()) && (bis); ++it) {
		u32 time;
		u16 final, initial;
		if (!(bis >> time >> final >> initial)) return false;
		it->setTime(time);
		it->setTransit(Boundary::Transit(final, initial, AcrossWordBoundary));
	    }
	} break;
	case 4: {
	    for (StaticBoundaries::iterator it = b->begin(); (it != b->end()) && (bis); ++it) {
		u32 time;
		u16 final, initial;
		u8 boundary;
		if (!(bis >> time >> final >> initial >> boundary)) return false;
		it->setTime(time);
		it->setTransit(Boundary::Transit(final, initial, boundary));
	    }
	} break;
	default:
	    defect();
	}
	return bis;
    }

    bool readBoundariesBinary(ConstLatticeRef l, std::istream &is) {
	StaticBoundaries *b = new StaticBoundaries;
	if (!readBinary(b, is)) return false;
	l->setBoundaries(ConstBoundariesRef(b));
	return true;
    }

    bool writeBinary(const StaticBoundaries *b, std::ostream &os) {
	Core::BinaryOutputStream bos(os);
	bos.write(BinaryHeader::magic, BinaryHeader::magicSize);
	bos << (u32) BinaryHeader::version;
	bos << (u32) b->size();
	switch (BinaryHeader::version) {
	case 3:
	    for (StaticBoundaries::const_iterator it = b->begin(); it != b->end(); ++it) {
		bos << (u32) it->time();
		bos << (u16) it->transit().final << (u16) it->transit().initial;
	    }
	    break;
	case 4:
	    for (StaticBoundaries::const_iterator it = b->begin(); it != b->end(); ++it) {
		bos << (u32) it->time();
		bos << (u16) it->transit().final << (u16) it->transit().initial << (u8) it->transit().boundary;
	    }
	    break;
	default:
	    defect();
	}
	return bos;
    }

    bool writeBoundariesBinary(ConstLatticeRef l, std::ostream &os) {
	bool success = false;
	const StaticBoundaries *b = dynamic_cast<const StaticBoundaries*>(l->getBoundaries().get());
	if (b) {
	    success = writeBinary(b, os);
	} else {
	    StaticBoundaries *sb = new StaticBoundaries;
	    copyBoundaries(l, sb);
	    success = writeBinary(sb, os);
	    delete sb;
	}
	return success;
    }
    // -------------------------------------------------------------------------


    // -------------------------------------------------------------------------
    const u32 IoFlag::WriteHead = 0x00000001;
    const u32 IoFlag::WriteBody = 0x00000002;
    const u32 IoFlag::WriteAll  = 0x00000003;
    // -------------------------------------------------------------------------


    // -------------------------------------------------------------------------
    const Core::ParameterString IoFormat::paramEncoding(
	"encoding",
	"output character set encoding",
	"utf-8");
    const Core::ParameterInt IoFormat::paramMargin(
	"margin",
	"maximum line length",
	0, 0);
    const Core::ParameterInt IoFormat::paramIndentation(
	"indentation",
	"number of blanks for indenting output",
	2, 0);

    const Core::Choice IoFormat::choiceLatticeFormat(
	"flf",  IoFormat::LatticeFormatFlf,    // fsa based lattice format
	"htk",  IoFormat::LatticeFormatHtkSlf, // htk's standard lattice format
	"lattice-processor",  IoFormat::LatticeFormatLatticeProcessor, // deprecated, not supported by all i/o routines,
	"openfst", IoFormat::LatticeFormatOpenFst,
	Core::Choice::endMark());
    const Core::ParameterChoice IoFormat::paramLatticeFormat(
	"format",
	&IoFormat::choiceLatticeFormat,
	"format of word lattices",
	LatticeFormatFlf);

    const Core::Choice IoFormat::choiceCnFormat(
	"text", IoFormat::CnFormatText,
	"xml", IoFormat::CnFormatXml,
	Core::Choice::endMark());
    const Core::ParameterChoice IoFormat::paramCnFormat(
	"format",
	&IoFormat::choiceCnFormat,
	"format of CNs",
	CnFormatXml);

    const Core::Choice IoFormat::choicePosteriorCnFormat(
	"text", IoFormat::PosteriorCnFormatText,
	"xml", IoFormat::PosteriorCnFormatXml,
	"flow-alignment", IoFormat::PosteriorCnFormatFlowAlignment,
	Core::Choice::endMark());
    const Core::ParameterChoice IoFormat::paramPosteriorCnFormat(
	"format",
	&IoFormat::choicePosteriorCnFormat,
	"format of posterior CNs",
	PosteriorCnFormatXml);
    // -------------------------------------------------------------------------


    // -------------------------------------------------------------------------
    LatticeReader * LatticeIo::getReader(const Core::Configuration &config) {
	LatticeReader *reader = 0;
	switch (paramLatticeFormat(config)) {
	case LatticeFormatFlf:
	    reader = new FlfReader(Core::Configuration(config, "flf"));
	    break;
	case LatticeFormatHtkSlf:
	    reader = new HtkSlfReader(Core::Configuration(config, "htk"));
	    break;
	default:
	    defect();
	}
	return reader;
    }

    LatticeWriter * LatticeIo::getWriter(const Core::Configuration &config) {
	LatticeWriter *writer = 0;
	switch (paramLatticeFormat(config)) {
	case LatticeFormatFlf:
	    writer = new FlfWriter(config);
	    break;
	case LatticeFormatHtkSlf:
	    writer = new HtkSlfWriter(config);
	    break;
	default:
	    defect();
	}
	return writer;
    }
    // -------------------------------------------------------------------------


    // -------------------------------------------------------------------------
    class ReaderNode : public Node {
	friend class Network;
    public:
	static const Core::ParameterString paramDir;
    private:
	std::string dir_;
	std::string filename_;
	bool isValid_;
    protected:
	virtual bool read(const std::string &filename) = 0;

	const std::string dir() {
	    return dir_;
	}

	void load() {
	    if (isValid_) return;
	    isValid_ = true;
	    filename_ = connected(1) ? requestSegment(1)->segmentIdOrDie() : requestString(2);
	    if (!dir().empty())
		filename_ = Core::joinPaths(dir(), filename_).c_str();
	    if (read(filename_))
		log("ReaderNode: read %s", filename_.c_str());
	    else
		error("ReaderNode: Failed to read %s", filename_.c_str());
	}

    public:
	ReaderNode(const std::string &name, const Core::Configuration &config) :
	    Node(name, config), dir_() {}
	virtual ~ReaderNode() {}

	virtual void init(const std::vector<std::string> &arguments) {
	    if (!connected(1) && !connected(2))
		criticalError("ReaderNode: Need data source either at port 1 or at port 2.");
	    dir_ = paramDir(config);
	    isValid_ = false;
	}

	virtual void sync() {
	    filename_ = "";
	    isValid_ = false;
	}
    };
    const Core::ParameterString ReaderNode::paramDir(
	"path",
	"interpret file names relative to path",
	"");
    // -------------------------------------------------------------------------


    // -------------------------------------------------------------------------
    class LatticeReaderNode : public ReaderNode {
	typedef ReaderNode Precursor;
    private:
	LatticeReader *reader_;
	ConstLatticeRef l_;
    protected:
	virtual bool read(const std::string &filename) {
	    l_ = reader_->read(filename);
	    return l_;
	}
    public:
	LatticeReaderNode(const std::string &name, const Core::Configuration &config) :
	    Precursor(name, config), reader_(0) {}
	virtual ~LatticeReaderNode() {
	    delete reader_;
	}

	virtual void init(const std::vector<std::string> &arguments) {
	    Precursor::init(arguments);
	    reader_ = LatticeIo::getReader(config);
	    if (!reader_) {
		criticalError("LatticeReaderNode: Could not create reader instance");
		delete reader_; reader_ = 0;
	    }
	}

	virtual void sync() {
	    Precursor::sync();
	    l_.reset();
	}

	virtual ConstLatticeRef sendLattice(Port to) {
	    verify_(to == 0);
	    load();
	    return l_;
	}
    };
    NodeRef createLatticeReaderNode(const std::string &name, const Core::Configuration &config) {
	return NodeRef(new LatticeReaderNode(name, config));
    }
    // -------------------------------------------------------------------------


    // -------------------------------------------------------------------------
    class FsaReaderNode : public ReaderNode {
	typedef ReaderNode Precursor;
    public:
	static const Core::ParameterString paramName;
    private:
	ConstSemiringRef semiring_;
	Fsa::Type type_;
	Lexicon::AlphabetId inputAlphabetId_;
	Lexicon::AlphabetId outputAlphabetId_;
	Fsa::ConstAutomatonRef fsa_;

    protected:
	virtual bool read(const std::string &filename) {
	    fsa_ = Fsa::read(filename);
	    if (!fsa_)
		error("FsaReaderNode: Failed to read fsa \"%s\"",
			      filename.c_str());
	    else if (fsa_->type() != type_)
		error("FsaReaderNode: Expected type \"%s\", got \"%s\"",
		      Fsa::TypeChoice[Core::Choice::Value(type_)].c_str(),
		      Fsa::TypeChoice[Core::Choice::Value(fsa_->type())].c_str());
	    return true;
	}

    public:
	FsaReaderNode(const std::string &name, const Core::Configuration &config) :
	    Precursor(name, config) {
	    semiring_ = Semiring::create(Fsa::SemiringTypeTropical, 1);
	    semiring_->setKey(0, "weight");
	}
	virtual ~FsaReaderNode() {}

	virtual void init(const std::vector<std::string> &arguments) {
	    Precursor::init(arguments);
	    if (!dir().empty())
		warning("FsaReaderNode: If filenames have qualifiers the relative "
			"interpretation of the filename to \"%s\" might fail.",
			dir().c_str());
	    if (!paramName(select("alphabet")).empty()) {
		type_ = Fsa::TypeAcceptor;
		inputAlphabetId_ = outputAlphabetId_ = Lexicon::us()->alphabetId(paramName(select("alphabet")), true);
	    } else {
		type_ = Fsa::TypeTransducer;
		inputAlphabetId_ = Lexicon::us()->alphabetId(paramName(select("input-alphabet")), true);
		outputAlphabetId_ = Lexicon::us()->alphabetId(paramName(select("output-alphabet")), true);
	    }
	}

	virtual void sync() {
	    Precursor::sync();
	    fsa_.reset();
	}

	virtual ConstLatticeRef sendLattice(Port to) {
	    verify_(to == 0);
	    load();
	    return (fsa_) ? fromFsa(fsa_, semiring_, semiring_->one(), 0, inputAlphabetId_, outputAlphabetId_) : ConstLatticeRef();
	}

	virtual Fsa::ConstAutomatonRef sendFsa(Port to) {
	    verify_(to == 1);
	    load();
	    return fsa_;
	}
    };
    const Core::ParameterString FsaReaderNode::paramName(
	"name",
	"name",
	"lemma");

    NodeRef createFsaReaderNode(const std::string &name, const Core::Configuration &config) {
	return NodeRef(new FsaReaderNode(name, config));
    }
    // -------------------------------------------------------------------------


    // -------------------------------------------------------------------------
    class CtmReaderNode : public Node {
	friend class Network;
    public:
	static const Core::ParameterString paramPath;
	static const Core::ParameterString paramEncoding;
	static const Core::ParameterStringVector paramScoreKeys;
	static const Core::ParameterFloat paramDefaultScore;

    private:
	typedef Speech::TimeframeIndex Time;

	struct CtmEntry {
	    Fsa::LabelId label;
	    Time start, end;
	    ScoresRef scores;
	};
	struct CtmRecording : public Core::Vector<CtmEntry> {
	    const_iterator cursor;
	};
	typedef Core::hash_map<std::string, CtmRecording, Core::StringHash> CtmCorpus;

    private:
	Fsa::ConstAlphabetRef alphabet_;
	ConstSemiringRef semiring_;
	ScoresRef defaultScores_;
	ScoreIdList ids_;
	CtmCorpus corpus_;
	ConstLatticeRef l_;
	bool isValid_;

    protected:
	State * grow(StaticLattice *s, State *sp, ScoresRef scores, Fsa::LabelId label) {
	    State *nextSp = s->newState();
	    sp->newArc(nextSp->id(), scores, label);
	    return nextSp;
	}

	ConstLatticeRef build(const std::string &name, Time start, Time end) {
	    CtmCorpus::iterator it = corpus_.find(name);
	    if (it == corpus_.end())
		return ConstLatticeRef();
	    CtmRecording &rec = it->second;
	    StaticBoundaries *b = new StaticBoundaries;
	    StaticLattice *s = new StaticLattice;
	    s->setBoundaries(ConstBoundariesRef(b));
	    s->setDescription(
		Core::form("ctm(%s,%d-%d)", name.c_str(), start, end));
	    s->setType(Fsa::TypeAcceptor);
	    s->setProperties(Fsa::PropertyAcyclic | Fsa::PropertyLinear, Fsa::PropertyAll);
	    s->setInputAlphabet(alphabet_);
	    s->setSemiring(semiring_);
	    State *sp = s->newState();
	    s->setInitialStateId(sp->id());

	    Time lastEnd = start;
	    //if ((rec.cursor != rec.end()) && (rec.cursor->start > lastEnd)) {
	    //	rec.cursor = rec.begin();
	    for (; (rec.cursor != rec.end()) && (rec.cursor->end <= lastEnd); ++rec.cursor);
	    if ((rec.cursor != rec.end()) && (rec.cursor->start < start)) {
		b->set(sp->id(), Boundary(0));
		// rec.cursor->end - start > start - rec.cursor->start
		if (rec.cursor->start + rec.cursor->end > start + start) {
		    sp = grow(s, sp, rec.cursor->scores, rec.cursor->label);
		    lastEnd = rec.cursor->end;
		    ++rec.cursor;
		} else {
		    sp = grow(s, sp, semiring_->clone(defaultScores_), Fsa::Epsilon);
		    ++rec.cursor;
		    lastEnd = (rec.cursor != rec.end()) ? rec.cursor->start : end;
		}
	    }
	    for (; (rec.cursor != rec.end()) && (rec.cursor->end <= end); ++rec.cursor) {
		if (lastEnd < rec.cursor->start) {
		    b->set(sp->id(), Boundary(lastEnd - start));
		    sp = grow(s, sp, semiring_->clone(defaultScores_), Fsa::Epsilon);
		}
		b->set(sp->id(), Boundary(rec.cursor->start - start));
		sp = grow(s, sp, rec.cursor->scores, rec.cursor->label);
		lastEnd = rec.cursor->end;
	    }
	    if (lastEnd < end) {
		// end - rec.cursor->start >= rec.cursor->end - end
		if ((rec.cursor != rec.end()) && (rec.cursor->start < end) &&
		    (rec.cursor->start + rec.cursor->end <= end + end)) {
		    if (lastEnd < rec.cursor->start) {
			b->set(sp->id(), Boundary(lastEnd - start));
			sp = grow(s, sp, semiring_->clone(defaultScores_), Fsa::Epsilon);
		    }
		    b->set(sp->id(), Boundary(rec.cursor->start - start));
		    sp = grow(s, sp, rec.cursor->scores, rec.cursor->label);
		} else {
		    b->set(sp->id(), Boundary(lastEnd - start));
		    sp = grow(s, sp, semiring_->clone(defaultScores_), Fsa::Epsilon);
		}
	    }
	    b->set(sp->id(), Boundary(end - start));
	    sp->setFinal(semiring_->clone(semiring_->one()));
	    return ConstLatticeRef(s);
	}

	ScoresRef str2scores(const TextFileParser::StringList &cols) const {
	    ScoresRef scoresRef = semiring_->clone(defaultScores_);
	    if (cols.size() > 5) {
		Scores &scores = *scoresRef;
		ScoreIdList::const_iterator itId = ids_.begin();
		TextFileParser::StringList::const_iterator itCol = cols.begin() + 5;
		for (; (itId != ids_.end()) && (itCol != cols.end()); ++itId, ++itCol)
		    if (*itId != Semiring::InvalidId)
			if (!Core::strconv(*itCol, scores[*itId]))
			    criticalError("CtmReaderNode: Failed to convert score %s", itCol->c_str());
	    }
	    return scoresRef;
	}

	Time str2time(const std::string &s) const {
	    float sec(0);
	    if (!Core::strconv(s, sec))
		criticalError("CtmReaderNode: Failed to convert %s", s.c_str());
	    return Time(Core::round(sec * 100.0));
	}

	void load() {
	    LexiconRef lexicon = Lexicon::us();
	    Fsa::LabelId unkId = lexicon->unkLemmaId();
	    const std::string path = paramPath(config);
	    if (!Core::isValidPath(path))
		criticalError("CtmReaderNode: Could not find \"%s\"",
			      path.c_str());
	    CtmRecording emptyCtmRecording;
	    CtmEntry ctmEntry;
	    TextFileParser tf(path, paramEncoding(config));
	    if (!tf)
		criticalError("CtmReaderNode: Could not open \"%s\"",
			      path.c_str());
	    std::string lastName;
	    CtmCorpus::iterator it = corpus_.end();
	    u32 nWords = 0;
	    for (;; ++nWords) {
		const TextFileParser::StringList &cols = tf.next();
		if (!tf)
		    break;
		if (cols.size() < 5)
		    criticalError("CtmReaderNode: Require at least five columns per line.");
		ctmEntry.scores = str2scores(cols);
		ctmEntry.start = str2time(cols[2]);
		ctmEntry.end = ctmEntry.start + str2time(cols[3]);
		if (cols[4] == "@")
		    ctmEntry.label = Fsa::Epsilon;
		else {
		    ctmEntry.label = lexicon->lemmaId(cols[4]);
		    if (ctmEntry.label == Fsa::InvalidLabelId)
			ctmEntry.label = unkId;
		}
		if (lastName != cols[0]) {
		    lastName = cols[0];
		    it = corpus_.find(cols[0]);
		}
		if (it != corpus_.end()) {
		    if (ctmEntry.start < it->second.back().end) {
			if ((it->second.back().start < ctmEntry.start) && ((it->second.back().end - ctmEntry.start) <= 1)) {
			    it->second.back().end = ctmEntry.start;
			} else
			    criticalError("CtmReaderNode: Chronological order violated %s:%d",
					  path.c_str(), tf.currentLineNumber());
		    }
		} else
		    it = corpus_.insert(std::make_pair(cols[0], emptyCtmRecording)).first;
		it->second.push_back(ctmEntry);
	    }
	    for (CtmCorpus::iterator it = corpus_.begin(); it != corpus_.end(); ++it)
		it->second.cursor = it->second.begin();
	    log("%d words loaded from \"%s\"", nWords, path.c_str());
	}

    public:
	CtmReaderNode(const std::string &name, const Core::Configuration &config) :
	    Node(name, config) {}
	virtual ~CtmReaderNode() {}

	virtual void init(const std::vector<std::string> &arguments) {
	    if (!connected(1))
		criticalError("ReaderNode: Need data source at port 1.");
	    KeyList keys = paramScoreKeys(config);
	    semiring_ = Semiring::create(select("semiring"));
	    if (!semiring_) {
		// keys define semiring
		semiring_ = Semiring::create(
		    Fsa::SemiringTypeTropical, keys.size(), ScoreList(), keys);
		for (ScoreId id = 0; id < keys.size(); ++id) ids_.push_back(id);
	    } else {
		// keys and semiring are defined separately
		for (KeyList::const_iterator itKey = keys.begin(); itKey != keys.end(); ++itKey)
		    ids_.push_back(semiring_->id(*itKey));
	    }
	    defaultScores_ = semiring_->clone(semiring_->one());
	    ScoreIdList::const_iterator itId = ids_.begin();
	    KeyList::const_iterator itKey = keys.begin();
	    for (; itId != ids_.end(); ++itId, ++itKey)
		if (*itId != Semiring::InvalidId)
		    defaultScores_->set(*itId, paramDefaultScore(select(*itKey)));
	    alphabet_ = Lexicon::us()->lemmaAlphabet();
	    log("Semiring is \"%s\".\n"
		"Default scores are %s.\n"
		"Alphabet is \"%s\".",
		semiring_->name().c_str(),
		semiring_->describe(defaultScores_, Fsa::HintShowDetails | HintUnscaled).c_str(),
		Lexicon::us()->alphabetName(Lexicon::LemmaAlphabetId).c_str());

	    load();
	    isValid_ = false;
	}

	virtual void sync() {
	    isValid_ = false;
	    l_.reset();
	}

	virtual ConstLatticeRef sendLattice(Port to) {
	    verify_(to == 0);
	    if (!isValid_) {
		ConstSegmentRef segment = requestSegment(1);
		if (!segment->hasRecordingId() || !segment->hasStartTime() || !segment->hasEndTime())
		    criticalError("CtmReaderNode: Require recording id, start time, and end time.");
		l_ = build(segment->recordingId(), Time(Core::round(segment->startTime() * 100.0)), Time(Core::round(segment->endTime() * 100.0)));
		isValid_ = true;
	    }
	    return l_;
	}
    };
    const Core::ParameterString CtmReaderNode::paramPath(
	"path",
	"path to ctm-file",
	"");
    const Core::ParameterString CtmReaderNode::paramEncoding(
	"encoding",
	"encoding of ctm-file",
	"utf-8");
    const Core::ParameterStringVector CtmReaderNode::paramScoreKeys(
	"scores",
	"CTM file provides scores for the given keys",
	"");
    const Core::ParameterFloat CtmReaderNode::paramDefaultScore(
	"default",
	"default score",
	Semiring::One);

    NodeRef createCtmReaderNode(const std::string &name, const Core::Configuration &config) {
	return NodeRef(new CtmReaderNode(name, config));
    }
    // -------------------------------------------------------------------------


    // -------------------------------------------------------------------------
    class WriterNode : public FilterNode {
    public:
	static const Core::ParameterString paramDir;
	static const Core::ParameterString paramFile;
	static const Core::ParameterString paramPrefix;
	static const Core::ParameterString paramSuffix;
    private:
	std::string dir_;
	std::string prefix_;
	std::string suffix_;
	std::string filename_;
    protected:
	virtual ConstLatticeRef filter(ConstLatticeRef l) {
	    std::string filename;
	    if (connected(1) || connected(2)) {
		filename = Core::joinPaths(dir_, connected(1) ? requestSegment(1)->segmentIdOrDie() : requestString(2));
		if (!prefix_.empty())
		    filename = Core::joinPaths(
			Core::directoryName(filename),
			prefix_ + Core::baseName(filename));
		if (!suffix_.empty())
		    filename += suffix_;
	    } else
		filename = filename_;
	    if (!Core::createDirectory(Core::directoryName(filename)))
		criticalError("WriterNode: Could not create directory \"%s\"",
			      Core::directoryName(filename).c_str());
	    write(filename, l);
	    return l;
	}

	virtual void write(const std::string &filename, ConstLatticeRef l) = 0;

    public:
	WriterNode(const std::string &name, const Core::Configuration &config) :
	    FilterNode(name, config) {}

	virtual void init(const std::vector<std::string> &arguments) {
	    dir_ = paramDir(config);
	    if (!dir_.empty() && !Core::isDirectory(dir_))
		if (!Core::createDirectory(dir_))
		    criticalError("WriterNode: Could not create directory \"%s\"",
				  dir_.c_str());
	    if (connected(1) || connected(2)) {
		prefix_ = paramPrefix(config);
		suffix_ = paramSuffix(config);
	    } else {
		filename_ = paramFile(config);
		if (filename_.empty())
		    criticalError("WriterNode: Need either a data source at port 1, at port 2, or a valid filename");
		filename_ = Core::joinPaths(dir_, filename_);
	    }
	}
    };
    const Core::ParameterString WriterNode::paramDir(
	"path",
	"interpret file names relative to path",
	"");
    const Core::ParameterString WriterNode::paramFile(
	"file",
	"file",
	"");
    const Core::ParameterString WriterNode::paramPrefix(
	"prefix",
	"prefix for filename",
	"");
    const Core::ParameterString WriterNode::paramSuffix(
	"suffix",
	"suffix filename",
	"");
    // -------------------------------------------------------------------------


    // -------------------------------------------------------------------------
    class LatticeWriterNode : public WriterNode {
	typedef WriterNode Precursor;
    private:
	LatticeWriter *writer_;
    protected:
	virtual void write(const std::string &path, ConstLatticeRef l) {
	    writer_->write(l, path);
	}
    public:
	LatticeWriterNode(const std::string &name, const Core::Configuration &config) :
	    Precursor(name, config), writer_(0) {}
	virtual ~LatticeWriterNode() {
	    delete writer_;
	}
	virtual void init(const std::vector<std::string> &arguments) {
	    Precursor::init(arguments);
	    writer_ = LatticeIo::getWriter(config);
	    if (!writer_) {
		criticalError("LatticeWriterNode: Could not create writer instance");
		delete writer_; writer_ = 0;
	    }
	}
    };

    NodeRef createLatticeWriterNode(const std::string &name, const Core::Configuration &config) {
	return NodeRef(new LatticeWriterNode(name, config));
    }
    // -------------------------------------------------------------------------


    // -------------------------------------------------------------------------
    class DrawerNode : public WriterNode {
	typedef WriterNode Precursor;
    public:
	static const Core::Choice hintChoice;
	static const Core::ParameterStringVector paramOptions;
    private:
	Fsa::Hint hints_;
    protected:
	virtual void write(const std::string &path, ConstLatticeRef l) {
	    drawDot(l, path, hints_);
	}

    public:
	DrawerNode(const std::string &name, const Core::Configuration &config) :
	    Precursor(name, config) {
	    std::vector<std::string> options = paramOptions(config);
	    hints_ = Fsa::HintNone;
	    for (std::vector<std::string>::const_iterator it = options.begin(); it != options.end(); ++it) {
		Core::Choice::Value hint = hintChoice[*it];
		if (hint == Core::Choice::IllegalValue)
		    warning("DrawerNode: Unknown drawing option \"%s\"", it->c_str());
		else
		    hints_ |= hint;
	    }
	}
    };
    const Core::Choice DrawerNode::hintChoice(
	"details",     Fsa::HintShowDetails,
	"detailed",    Fsa::HintShowDetails,
	"best",        Fsa::HintMarkBest,
	"probability", Fsa::HintAsProbability,
	"unscaled",    HintUnscaled,
	Core::Choice::endMark());
    const Core::ParameterStringVector DrawerNode::paramOptions(
	"hints",
	"drawing options (detailed, best, probability, unscaled)",
	"");

    NodeRef createDrawerNode(const std::string &name, const Core::Configuration &config) {
	return NodeRef(new DrawerNode(name, config));
    }
    // -------------------------------------------------------------------------

} // namespace Flf

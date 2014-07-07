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
#include <Core/Directory.hh>
#include <Core/TextStream.hh>
#include <Core/XmlStream.hh>
#include <Flow/Cache.hh>
#include <Fsa/Output.hh>
#include <Fsa/Basic.hh>
#include <Fsa/Static.hh>
#include <Fsa/Storage.hh>

#include "FlfCore/Basic.hh"
#include "Archive.hh"
#include "ConfusionNetworkIo.hh"
#include "FlfIo.hh"
#include "HtkSlfIo.hh"
#include "Convert.hh"
#include "Copy.hh"
#include "Info.hh"
#include "Lexicon.hh"
#include "Map.hh"
#include "Rescore.hh"
#include "TimeframeConfusionNetworkIo.hh"

#if 1
namespace Search { namespace Wfst {
struct LatticeArchiveReader : public Flf::LatticeArchiveReader {
    LatticeArchiveReader(const Core::Configuration &c, const std::string &p) :
	Flf::LatticeArchiveReader(c, p) {}
    virtual ~LatticeArchiveReader() {}
    Flf::ConstLatticeRef get(const std::string&) { return Flf::ConstLatticeRef(); }
    std::string defaultSuffix() const { return ""; }
}; } }
#endif


namespace Flf {

    // -------------------------------------------------------------------------
    const Core::ParameterBool paramInfo(
	"info",
	"info",
	false);

    const Core::ParameterString Archive::paramPath(
	"path",
	"path to lattices");
    const Core::ParameterString Archive::paramFile(
	"file",
	"file",
	"");
    const Core::ParameterString Archive::paramSuffix(
	"suffix",
	"suffix",
	"");

    Archive::Archive(
	const Core::Configuration &config,
	const std::string &pathname) :
	Core::Component(config),
	archive(0), isClosed_(false), pathname_(pathname) {
	suffix_ = paramSuffix(config);
    }

    Archive::~Archive() {
	if (!isClosed_)
	    warning("Archive was not properly closed.");
	delete archive;
    }

    void Archive::close() {
	finalize();
	isClosed_ = true;
    }

    const std::string & Archive::path() const {
	return pathname_;
    }

    const std::string & Archive::suffix() const {
	if (suffix_.empty()) suffix_ = defaultSuffix();
	return suffix_;
    }

    bool Archive::hasFile(const std::string &file) const {
	verify_(archive);
	return archive->hasFile(file);
    }

    Archive::const_iterator Archive::files() const {
	verify(archive);
	return archive->files();
    }
    // -------------------------------------------------------------------------





    /**
     *
     * Lattices
     *
     **/
    // -------------------------------------------------------------------------
    LatticeArchiveReader * LatticeArchive::getReader(
	const Core::Configuration &config) {
	std::string pathname = paramPath(config);
	if (pathname.empty()) return 0;

	LatticeArchiveReader *archiveReader = 0;
	switch (paramLatticeFormat(config)) {
	case LatticeFormatFlf:
	    archiveReader = new FlfArchiveReader(Core::Configuration(config, "flf"), pathname);
	    break;
	case LatticeFormatHtkSlf:
	    archiveReader = new HtkSlfArchiveReader(Core::Configuration(config, "htk"), pathname);
	    break;
	case LatticeFormatOpenFst:
	    archiveReader = new Search::Wfst::LatticeArchiveReader(Core::Configuration(config, "openfst"), pathname);
	    break;
	default:
	    defect();
	}

	if (archiveReader->hasFatalErrors()) {
	    delete archiveReader;
	    return 0;
	}

	return archiveReader;
    }

    LatticeArchiveWriter * LatticeArchive::getWriter(
	const Core::Configuration &config) {
	std::string pathname = paramPath(config);
	if (pathname.empty()) return 0;

	LatticeArchiveWriter *archiveWriter = 0;
	switch (paramLatticeFormat(config)) {
	case LatticeFormatFlf:
	    archiveWriter = new FlfArchiveWriter(Core::Configuration(config, "flf"), pathname);
	    break;
	case LatticeFormatHtkSlf:
	    archiveWriter = new HtkSlfArchiveWriter(Core::Configuration(config, "htk"), pathname);
	    break;
	case LatticeFormatLatticeProcessor:
	    archiveWriter = new LatticeProcessorArchiveWriter(Core::Configuration(config, "lattice-processor"), pathname);
	    break;
	default:
	    defect();
	}

	if (archiveWriter->hasFatalErrors()) {
	    delete archiveWriter;
	    return 0;
	}

	return archiveWriter;
    }
    // -------------------------------------------------------------------------


    // -------------------------------------------------------------------------
    const Core::ParameterString LatticeArchiveReader::paramSemiring(
	"semiring-type",
	"semiring type",
	"tropical");
    const Core::ParameterString LatticeArchiveReader::paramKeys(
	"keys",
	"keys",
	"");
    const Core::ParameterFloat LatticeArchiveReader::paramScale(
	"scale",
	"scale",
	Semiring::UndefinedScale);

    LatticeArchiveReader::LatticeArchiveReader(
	const Core::Configuration &config,
	const std::string &pathname) :
	Precursor(config, pathname) {
	semiring_ = Semiring::create(select("semiring"));
    }
    LatticeArchiveReader::~LatticeArchiveReader() {}
    // -------------------------------------------------------------------------


    // -------------------------------------------------------------------------
    class LatticeArchiveReaderNode : public Node {
	typedef Node Precursor;
    private:
	LatticeArchiveReader *reader_;
	bool info_;
	ConstLatticeRef buffer_;
	bool isValid_;
    public:
	LatticeArchiveReaderNode(const std::string &name, const Core::Configuration &config) :
	    Precursor(name, config), reader_(0) {}
	virtual ~LatticeArchiveReaderNode() {}

	virtual void init(const std::vector<std::string> &arguments) {
	    if (!connected(1) && !connected(2))
		criticalError("Data source either at port 1 or port 2 required.");
	    std::string readerPath = Archive::paramPath(config);
	    reader_ = LatticeArchive::getReader(config);
	    if (!reader_ || reader_->hasFatalErrors())
		criticalError("Failed to open archive \"%s\"  for reading.", readerPath.c_str());
	    else
		log("Archive \"%s\" is open for reading.", readerPath.c_str());
	    info_ = paramInfo(config);
	    isValid_ = false;
	}

	virtual void sync() {
	    buffer_.reset();
	    isValid_ = false;
	}

	virtual void finalize() {

	    verify(reader_);

	    reader_->close();
	    delete reader_;
	    reader_ = 0;
	}

	virtual ConstLatticeRef sendLattice(Port to) {
	    if (!isValid_) {
		std::string id = connected(1) ? requestSegment(1)->segmentIdOrDie() : requestString(2);
		buffer_ = reader_->get(id);
		if (!buffer_)
		    warning("Could not get lattice for id \"%s\".", id.c_str());
		if(info_)
		    info(buffer_, log());
		isValid_ = true;
	    }
	    return buffer_;
	}
    };
    NodeRef createLatticeArchiveReaderNode(const std::string &name, const Core::Configuration &config) {
	return NodeRef(new LatticeArchiveReaderNode(name, config));
    }
    // -------------------------------------------------------------------------


    // -------------------------------------------------------------------------
    LatticeArchiveWriter::LatticeArchiveWriter(
	const Core::Configuration &config,
	const std::string &pathname) : Precursor(config, pathname) {}

    LatticeArchiveWriter::~LatticeArchiveWriter() {}
    // -------------------------------------------------------------------------

    // -------------------------------------------------------------------------
    class LatticeArchiveWriterNode : public Node {
	typedef Node Precursor;
    private:
	LatticeArchiveWriter *writer_;
	bool info_;
	ConstLatticeRef buffer_;
	bool isValid_;
    public:
	LatticeArchiveWriterNode(const std::string &name, const Core::Configuration &config) :
	    Precursor(name, config), writer_(0) {}
	virtual ~LatticeArchiveWriterNode() {}

	virtual void init(const std::vector<std::string> &arguments) {
	    if (!connected(0))
		criticalError("Data source at port 0 required.");
	    if (!connected(1) && !connected(2))
		criticalError("Data source either at port 1 or port 2 required.");
	    std::string writerPath = Archive::paramPath(config);

	    writer_ = LatticeArchive::getWriter(config);
	    if (!writer_ || writer_->hasFatalErrors())
		criticalError("Failed to open lattice archive \"%s\" for writing.", writerPath.c_str());
	    else
		log("Archive \"%s\" is open for writing.", writerPath.c_str());
	    info_ = paramInfo(config);
	    isValid_ = false;
	}

	virtual void sync() {
	    buffer_.reset();
	    isValid_ = false;
	}

	virtual void finalize() {
	    writer_->close();
	    delete writer_;
	    writer_ = 0;
	}

	virtual ConstLatticeRef sendLattice(Port to) {
	    verify(to == 0);
	    if (isValid_)
		return buffer_;
	    buffer_ = requestLattice(0);
	    std::string id = connected(1) ? requestSegment(1)->segmentIdOrDie() : requestString(2);
	    if (buffer_)
		writer_->store(id, buffer_);
	    if(info_)
		info(buffer_, log());
	    isValid_ = true;
	    return buffer_;
	}
    };
    NodeRef createLatticeArchiveWriterNode(const std::string &name, const Core::Configuration &config) {
	return NodeRef(new LatticeArchiveWriterNode(name, config));
    }
    // -------------------------------------------------------------------------


    /*
      deprecated
    */
    // -------------------------------------------------------------------------
    /*
      Lattice-Processor-Archive-Writer
    */
    const Core::ParameterFloat LatticeProcessorArchiveWriter::paramPronunciationScale(
	"pronunciation-scale",
	"pronunciation scale",
	Semiring::UndefinedScale);

    LatticeProcessorArchiveWriter::LatticeProcessorArchiveWriter(
	const Core::Configuration &config,
	const std::string &pathname) :
	Precursor(config, pathname) {
	pronunciationScale_ = paramPronunciationScale(config, Semiring::UndefinedScale);
	if (pronunciationScale_ == Semiring::UndefinedScale)
	    criticalError("You forgot to specify a pronunciation scale");
	archive = Core::Archive::create(config, pathname, Core::Archive::AccessModeWrite);
	if (!archive)
	    criticalError("Failed to open lattice archive \"%s\" for writing", pathname.c_str());
	Core::Ref<Fsa::StorageAutomaton> f = Core::ref(new Fsa::StaticAutomaton(Fsa::TypeAcceptor));
	f->setInputAlphabet(Lexicon::us()->lemmaPronunciationAlphabet());
	Core::ArchiveWriter os(*archive, "alphabets.binfsa.gz", true);
	if (!Fsa::write(f, "bin", os, Fsa::storeInputAlphabet))
	    error("Failed to store alphabet");
    }

    LatticeProcessorArchiveWriter::~LatticeProcessorArchiveWriter() {}

    void LatticeProcessorArchiveWriter::store(const std::string &id, ConstLatticeRef l) {
	ScoreId amId = l->semiring()->id("am");
	require(l->semiring()->hasId(amId));
	ScoreId lmId = l->semiring()->id("lm");
	require(l->semiring()->hasId(lmId));

	l = projectInput(mapInput(l, MapToLemmaPronunciation));
	if (l->semiring()->scale(amId) != Semiring::DefaultScale)
	    l = multiply(l, amId, l->semiring()->scale(amId), RescoreModeInPlaceCache);
	if (l->semiring()->scale(lmId) != Semiring::DefaultScale)
	    l = multiply(l, lmId, l->semiring()->scale(lmId), RescoreModeInPlaceCache);
	l = extendByPronunciationScore(l, Lexicon::us()->lemmaPronunciationAlphabet(), lmId, pronunciationScale_, RescoreModeInPlaceCache);
	l = persistent(l);

	{
	    Core::ArchiveWriter os(*archive, id + ".binfsa.gz", true);
	    if (!Fsa::write(toFsa(l, amId), "bin", os, Fsa::storeStates))
		error("Failed to store am scores");
	}
	{
	    Core::ArchiveWriter os(*archive, id + "-lm.binfsa.gz", true);
	    if (!Fsa::write(toFsa(l, lmId), "bin", os, Fsa::storeStates))
		error("Failed to store lm scores");
	}
	if (l->getBoundaries()->valid()) {
	    Core::ArchiveWriter os(*archive, id + ".binwb.gz", true);
	    if (!writeBoundariesBinary(l, os))
		error("Failed to store word boundaries");
	}
    }
    // -------------------------------------------------------------------------


    /**
     *
     * Confusion Networks
     *
     **/
    // -------------------------------------------------------------------------
    ConfusionNetworkArchive::ConfusionNetworkArchive(
	const Core::Configuration &config,
	const std::string &pathname,
	CnFormat format) :
	Precursor(config, pathname), format(format) {
	encoding = paramEncoding(config);
    }

    std::string ConfusionNetworkArchive::defaultSuffix() const {
	switch (format) {
	case CnFormatText:
	    return ".cn.txt.gz";
	case CnFormatXml:
	    return ".cn.xml.gz";
	default:
	    defect();
	    return "";
	}
    }

    ConfusionNetworkArchiveReader * ConfusionNetworkArchive::getReader(const Core::Configuration &config, CnFormat format) {
	std::string pathname = paramPath(config);
	if (pathname.empty()) return 0;
	return new ConfusionNetworkArchiveReader(config, pathname, format);
    }

    ConfusionNetworkArchiveWriter * ConfusionNetworkArchive::getWriter(const Core::Configuration &config, CnFormat format) {
	std::string pathname = paramPath(config);
	if (pathname.empty()) return 0;
	return new ConfusionNetworkArchiveWriter(config, pathname, format);
    }
    // -------------------------------------------------------------------------


    // -------------------------------------------------------------------------
    namespace {
	void copyStub(ConstConfusionNetworkRef from, ConfusionNetworkRef to) {
	    to->alphabet = from->alphabet;
	    to->semiring = from->semiring;
	    to->normalizedProperties = from->normalizedProperties;;
	    to->nBestAlignmentProperties = from->nBestAlignmentProperties;
	    to->oracleAlignmentProperties = from->oracleAlignmentProperties;
	}
    } // namespace

    ConfusionNetworkArchiveReader::ConfusionNetworkArchiveReader(
	const Core::Configuration &config,
	const std::string &pathname,
	CnFormat format) :
	Precursor(config, pathname, format) {
	if (format != CnFormatXml)
	    criticalError("Format \"%s\" is not supported yet.", choiceCnFormat[format].c_str());
	archive = Core::Archive::create(config, pathname, Core::Archive::AccessModeRead);
	if (!archive)
	    criticalError("Failed to open CN archive \"%s\" for reading", pathname.c_str());
	getHeader_ = true;
    }
    ConfusionNetworkArchiveReader::~ConfusionNetworkArchiveReader() {}

    ConstConfusionNetworkRef ConfusionNetworkArchiveReader::getHeader() {
	if (getHeader_) {
	    switch (format) {
	    case CnFormatXml: {
		InputStream is("header" + suffix(), archive);
		if (!(is))
		    criticalError("Bad input stream.");
		ConfusionNetworkRef cn = ConfusionNetworkRef(new ConfusionNetwork);
		header_ = readConfusionNetworkFromXml(is, cn); }
	    default:
		;
	    }
	    getHeader_ = false;
	}
	return header_;
    }

    ConstConfusionNetworkRef ConfusionNetworkArchiveReader::get(const std::string &id) {
	ConstConfusionNetworkRef header = getHeader();
	std::string filename = id + suffix();
	if (!archive->hasFile(filename)) {
	    warning("Could not find \"%s\".", filename.c_str());
	    return ConstConfusionNetworkRef();
	}
	ConfusionNetworkRef cn;
	if (header) {
	    cn = ConfusionNetworkRef(new ConfusionNetwork);
	    copyStub(header, cn);
	}
	InputStream is(filename, archive);
	if (!(is))
	    criticalError("Bad input stream.");
	switch (format) {
	case CnFormatXml:
	    return readConfusionNetworkFromXml(is, cn);
	default:
	    defect();
	    return ConstConfusionNetworkRef();
	}
    }
    // -------------------------------------------------------------------------


    // -------------------------------------------------------------------------
    ConfusionNetworkArchiveWriter::ConfusionNetworkArchiveWriter(
	const Core::Configuration &config,
	const std::string &pathname,
	CnFormat format) :
	Precursor(config, pathname, format) {
	archive = Core::Archive::create(config, pathname, Core::Archive::AccessModeWrite);
	if (!archive)
	    criticalError("Failed to open CN  archive \"%s\" for reading", pathname.c_str());
	margin = paramMargin(config);
	indentation = paramIndentation(config);
	storeHeader_ = true;
    }

    ConfusionNetworkArchiveWriter::~ConfusionNetworkArchiveWriter() {}

    void ConfusionNetworkArchiveWriter::storeHeader(ConstConfusionNetworkRef cn) {
	if (storeHeader_) {
	    switch (format) {
	    case CnFormatXml: {
		std::string filename = "header" + suffix();
		OutputStream os(filename, archive);
		if (!(os))
		    criticalError("Output stream is bad.");
		Core::TextOutputStream tos(os.steal());
		tos.setEncoding(encoding);
		tos.setMargin(margin);
		tos.setIndentation(indentation);
		Core::XmlWriter xml(tos);
		xml.generateFormattingHints();
		xml.putDeclaration();
		writeConfusionNetworkAsXml(xml, cn, ConstSegmentRef(), IoFlag::WriteHead); }
	    default:
		;
	    }
	    storeHeader_ = false;
	}
    }

    void ConfusionNetworkArchiveWriter::store(const std::string &id, ConstConfusionNetworkRef cn, ConstSegmentRef segment) {
	storeHeader(cn);
	OutputStream os(id + suffix(), archive);
	if (!(os))
	    criticalError("Output stream is bad.");
	switch (format) {
	case CnFormatText: {
	    Core::TextOutputStream tos(os.steal());
	    tos.setEncoding(encoding);
	    writeConfusionNetworkAsText(tos, cn, segment);
	    break; }
	case CnFormatXml: {
	    Core::TextOutputStream tos(os.steal());
	    tos.setEncoding(encoding);
	    tos.setMargin(margin);
	    tos.setIndentation(indentation);
	    Core::XmlWriter xml(tos);
	    xml.generateFormattingHints();
	    xml.putDeclaration();
	    writeConfusionNetworkAsXml(xml, cn, segment, IoFlag::WriteBody);
	    break; }
	default:
	    defect();
	}
    }
    // -------------------------------------------------------------------------


    // -------------------------------------------------------------------------
    class ConfusionNetworkArchiveReaderNode : public Node {
	typedef Node Precursor;
    private:
	ConfusionNetworkArchiveReader *archiveReader_;
	bool info_;
	ConstConfusionNetworkRef buffer_;
	bool isValid_;
    public:
	ConfusionNetworkArchiveReaderNode(const std::string &name, const Core::Configuration &config) :
	    Precursor(name, config), archiveReader_(0) {
	    isValid_ = false;
	}
	virtual ~ConfusionNetworkArchiveReaderNode() {}

	virtual void init(const std::vector<std::string> &arguments) {
	    if (!connected(1) && !connected(2))
		criticalError("Data source either at port 1 or port 2 required.");
	    IoFormat::CnFormat format = IoFormat::CnFormat(IoFormat::paramCnFormat(config));
	    std::string readerPath = Archive::paramPath(config);
	    archiveReader_ = ConfusionNetworkArchive::getReader(config, format);
	    if (!archiveReader_ || archiveReader_->hasFatalErrors())
		criticalError("Failed to open archive \"%s\" for reading.", readerPath.c_str());
	    else
		log("Archive \"%s\" is open for reading.", readerPath.c_str());
	    info_ = paramInfo(config);
	}

	virtual void sync() {
	    buffer_.reset();
	    isValid_ = false;
	}

	virtual void finalize() {
	    archiveReader_->close();
	    delete archiveReader_;
	    archiveReader_ = 0;
	}

	virtual ConstConfusionNetworkRef sendCn(Port to) {
	    if (!isValid_) {
		std::string id = connected(1) ? requestSegment(1)->segmentIdOrDie() : requestString(2);
		log("Read CN for id \"%s\".", id.c_str());
		buffer_ = archiveReader_->get(id);
		if (!buffer_)
		    warning("Could not get CN for id \"%s\".", id.c_str());
		isValid_ = true;
	    }
	    return buffer_;
	}
    };
    NodeRef createConfusionNetworkArchiveReaderNode(const std::string &name, const Core::Configuration &config) {
	return NodeRef(new ConfusionNetworkArchiveReaderNode(name, config));
    }
    // -------------------------------------------------------------------------


    // -------------------------------------------------------------------------
    class ConfusionNetworkArchiveWriterNode : public Node {
	typedef Node Precursor;
    private:
	ConfusionNetworkArchiveWriter *archiveWriter_;
	bool info_;
	ConstConfusionNetworkRef buffer_;
	bool isValid_;
    public:
	ConfusionNetworkArchiveWriterNode(const std::string &name, const Core::Configuration &config) :
	    Precursor(name, config), archiveWriter_(0) {}
	virtual ~ConfusionNetworkArchiveWriterNode() {}
	virtual void init(const std::vector<std::string> &arguments) {
	    if (!connected(0))
		criticalError("Data source at port 0 required.");
	    if (!connected(1) && !connected(2))
		criticalError("Data source either at port 1 or port 2 required.");
	    IoFormat::CnFormat format = IoFormat::CnFormat(IoFormat::paramCnFormat(config));
	    archiveWriter_ = ConfusionNetworkArchive::getWriter(config, format);
	    std::string writerPath = Archive::paramPath(config);
	    if (!archiveWriter_ || archiveWriter_->hasFatalErrors())
		criticalError("Failed to open lattice archive \"%s\" for writing.", writerPath.c_str());
	    else
		log("Archive \"%s\" is open for writing.", writerPath.c_str());
	    info_ = paramInfo(config);
	    isValid_ = false;
	}
	virtual void sync() {
	    buffer_.reset();
	    isValid_ = false;
	}
	virtual void finalize() {
	    archiveWriter_->close();
	    delete archiveWriter_;
	    archiveWriter_ = 0;
	}
	virtual ConstConfusionNetworkRef sendCn(Port to) {
	    verify(to == 0);
	    if (isValid_)
		return buffer_;
	    buffer_ = requestCn(0);
	    if (buffer_) {
		ConstSegmentRef segment;
		if (connected(1)) {
		    segment = requestSegment(1);
		} else {
		    Segment *sp = new Segment;
		    sp->setSegmentId(requestString(2));
		    segment = ConstSegmentRef(sp);
		}
		archiveWriter_->store(segment->segmentIdOrDie(), buffer_, segment);
	    }
	    isValid_ = true;
	    return buffer_;
	}
    };
    NodeRef createConfusionNetworkArchiveWriterNode(const std::string &name, const Core::Configuration &config) {
	return NodeRef(new ConfusionNetworkArchiveWriterNode(name, config));
    }
    // -------------------------------------------------------------------------



    /**
     *
     * Posterior Confusion Networks
     *
     **/
    // -------------------------------------------------------------------------
    PosteriorCnArchive::PosteriorCnArchive(
	const Core::Configuration &config,
	const std::string &pathname,
	PosteriorCnFormat format) :
	Precursor(config, pathname), format(format) {
	encoding = paramEncoding(config);
    }

    std::string PosteriorCnArchive::defaultSuffix() const {
	switch (format) {
	case PosteriorCnFormatText:
	    return ".fcn.txt.gz";
	case PosteriorCnFormatXml:
	    return ".fcn.xml.gz";
	default:
	    defect();
	    return "";
	}
    }

    PosteriorCnArchiveReader * PosteriorCnArchive::getReader(const Core::Configuration &config, PosteriorCnFormat format) {
	std::string pathname = paramPath(config);
	if (pathname.empty()) return 0;
	return new PosteriorCnArchiveReader(config, pathname, format);
    }

    PosteriorCnArchiveWriter * PosteriorCnArchive::getWriter(const Core::Configuration &config, PosteriorCnFormat format) {
	std::string pathname = paramPath(config);
	if (pathname.empty()) return 0;
	return new PosteriorCnArchiveWriter(config, pathname, format);
    }
    // -------------------------------------------------------------------------


    // -------------------------------------------------------------------------
    namespace {
	void copyStub(ConstPosteriorCnRef from, PosteriorCnRef to) {
	    to->alphabet = from->alphabet;
	}
    } // namespace

    PosteriorCnArchiveReader::PosteriorCnArchiveReader(
	const Core::Configuration &config,
	const std::string &pathname,
	PosteriorCnFormat format) :
	Precursor(config, pathname, format) {
	if (format != PosteriorCnFormatXml)
	    criticalError("Format \"%s\" is not supported yet.", choicePosteriorCnFormat[format].c_str());
	archive = Core::Archive::create(config, pathname, Core::Archive::AccessModeRead);
	if (!archive)
	    criticalError("Failed to open fCN archive \"%s\" for reading", pathname.c_str());
	getHeader_ = true;
    }
    PosteriorCnArchiveReader::~PosteriorCnArchiveReader() {}

    ConstPosteriorCnRef PosteriorCnArchiveReader::getHeader() {
	if (getHeader_) {
	    switch (format) {
	    case PosteriorCnFormatXml: {
		InputStream is("header" + suffix(), archive);
		if (!(is))
		    criticalError("Bad input stream.");
		PosteriorCnRef cn = PosteriorCnRef(new PosteriorCn);
		header_ = readPosteriorCnFromXml(is, cn);
		verify(header_); }
	    default:
		;
	    }
	    getHeader_ = false;
	}
	return header_;
    }

    ConstPosteriorCnRef PosteriorCnArchiveReader::get(const std::string &id) {
	ConstPosteriorCnRef header = getHeader();
	std::string filename = id + suffix();
	if (!archive->hasFile(filename)) {
	    warning("Could not find \"%s\".", filename.c_str());
	    return ConstPosteriorCnRef();
	}
	PosteriorCnRef cn;
	if (header) {
	    cn = PosteriorCnRef(new PosteriorCn);
	    copyStub(header, cn);
	}
	InputStream is(filename, archive);
	if (!(is))
	    criticalError("Bad input stream.");
	switch (format) {
	case PosteriorCnFormatXml:
	    return readPosteriorCnFromXml(is, cn);
	default:
	    defect();
	    return ConstPosteriorCnRef();
	}
    }
    // -------------------------------------------------------------------------


    // -------------------------------------------------------------------------
    PosteriorCnArchiveWriter::PosteriorCnArchiveWriter(
	const Core::Configuration &config,
	const std::string &pathname,
	PosteriorCnFormat format) :
	Precursor(config, pathname, format) {
	if ((format != PosteriorCnFormatXml) && (format != PosteriorCnFormatText))
	    criticalError("Format \"%s\" is not supported yet.", choicePosteriorCnFormat[format].c_str());
	archive = Core::Archive::create(config, pathname, Core::Archive::AccessModeWrite);
	if (!archive)
	    criticalError("Failed to open posterior CN archive \"%s\" for writing", pathname.c_str());
	margin = paramMargin(config);
	indentation = paramIndentation(config);
	storeHeader_ = true;
    }

    PosteriorCnArchiveWriter::~PosteriorCnArchiveWriter() {}

    void PosteriorCnArchiveWriter::storeHeader(ConstPosteriorCnRef cn) {
	if (storeHeader_) {
	    switch (format) {
	    case PosteriorCnFormatXml: {
		std::string filename = "header" + suffix();
		OutputStream os(filename, archive);
		if (!(os))
		    criticalError("Output stream is bad.");
		Core::TextOutputStream tos(os.steal());
		tos.setEncoding(encoding);
		tos.setMargin(margin);
		tos.setIndentation(indentation);
		Core::XmlWriter xml(tos);
		xml.generateFormattingHints();
		xml.putDeclaration();
		writePosteriorCnAsXml(xml, cn, ConstSegmentRef(), IoFlag::WriteHead);
		storeHeader_ = false; }
	    default:
		;
	    }
	    storeHeader_ = false;
	}
    }

    void PosteriorCnArchiveWriter::store(const std::string &id, ConstPosteriorCnRef cn, ConstSegmentRef segment) {
	storeHeader(cn);
	OutputStream os(id + suffix(), archive);
	if (!(os))
	    criticalError("Bad output stream.");
	switch (format) {
	case PosteriorCnFormatText: {
	    Core::TextOutputStream tos(os.steal());
	    tos.setEncoding(encoding);
	    writePosteriorCnAsText(tos, cn, segment);
	    break; }
	case PosteriorCnFormatXml: {
	    Core::TextOutputStream tos(os.steal());
	    tos.setEncoding(encoding);
	    tos.setMargin(margin);
	    tos.setIndentation(indentation);
	    Core::XmlWriter xml(tos);
	    xml.generateFormattingHints();
	    xml.putDeclaration();
	    writePosteriorCnAsXml(xml, cn, segment, IoFlag::WriteBody);
	    break; }
	default:
	    defect();
	}
    }
    // -------------------------------------------------------------------------


    // -------------------------------------------------------------------------
    class PosteriorCnArchiveReaderNode : public Node {
	typedef Node Precursor;
    private:
	PosteriorCnArchiveReader *archiveReader_;
	bool info_;
	ConstPosteriorCnRef buffer_;
	bool isValid_;
    public:
	PosteriorCnArchiveReaderNode(const std::string &name, const Core::Configuration &config) :
	    Precursor(name, config), archiveReader_(0) {
	    isValid_ = false;
	}
	virtual ~PosteriorCnArchiveReaderNode() {}

	virtual void init(const std::vector<std::string> &arguments) {
	    if (!connected(1) && !connected(2))
		criticalError("Data source either at port 1 or port 2 required.");
	    IoFormat::PosteriorCnFormat format = IoFormat::PosteriorCnFormat(IoFormat::paramPosteriorCnFormat(config));
	    std::string readerPath = Archive::paramPath(select("archive"));
	    archiveReader_ = PosteriorCnArchive::getReader(select("archive"), format);
	    if (!archiveReader_ || archiveReader_->hasFatalErrors())
		criticalError("Failed to open archive \"%s\" for reading.", readerPath.c_str());
	    else
		log("Archive \"%s\" is open for reading.", readerPath.c_str());
	    info_ = paramInfo(config);
	}

	virtual void sync() {
	    buffer_.reset();
	    isValid_ = false;
	}

	virtual void finalize() {
	    archiveReader_->close();
	    delete archiveReader_;
	    archiveReader_ = 0;
	}

	virtual ConstPosteriorCnRef sendPosteriorCn(Port to) {
	    if (!isValid_) {
		std::string id = connected(1) ? requestSegment(1)->segmentIdOrDie() : requestString(2);
		log("Read fCN for id \"%s\".", id.c_str());
		buffer_ = archiveReader_->get(id);
		if (!buffer_)
		    warning("Could not get fCN for id \"%s\".", id.c_str());
		isValid_ = true;
	    }
	    return buffer_;
	}
    };
    NodeRef createPosteriorCnArchiveReaderNode(const std::string &name, const Core::Configuration &config) {
	return NodeRef(new PosteriorCnArchiveReaderNode(name, config));
    }
    // -------------------------------------------------------------------------


    // -------------------------------------------------------------------------
    class PosteriorCnArchiveWriterNode : public Node {
	typedef Node Precursor;
    public:
	static const Core::ParameterFloat paramPruningThreshold;
    private:
	PosteriorCnArchiveWriter *archiveWriter_;
	Flow::Cache *flowCache_;
	bool info_;
	ConstPosteriorCnRef buffer_;
	bool isValid_;
    public:
	PosteriorCnArchiveWriterNode(const std::string &name, const Core::Configuration &config) :
	    Precursor(name, config), archiveWriter_(0), flowCache_(0) {}
	virtual ~PosteriorCnArchiveWriterNode() {
	    delete archiveWriter_;
	    delete flowCache_;
	}
	virtual void init(const std::vector<std::string> &arguments) {
	    if (!connected(0))
		criticalError("Data source at port 0 required.");
	    if (!connected(1) && !connected(2))
		criticalError("Data source either at port 1 or port 2 required.");
	    IoFormat::PosteriorCnFormat format = IoFormat::PosteriorCnFormat(IoFormat::paramPosteriorCnFormat(config));
	    switch (format) {
	    case IoFormat::PosteriorCnFormatText:
	    case IoFormat::PosteriorCnFormatXml: {
		std::string writerPath = Archive::paramPath(select("archive"));
		archiveWriter_ = PosteriorCnArchive::getWriter(select("archive"));
		if (!archiveWriter_ || archiveWriter_->hasFatalErrors())
		    criticalError("Failed to open lattice archive \"%s\" for writing.", writerPath.c_str());
		else
		    log("Archive \"%s\" is open for writing.", writerPath.c_str());
	    } break;
	    case IoFormat::PosteriorCnFormatFlowAlignment: {
		flowCache_ = new Flow::Cache(select("flow-cache"));
		flowCache_->open(Core::Archive::AccessModeWrite);
		if (!flowCache_->isOpen())
		    criticalError("Failed to open flow archive \"%s\".", flowCache_->path().c_str());
	    } break;
	    default:
		defect();
	    }
	    info_ = paramInfo(config);
	    isValid_ = false;
	}
	virtual void sync() {
	    buffer_.reset();
	    isValid_ = false;
	}
	virtual void finalize() {
	    if (archiveWriter_)
		archiveWriter_->close();
	    if (flowCache_)
		flowCache_->close();
	}
	virtual ConstPosteriorCnRef sendPosteriorCn(Port to) {
	    verify(to == 0);
	    if (isValid_)
		return buffer_;
	    buffer_ = requestPosteriorCn(0);
	    if (buffer_) {
		ConstSegmentRef segment;
		if (connected(1)) {
		    segment = requestSegment(1);
		} else {
		    Segment *sp = new Segment;
		    sp->setSegmentId(requestString(2));
		    segment = ConstSegmentRef(sp);
		}
		if (archiveWriter_)
		    archiveWriter_->store(segment->segmentIdOrDie(), buffer_, segment);
		else
		    writePosteriorCnAsFlowAlignment(*flowCache_, buffer_, segment);
	    }
	    isValid_ = true;
	    return buffer_;
	}
    };
    NodeRef createPosteriorCnArchiveWriterNode(const std::string &name, const Core::Configuration &config) {
	return NodeRef(new PosteriorCnArchiveWriterNode(name, config));
    }
    // -------------------------------------------------------------------------

} // namespace Flf

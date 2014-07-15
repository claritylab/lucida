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
#include <iomanip>
#include <sstream>

#include <Core/Application.hh>
#include <Core/Assertions.hh>
#include <Core/BinaryStream.hh>
#include <Core/Choice.hh>
#include <Core/Directory.hh>
#include <Core/IoUtilities.hh>
#include <Core/StringUtilities.hh>
#include <Core/XmlBuilder.hh>
#include <Core/XmlParser.hh>
#include <Fsa/Input.hh>
#include <Fsa/Output.hh>
#include <Fsa/Storage.hh>
#include <Fsa/Utility.hh>

#include "FlfCore/Basic.hh"
#include "Copy.hh"
#include "FlfIo.hh"
#include "Lexicon.hh"


namespace Flf {

    // -------------------------------------------------------------------------
    const Core::ParameterString FsaDescriptor::paramFormat(
	"format",
	"format",
	"");
    const Core::ParameterString FsaDescriptor::paramFile(
	"file",
	"file name",
	"");

    FsaDescriptor::FsaDescriptor(const std::string &qualifiedFile) {
	Fsa::QualifiedFilename qf(Fsa::splitQualifiedFilename(qualifiedFile));
	format = qf.first;
	file = qf.second;
    }

    FsaDescriptor::FsaDescriptor(const Core::Configuration &config) {
	format = paramFormat(config);
	file = paramFile(config);
    }

    void FsaDescriptor::clear() {
	format.clear();
	file.clear();
    }

    std::string FsaDescriptor::path(const std::string &root) const {
	if (file.empty()) return "";
	return Core::joinPaths(root, file);
    }

    std::string FsaDescriptor::qualifiedPath(const std::string &root) const {
	if (file.empty()) return "";
	std::string qualifiedFilename;
	if (!format.empty())
	    qualifiedFilename.append(format).append(":");
	qualifiedFilename.append(Core::joinPaths(root, file));
	return qualifiedFilename;
    }

    FsaDescriptor FsaDescriptor::joinPaths(const FsaDescriptor &desc, const std::string &path) {
	return FsaDescriptor(desc.format, Core::joinPaths(path, desc.path()));
    }
    // -------------------------------------------------------------------------


    // -------------------------------------------------------------------------
    std::string BoundariesDescriptor::operator() (const std::string &root) const {
	if (file.empty()) return "";
	return Core::joinPaths(root, file);
    }
    // -------------------------------------------------------------------------


    // -------------------------------------------------------------------------
    std::string FlfDescriptor::info() const {
	std::ostringstream oss;
	oss << "lattice descriptor:" << std::endl
	    << "  semiring type:         " << getSemiringTypeName(semiringType) << std::endl
	    << "  lattice root:          " << latticeRoot << std::endl
	    << "  structure file:        " << structureFile(latticeRoot) << std::endl;
	if (hasInputAlphabet) {
	    oss << "  input-alphabet name:   " << inputAlphabetName << std::endl;
	    if (inputAlphabetFile)
		oss << "  input-alphabet file:   " << inputAlphabetFile(latticeRoot) << std::endl;
	    else
		oss << "  no input-alphabet file" << std::endl;
	}
	else
	    oss << "  no input-alphabet" << std::endl;
	if (hasOutputAlphabet) {
	    oss << "  output-alphabet name:  " << outputAlphabetName << std::endl;
	    if (outputAlphabetFile)
		oss << "  output-alphabet file:   " << outputAlphabetFile(latticeRoot) << std::endl;
	    else
		oss << "  no output-alphabet file" << std::endl;
	}
	else
	    oss << "  no output-alphabet" << std::endl;
	if (hasBoundaries)
	    oss << "  boundaries file:       " << boundariesFile(latticeRoot) << std::endl;
	else
	    oss << "  no boundaries" << std::endl;
	oss << "  scores:" << std::endl;
	oss << "  " << std::setw(4)  << " " << "   "
	    << std::setw(16) << std::left << "key"    << "  "
	    << std::setw(5)  << std::left << "scale"  << "  "
	    << "fsa" << std::endl;
	KeyList::const_iterator itK = keys.begin();
	ScoreList::const_iterator itS = scales.begin();
	FsaDescriptorList::const_iterator itF = scoreFiles.begin();
	for (size_t id = 1; id <= scoreFiles.size(); ++id, ++itK, ++itS, ++itF) {
	    oss << "  " << std::setw(4) << std::right << id << ".  ";
	    oss << std::setw(16) << std::left << *itK << "  ";
	    if (*itS == Semiring::UndefinedScale)
		oss << "undef  ";
	    else
		oss << std::setw(5) << std::right << *itS << "  ";
	    if (*itF)
		oss << (*itF)(latticeRoot);
	    oss << std::endl;
	}
	return oss.str();
    }
    // -------------------------------------------------------------------------


    // -------------------------------------------------------------------------
    namespace {
	typedef std::set<Key> KeySet;

	void partial(FlfDescriptorRef desc, const KeySet &keys, bool onlyScores) {
	    FsaDescriptor alternativeStructureFile;
	    bool useAlternativeStructureFile = false;
	    KeyList::iterator itKey = desc->keys.begin();
	    ScoreList::iterator itScale = desc->scales.begin();
	    FsaDescriptorList::iterator itScoreFile = desc->scoreFiles.begin();
	    while (itKey != desc->keys.end())
		if (keys.find(*itKey) == keys.end()) {
		    if (*itScoreFile == desc->structureFile)
			useAlternativeStructureFile = true;
		    itKey = desc->keys.erase(itKey);
		    itScale = desc->scales.erase(itScale);
		    itScoreFile = desc->scoreFiles.erase(itScoreFile);
		} else {
		    if (!alternativeStructureFile)
			alternativeStructureFile = *itScoreFile;
		    ++itKey;
		    ++itScale;
		    ++itScoreFile;
		}
	    if (onlyScores) {
		desc->structureFile.clear();
		desc->boundariesFile.clear();
		desc->hasBoundaries = false;
		desc->inputAlphabetFile.clear();
		desc->hasInputAlphabet = false;
		desc->outputAlphabetFile.clear();
		desc->hasOutputAlphabet = false;
	    } else if (useAlternativeStructureFile && alternativeStructureFile)
		desc->structureFile = alternativeStructureFile;
	}

	void append(FlfDescriptorRef desc, const KeyList &keys, const ScoreList &scales) {
	    verify(keys.size() == scales.size());
	    KeyList::const_iterator itKey = keys.begin();
	    ScoreList::const_iterator itScale = scales.begin();
	    for (; itKey != keys.end(); ++itKey, ++itScale) {
		desc->keys.push_back(*itKey);
		desc->scales.push_back(*itScale);
		desc->scoreFiles.push_back(FsaDescriptor());
	    }
	}
    } // namespace
    // -------------------------------------------------------------------------


    // -------------------------------------------------------------------------
    FlfContext::FlfContext() {
	clear();
    }

    FlfContext::~FlfContext() {
	clear();
    }

    ConstSemiringRef FlfContext::semiring() const {
	return semiring_;
    }

    void FlfContext::setSemiring(ConstSemiringRef semiring) {
	semiring_ = semiring;
    }

    Fsa::Type FlfContext::fsaType() const {
	return fsaType_;
    }

    void FlfContext::setFsaType(Fsa::Type fsaType) {
	fsaType_ = fsaType;
    }

    Lexicon::AlphabetMapRef FlfContext::inputAlphabetMap() const {
	return inputAlphabetMap_;
    }

    Lexicon::AlphabetMapRef FlfContext::outputAlphabetMap() const {
	return outputAlphabetMap_;
    }

    namespace {
	std::pair<Fsa::ConstAlphabetRef, Fsa::ConstAlphabetRef> loadAlphabets(const FsaDescriptor &qf, Core::Archive *archive) {
	    if (!qf)
		return std::make_pair(Fsa::ConstAlphabetRef(), Fsa::ConstAlphabetRef());
	    Fsa::StaticAutomaton s;
	    s.setSemiring(Fsa::TropicalSemiring);
	    InputStream is(qf.file, archive);
	    if (!is || !Fsa::read(&s, qf.format, is)) {
		Core::Application::us()->criticalError(
		    "Failed to read fsa \"%s\"",
		    qf.file.c_str());
		return std::make_pair(Fsa::ConstAlphabetRef(), Fsa::ConstAlphabetRef());
	    }
	    return std::make_pair(s.getInputAlphabet(), s.getOutputAlphabet());
	}

	Lexicon::AlphabetMapRef mapAlphabet(Fsa::ConstAlphabetRef alphabet, const std::string &alphabetName) {
	    Lexicon::AlphabetId alphabetId = Lexicon::UnknownAlphabetId;
	    if (alphabetName.empty()) {
		Core::Application::us()->warning(
		    "No lexicon alphabet specified, assume lemma pronunciation alphabet.");
		alphabetId = Lexicon::LemmaPronunciationAlphabetId;
	    } else
		alphabetId = Lexicon::us()->alphabetId(alphabetName, true);
	    return Lexicon::us()->alphabetMap(alphabet, alphabetId);
	}
    } // namespace

    void FlfContext::setInputAlphabet(Fsa::ConstAlphabetRef alphabet, const std::string &alphabetName) {

	verify(alphabet);

	inputAlphabetMap_ = mapAlphabet(alphabet, alphabetName);
	fsaType_ = Fsa::TypeAcceptor;
    }

    void FlfContext::setOutputAlphabet(Fsa::ConstAlphabetRef alphabet, const std::string &alphabetName) {

	verify(alphabet);

	outputAlphabetMap_ = mapAlphabet(alphabet, alphabetName);
	fsaType_ = Fsa::TypeTransducer;
    }

    void FlfContext::loadInputAlphabet(const FsaDescriptor &alphabetQf, Core::Archive *archive, const std::string &alphabetName) {
	require(alphabetQf);
	std::pair<Fsa::ConstAlphabetRef, Fsa::ConstAlphabetRef> alphabets = loadAlphabets(alphabetQf, archive);
	Fsa::ConstAlphabetRef alphabet = (alphabets.first) ? alphabets.first : alphabets.second;
	if (!alphabet)
	    Core::Application::us()->criticalError(
		"Failed to read input alphabet from \"%s\"",
		alphabetQf.file.c_str());
	setInputAlphabet(alphabet, alphabetName);
    }

    void FlfContext::loadOutputAlphabet(const FsaDescriptor &alphabetQf, Core::Archive *archive, const std::string &alphabetName) {
	require(alphabetQf);
	std::pair<Fsa::ConstAlphabetRef, Fsa::ConstAlphabetRef> alphabets = loadAlphabets(alphabetQf, archive);
	Fsa::ConstAlphabetRef alphabet = (alphabets.second) ? alphabets.second : alphabets.first;
	if (!alphabet)
	    Core::Application::us()->criticalError(
		"Failed to read output alphabet from \"%s\"",
		alphabetQf.file.c_str());
	setOutputAlphabet(alphabet, alphabetName);
    }

    void FlfContext::overhaul(FlfDescriptorRef desc, const Fsa::StaticAutomaton &structureFsa) const {
	if (!semiring_)
	    Core::Application::us()->criticalError(
		"FlfContext: No semiring set");
	if (desc->scoreFiles.size() != semiring_->size())
	    Core::Application::us()->criticalError(
		"FlfContext: Semiring dimension mismatch, expected %zu, got %zu",
		desc->scoreFiles.size(),
		semiring_->size());
	if (fsaType_ == Fsa::TypeUnknown)
	    Core::Application::us()->criticalError(
		"FlfContext: No fsa type set");
	if (!inputAlphabetMap_)
	    Core::Application::us()->criticalError(
		"FlfContext: Missing input alphabet");
	if ((fsaType_ != Fsa::TypeAcceptor) && (!outputAlphabetMap_))
	    Core::Application::us()->criticalError(
		"FlfContext: Missing output alphabet in not-acceptor automaton");
    }

    void FlfContext::adapt(FlfDescriptorRef desc, const Fsa::StaticAutomaton &structureFsa, Core::Archive *archive) {
	// adapt semiring
	if (!semiring_) {
	    Fsa::SemiringType type = desc->semiringType;
	    if (type == Fsa::SemiringTypeUnknown)
		type = Fsa::getSemiringType(structureFsa.semiring());
	    if ((type != Fsa::SemiringTypeTropical) && (type != Fsa::SemiringTypeLog)) {
		Core::Application::us()->warning(
		    "FlfContext: No or unknown semiring type specified, assume tropical semiring");
		type = Fsa::SemiringTypeTropical;
	    }
	    setSemiring(Semiring::create(
			    type,
			    desc->scoreFiles.size(),
			    desc->scales,
			    desc->keys));
	    desc->keys = semiring_->keys();
	}
	// adapt alphabets => fsa type
	if (fsaType_ == Fsa::TypeUnknown) {
	    require(!inputAlphabetMap_ && !outputAlphabetMap_);
	    if ((bool)desc->inputAlphabetFile)
		loadInputAlphabet(
		    FsaDescriptor::joinPaths(desc->inputAlphabetFile, desc->latticeRoot),
		    archive,
		    desc->inputAlphabetName);
	    else
		setInputAlphabet(
		    structureFsa.getInputAlphabet(),
		    desc->inputAlphabetName);
	    if (structureFsa.type() != Fsa::TypeAcceptor) {
		if ((bool)desc->outputAlphabetFile) {
		    loadOutputAlphabet(
			FsaDescriptor::joinPaths(desc->outputAlphabetFile, desc->latticeRoot),
			archive,
			desc->outputAlphabetName);
		} else
		    setOutputAlphabet(
			structureFsa.getOutputAlphabet(),
			desc->outputAlphabetName);
	    }
	}
	overhaul(desc, structureFsa);
    }

    void FlfContext::update(FlfDescriptorRef desc, const Fsa::StaticAutomaton &structureFsa, Core::Archive *archive) {
	clear();
	adapt(desc, structureFsa, archive);
    }

    void FlfContext::clear() {
	semiring_.reset();
	fsaType_ = Fsa::TypeUnknown;
	inputAlphabetMap_.reset();
	outputAlphabetMap_.reset();
    }

    std::string FlfContext::info() const {
	std::ostringstream oss;
	oss << "lattice context:" << std::endl
	    << "  semiring:              " << ((semiring_) ? semiring_->name() : "") << std::endl
	    << "  fsa type:              " << Fsa::TypeChoice[fsaType_] << std::endl;
	if (inputAlphabetMap_)
	    oss << "  input alphabet name:   " << Lexicon::us()->alphabetName(Lexicon::us()->alphabetId(inputAlphabetMap_->to())) << std::endl;
	else
	    oss << "  no input alphabet" << std::endl;
	if (outputAlphabetMap_)
	    oss << "  output alphabet name:   " << Lexicon::us()->alphabetName(Lexicon::us()->alphabetId(outputAlphabetMap_->to())) << std::endl;
	else
	    oss << "  no output alphabet" << std::endl;
	return oss.str();
    }
    // -------------------------------------------------------------------------


    // -------------------------------------------------------------------------
    /*
      Flf Descriptor Parser
    */
    namespace {
	class FlfDescriptorXmlParser : public Core::XmlSchemaParser {
	    typedef FlfDescriptorXmlParser Self;
	    typedef Core::XmlSchemaParser Precursor;
	private:
	    FlfDescriptor *desc_;
	    size_t n_;
	    ScoreId id_;
	    s16 hasId_;
	    std::string cdata_;

	private:
	    void appendCdata(const char *ch, int len) {
		cdata_.append((char*)(ch), len);
	    }

	    void clearCdata(const Core::XmlAttributes atts) {
		cdata_.clear();
	    }

	    void lattice(const Core::XmlAttributes atts) {
		const char *c;
		c = atts["semiring"];
		if (c) {
		    if (*c != '\0') {
			desc_->semiringType = getSemiringType(std::string(c));
			if (desc_->semiringType == Fsa::SemiringTypeUnknown)
			    parser()->error("semiring must be of type \"log\" or \"tropical\" not  \"%s\"", c);
		    } else
			desc_->semiringType = Fsa::SemiringTypeUnknown;
		}
		c = atts["id"];
		if (c) {
		    desc_->id = c;
		}
	    }

	    bool setFsaDescriptor(const Core::XmlAttributes &atts, FsaDescriptor &fd) {
		const char *c;
		c = atts["file"];
		if (c) Core::strconv(std::string(c), fd.file);
		else fd.file.clear();
		c = atts["format"];
		if (c) Core::strconv(std::string(c), fd.format);
		else fd.format.clear();
		return !fd.file.empty();
	    }

	    void inputAlphabetFile(const Core::XmlAttributes atts) {
		desc_->hasInputAlphabet = true;
		setFsaDescriptor(atts, desc_->inputAlphabetFile);
		const char *c = atts["name"];
		if (c) Core::strconv(std::string(c), desc_->inputAlphabetName);
		else desc_->inputAlphabetName.clear();
	    }

	    void outputAlphabetFile(const Core::XmlAttributes atts) {
		desc_->hasOutputAlphabet = true;
		setFsaDescriptor(atts, desc_->outputAlphabetFile);
		const char *c = atts["name"];
		if (c) Core::strconv(std::string(c), desc_->outputAlphabetName);
		else desc_->outputAlphabetName.clear();
	    }

	    void boundariesFile(const Core::XmlAttributes atts) {
		desc_->hasBoundaries = true;
		const char *c = atts["file"];
		if (c) Core::strconv(std::string(c), desc_->boundariesFile.file);
		else desc_->boundariesFile.file.clear();
		if (desc_->boundariesFile.file.empty())
		    parser()->warning("boundaries-tag given, but no file specified");
	    }

	    void structureFile(const Core::XmlAttributes atts) {
		if (!setFsaDescriptor(atts, desc_->structureFile))
		    parser()->warning("structure-tag given, but no file specified");
	    }

	    void scores(const Core::XmlAttributes atts) {
		const char *c = atts["n"];
		if (c) {
		    if (!Core::strconv(std::string(c), n_))
			parser()->error("could not parse n: %s", cdata_.c_str());
		    verify(n_ >= 0);
		    desc_->keys.resize(n_);
		    desc_->scales.resize(n_, Semiring::UndefinedScale);
		    desc_->scoreFiles.resize(n_);
		}
	    }

	    void dim(const Core::XmlAttributes atts) {
		const char *c = atts["id"];
		if (hasId_ == -1) hasId_ = bool(c);
		if (c) {
		    if (!(hasId_))
			parser()->error("do not mix dim-tags with and w/o id-attribute");
		    if (!Core::strconv(std::string(c), id_))
			parser()->error("could not parse id: %s", cdata_.c_str());
		} else ++id_;
		if (n_ == 0) {
		    if (desc_->keys.size() <= id_) {
			desc_->keys.resize(id_ + 1);
			desc_->scales.resize(id_ + 1, Semiring::UndefinedScale);
			desc_->scoreFiles.resize(id_ + 1);
		    }
		} else if (size_t(id_) >= n_)
		    parser()->error("id %zu found, but dimensionality is set to %zu", id_, n_);
	    }

	    void name() {
		Core::strconv(cdata_, desc_->keys[id_]);
		if (desc_->keys[id_].empty())
		    parser()->warning("name-tag given, but no name specified");
		cdata_.clear();
	    }

	    void scale() {
		if (!Core::strconv(cdata_, desc_->scales[id_]))
		    parser()->error("could not parse scale[%zu]: %s", id_, cdata_.c_str());
		cdata_.clear();
	    }

	    void weightsFile(const Core::XmlAttributes atts) {
		if (!setFsaDescriptor(atts, desc_->scoreFiles[id_]))
		    parser()->warning("weights-tag given, but no file specified");
	    }

	public:
	    FlfDescriptorXmlParser(const Core::Configuration &config) :
		Precursor(config), desc_(0), n_(0), id_(0), hasId_(-1) {
		Core::XmlMixedElement *rootElement = new Core::XmlMixedElementRelay
		    ("lattice", this, startHandler(&Self::lattice), 0, 0,
		     XML_CHILD(new Core::XmlIgnoreElement(
				   "head", this)),
		     XML_CHILD(new Core::XmlEmptyElementRelay(
				   "alphabet", this,
				   startHandler(&Self::inputAlphabetFile))),
		     XML_CHILD(new Core::XmlEmptyElementRelay(
				   "input-alphabet", this,
				   startHandler(&Self::inputAlphabetFile))),
		     XML_CHILD(new Core::XmlEmptyElementRelay(
				   "output-alphabet", this,
				   startHandler(&Self::outputAlphabetFile))),
		     XML_CHILD(new Core::XmlEmptyElementRelay(
				   "boundaries", this,
				   startHandler(&Self::boundariesFile))),
		     XML_CHILD(new Core::XmlEmptyElementRelay(
				   "structure", this,
				   startHandler(&Self::structureFile))),
		     XML_CHILD(new Core::XmlMixedElementRelay(
				   "scores", this, startHandler(&Self::scores), 0, 0,
				   XML_CHILD(new Core::XmlMixedElementRelay(
						 "dim", this, startHandler(&Self::dim), 0, 0,
						 XML_CHILD(new Core::XmlMixedElementRelay(
							       "name", this, startHandler(&Self::clearCdata),
							       endHandler(&Self::name), charactersHandler(&Self::appendCdata),
							       XML_NO_MORE_CHILDREN)),
						 XML_CHILD(new Core::XmlMixedElementRelay(
							       "scale", this, startHandler(&Self::clearCdata),
							       endHandler(&Self::scale), charactersHandler(&Self::appendCdata),
							       XML_NO_MORE_CHILDREN)),
						 XML_CHILD(new Core::XmlEmptyElementRelay(
							       "fsa", this, startHandler(&Self::weightsFile))),
						 XML_NO_MORE_CHILDREN)),
				   XML_NO_MORE_CHILDREN)),
		     XML_NO_MORE_CHILDREN);
		setRoot(rootElement);
	    }

	    bool parse(FlfDescriptor *desc, std::istream &i) {
		desc_ = desc;
		verify(desc_);
		return (Precursor::parseStream(i) == 0);
	    }
	};
	FlfDescriptorXmlParser * parser_ = 0;
    } // namespace
    // -------------------------------------------------------------------------


    // -------------------------------------------------------------------------
    /*
      Flf-Reader
    */
    const u32 FlfReader::TrustContext     = 0;
    const u32 FlfReader::AdaptContext     = 1;
    // const u32 FlfReader::CheckContext     = 2;
    // const u32 FlfReader::ResetContext     = 4;
    const u32 FlfReader::UpdateContext    = 5;

    const Core::ParameterString FlfReader::paramContextMode(
	"context-mode",
	"context mode",
	"update");
    const Core::Choice ContextModeChoice = Core::Choice(
	"trust",  0,
	"adapt",  1,
	"update", 5,
	Core::Choice::endMark());

    const Core::ParameterStringVector FlfReader::paramKeys(
	"keys",
	"sequence of keys");
    const Core::ParameterFloat FlfReader::paramScale(
	"scale",
	"scale",
	Semiring::DefaultScale);

    FlfReader::FlfReader(const Core::Configuration &config) :
	Precursor(config), contextHandling_(UpdateContext) {
	context_ = FlfContextRef(new FlfContext());
	Core::Choice::Value contextHandling =
	    ContextModeChoice[paramContextMode(config)];
	if (contextHandling == Core::Choice::IllegalValue)
	    error("Unknown context mode \"%s\"",
		  paramContextMode(config).c_str());
	else
	    setContextHandling(contextHandling);
	init();
    }

    FlfReader::FlfReader(const Core::Configuration &config, FlfContextRef context, u32 contextHandling) :
	Precursor(config), contextHandling_(contextHandling), context_(context) {
	require(context_);
	init();
    }

    void FlfReader::init() {
	KeyList partialKeys = paramKeys(select("partial"));
	for (KeyList::const_iterator it = partialKeys.begin(); it != partialKeys.end(); ++it)
	    partialKeys_.insert(*it);
	const Core::Configuration appendConfig(config, "append");
	appendKeys_ = paramKeys(appendConfig);
	for (KeyList::const_iterator it = appendKeys_.begin(); it != appendKeys_.end(); ++it) {
	    const Core::Configuration appendKeyConfig(appendConfig, *it);
	    appendScales_.push_back(paramScale(appendKeyConfig));
	}
    }

    FlfReader::~FlfReader() {}

    FlfContextRef FlfReader::context() const {
	return context_;
    }

    void FlfReader::setContext(FlfContextRef context) {
	require(context);
	context_ = context;
    }

    void FlfReader::setContextHandling(u32 contextHandling) {
	contextHandling_ = contextHandling;
    }

    u32 FlfReader::contextHandling() const {
	return contextHandling_;
    }

    FlfDescriptorRef FlfReader::readDescriptor(const std::string &filename, Core::Archive *archive) {
	FlfDescriptor * desc = new FlfDescriptor();
	desc->latticeRoot = Core::directoryName(filename);
	if (!parser_)
	    parser_ = new FlfDescriptorXmlParser(getConfiguration());
	InputStream is(filename, archive);
	if (!is || !parser_->parse(desc, is)) {
	    error("Could not parse \"%s\"", filename.c_str());
	    delete desc;
	    return FlfDescriptorRef();
	}
	return FlfDescriptorRef(desc);
    }

    void FlfReader::conform(FlfDescriptorRef desc, const Fsa::StaticAutomaton &structureFsa, Core::Archive *archive) {
	switch (contextHandling_) {
	case TrustContext:
	    context_->overhaul(desc, structureFsa);
	    break;
	case AdaptContext:
	    context_->adapt(desc, structureFsa, archive);
	    break;
	case UpdateContext:
	    context_->update(desc, structureFsa, archive);
	    break;
	default:
	    defect();
	}
    }

    void FlfReader::addStructure(StaticLattice &l, const Fsa::StaticAutomaton &f) {
	if (f.maxStateId() == Fsa::InvalidStateId)
	    return;
	ConstSemiringRef semiring = l.semiring();
	Lexicon::AlphabetMap &mapInputLabel = *context_->inputAlphabetMap();
	Lexicon::AlphabetMap &mapOutputLabel = (context_->fsaType() == Fsa::TypeAcceptor) ?
	    *context_->inputAlphabetMap() : *context_->outputAlphabetMap();
	for (Fsa::StateId fsaSid = 0; fsaSid <= f.maxStateId(); ++fsaSid) {
	    const Fsa::State *fsaSp = f.fastState(fsaSid);
	    if (fsaSp) {
		State *sp = new State(fsaSp->id(), fsaSp->tags(),
				      fsaSp->isFinal() ?
				      semiring->clone(semiring->defaultWeight()) :
				      semiring->one());
		for (Fsa::State::const_iterator fsaA = fsaSp->begin(); fsaA != fsaSp->end(); ++fsaA) {
		    /*
		    if (fsaA->input() == Fsa::InvalidLabelId)
			Core::Application::us()->warning("invalid label id");
		    */
		    sp->newArc(
			fsaA->target(),
			semiring->clone(semiring->defaultWeight()),
			mapInputLabel[fsaA->input()], mapOutputLabel[fsaA->output()]);
		}
		sp->minimize();
		l.setState(sp);
	    }
	}
	l.setInitialStateId(f.initialStateId());
    }

    void FlfReader::addScores(StaticLattice &l, ScoreId id, const Fsa::StaticAutomaton &f) {
	require(l.size() == f.size());
	for (Fsa::StateId fsaSid = 0; fsaSid < f.size(); ++fsaSid) {
	    const Fsa::State *fsaSp(f.fastState(fsaSid));
	    if (fsaSp) {
		const Fsa::State *fsaSp(f.fastState(fsaSid));
		State *sp(l.fastState(fsaSid));
		require(sp);
		if (fsaSp->isFinal()) {
		    require(sp->isFinal());
		    sp->weight_->set(id, Flf::Score(fsaSp->weight()));
		}
		require(sp->nArcs() == fsaSp->nArcs());
		State::const_iterator a = sp->begin();
		for (Fsa::State::const_iterator fsaA = fsaSp->begin(); fsaA != fsaSp->end(); ++fsaA, ++a) {
		    require(a->target() == fsaA->target());
		    a->setScore(id, Flf::Score(fsaA->weight()));
		}
	    }
	}
    }

    ConstLatticeRef FlfReader::read(FlfDescriptorRef desc, Core::Archive *archive) {
	require(desc);
	if (!partialKeys_.empty())
	    partial(desc, partialKeys_, false);
	if (!appendKeys_.empty())
	    append(desc, appendKeys_, appendScales_);

	if (!desc->structureFile) {
	    warning("No structure file specified");
	    return ConstLatticeRef();
	}
	Fsa::StaticAutomaton structureFsa;
	structureFsa.setSemiring(Fsa::TropicalSemiring);
	structureFsa.setType(context_->fsaType());
	if (context_->fsaType() == Fsa::TypeAcceptor)
	    structureFsa.setInputAlphabet(context_->inputAlphabetMap()->from());
	if (context_->fsaType() == Fsa::TypeTransducer)
	    structureFsa.setOutputAlphabet(context_->outputAlphabetMap()->from());
	{
	    std::string filename = Core::joinPaths(desc->latticeRoot, desc->structureFile.file);
	    InputStream is(filename, archive);
	    if (!is || !Fsa::read(&structureFsa, desc->structureFile.format, is)) {
		warning("Could not read structure from fsa \"%s\"", filename.c_str());
		return ConstLatticeRef();
	    }
	    if (!structureFsa.hasProperty(Fsa::PropertyAcyclic))
		warning("Assume \"%s\" to be acyclic",
			filename.c_str());
	}
	conform(desc, structureFsa, archive);

	log() << desc->info() << context()->info();

	StaticLattice *lat = new StaticLattice("static-lattice", context_->fsaType());
	ConstLatticeRef l(lat);
	if (!desc->id.empty()) lat->setDescription(desc->id);
	lat->setSemiring(context_->semiring());
	lat->setInputAlphabet(context_->inputAlphabetMap()->to());
	if (context_->fsaType() != Fsa::TypeAcceptor)
	    lat->setOutputAlphabet(context_->outputAlphabetMap()->to());
	lat->addProperties(Fsa::PropertyAcyclic | PropertyCrossWord);
	addStructure(*lat, structureFsa);
	{
	    Fsa::StaticAutomaton scoresFsa;
	    scoresFsa.setSemiring(Fsa::TropicalSemiring);
	    scoresFsa.setType(context_->fsaType());
	    if (context_->fsaType() == Fsa::TypeAcceptor)
		scoresFsa.setInputAlphabet(context_->inputAlphabetMap()->from());
	    if (context_->fsaType() == Fsa::TypeTransducer)
		scoresFsa.setOutputAlphabet(context_->outputAlphabetMap()->from());
	    for (ScoreId id = 0; id < ScoreId(desc->scoreFiles.size()); ++id)
		if ((bool)desc->scoreFiles[id]) {
		    if (desc->scoreFiles[id] == desc->structureFile)
			addScores(*lat, id, structureFsa);
		    else {
			scoresFsa.setSemiring(structureFsa.semiring());
			std::string filename = Core::joinPaths(desc->latticeRoot, desc->scoreFiles[id].file);
			InputStream is(filename, archive);
			if (!is || !Fsa::read(&scoresFsa, desc->scoreFiles[id].format, is))
			    warning("Could not read scores from fsa \"%s\"", filename.c_str());
			else
			    addScores(*lat, id, scoresFsa);
			scoresFsa.clear();
		    }
		}
	}
	if ((bool)desc->boundariesFile) {
	    std::string filename = desc->boundariesFile(desc->latticeRoot);
	    InputStream is(filename, archive);
	    if (!is || !readBoundariesBinary(l, is)) {
		warning("Could not read boundaries from \"%s\"", filename.c_str());
		l->setBoundaries(InvalidBoundaries);
	    }
	}
	return l;
    }

    ConstLatticeRef FlfReader::read(const std::string &filename, Core::Archive *archive) {
	FlfDescriptorRef desc = readDescriptor(filename, archive);
	return read(desc, archive);
    }
    // -------------------------------------------------------------------------


    // -------------------------------------------------------------------------
    /*
      Flf-Writer
    */
    const Core::ParameterStringVector FlfWriter::paramKeys(
	"keys",
	"sequence of keys");
    const Core::ParameterBool FlfWriter::paramAdd(
	"add",
	"add selected dimensions to existing lattice, "
	"i.e. do not store alphabets, boundaries, nor structure.",
	false);

    FlfWriter::FlfWriter(const Core::Configuration &config) :
	Precursor(config) {
	KeyList partialKeys = paramKeys(select("partial"));
	if (!partialKeys.empty()) {
	    partial_ = true;
	    for (KeyList::const_iterator it = partialKeys.begin(); it != partialKeys.end(); ++it)
		partialKeys_.insert(*it);
	    partialAdd_ = paramAdd(select("partial"));
	} else
	    partial_ = partialAdd_ = false;
    }

    FlfWriter::~FlfWriter() {}

    FlfDescriptorRef FlfWriter::buildDescriptor(
	ConstLatticeRef f,
	const std::string &descFilename) const {
	FlfDescriptor *desc = new FlfDescriptor;
	std::string basename = Core::baseName(descFilename);
	std::string suffix = "";
	if (basename == "-") {
	    warning("you try to write lattice in fsa-based format to standard out");
	    basename = "stdout";
	} else {
	    if (basename.find(".gz", basename.size() - 3) != std::string::npos) {
		basename.erase(basename.size() - 3);
		suffix = ".gz";
	    }
	}
	basename = Core::stripExtension(basename);
	desc->latticeRoot = Core::directoryName(descFilename);
	desc->semiringType = f->semiring()->type();
	desc->hasInputAlphabet = true;
	desc->inputAlphabetName = Lexicon::us()->alphabetName(
	    Lexicon::us()->alphabetId(f->getInputAlphabet()));
	desc->inputAlphabetFile = FsaDescriptor(
	    "bin", Core::form("%s-input-alphabet.binfsa%s",
			      basename.c_str(), suffix.c_str()));
	if (f->type() != Fsa::TypeAcceptor) {
	    desc->hasOutputAlphabet = true;
	    desc->outputAlphabetName = Lexicon::us()->alphabetName(
		Lexicon::us()->alphabetId(f->getOutputAlphabet()));
	    desc->outputAlphabetFile = FsaDescriptor(
		"bin", Core::form("%s-output-alphabet.binfsa%s",
				  basename.c_str(), suffix.c_str()));
	} else
	    desc->hasOutputAlphabet = false;
	if (f->getBoundaries()->valid()) {
	    desc->hasBoundaries = true;
	    desc->boundariesFile = BoundariesDescriptor(
		Core::form("%s.binwb%s",
			   basename.c_str(), suffix.c_str()));
	} else
	    desc->hasBoundaries = false;
	desc->keys = f->semiring()->keys();
	desc->scales = f->semiring()->scales();
	desc->scoreFiles.resize(f->semiring()->size());
	if (desc->scoreFiles.empty()) {
	    desc->structureFile.format = "bin";
	    desc->structureFile.file = Core::form(
		"%s.binfsa%s", basename.c_str(), suffix.c_str());
	} else {
	    KeyList::const_iterator itKey = desc->keys.begin();
	    for (u32 i = 0; i < desc->scoreFiles.size(); ++i, ++itKey) {
		desc->scoreFiles[i].format = "bin";
		if (itKey->empty())
		    desc->scoreFiles[i].file = Core::form(
			"%s-dim%d.binfsa%s", basename.c_str(), i, suffix.c_str());
		else
		    desc->scoreFiles[i].file = Core::form(
			"%s-%s.binfsa%s", basename.c_str(), itKey->c_str(), suffix.c_str());
	    }
	    desc->structureFile = desc->scoreFiles[0];
	}
	return FlfDescriptorRef(desc);
    }

    bool FlfWriter::writeDescriptor(
	FlfDescriptorRef desc,
	const std::string &descFilename,
	Core::Archive *archive) const {
	require(!descFilename.empty() && (bool)desc->structureFile);
	OutputStream os(descFilename, archive);
	Core::XmlOutputStream xml(os.steal());
	xml.generateFormattingHints(true);
	xml.setIndentation(2);
	{
	    Core::XmlOpen openLattice("lattice");
	    openLattice + Core::XmlAttribute("semiring", getSemiringTypeName(desc->semiringType));
	    if (!desc->id.empty())
		openLattice + Core::XmlAttribute("id", desc->id);
	    xml << openLattice;
	}
	std::ostringstream oss; oss << Core::timestamp;
	xml << Core::XmlOpen("head")
	    << Core::XmlFull("creator", "RWTH-i6")
	    << Core::XmlFull("timestamp", oss.str())
	    << Core::XmlClose("head");
	xml << Core::XmlEmpty("structure")
	    + Core::XmlAttribute("format", desc->structureFile.format)
	    + Core::XmlAttribute("file", desc->structureFile.file);
	if (desc->hasInputAlphabet)
	    xml << Core::XmlEmpty( (desc->hasOutputAlphabet ? "input-alphabet" : "alphabet") )
		+ Core::XmlAttribute("name", desc->inputAlphabetName)
		+ Core::XmlAttribute("format", desc->inputAlphabetFile.format)
		+ Core::XmlAttribute("file", desc->inputAlphabetFile.file);
	if (desc->hasOutputAlphabet)
	    xml << Core::XmlEmpty("output-alphabet")
		+ Core::XmlAttribute("name", desc->outputAlphabetName)
		+ Core::XmlAttribute("format", desc->outputAlphabetFile.format)
		+ Core::XmlAttribute("file", desc->outputAlphabetFile.file);
	if (desc->hasBoundaries)
	    xml << Core::XmlEmpty("boundaries")
		+ Core::XmlAttribute("format", "bin")
		+ Core::XmlAttribute("file", desc->boundariesFile.file);
	xml << Core::XmlOpen("scores")
	    + Core::XmlAttribute("n", desc->scoreFiles.size());
	KeyList::const_iterator keyIt = desc->keys.begin();
	ScoreList::const_iterator scaleIt = desc->scales.begin();
	FsaDescriptorList::const_iterator fsaIt = desc->scoreFiles.begin();
	for (ScoreId id = 0; id < desc->scoreFiles.size(); ++id, ++keyIt, ++scaleIt, ++fsaIt) {
	    xml << Core::XmlOpen("dim")
		+ Core::XmlAttribute("id", id);
	    if (!keyIt->empty())
		xml << Core::XmlFull("name", *keyIt);
	    if ((*scaleIt != Semiring::UndefinedScale)
		&& (*scaleIt != Semiring::DefaultScale))
		xml << Core::XmlFull("scale", *scaleIt);
	    if (*fsaIt)
		xml << Core::XmlEmpty("fsa")
		    + Core::XmlAttribute("format", fsaIt->format)
		    + Core::XmlAttribute("file", fsaIt->file);
	    xml << Core::XmlClose("dim");
	}
	xml << Core::XmlClose("scores")
	    << Core::XmlClose("lattice");
	return xml;
    }

    bool FlfWriter::write(
	ConstLatticeRef f,
	FlfDescriptorRef desc,
	const std::string &descFilename,
	bool storeAlphabet,
	Core::Archive *archive) const
    {
	verify(desc);
	ConstSemiringRef semiring = f->semiring();
	if (partialAdd_) {
	    if (!writeDescriptor(desc, descFilename, archive))
		return false;
	    partial(desc, partialKeys_, true);
	} else {
	    if (partial_)
		partial(desc, partialKeys_, false);
	    if (!writeDescriptor(desc, descFilename, archive))
		return false;
	}
	f = persistent(f);
	if (desc->hasInputAlphabet && storeAlphabet) {
	    OutputStream os(Core::joinPaths(desc->latticeRoot, desc->inputAlphabetFile.file), archive);
	    if (!Fsa::write(toUnweightedFsa(f, Fsa::Weight(Semiring::One)), desc->inputAlphabetFile.format, os, Fsa::storeInputAlphabet))
		return false;
	}
	if (desc->hasOutputAlphabet && storeAlphabet) {
	    OutputStream os(Core::joinPaths(desc->latticeRoot, desc->outputAlphabetFile.file), archive);
	    if (!Fsa::write(toUnweightedFsa(f, Fsa::Weight(Semiring::One)), desc->outputAlphabetFile.format, os, Fsa::storeOutputAlphabet))
		return false;
	}
	if (desc->hasBoundaries && (bool)desc->boundariesFile) {
	    OutputStream os(Core::joinPaths(desc->latticeRoot, desc->boundariesFile.file), archive);
	    if (!writeBoundariesBinary(f, os))
		return false;
	}
	bool embeddedStructure = false;
	FsaDescriptorList::const_iterator itScoreFile = desc->scoreFiles.begin();
	for (KeyList::const_iterator itKey = desc->keys.begin(), endKey = desc->keys.end();
	     itKey != endKey; ++itKey, ++itScoreFile) if (*itScoreFile) {
		ScoreId dim = semiring->id(*itKey);
		verify(semiring->hasId(dim));
		OutputStream os(Core::joinPaths(desc->latticeRoot, itScoreFile->file), archive);
		if (!Fsa::write(toFsa(f, dim), itScoreFile->format, os, Fsa::storeStates))
		    return false;
		if (*itScoreFile == desc->structureFile)
		    embeddedStructure = true;
	    }
	if (!embeddedStructure && (bool)desc->structureFile) {
	    OutputStream os(Core::joinPaths(desc->latticeRoot, desc->structureFile.file), archive);
	    if (!Fsa::write(toUnweightedFsa(f, Fsa::Weight(Semiring::One)), desc->structureFile.format, os, Fsa::storeStates))
		return false;
	}
	return true;
    }

    bool FlfWriter::write(
	ConstLatticeRef f,
	const std::string &descFilename,
	bool storeAlphabet,
	Core::Archive *archive) const {
	return write(f, buildDescriptor(f, descFilename), descFilename, storeAlphabet, archive);
    }
    // -------------------------------------------------------------------------


    // -------------------------------------------------------------------------
    /*
      Flf-Archive-Reader
    */
    const Core::ParameterString FlfArchiveReader::paramLatticeType(
	"lattice-type",
	"lattice type",
	"");
    const Core::ParameterString FlfArchiveReader::paramName(
	"name",
	"name",
	"");

    FlfArchiveReader::FlfArchiveReader(
	const Core::Configuration &config,
	const std::string &pathname) :
	Precursor(config, pathname) {
	archive = Core::Archive::create(config, pathname, Core::Archive::AccessModeRead);
	if (!archive)
	    criticalError("Failed to open lattice archive \"%s\" for reading", pathname.c_str());
	FlfContextRef context = FlfContextRef(new FlfContext());

	// set semiring, if specified
	context->setSemiring(semiring());
	// get adapting reader, i.e. missing context is completed by lattice
	reader_ = new FlfReader(config, context, FlfReader::AdaptContext);

	Core::Component::Message msg(log());
	if (semiring()) {
	    bool discardFlfHeader = false;
	    discardFlfHeader = !(paramSuffix(select("boundaries")).empty()
				 && paramSuffix(select("structure")).empty());
	    for (KeyList::const_iterator itKey = semiring()->keys().begin();
		 !discardFlfHeader && (itKey != semiring()->keys().end()); ++itKey)
		discardFlfHeader = !paramSuffix(select(*itKey)).empty();
	    if (discardFlfHeader) {
		setDescriptor(config, context);
		msg << "Ignore flf-headers; build lattices from set of fsas:\n";
		setFsas("[id]");
		msg << desc_->info() << "\n";
	    }
	}
	msg << "Use the following predefined context for reading lattices:\n"
	    << context->info().c_str() << "\n";
    }

    FlfArchiveReader::~FlfArchiveReader() {
	delete reader_;
    }

    void FlfArchiveReader::setDescriptor(const Core::Configuration &config, FlfContextRef context) {
	ConstSemiringRef _semiring = semiring();
	require(_semiring);

	// get suffixes for boundaries, structure fsa, and score fsas
	boundariesSuffix_ = paramSuffix(select("boundaries"));
	suffixes_.resize(_semiring->size());
	const Core::Configuration structureConfig(config, "structure");
	structureSuffix_ = paramSuffix(structureConfig, "");
	std::string structureFormat = FsaDescriptor::paramFormat(structureConfig, "bin");
	StringList formats(_semiring->size());
	StringList::iterator itSuffix = suffixes_.begin();
	StringList::iterator itFormat = formats.begin();
	for (KeyList::const_iterator itKey = _semiring->keys().begin();
	     itKey != _semiring->keys().end(); ++itKey, ++itSuffix, ++itFormat) {
	    const Core::Configuration keyConfig(config, *itKey);
	    *itSuffix = paramSuffix(keyConfig, "");
	    *itFormat = FsaDescriptor::paramFormat(keyConfig, "bin");
	    if (structureSuffix_.empty() && !itSuffix->empty()) {
		structureSuffix_ = *itSuffix;
		structureFormat = *itFormat;
	    }
	}
	if (structureSuffix_.empty())
	    criticalError("Discard flf-header, but no suffix for structure file given.");

	// create flf descriptor
	desc_ = FlfDescriptorRef(new FlfDescriptor());
	desc_->semiringType = _semiring->type();
	desc_->scales.resize(_semiring->size());
	ScoreList::iterator itDescScale = desc_->scales.begin();
	for (ScoreList::const_iterator itScale = semiring()->scales().begin();
	     itScale != semiring()->scales().end(); ++itScale, ++itDescScale)
	    *itDescScale = *itScale;
	desc_->keys.resize(_semiring->size());
	KeyList::iterator itDescKey = desc_->keys.begin();
	for (KeyList::const_iterator itKey = semiring()->keys().begin();
	     itKey != semiring()->keys().end(); ++itKey, ++itDescKey)
	    *itDescKey = *itKey;
	desc_->latticeRoot = "";
	desc_->structureFile = FsaDescriptor(structureFormat, "");
	desc_->hasInputAlphabet = false;
	desc_->hasOutputAlphabet = false;
	desc_->boundariesFile = BoundariesDescriptor();
	desc_->hasBoundaries = !boundariesSuffix_.empty();
	desc_->scoreFiles.resize(_semiring->size());
	FsaDescriptorList::iterator itFsa = desc_->scoreFiles.begin();
	for (StringList::const_iterator itFormat = formats.begin();
	     itFormat != formats.end(); ++itFormat, ++itFsa)
	    itFsa->format = *itFormat;

	// load alphabets
	// input alphabet, required -> acceptor
	const Core::Configuration inputAlphabetConfig(config, "input-alphabet");
	std::string inputAlphabetName = paramName(inputAlphabetConfig);
	FsaDescriptor inputAlphabetQf(inputAlphabetConfig);
	if (!inputAlphabetQf) {
	    if (hasFile("input-alphabet.binfsa.gz"))
		inputAlphabetQf = FsaDescriptor("bin:input-alphabet.binfsa.gz");
	    else if (hasFile("alphabets.binfsa.gz"))
		inputAlphabetQf = FsaDescriptor("bin:alphabets.binfsa.gz");
	    else if (hasFile("alphabets.binfsa"))
		inputAlphabetQf = FsaDescriptor("bin:alphabets.binfsa");
	}
	if (inputAlphabetName.empty())
	    inputAlphabetName = "lemma-pronunciation";
	if (inputAlphabetQf)
	    context->loadInputAlphabet(inputAlphabetQf, archive, inputAlphabetName);
	else
	    context->setInputAlphabet(Fsa::ConstAlphabetRef(), inputAlphabetName);

	//output alphabet, optional -> transducer
	const Core::Configuration outputAlphabetConfig(config, "output-alphabet");
	std::string outputAlphabetName = paramName(outputAlphabetConfig);
	FsaDescriptor outputAlphabetQf(outputAlphabetConfig);
	if (!outputAlphabetQf) {
	    if (hasFile("output-alphabet.binfsa.gz"))
		outputAlphabetQf =  FsaDescriptor("bin:output-alphabet.binfsa.gz");
	}
	if (outputAlphabetName.empty() && outputAlphabetQf)
	    outputAlphabetName = "lemma-pronunciation";
	if (!outputAlphabetName.empty()) {
	    if (outputAlphabetQf)
		context->loadOutputAlphabet(outputAlphabetQf, archive, outputAlphabetName);
	    else
		context->setOutputAlphabet(Fsa::ConstAlphabetRef(), outputAlphabetName);
	}
    }

    void FlfArchiveReader::setFsas(const std::string &id) {
	desc_->structureFile.file = id + structureSuffix_;
	FsaDescriptorList::iterator itFsa = desc_->scoreFiles.begin();
	for (StringList::const_iterator itSuffix = suffixes_.begin();
	     itSuffix != suffixes_.end(); ++itSuffix, ++itFsa)
	    if (!itSuffix->empty()) itFsa->file = id + *itSuffix;
	if (desc_->hasBoundaries)
	    desc_->boundariesFile.file = id + boundariesSuffix_;
    }

    ConstLatticeRef FlfArchiveReader::get(const std::string &id) {
	if (desc_) {
	    setFsas(id);
	    return reader_->read(desc_, archive);
	} else {
	    std::string descFilename = id + suffix();
	    if (hasFile(descFilename))
		return reader_->read(descFilename, archive);
	    else {
		error("Could not find lattice descriptor file \"%s\"",
		      descFilename.c_str());
		return ConstLatticeRef();
	    }
	}
    }
    // -------------------------------------------------------------------------


    // -------------------------------------------------------------------------
    /*
      Flf-Archive-Writer
    */
    FlfArchiveWriter::FlfArchiveWriter(
	const Core::Configuration &config,
	const std::string &pathname) :
	Precursor(config, pathname) {
	archive = Core::Archive::create(config, pathname, Core::Archive::AccessModeWrite);
	if (!archive)
	    criticalError("Failed to open lattice archive \"%s\" for writing", pathname.c_str());
	writer_ = new FlfWriter(config);

	// input alphabet
	inputAlphabetQf_ = FsaDescriptor(select("input-alphabet"));
	if (!inputAlphabetQf_)
	    inputAlphabetQf_ = FsaDescriptor("bin:input-alphabet.binfsa.gz");
	// output alphabet
	outputAlphabetQf_ = FsaDescriptor(select("output-alphabet"));
	if (!outputAlphabetQf_)
	    outputAlphabetQf_ = FsaDescriptor("bin:output-alphabet.binfsa.gz");
	alphabetQf_ = FsaDescriptor(select("alphabets"));
    }

    FlfArchiveWriter::~FlfArchiveWriter() {
	delete writer_;
    }

    void FlfArchiveWriter::finalize() {
	storeAlphabets();
	Precursor::finalize();
    }

    void FlfArchiveWriter::storeAlphabets() {
	Core::Ref<Fsa::StorageAutomaton> f = Core::ref(new Fsa::StaticAutomaton(Fsa::TypeTransducer));
	if (inputAlphabet_) {
	    f->setInputAlphabet(inputAlphabet_);
	    if (inputAlphabetQf_) {
		log("store input alphabet \"%s\"", inputAlphabetQf_.qualifiedPath().c_str());
		Core::ArchiveWriter os(*archive, inputAlphabetQf_.file, true);
		if (!Fsa::write(f, inputAlphabetQf_.format, os, Fsa::storeInputAlphabet))
		    error("Failed to store input alphabet \"%s\"", inputAlphabetQf_.qualifiedPath().c_str());
	    }
	}
	if (outputAlphabet_) {
	    f->setOutputAlphabet(outputAlphabet_);
	    if (outputAlphabetQf_) {
		log("store output alphabet \"%s\"", outputAlphabetQf_.qualifiedPath().c_str());
		Core::ArchiveWriter os(*archive, outputAlphabetQf_.file, true);
		if (!Fsa::write(f, outputAlphabetQf_.format, os, Fsa::storeOutputAlphabet))
		    error("Failed to store output alphabet \"%s\"", outputAlphabetQf_.qualifiedPath().c_str());
	    }
	}
	if (alphabetQf_) {
	    log("store alphabet \"%s\"", alphabetQf_.qualifiedPath().c_str());
	    Core::ArchiveWriter os(*archive, alphabetQf_.file, true);
	    if (!Fsa::write(f, alphabetQf_.format, os, Fsa::storeInputAlphabet | Fsa::storeOutputAlphabet))
		error("Failed to store alphabet \"%s\"", alphabetQf_.qualifiedPath().c_str());
	}
    }

    FlfDescriptorRef FlfArchiveWriter::buildDescriptor(ConstLatticeRef l, const std::string &id, const std::string &filename) {
	FlfDescriptorRef desc = writer_->buildDescriptor(l, filename);
	desc->id = id;
	require(!id.empty() && (id.at(0) != '/') && (id.at(id.size()-1) != '/'));
	std::string root = "";
	std::vector<std::string> pathes = Core::split(id, "/");
	for (u32 i = 0; i < pathes.size() - 1; ++i)
	    if (!pathes[i].empty() && (pathes[i] != ".")) {
		require(pathes[i] != "..");
		root += "../";
	    }
	if (desc->hasInputAlphabet) {
	    if (!inputAlphabet_)
		inputAlphabet_ = l->getInputAlphabet();
	    desc->inputAlphabetFile = FsaDescriptor::joinPaths(inputAlphabetQf_, root);
	}
	if (desc->hasOutputAlphabet) {
	    if (!outputAlphabet_)
		outputAlphabet_ = l->getOutputAlphabet();
	    desc->outputAlphabetFile = FsaDescriptor::joinPaths(outputAlphabetQf_, root);
	}
	return desc;
    }

    void FlfArchiveWriter::store(const std::string &id, ConstLatticeRef l) {
	std::string filename = id + suffix();
	FlfDescriptorRef desc = buildDescriptor(l, id, filename);
	if (!writer_->write(l, desc, filename, false, archive))
	    error("Failed to store lattice \"%s\"", id.c_str());
    }
    // -------------------------------------------------------------------------

} // namespace Flf

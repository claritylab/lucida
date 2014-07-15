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
#include <Core/Application.hh>
#include <Core/Choice.hh>
#include <Core/IoUtilities.hh>
#include <Core/Parameter.hh>

#include "FlfCore/Traverse.hh"
#include "Copy.hh"
#include "Lexicon.hh"
#include "Map.hh"
#include "HtkSlfIo.hh"


namespace Flf {

    HtkSlfContext::HtkSlfContext() {
	silId_ = Lexicon::us()->siLemmaPronunciationId();
	if (silId_ == Fsa::InvalidLabelId)
	    Core::Application::us()->
		warning("Lexicon does not provide a pronunciation for silence.");
	unkId_ = Lexicon::us()->unkLemmaPronunciationId();
	if (unkId_ == Fsa::InvalidLabelId)
	    Core::Application::us()->
		warning("Lexicon does not provide a pronunciation for the unknown word.");
	type_ = HtkSlfForward;
	fps_ = 0.0;
	isCapitalize_ = false;
	mergePenalty_ = true;
	clear();
    }

    HtkSlfContext::~HtkSlfContext() {}

    void HtkSlfContext::setType(HtkSlfType type) {
	type_ = type;
    }

    void HtkSlfContext::setFps(f32 fps) {
	fps_ = fps;
    }

    void HtkSlfContext::setCapitalize(bool isCapitalize) {
	isCapitalize_ = isCapitalize;
    }

    void HtkSlfContext::setBase(f32 base) {
	base_ = base;
    }

    void HtkSlfContext::setEpsSymbol(const std::string &epsSymbol) {
	epsSymbol_ = epsSymbol;
    }

    namespace {
	void setKeyAndId(ConstSemiringRef semiring, const Key &key, ScoreId &id, ScoreId defaultId) {
	    id = semiring->id(key);
	    if (!semiring->hasId(id)) {
		id = defaultId;
		semiring->setKey(id, key);
	    }
	}
    }
    void HtkSlfContext::setSemiring(ConstSemiringRef semiring) {
	if (semiring) {
	    require(semiring->size() >= 2);
	    setKeyAndId(semiring, "am", amId_, 0);
	    setKeyAndId(semiring, "lm", lmId_, 1);
	    if (!mergePenalty_) {
		require(semiring->size() >= 3);
		setKeyAndId(semiring, "penalties", penaltyId_, 2);
	    }
	}
	semiring_ = semiring;
    }

    void HtkSlfContext::setMergePenalties(bool mergePenalty) {
	mergePenalty_ = mergePenalty;
	if (semiring_) setSemiring(semiring_);
    }

    void HtkSlfContext::setPenalties(Score wrdPenalty, Score silPenalty) {
	wrdPenalty_ = wrdPenalty;
	silPenalty_ = (silPenalty == Semiring::Invalid) ? wrdPenalty : silPenalty;
    }

    void HtkSlfContext::setLmName(const std::string &lmName) {
	lmName_ = lmName;
    }

    std::string HtkSlfContext::info() const {
	std::ostringstream oss;
	oss << "htk slf context" << std::endl
	    << "  type:            " << ( (type_ == HtkSlfForward) ? "forward" : "backward" ) << std::endl
	    << "  fps:             " << fps_ << std::endl
	    << "  capitalize:      " << (isCapitalize_   ? "true" : "false") << std::endl
	    << "  merge penalties: " << (mergePenalty_ ? "true" : "false") << std::endl;
	oss << std::endl;
	if (base_ == Core::Type<f32>::min)
	    oss << "  base:            n/a" << std::endl;
	else if (base_ == -1.0)
	    oss << "  base:            e" << std::endl;
	else
	    oss << "  base:            " << base_ << std::endl;
	oss << "  eps-symbol:      " << epsSymbol_ << std::endl;
	if (semiring_) {
	    oss << "  semiring:        " << semiring_->name() << std::endl
		<< "    am    :        " << amId_ << std::endl
		<< "    lm    :        " << lmId_ << std::endl;
	    if (!mergePenalty_)
		oss << "    penalty:       " << penaltyId_ << std::endl;
	} else
	    oss << "  semiring:        n/a" << std::endl;
	oss << "  lm-name:         " << lmName_ << std::endl;
	if (wrdPenalty_ == Semiring::Invalid)
	    oss << "  word penalty:    n/a" << std::endl;
	else
	    oss << "  word penalty:    " << wrdPenalty_ << std::endl;
	if (silPenalty_ == Semiring::Invalid)
	    oss << "  sil. penenalty:  n/a" << std::endl;
	else
	    oss << "  sil. penenalty:  " << silPenalty_ << std::endl;
	oss << std::endl;
	Lexicon::LemmaPronunciationAlphabetRef lemmaPronunciationAlphabet =
	    Lexicon::us()->lemmaPronunciationAlphabet();
	if (unkId_ != Fsa::InvalidLabelId)
	    oss << "  unknown:         \"" << std::string(lemmaPronunciationAlphabet->symbol(unkId_)) << "\"" << std::endl;
	else
	    oss << "  unknown:         n/a" << std::endl;
	if (silId_ != Fsa::InvalidLabelId)
	    oss << "  silence:         \"" << std::string(lemmaPronunciationAlphabet->symbol(silId_)) << "\"" << std::endl;
	else
	    oss << "  silence:         n/a" << std::endl;
	return oss.str();
    }

    void HtkSlfContext::clear() {
	base_ = Core::Type<f32>::min;
	semiring_ = ConstSemiringRef();
	wrdPenalty_ = silPenalty_ = Semiring::Invalid;
	// epsSymbol_ = Lexicon::us()->lemmaAlphabet()->specialSymbol(Fsa::Epsilon);
	epsSymbol_ = "!NULL";
	lmName_.clear();
    }

    namespace {
	const Core::Choice TypeChoice(
	    "forward",  HtkSlfForward,
	    "backward",  HtkSlfBackward,
	    Core::Choice::endMark());
	const Core::ParameterChoice paramType(
	    "slf-type",
	    &TypeChoice,
	    "htk slf type",
	    HtkSlfForward);
	const Core::ParameterFloat paramFps(
	    "fps",
	    "timeframe index = time * scale",
	    100.0, 0.0);
	const Core::ParameterBool paramCapitalize(
	    "capitalize",
	    "capitalize orthography",
	    false);
	const Core::ParameterFloat paramWordPenalty(
	    "word-penalty",
	    "word penalty",
	    Semiring::Invalid);
	const Core::ParameterFloat paramSilPenalty(
	    "silence-penalty",
	    "silence penalty",
	    Semiring::Invalid);
	const Core::ParameterBool paramMergePenalties(
	    "merge-penalties",
	    "merge word and silence penalties into am-score",
	    false);
	const Core::ParameterString paramEpsSymbol(
	    "eps-symbol",
	    "Epsilon symbol",
	    "!NULL");
    } // namespace
    HtkSlfContextRef HtkSlfContext::create(const Core::Configuration &config) {
	HtkSlfContextRef context = HtkSlfContextRef(new HtkSlfContext());
	// set type
	context->setType(HtkSlfType(paramType(config)));
	// set fps
	context->setFps(paramFps(config));
	// set capitalize
	context->setCapitalize(paramCapitalize(config));
	// set penalties, if specified
	Score wrdPenalty = paramWordPenalty(config, Semiring::Invalid);
	Score silPenalty = paramSilPenalty(config, Semiring::Invalid);
	if (wrdPenalty != Semiring::Invalid)
	    context->setPenalties(wrdPenalty, silPenalty);
	else if (silPenalty != Semiring::Invalid)
	    Core::Application::us()->error(
		"Cannot set silence penalty without setting word penalty");
	// set merge penalties
	context->setMergePenalties(paramMergePenalties(config));
	// set eps-word
	context->setEpsSymbol(paramEpsSymbol(config));
	return context;
    }

    /*
      HtkSlf-Reader
    */
    namespace {
	const Core::ParameterBool paramSetCoarticulation(
	    "set-coarticulation",
	    "set coarticulation for across word models",
	    false);
    }
    class HtkSlfBuilder : public Core::Component {
	typedef HtkSlfBuilder Self;
	typedef Core::Component Precursor;
    private:
	typedef std::pair<std::string, std::string> StringPair;
	typedef std::vector<StringPair>             StringPairList;

    protected:
	typedef enum StatusEnum {
	    htkOk = 0,
	    htkPropertyParseError,
	    htkUnexpectedLine,
	    htkParseHeaderError,
	    htkNoSemiring,
	    htkNoFps,
	    htkNoPronunciationError,
	    htkUnexpectedNode,
	    htkParseNodeError,
	    htkAddNodeError,
	    htkParseLinkError,
	    htkAddLinkError,
	    htkNoInitialStateError,
	    htkNoFinalStateError
	} Status;

	typedef enum StateEnum {
	    StateIdle = 0,
	    StateHeader,
	    StateNodes,
	    StateLinks,
	    StateFinalize
	} BuilderState;

	typedef u16 TransitState;
	static const TransitState TransitUnchecked       = 0;
	static const TransitState TransitChecked         = 1;

	static const Bliss::Phoneme::Id InvalidPhonemeId;

	struct Node {
	    State *sp;
	    Fsa::LabelId labelId;
	    TransitState transit;
	    Bliss::Phoneme::Id initialPhonemeId, finalPhonemeId;
	    Node() : sp(0), labelId(Fsa::InvalidLabelId),
		     transit(TransitUnchecked),
		     initialPhonemeId(InvalidPhonemeId),
		     finalPhonemeId(InvalidPhonemeId) {}
	};
	typedef std::vector<Node> NodeList;

    private:
	HtkSlfContextRef context_;
	u32 contextHandling_;
	bool logComments_;
	Status status_;
	BuilderState state_;
	HtkSlfHeader header_;
	Score scale_;
	Score wrdPenalty_, silPenalty_;
	std::string line_;
	u32 lineNo_;
	StaticLattice * lat_;
	StaticBoundaries * boundaries_;
	NodeList nodes_;

	LexiconRef lexicon_;
	Lexicon::LemmaPronunciationAlphabetRef lpAlphabet_;

    protected:
	void comment(const std::string & s) {
	    if (logComments_)
		log("comment: ") << s.c_str();
	}
	f32 convert(f32 score) {
	    return (scale_ == 0.0) ? -::log(score) : scale_ * score;
	}

	void adaptBase(const HtkSlfHeader &header);
	// void checkBase(const HtkSlfHeader &header);
	void adaptSemiring(const HtkSlfHeader &header);
	// void checkSemiring(const HtkSlfHeader &header);
	void adaptPenalties(const HtkSlfHeader &header);
	// void checkPenalties(const HtkSlfHeader &header);
	void adaptOther(const HtkSlfHeader &header);
	// void checkOther(const HtkSlfHeader &header);
	void conformContext(const HtkSlfHeader &header);

	Speech::TimeframeIndex timeframeIndex(f32 time) const;
	Fsa::LabelId lemmaPronunciationId(std::string &label, s32 variant) const;

	void nextLine(std::istream & is);
	Status addProperty(const std::string  &s, StringPairList &props);
	Status header(const StringPairList & props, HtkSlfHeader &header);
	Status node(const StringPairList & props);
	Status link(const StringPairList & props);
	Status setInitialAndFinalStates();
	Status setCoarticulation();

	bool checkStatus(Status status);
	void clear();
	void start();
	void readHeader(std::istream &i, HtkSlfHeader &);
	void init(StaticLattice*);
	void readNodes(std::istream &i);
	void readLinks(std::istream &i);
	void finalize();

    public:
	HtkSlfBuilder(
	    const Core::Configuration & config,
	    HtkSlfContextRef context);
	~HtkSlfBuilder();

	HtkSlfContextRef context() const { return context_; }
	void setContext(HtkSlfContextRef context);

	u32 contextHandling() const { return contextHandling_; }
	void setContextHandling(u32);

	bool logComments() const { return logComments_; }
	void setLogComments(bool logComments) { logComments_ = logComments; }

	bool build(StaticLattice*, std::istream &i);
	bool buildHeader(HtkSlfHeader &header, std::istream &i);
    };


    const Bliss::Phoneme::Id HtkSlfBuilder::InvalidPhonemeId = Core::Type<Bliss::Phoneme::Id>::max;

    HtkSlfBuilder::HtkSlfBuilder(
	const Core::Configuration & config,
	HtkSlfContextRef context) :
	Precursor(config),
	context_(context),
	contextHandling_(HtkSlfReader::UpdateContext),
	logComments_(false) {
	lexicon_ = Lexicon::us();
	lpAlphabet_ = lexicon_->lemmaPronunciationAlphabet();
	lat_ = 0;
	boundaries_ = 0;
	clear();
    }

    HtkSlfBuilder::~HtkSlfBuilder() {
	clear();
    }

    void HtkSlfBuilder::setContext(HtkSlfContextRef context) {
	require(context);
	context_ = context;
    }

    void HtkSlfBuilder::setContextHandling(u32 contextHandling) {
	contextHandling_ = contextHandling;
    }

    void HtkSlfBuilder::adaptBase(const HtkSlfHeader &header) {
	if (context_->base() == Core::Type<f32>::min)
	    context_->setBase(header.base);
    }

    /*
    void HtkSlfBuilder::checkBase(const HtkSlfHeader &header) {
	if (context_->base() != Core::Type<f32>::min) {
	    if (context_->base() != header.base)
		error("Base mismatch; expected %f, got %f.",
		      context_->base(), header.base);
	} else
	    warning("Base check failed, no base set.");
    }
    */

    void HtkSlfBuilder::adaptSemiring(const HtkSlfHeader &header) {
	if (!(context_->semiring())) {
	    if (context_->mergePenalty())
		context_->setSemiring(ConstSemiringRef(new TropicalSemiring(2)));
	    else
		context_->setSemiring(ConstSemiringRef(new TropicalSemiring(3)));
	    context_->semiring()->setScale(context_->lmId(), header.lmScale);
	}
    }

    /*
    void HtkSlfBuilder::checkSemiring(const HtkSlfHeader &header) {
	if (context_->semiring()) {
	    Score lmScale = context_->semiring()->scale(context_->lmId());
	    if (lmScale != header.lmScale)
		error("Scales mismatch; expected lm scale of %f, got %f.",
		      lmScale,
		      header.lmScale);
	} else
	    warning("Semiring check failed, no semiring set.");
    }
    */

    void HtkSlfBuilder::adaptPenalties(const HtkSlfHeader &header) {
	if (!context_->hasPenalties()) {
	    context_->setPenalties(header.wrdPenalty, header.silPenalty);
	}
    }

    /*
    void HtkSlfBuilder::checkPenalties(const HtkSlfHeader &header) {
	if (context_->hasPenalties()) {
	    if (header.wrdPenalty != context_->wordPenalty())
		error("Word penalty mismatch; expected %f, got %f.",
		      context_->wordPenalty(),
		      header.wrdPenalty);
	    if (header.silPenalty != context_->silPenalty())
		error("Silence penalty mismatch; expected %f, got %f.",
		      context_->silPenalty(),
		      header.silPenalty);
	} else
	    warning("Penalties check failed, no penalties set.");
    }
    */

    void HtkSlfBuilder::adaptOther(const HtkSlfHeader &header) {
	if (context_->lmName().empty())
	    context_->setLmName(header.lmName);
    }

    /*
    void HtkSlfBuilder::checkOther(const HtkSlfHeader &header) {
	if (!context_->lmName().empty()) {
	    if (!context_->cmpLm(header.lmName))
		error("Language model mismatch; expected %s, got %s.",
		      context_->lmName().c_str(), header.lmName.c_str());
	} else
	    warning("Language model check failed; no model name set.");
    }
    */

    void HtkSlfBuilder::conformContext(const HtkSlfHeader &header) {
	if (contextHandling_ & HtkSlfReader::ResetContext)
	    context_->clear();
	if (contextHandling_ & HtkSlfReader::AdaptContext) {
	    adaptBase(header);
	    adaptSemiring(header);
	    adaptPenalties(header);
	    adaptOther(header);
	}
	if (!(context_->semiring())) {
	    status_ = htkNoSemiring;
	    return;
	}
	// do some pre-processing
	if ((context_->base() == -1.0) || (context_->base() == 0.0))
	    scale_ = context_->base();
	else
	    scale_ = -::log(context_->base());
	wrdPenalty_ = (context_->wordPenalty() == Semiring::One) ?
	    Semiring::One :
	    convert(context_->wordPenalty());
	silPenalty_ = (context_->silPenalty() == Semiring::One) ?
	    Semiring::One :
	    convert(context_->silPenalty());
    }

    HtkSlfBuilder::Status HtkSlfBuilder::header(const StringPairList & props, HtkSlfHeader &header) {
	s32 required = 2;
	header.reset();

	for (StringPairList::const_iterator it = props.begin();
	     it != props.end(); ++it) {
	    switch (it->first.at(0)) {
	    case 'V':
		if (!Core::strconv(it->second, header.version))
		    error("Unable to parse \"%s\".", it->second.c_str());
		break;
	    case 'U':
		if (!Core::strconv(it->second, header.utterance))
		    error("Unable to parse \"%s\".", it->second.c_str());
		break;
	    case 'S':
		warning("Sub-Lattices are not supported.");
		break;
	    case 'N':
		if (!Core::strconv(it->second, header.nNodes))
		    error("Unable to parse #nodes \"%s\".", it->second.c_str());
		else
		    --required;
		break;
	    case 'L':
		if (!Core::strconv(it->second, header.nLinks))
		    error("Unable to parse #links \"%s\".", it->second.c_str());
		else
		    --required;
		break;
	    default:
		if (it->first == "base") {
		    if (!Core::strconv(it->second, header.base))
			error("Unable to parse base \"%s\".", it->second.c_str());
		} else if (it->first == "lmname") {
		    if (!Core::strconv(it->second, header.lmName))
			error("Unable to parse name \"%s\".", it->second.c_str());
		} else if (it->first == "lmscale") {
		    if (!Core::strconv(it->second, header.lmScale))
			error("Unable to parse scale \"%s\".", it->second.c_str());
		} else if (it->first == "wdpenalty") {
		    if (!Core::strconv(it->second, header.wrdPenalty)) {
			error("Unable to parse penalty \"%s\".", it->second.c_str());
			header.wrdPenalty = Semiring::One;
		    }
		} else if (it->first == "silpenalty") {
		    if (!Core::strconv(it->second, header.silPenalty)) {
			error("Unable to parse penalty \"%s\".", it->second.c_str());
			header.silPenalty = Semiring::Invalid;
		    }
		}
	    }
	}
	if (header.silPenalty == Semiring::Invalid)
	    header.silPenalty = header.wrdPenalty;
	return (required) ? htkParseHeaderError : htkOk;
    }

    Fsa::LabelId HtkSlfBuilder::lemmaPronunciationId(std::string &symbol, s32 variant) const {
	if (symbol == context_->epsSymbol()) {
	    if (variant != -1)
		warning("Expect variant -1 for epsilon symbol \"%s\", found %d.",
			symbol.c_str(), variant);
	    return Fsa::Epsilon;
	}
	Fsa::LabelId label = lpAlphabet_->specialIndex(symbol);
	if (label != Fsa::InvalidLabelId) {
	    if (label != Fsa::Epsilon)
		error("No special symbols besides epsilon are treated correctly; found special symbol \"%s\".",
		      symbol.c_str());
	    else if (variant != -1)
		warning("Expected variant -1 for special symbol \"%s\", found %d.",
			symbol.c_str(), variant);
	    return label;
	}
	if (context_->capitalize())
	    Core::convertToUpperCase(symbol);
	if (variant == -1)
	    variant = 0;
	label = lexicon_->lemmaPronunciationId(symbol, variant);
	if (label == Fsa::InvalidLabelId) {
	    warning("Could not find any pronunciation for word \"%s\", substitute by unknown.",
		    symbol.c_str());
	    return context_->unknown();
	}
	return label;
    }

    Speech::TimeframeIndex HtkSlfBuilder::timeframeIndex(f32 time) const {
	return Speech::TimeframeIndex(::round(context_->fps() * time));
    }

    HtkSlfBuilder::Status HtkSlfBuilder::node(const StringPairList &props) {
	s32 required = 2;
	Fsa::StateId id = Fsa::InvalidStateId;
	Boundary boundary;
	std::string label;
	s32 variant = -1;

	for (StringPairList::const_iterator it = props.begin();
	     it != props.end(); ++it) {
	    switch (it->first.at(0)) {
	    case 'I':
		if (!Core::strconv(it->second, id))
		    error("Unable to parse node id \"%s\".", it->second.c_str());
		else
		    --required;
		break;
	    case 't':
		f32 time;
		if (!Core::strconv(it->second, time))
		    error("Unable to parse time \"%s\".", it->second.c_str());
		else {
		    --required;
		    boundary.setTime(timeframeIndex(time));
		}
		break;
	    case 'L':
		warning("Sub-lattices are not supported.");
		break;
	    case 'W':
		if (!Core::strconv(it->second, label))
		    error("Unable to parse label \"%s\".", it->second.c_str());
		break;
	    case 'v':
		if (!Core::strconv(it->second, variant))
		    error("Unable to parse variant \"%s\".", it->second.c_str());
		break;
	    case 'd':
		if (id == 0) // We assume that the "I" property always occurs before the "d" property
		    warning("No support for the div field.");
		break;
	    }
	}
	if (required)
	    return htkParseNodeError;

	if (id >= header_.nNodes)
	    return htkAddNodeError;
	if (lat_->hasState(id))
	    return htkAddNodeError;
	State *sp = lat_->createState(id);
	lat_->setState(sp);
	boundaries_->set(id, boundary);
	nodes_[id].sp = sp;
	if (!label.empty()) {
	    if ((nodes_[id].labelId = lemmaPronunciationId(label, variant)) == Fsa::InvalidLabelId) {
		error("Could not find any pronunciation for orthography %s (node %d).",
		      label.c_str(), id);
		return htkAddNodeError;
	    }
	}
	return htkOk;
    }

    HtkSlfBuilder::Status HtkSlfBuilder::link(const StringPairList &props) {
	s32 required = 3;
	Fsa::StateId linkId = Fsa::InvalidStateId;
	Fsa::StateId sourceId, targetId;
	Score amScore = Semiring::One;
	Score lmScore = Semiring::One;
	std::string label;
	s32 variant = -1;
	for (StringPairList::const_iterator it = props.begin();
	     it != props.end(); ++it) {
	    switch (it->first.at(0)) {
	    case 'J':
		if (!Core::strconv(it->second, linkId))
		    error("Unable to parse id \"%s\".", it->second.c_str());
		else
		    --required;
		break;
	    case 'S':
		if (!Core::strconv(it->second, sourceId))
		    error("Unable to parse id \"%s\".", it->second.c_str());
		else
		    --required;
		break;
	    case 'E':
		if (!Core::strconv(it->second, targetId))
		    error("Unable to parse id \"%s\".", it->second.c_str());
		else
		    --required;
		break;
	    case 'a':
		if (!Core::strconv(it->second, amScore))
		    error("Unable to parse score \"%s\".", it->second.c_str());
		else
		    amScore = convert(amScore);
		break;
	    case 'l':
		if (!Core::strconv(it->second, lmScore))
		    error("Unable to parse score \"%s\".", it->second.c_str());
		else
		    lmScore = convert(lmScore);
		break;
	    case 'W':
		if (!Core::strconv(it->second, label))
		    error("Unable to parse label \"%s\".", it->second.c_str());
		break;
	    case 'v':
		if (!Core::strconv(it->second, variant))
		    error("Unable to parse variant \"%s\".", it->second.c_str());
		break;
	    case 'd':
		if (linkId == 0) // We assume that the "J" field always occurs before the "d" field
		    warning("No support for the div field.");
		break;
	    case 'n':
		if (linkId == 0) // We assume that the "J" field always occurs before the "n" field
		    warning("No support for the ngram field.");
		break;
	    }
	}
	if (required)
	    return htkParseLinkError;

	if (linkId >= header_.nLinks)
	    return htkAddLinkError;
	if ((sourceId >= header_.nNodes) || (!(nodes_[sourceId].sp)))
	    return htkAddLinkError;
	if ((targetId >= header_.nNodes) || (!(nodes_[targetId].sp)))
	    return htkAddLinkError;

	Fsa::LabelId labelId;
	if (label.empty()) {
	    labelId = (context_->type() == HtkSlfForward) ?
		nodes_[targetId].labelId :
		nodes_[sourceId].labelId;
	} else {
	    labelId = lemmaPronunciationId(label, variant);
	}
	if (labelId == Fsa::InvalidLabelId) {
	    error("Could not find any pronunciation for orthography %s (link %d).",
		  label.c_str(), linkId);
	    return htkAddLinkError;
	}
	Score penalty = 0.0;
	if ((Fsa::FirstLabelId <= labelId) && (labelId <= Fsa::LastLabelId))
	    penalty = (labelId == context_->silence()) ?
		silPenalty_ :
		wrdPenalty_;
	ScoresRef scores = context_->semiring()->create();
	if (context_->mergePenalty()) {
	    scores->set(context_->amId(), amScore + penalty);
	} else {
	    scores->set(context_->amId(), amScore);
	    scores->set(context_->penaltyId(), penalty);
	}
	scores->set(context_->lmId(), lmScore);
	if (!nodes_[sourceId].sp->newArc(targetId, scores, Fsa::LabelId(labelId)))
	    return htkAddLinkError;
	return htkOk;
    }

    HtkSlfBuilder::Status HtkSlfBuilder::setInitialAndFinalStates() {
	if (nodes_.empty() || !nodes_[0].sp)
	    return htkNoInitialStateError;
	lat_->setInitialStateId(Fsa::StateId(0));
	bool hasFinalState = false;
	for (NodeList::iterator it = nodes_.begin(); it != nodes_.end(); ++it)
	    if (it->sp && !it->sp->hasArcs()) {
		it->sp->setFinal(lat_->semiring()->clone(lat_->semiring()->one()));
		hasFinalState = true;
	    }
	if (!hasFinalState)
	    return htkNoFinalStateError;
	return htkOk;
    }

    HtkSlfBuilder::Status HtkSlfBuilder::setCoarticulation() {
	Lexicon::LemmaPronunciationAlphabetRef lpAlphabet = Lexicon::us()->lemmaPronunciationAlphabet();
	Core::Ref<const Bliss::PhonemeInventory> phonemeInventory = Lexicon::us()->phonemeInventory();
	Node &initial = nodes_[0];
	initial.initialPhonemeId = initial.finalPhonemeId = Bliss::Phoneme::term;
	Fsa::Stack<Fsa::StateId> stack;
	stack.push(0);
	while (!stack.isEmpty()) {
	    Fsa::StateId sourceId = stack.pop();
	    State *sp = nodes_[sourceId].sp;
	    Node &source = nodes_[sourceId];
	    if (!sp->hasArcs()) {
		source.initialPhonemeId = Bliss::Phoneme::term;
		continue;
	    }
	    Bliss::Phoneme::Id initialPhonemeId, finalPhonemeId;
	    for (State::const_iterator a = sp->begin(); a != sp->end(); ++a) {
		Fsa::StateId targetId = a->target();
		Node &target = nodes_[targetId];
		const Bliss::Pronunciation *p =
		    ((Fsa::FirstLabelId <= a->input()) && (a->input() <= Fsa::LastLabelId)) ?
		    lpAlphabet->lemmaPronunciation(a->input())->pronunciation() :
		    0;
		if (p && p->length()) {
		    initialPhonemeId = (*p)[0];
		    initialPhonemeId =
			(phonemeInventory->phoneme(initialPhonemeId)->isContextDependent()) ?
			initialPhonemeId : Bliss::Phoneme::term;
		    finalPhonemeId = (*p)[p->length() - 1];
		    finalPhonemeId =
			(phonemeInventory->phoneme(finalPhonemeId)->isContextDependent()) ?
			finalPhonemeId : Bliss::Phoneme::term;
		} else {
		    if (!target.sp->isFinal()) {
			initialPhonemeId = source.finalPhonemeId;
			finalPhonemeId = initialPhonemeId;
		    } else {
			// special treatment required for HTKs from FLF lattices
			initialPhonemeId = finalPhonemeId = Bliss::Phoneme::term;
		    }
		}
		if (source.initialPhonemeId == InvalidPhonemeId)
		    source.initialPhonemeId = initialPhonemeId;
		if ((initialPhonemeId == Bliss::Phoneme::term)
		    || (initialPhonemeId != source.initialPhonemeId))
		    source.initialPhonemeId = source.finalPhonemeId = Bliss::Phoneme::term;
		if (target.finalPhonemeId == InvalidPhonemeId)
		    target.finalPhonemeId = finalPhonemeId;
		if ((finalPhonemeId == Bliss::Phoneme::term)
		    || (finalPhonemeId != target.finalPhonemeId))
		    target.initialPhonemeId = target.finalPhonemeId = Bliss::Phoneme::term;
		if (target.transit == TransitUnchecked) {
		    target.transit = TransitChecked;
		    stack.push(targetId);
		}
	    }
	}
	for (NodeList::iterator it = nodes_.begin(); it != nodes_.end(); ++it)
	    if (it->transit != TransitUnchecked) {
		Fsa::StateId sid = it->sp->id();
		(*boundaries_)[sid].setTransit(
		    Boundary::Transit(it->finalPhonemeId, it->initialPhonemeId, AcrossWordBoundary));
	    }
	lat_->addProperties(PropertyCrossWord);
	return htkOk;
    }

    namespace {
	const char * HtkStatusMessage[] = {
	    "Everything's fine!.",
	    "Error while parsing property.",
	    "Unexpected line; don't know what to do with that line.",
	    "Error while parsing header; header is not well defined.",
	    "Semiring is not set.",
	    "Fps is not set.",
	    "Error while parsing orthography; lexicon does not contain any matching pronunciation.",
	    "Unexpected node; did not expect the definition of a node.",
	    "Error while parsing node.",
	    "Error while adding node.",
	    "Error while parsing link.",
	    "Error while adding link.",
	    "No initial state, i.e. the lattice is empty.",
	    "No final state."
	};
    } // namespace
    bool HtkSlfBuilder::checkStatus(Status status) {
	if (status != htkOk) {
	    error("Error while parsing \n%5d: %s",
		  lineNo_, HtkStatusMessage[status]) ;
	    return false;
	} else return true;
    }

    void HtkSlfBuilder::nextLine(std::istream &is) {
	do {
	    ++lineNo_;
	    std::getline(is, line_);
	    Core::stripWhitespace(line_);
	    if (!line_.empty() && (line_.at(0) == '#'))
		{ comment(line_); line_.clear(); }
	} while (line_.empty() && is);
    }

    HtkSlfBuilder::Status HtkSlfBuilder::addProperty(const std::string &s, StringPairList &props) {
	const char * c = s.c_str();
	const char * keyStart, * keyEnd, * valueStart, * valueEnd;

	while (*c != '\0') {
	    for (; ::isspace(*c); ++c);
	    if (*c == '\0')
		break;
	    keyStart = c;

	    for (; (*c != '=') && !::isspace(*c) && (*c != '\0'); ++c);
	    if (*c == '\0')
		return htkPropertyParseError;
	    keyEnd = c;

	    for (; ::isspace(*c); ++c);
	    if (*c != '=')
		return htkPropertyParseError;
	    ++c;

	    for (; ::isspace(*c); ++c);
	    if (*c == '\0')
		return htkPropertyParseError;

	    if (*c == '"') {
		valueStart = ++c;

		for (; (*c != '"') && (*c != '\0'); ++c);
		if (*c == '\0')
		    return htkPropertyParseError;
		valueEnd = c;
		++c;
	    } else {
		valueStart = c;

		for (; !::isspace(*c) && (*c != '\0'); ++c);
		valueEnd = c;
	    }

	    props.push_back(StringPair(
				std::string(keyStart, keyEnd - keyStart),
				std::string(valueStart, valueEnd - valueStart)));
	}
	return htkOk;
    }

    void HtkSlfBuilder::clear() {
	line_.clear();
	nodes_.clear();
	lat_ = 0;
	delete boundaries_;
	boundaries_ = 0;
	state_ = StateIdle;
    }

    void HtkSlfBuilder::start() {
	require(state_ == StateIdle);
	lineNo_ = 0;
	status_ = htkOk;
	state_ = StateHeader;
    }

    void HtkSlfBuilder::readHeader(std::istream &is, HtkSlfHeader &_header) {
	require(state_ == StateHeader);
	StringPairList props;
	nextLine(is);
	while ((state_ == StateHeader) && (status_ == htkOk) && is) {
	    switch (line_.at(0)) {
	    case 'I':
		state_ = StateNodes;
		break;
	    case 'J':
		state_ = StateLinks;
		break;
	    default:
		status_ = addProperty(line_, props);
		nextLine(is);
	    }
	}
	if (status_ == htkOk)
	    status_ = header(props, _header);
    }

    void HtkSlfBuilder::init(StaticLattice *lat) {
	conformContext(header_);
	if (status_ != htkOk) return;
	require(lat);
	lat_ = lat;
	if (!header_.utterance.empty())
	    lat_->setDescription(header_.utterance);
	lat_->setType(Fsa::TypeAcceptor);
	lat_->setSemiring(context_->semiring());
	lat_->setInputAlphabet(Lexicon::us()->lemmaPronunciationAlphabet());
	lat->addProperties(Fsa::PropertyAcyclic);
	boundaries_ = new StaticBoundaries;
	boundaries_->resize(header_.nNodes);
	nodes_.resize(header_.nNodes);
    }

    void HtkSlfBuilder::readNodes(std::istream &is) {
	require(state_ == StateNodes);
	StringPairList props;
	while ((state_ == StateNodes) && (status_ == htkOk) && is) {
	    switch (line_.at(0)) {
	    case 'I':
		if ((status_ = addProperty(line_, props)) != htkOk)
		    break;
		if ((status_ = node(props)) != htkOk)
		    break;
		props.clear();
		nextLine(is);
		break;
	    case 'J':
		state_ = StateLinks;
		break;
	    default:
		status_ = htkUnexpectedLine;
	    }
	}
    }

    void HtkSlfBuilder::readLinks(std::istream &is) {
	require(state_ == StateLinks);
	StringPairList props;
	while ((status_ == htkOk) && is) {
	    switch (line_.at(0)) {
	    case 'I':
		status_ = htkUnexpectedNode;
		break;
	    case 'J':
		if ((status_ = addProperty(line_, props)) != htkOk)
		    break;
		if ((status_ = link(props)) != htkOk)
		    break;
		props.clear();
		nextLine(is);
		break;
	    default:
		status_ = htkUnexpectedLine;
	    }
	}
	state_ = StateFinalize;
    }

    void HtkSlfBuilder::finalize() {
	require(state_ == StateFinalize);
	if ((status_ = setInitialAndFinalStates()) != htkOk)
	    return;

	if (paramSetCoarticulation(config) &&
	    ((status_ = setCoarticulation()) != htkOk))
	    return;

	lat_->setBoundaries(ConstBoundariesRef(boundaries_));
	boundaries_ = 0;
    }

    bool HtkSlfBuilder::buildHeader(HtkSlfHeader &header, std::istream &is) {
	start();
	if (!checkStatus(status_))
	    { clear(); return false; }
	readHeader(is, header);
	if (!checkStatus(status_))
	    { clear(); return false; }
	clear();
	return true;
    }

    bool HtkSlfBuilder::build(StaticLattice *lat, std::istream &is) {
	start();
	if (!checkStatus(status_))
	    { clear(); return false; }
	readHeader(is, header_);
	if (!checkStatus(status_))
	    { clear(); return false; }
	init(lat);
	if (!checkStatus(status_))
	    { clear(); return false; }
	readNodes(is);
	if (!checkStatus(status_))
	    { clear(); return false; }
	readLinks(is);
	if (!checkStatus(status_))
	    { clear(); return false; }
	finalize();
	if (!checkStatus(status_))
	    { clear(); return false; }
	clear();
	return true;
    }

    const u32 HtkSlfReader::TrustContext     = 0;
    const u32 HtkSlfReader::AdaptContext     = 1;
    const u32 HtkSlfReader::CheckContext     = 2;
    const u32 HtkSlfReader::ResetContext     = 4;
    const u32 HtkSlfReader::UpdateContext    = 5;
    const Core::ParameterString HtkSlfReader::paramContextMode(
	"context-mode",
	"context mode",
	"update");
    const Core::Choice ContextModeChoice = Core::Choice(
	"trust",  0,
	"adapt",  1,
	"update", 5,
	Core::Choice::endMark());
    const Core::ParameterBool HtkSlfReader::paramLogComments(
	"log-comments",
	"log all comments in htk slf file",
	false);
    const Core::ParameterString HtkSlfReader::paramEncoding(
	"encoding",
	"encoding of htk lattice", "utf-8");

    HtkSlfReader::HtkSlfReader(const Core::Configuration &config) :
	Precursor(config) {
	HtkSlfContextRef context = HtkSlfContext::create(config);
	builder_ = new HtkSlfBuilder(config, context);
	Core::Choice::Value contextHandling =
	    ContextModeChoice[paramContextMode(config)];
	if (contextHandling == Core::Choice::IllegalValue)
	    error("Unknown context mode \"%s\".",
		  paramContextMode(config).c_str());
	else
	    builder_->setContextHandling(contextHandling);
	builder_->setLogComments(paramLogComments(config));
	encoding_ = paramEncoding(config);
    }

    HtkSlfReader::HtkSlfReader(const Core::Configuration &config, HtkSlfContextRef context, u32 contextHandling) :
	Precursor(config) {
	require(context);
	builder_ = new HtkSlfBuilder(config, context);
	builder_->setContextHandling(contextHandling);
	encoding_ = paramEncoding(config);
    }

    HtkSlfReader::~HtkSlfReader() {
	delete builder_;
    }

    HtkSlfContextRef HtkSlfReader::context() const {
	return builder_->context();
    }

    void HtkSlfReader::setContext(HtkSlfContextRef context) {
	builder_->setContext(context);
    }

    u32 HtkSlfReader::contextHandling() const {
	return builder_->contextHandling();
    }

    void HtkSlfReader::setContextHandling(u32 contextHandling) {
	builder_->setContextHandling(contextHandling);
    }

    HtkSlfHeader* HtkSlfReader::readHeader(const std::string &filename, Core::Archive *archive) {
	InputStream is(filename, archive);
	if (!is)
	    return 0;
	Core::TextInputStream tis(is.steal());
	tis.setEncoding(encoding_);
	if (!(tis))
	    return 0;
	HtkSlfHeader *header = new HtkSlfHeader;
	if (!builder_->buildHeader(*header, tis)) {
	    delete header;
	    return 0;
	}
	return header;
    }

    ConstLatticeRef HtkSlfReader::read(const std::string &filename, Core::Archive *archive) {
	InputStream is(filename, archive);
	if (!is)
	    return ConstLatticeRef();
	Core::TextInputStream tis(is.steal());
	tis.setEncoding(encoding_);
	if (!(tis))
	    return ConstLatticeRef();

	log() << context()->info();

	StaticLattice *lat = new StaticLattice;
	if (!builder_->build(lat, tis)) {
	    delete lat;
	    return ConstLatticeRef();
	}
	return ConstLatticeRef(lat);
    }


    /*
      HtkSlf-Writer
    */
    class HtkSlfWriter::ToWordVariant {
    protected:
	Lexicon* lexicon;
	Lexicon::LemmaAlphabetRef lAlphabet;
    public:
	ToWordVariant() {
	    lexicon = Lexicon::us().get();
	    lAlphabet = lexicon->lemmaAlphabet();
	}
	virtual ~ToWordVariant() {}
	void write(std::ostream &os, Fsa::LabelId label) const {
	    if ((Fsa::FirstLabelId <= label) && (label <= Fsa::LastLabelId)) {
		writeWordVariant(os, label);
	    } else if (label == Fsa::Epsilon) {
		writeEpsilon(os);
	    } else {
		os << " W=\"" << lAlphabet->specialSymbol(label) << "\" v=-1";
	    }
	}
	void writeEpsilon(std::ostream &os) const {
	    //os << " W=\"" << lpAlphabet->specialSymbol(Fsa::Epsilon) << "\" v=-1";
	    os << " W=\"!NULL\" v=-1";
	}
	virtual void writeWordVariant(std::ostream &os, Fsa::LabelId labelId) const = 0;
    };

    class LemmaPronunciationToWordVariant : public HtkSlfWriter::ToWordVariant {
    protected:
	Lexicon::LemmaPronunciationAlphabetRef lpAlphabet;
    public:
	LemmaPronunciationToWordVariant() : ToWordVariant() {
	    lpAlphabet = lexicon->lemmaPronunciationAlphabet();
	}
	virtual void writeWordVariant(std::ostream &os, Fsa::LabelId labelId) const {
	    std::pair<const Bliss::Lemma*, s32> lv = lexicon->lemmaPronunciationVariant(lpAlphabet->lemmaPronunciation(labelId));
	    std::string w = lv.first->preferredOrthographicForm();
	    Core::stripWhitespace(w);
	    os << " W=\"" << w << "\" v=" << lv.second;
	}
    };

    class LemmaToWordVariant : public HtkSlfWriter::ToWordVariant {
    public:
	LemmaToWordVariant() : HtkSlfWriter::ToWordVariant() {}
	virtual void writeWordVariant(std::ostream &os, Fsa::LabelId label) const {
	    std::string w = lAlphabet->lemma(label)->preferredOrthographicForm();
	    Core::stripWhitespace(w);
	    os << " W=\"" << w << "\" v=0";
	}
    };



    HtkSlfWriter::HtkSlfWriter(const Core::Configuration &config) :
	Precursor(config) {
	label2wv_ = 0;
	lp2wv_ = new LemmaPronunciationToWordVariant();
	l2wv_ = new LemmaToWordVariant();
	encoding_ = HtkSlfReader::paramEncoding(config);
	fps_ = paramFps(config);
    }

    HtkSlfWriter::~HtkSlfWriter() {
	delete lp2wv_;
	delete l2wv_;
    }

    void HtkSlfWriter::setEncoding(const std::string encoding) {
	encoding_ = encoding;
    }

    void HtkSlfWriter::setFps(f32 fps) {
	fps_ = fps;
    }

    void HtkSlfWriter::writeHeader(HtkSlfHeader &header, std::ostream &os) const {
	os << "# RWTH-i6 " << std::endl;
	os << "# " << Core::timestamp << std::endl;
	if (!header.version.empty())
	    os << "VERSION=" << header.version << std::endl;
	if (!header.utterance.empty())
	    os << "UTTERANCE=" << header.utterance << std::endl;
	if (header.base != -1.0)
	    os << "base=" << header.base << std::endl;
	if (!header.lmName.empty())
	    os << "lmname=" << header.lmName << std::endl;
	if (header.lmScale != Semiring::UndefinedScale)
	    os << "lmscale=" << header.lmScale << std::endl;
	if (header.wrdPenalty != Semiring::Invalid)
	    os << "wdpenalty=" << -header.wrdPenalty;
	if (header.silPenalty != Semiring::Invalid)
	    os << "silpenalty=" << -header.wrdPenalty;
	os << std::endl;
	os << "# Lattice Size" << std::endl;
	os << "NODES=" << header.nNodes << std::endl
	   << "LINKS=" << header.nLinks << std::endl;
    }

    void HtkSlfWriter::writeNodes(ConstLatticeRef f, HtkSlfHeader &header, StateIdMapping &mapping, std::ostream &os) const {
	ConstBoundariesRef boundaries = f->getBoundaries();
	u32 nodeId = 0;
	os << std::endl;
	os << "# Nodes" << std::endl;
	for (std::vector<Fsa::StateId>::const_iterator it = mapping.htk2fsa.begin();
	     it != mapping.htk2fsa.end(); ++it, ++nodeId) {
	    os << "I=" << nodeId;
	    const Boundary &b = boundaries->get(*it);
	    if (b.valid())
		os << " t=" << (b.time() / fps_);
	    os << std::endl;
	}
	if (!mapping.finals.empty()) {
	    os << "I=" << nodeId;
	    ConstStateRefList::const_iterator it = mapping.finals.begin();
	    const Boundary &b = boundaries->get((*it)->id());
	    if (b.valid()) {
		for (++it; it != mapping.finals.end(); ++it)
#if 1
		if (b != boundaries->get((*it)->id())) {
		    warning("inconsistent boundaries at '") << (*it)->id() << "': " <<
			b.time() << " vs. " << boundaries->get((*it)->id()).time();
		}
#endif
		os << " t=" << (b.time() / fps_);
	    }
	    os << std::endl;
	}
    }

    void HtkSlfWriter::writeLinks(ConstLatticeRef f, HtkSlfHeader &header, StateIdMapping &mapping, std::ostream &os) const {
	ScoreId amId = f->semiring()->id("am");
	ScoreId lmId = f->semiring()->id("lm");
	u32 nodeId = 0;
	u32 linkId = 0;
	os << std::endl;
	os << "# Links" << std::endl;
	for (std::vector<Fsa::StateId>::const_iterator it = mapping.htk2fsa.begin(); it != mapping.htk2fsa.end(); ++it, ++nodeId) {
	    ConstStateRef sr = f->getState(*it);
	    for (State::const_iterator a = sr->begin(); a != sr->end(); ++a, ++linkId) {
		os << "J=" << linkId << " S=" << nodeId << " E=" << mapping.fsa2htk[a->target()];
		label2wv_->write(os, a->input());
		if (amId != Semiring::InvalidId)
		    os << " a=" << -a->score(amId);
		if (lmId != Semiring::InvalidId)
		    os << " l=" << -a->score(lmId);
		os  << std::endl;
	    }
	}
	if (!mapping.finals.empty()) {
	    u32 targetId = mapping.htk2fsa.size();
	    for (ConstStateRefList::const_iterator it = mapping.finals.begin(); it != mapping.finals.end(); ++it, ++linkId) {
		os << "J=" << linkId << " S=" << mapping.fsa2htk[(*it)->id()] << " E=" << targetId;
		label2wv_->writeEpsilon(os);
		if (amId != Semiring::InvalidId)
		    os << " a=" << -(*it)->weight()->get(amId);
		if (lmId != Semiring::InvalidId)
		    os << " l=" << -(*it)->weight()->get(lmId);
		os  << std::endl;
	    }
	}
    }

    namespace {
	class HtkSlfHeaderAndMappingBuilder : public TraverseState {
	private:
	    HtkSlfHeader &header_;
	    HtkSlfWriter::StateIdMapping &mapping_;

	public:
	    HtkSlfHeaderAndMappingBuilder(
		ConstLatticeRef f,
		HtkSlfHeader &header,
		HtkSlfWriter::StateIdMapping &mapping,
		bool topological) :
		TraverseState(f),
		header_(header),
		mapping_(mapping) {

		header_.version = "1.0";
		header_.utterance = "";
		header_.base = -1.0;
		header_.lmName = "";
		ScoreId lmId = f->semiring()->id("lm");
		header_.lmScale = (f->semiring()->hasId(lmId)) ?
		    f->semiring()->scale(lmId) :
		    Semiring::UndefinedScale;
		header_.wrdPenalty = header_.silPenalty = Semiring::Invalid;
		header_.nNodes = header_.nLinks = 0;
		if(topological)
		    traverseInTopologicalOrder();
		else
		traverse();

		if ((mapping_.finals.size() == 1)
		    && !mapping_.finals.front()->hasArcs()
		    && (f->semiring()->compare(mapping_.finals.front()->weight(), f->semiring()->one()) == 0)) {
		    mapping_.finals.clear();
		} else {
		    ++header_.nNodes;
		    header_.nLinks += mapping_.finals.size();
		}
	    }

	    void exploreState(ConstStateRef sr) {
		if (sr->isFinal())
		    mapping_.finals.push_back(sr);
		else
		    require(sr->hasArcs());
		mapping_.htk2fsa.push_back(sr->id());
		mapping_.fsa2htk.grow(sr->id());
		mapping_.fsa2htk[sr->id()] = header_.nNodes;
		++header_.nNodes;
		header_.nLinks += sr->nArcs();
	    }
	};
    } // namespace
    bool HtkSlfWriter::buildHeaderAndMapping(
	ConstLatticeRef f,
	HtkSlfHeader &header,
	StateIdMapping &mapping,
	bool topological) const {
	header.reset();
	mapping.clear();
	if (!f->semiring()->hasId(f->semiring()->id("am")))
	    warning("lattice defines no acoustic scores");
	if (!f->semiring()->hasId(f->semiring()->id("lm")))
	    warning("lattice defines no language model scores");
	if (!header.base == -1.0)
	    warning("all scores and penalties are considered negative natural logarithms of probabilities");
	HtkSlfHeaderAndMappingBuilder build(f, header, mapping, topological);
	return true;
    }

    bool HtkSlfWriter::write(
	ConstLatticeRef l,
	HtkSlfHeader &header,
	StateIdMapping &mapping,
	const std::string &filename,
	Core::Archive *archive) const {
	switch (Lexicon::us()->alphabetId(l->getInputAlphabet())) {
	case Lexicon::LemmaPronunciationAlphabetId:
	    label2wv_ = lp2wv_;
	    break;
	case Lexicon::LemmaAlphabetId:
	    label2wv_ = l2wv_;
	    break;
	default:
	    defect();
	}
	OutputStream os(filename, archive);
	if (!(os))
	    return false;
	Core::TextOutputStream tos(os.steal());
	tos.setEncoding(encoding_);
	writeHeader(header, tos);
	writeNodes(l, header, mapping, tos);
	writeLinks(l, header, mapping, tos);
	return tos;
    }

    bool HtkSlfWriter::write(
	ConstLatticeRef l,
	const std::string &filename,
	Core::Archive *archive) const {
	l = persistent(mapInputToLemmaOrLemmaPronunciation(l));
	HtkSlfHeader header;
	StateIdMapping mapping;
	if (!buildHeaderAndMapping(l, header, mapping))
	    return false;
	else
	    return write(l, header, mapping, filename, archive);
    }



    /*
      HtkSlf-Archive-Reader
    */
    HtkSlfArchiveReader::HtkSlfArchiveReader(
	const Core::Configuration &config,
	const std::string &pathname) : Precursor(config, pathname) {
	archive = Core::Archive::create(config, pathname, Core::Archive::AccessModeRead);
	if (!archive)
	    criticalError("Failed to open lattice archive \"%s\" for reading", pathname.c_str());
	HtkSlfContextRef context = HtkSlfContext::create(config);
	// set semiring, if specified
	context->setSemiring(semiring());
	log("use the following predefined context for reading htk slfs:\n\n%s",
	    context->info().c_str());
	// get adapting reader, i.e. missing context is completed by htk slf
	reader_ = new HtkSlfReader(config, context, HtkSlfReader::AdaptContext);
    }

    HtkSlfArchiveReader::~HtkSlfArchiveReader() {
	delete reader_;
    }

    ConstLatticeRef HtkSlfArchiveReader::get(const std::string &id) {
	std::string filename = id + suffix();
	return reader_->read(filename, archive);
    }



    /*
      HtkSlf-Archive-Writer
    */
    HtkSlfArchiveWriter::HtkSlfArchiveWriter(
	const Core::Configuration &config,
	const std::string &pathname) : Precursor(config, pathname) {
	archive = Core::Archive::create(config, pathname, Core::Archive::AccessModeWrite);
	if (!archive)
	    criticalError("Failed to open lattice archive \"%s\" for writing", pathname.c_str());
	writer_ = new HtkSlfWriter(config);
    }

    HtkSlfArchiveWriter::~HtkSlfArchiveWriter() {
	delete writer_;
    }

    void HtkSlfArchiveWriter::store(const std::string &id, ConstLatticeRef l) {
	l = persistent(mapInputToLemmaOrLemmaPronunciation(l));
	std::string filename = id + suffix();
	HtkSlfHeader header;
	HtkSlfWriter::StateIdMapping mapping;
	if (!writer_->buildHeaderAndMapping(l, header, mapping)) {
	    error("Failed to store htk slf \"%s\"", id.c_str());
	    return;
	}
	header.utterance = id;
	if (!writer_->write(l, header, mapping, filename, archive))
	    error("Failed to store lattice \"%s\"", id.c_str());
    }

} //namespace Flf

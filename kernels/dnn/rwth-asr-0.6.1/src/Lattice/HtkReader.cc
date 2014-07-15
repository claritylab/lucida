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
#include <Fsa/Arithmetic.hh>
#include <Fsa/Cache.hh>
#include <Fsa/Project.hh>
#include <Fsa/Static.hh>
#include "Compose.hh"

#include "HtkReader.hh"

using namespace Lattice;

const Core::ParameterFloat HtkReader::paramTimeframeIndexScale(
    "timeframe-index-scale",
    "timeframe index = time * scale",
    100, 0);

HtkReader::HtkReader(
    const Core::Configuration & config,
    Bliss::LexiconRef lexiconRef,
    f64 amScale,
    f64 lmScale,
    f64 penaltyScale,
    f64 pronunciationScale,
    bool keepVariants) :
    Precursor(config),
    lexiconRef_(lexiconRef),
    amScale_(amScale),
    lmScale_(lmScale),
    penaltyScale_(penaltyScale),
    keepVariants_(keepVariants),
    wordPenalty_(0.0),
    silPenalty_(0.0),
    unkLemmaId_(Fsa::InvalidLabelId),
    silLemmaId_(Fsa::InvalidLabelId),
    lattice_(0),
    wordBoundaries_(0),
    states_(),
    timeframeIndexScale_(paramTimeframeIndexScale(config)),
    timeOffset_(Core::Type<f64>::min) {
    orthParser_ = new Bliss::OrthographicParser(config, lexiconRef_);
    htkAlphabetRef_ = lexiconRef_->lemmaAlphabet();
    lemmaPronunciationToHtkTransducerRef_ =
	lexiconRef_->createLemmaPronunciationToLemmaTransducer();
    if (pronunciationScale != 1) {
	lemmaPronunciationToHtkTransducerRef_ =
	    Fsa::cache(Fsa::multiply(lemmaPronunciationToHtkTransducerRef_, Fsa::Weight((f32)pronunciationScale)));
    }
    const Bliss::Lemma *unknown = lexiconRef_->specialLemma("unknown");
    if (unknown) unkLemmaId_ = unknown->id();
    else warning() << "could not find label id for the unknown word; use the invalid label id";
    const Bliss::Lemma *silence = lexiconRef_->specialLemma("silence");
    if (silence) silLemmaId_ = silence->id();
    else warning() << "could not find label id for the silence word";
    isCapitalize_ = false;
    log("htk lattice settings:\nam-scale      : %f\nlm-scale      : %f\npenalty-scale : %f\npronunciation-scale : %f",
	amScale_, lmScale_, penaltyScale_, pronunciationScale);
    line_ = 0;
    status_ = htkOk;
}

HtkReader::~HtkReader() {
    delete orthParser_;
}

Fsa::Weight HtkReader::getWeight(const Link & link, Fsa::LabelId lemmaId) {
    f64 weight = -(link.amScore * amScale_  + link.lmScore * lmScale_);
    if (penaltyScale_ != 0.0) {
	if ((lemmaId == silLemmaId_) && (silLemmaId_ != Fsa::InvalidLabelId))
	    weight += penaltyScale_ * silPenalty_;
	else
	    weight += penaltyScale_ * wordPenalty_;
    }
    return Fsa::Weight(weight);
}

Fsa::StateId HtkReader::getStateId(u32 id) {
    return Fsa::StateId(id);
}

HtkReader::Status HtkReader::init() {
    lattice_->setType(Fsa::TypeAcceptor);
    lattice_->setSemiring(Fsa::getSemiring(Fsa::SemiringTypeTropical));
    lattice_->setInputAlphabet(htkAlphabetRef_);
    return htkOk;
}

HtkReader::Status HtkReader::addState(Node & node) {
    require(size_t(node.id) == states_.size());
    State state;
    state.fsaState = new Fsa::State(getStateId(node.id));
    state.label  = node.label;
    state.lemmas = node.lemmas;
    lattice_->setState(state.fsaState);
    /*! \todo across word model is not handled correctly yet */
    WordBoundary wordBoundary(node.timeframeIndex, WordBoundary::Transit());
    wordBoundaries_->set(node.id, wordBoundary);
    states_.push_back(state);
    return htkOk;
}

HtkReader::Status HtkReader::addArc(Link & link) {
    require(
	size_t(link.sourceState) < states_.size()
	&& size_t(link.targetState) < states_.size());
    if (link.hasLemmas()) {
	for (LemmaIterator it = link.lemmas.first; it != link.lemmas.second; ++it) {
	    if (!states_[size_t(link.sourceState)].fsaState->newArc(
		    getStateId(link.targetState),
		    getWeight(link, it->id()),
		    it->id()))
		return htkAddLinkError;
	}
    } else {
	warning() << "No matching lemma for orthographic form \"" << link.label << "\" found.\n"
		  << "Substitute by unknown word.";
	if (!states_[size_t(link.sourceState)].fsaState->newArc(
		getStateId(link.targetState),
		getWeight(link, unkLemmaId_),
		unkLemmaId_))
	    return htkAddLinkError;

//    warning() << "No matching lemma for orthographic form \"" << link.label << "\" found.\n"
//		  << "Substitute by epsilon.";
//	if (!states_[size_t(link.sourceState)].fsaState->newArc(
//		getStateId(link.targetState),
//		getWeight(link, Fsa::Epsilon),
//		Fsa::Epsilon))
//	    return htkAddLinkError;
    }
    return htkOk;
}

HtkReader::Status HtkReader::setInitialState() {
    if (states_.empty())
	return htkNoInitialStateError;
    lattice_->setInitialStateId(Fsa::StateId(0));
    return htkOk;
}

HtkReader::Status HtkReader::setFinalStates() {
    bool hasFinalState = false;
    for (StateList::iterator it = states_.begin(); it != states_.end(); ++it)
	if (!it->fsaState->hasArcs()) {
	    it->fsaState->setFinal(Fsa::Weight(0.0));
	    hasFinalState = true;
	}
    return (hasFinalState) ? htkOk : htkNoFinalStateError;
}

HtkReader::Status HtkReader::add(const std::string  & s, StringPairList & props) {
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

HtkReader::LemmaRange HtkReader::parseLabel(std::string & label, const std::string & variant) {
    if (isCapitalize_)
	Core::convertToUpperCase(label);
    if (keepVariants_ && (!variant.empty())) {
	label.append("/");
	label.append(variant);
    }
    return orthParser_->lemmas(label);
}

HtkReader::Status HtkReader::set(const StringPairList & props, Node & node) {
    s32 required = 1;
    std::string variant;
    node.label.clear();
    node.timeframeIndex = Speech::InvalidTimeframeIndex;
    for (StringPairList::const_iterator it = props.begin();
	 it != props.end(); ++it) {
	switch (it->first.at(0)) {
	case 'I':
	    if (!Core::strconv(it->second, node.id))
		error("Unable to parse \"%s\"", it->second.c_str());
	    else
		--required;
	    break;
	case 'W':
	    if (!Core::strconv(it->second, node.label)) {
		error("Unable to parse \"%s\"", it->second.c_str());
		node.label.clear();
	    }
	    break;
	case 'v':
	    s32 variantNo;
	    if (!Core::strconv(it->second, variantNo))
		error("Unable to parse \"%s\"", it->second.c_str());
	    else
		if (variantNo > 0) variant = it->second;
	    break;
	case 't':
	    f64 time;
	    if (!Core::strconv(it->second, time))
		error("Unable to parse \"%s\"", it->second.c_str());
	    node.timeframeIndex = timeframeIndex(time);
	    break;
	}
    }
    if (!node.label.empty())
	node.lemmas = parseLabel(node.label, variant);
    else
	node.lemmas = LemmaRange(LemmaIterator(), LemmaIterator());
    return (required) ? htkParseNodeError : htkOk;
}

HtkReader::Status HtkReader::set(const StringPairList & props, Link & link) {
    s32 required = 3;
    std::string variant;
    link.label.clear();
    link.amScore = 0.0;
    link.lmScore = 0.0;
    for (StringPairList::const_iterator it = props.begin();
	 it != props.end(); ++it) {
	switch (it->first.at(0)) {
	case 'J':
	    if (!Core::strconv(it->second, link.id))
		error("Unable to parse \"%s\"", it->second.c_str());
	    else
		--required;
	    break;
	case 'S':
	    if (!Core::strconv(it->second, link.sourceState))
		error("Unable to parse \"%s\"", it->second.c_str());
	    else
		--required;
	    break;
	case 'E':
	    if (!Core::strconv(it->second, link.targetState))
		error("Unable to parse \"%s\"", it->second.c_str());
	    else
		--required;
	    break;
	case 'W':
	    if (!Core::strconv(it->second, link.label)) {
		error("Unable to parse \"%s\"", it->second.c_str());
		link.label.clear();
	    }
	    break;
	case 'v':
	    s32 variantNo;
	    if (!Core::strconv(it->second, variantNo))
		error("Unable to parse \"%s\"", it->second.c_str());
	    else
		if (variantNo > 0) variant = it->second;
	    break;
	case 'a':
	    if (!Core::strconv(it->second, link.amScore))
		error("Unable to parse \"%s\"", it->second.c_str());
	    break;
	case 'l':
	    if (!Core::strconv(it->second, link.lmScore))
		error("Unable to parse \"%s\"", it->second.c_str());
	    break;
	}
    }
    if (!link.label.empty())
	link.lemmas = parseLabel(link.label, variant);
    else {
	link.label  = states_[link.sourceState].label;
	link.lemmas = states_[link.sourceState].lemmas;
    }
    return (required) ? htkParseLinkError : htkOk;
}

const char * HtkStatusMessage[] = {
    "Everything's fine!",
    "Error while parsing property",
    "Unexpected line; don't know what to do with that line",
    "Unexpected node; did not expect the definition of a node",
    "Error while parsing node",
    "Error while adding node",
    "Error while parsing link",
    "Error while adding link",
    "No initial state, i.e. the lattice is empty",
    "No final state"
};
bool HtkReader::checkStatus(Status status) {
    if (status != htkOk) {
	error("Error while parsing \n%3d: %s", line_, HtkStatusMessage[status]) ;
	return false;
    } else return true;
}


void HtkReader::nextLine(std::istream & is, std::string & s) {
    do {
	++line_;
	std::getline(is, s);
	Core::stripWhitespace(s);
	if (!s.empty() && (s.at(0) == '#'))
	    { comment(s); s.clear(); }
    } while (s.empty() && is);
}

ConstWordLatticeRef HtkReader::read(std::istream & is) {
    lattice_ = new Fsa::StaticAutomaton;
    wordBoundaries_ = new WordBoundaries;

    line_ = 0;
    status_ = htkOk;
    u32 state = 0;
    StringPairList _props;
    std::string s;

    {
	if (!checkStatus(status_ = start()))
	    return ConstWordLatticeRef();
    }
    {
	// header
	nextLine(is, s);
	while ((state == 0) && (status_ == htkOk) && is) {
	    switch (s.at(0)) {
	    case 'I':
		state = 1;
		break;
	    case 'J':
		state = 2;
		break;
	    default:
		status_ = add(s, _props);
		nextLine(is, s);
	    }
	}
	status_ = header(_props);
	_props.clear();
    }
    {
	// nodes
	Node _node;
	while ((state == 1) && (status_ == htkOk) && is) {
	    switch (s.at(0)) {
	    case 'I':
		if ((status_ = add(s, _props)) != htkOk)
		    break;
		if ((status_ = set(_props, _node)) != htkOk)
		    break;
		if ((status_ = node(_node)) != htkOk)
		    break;
		_props.clear();
		nextLine(is, s);
		break;
	    case 'J':
		state = 2;
		break;
	    default:
		status_ = htkUnexpectedLine;
	    }
	}
	if (!checkStatus(status_))
	    return ConstWordLatticeRef();
    }
    {
	// links
	Link _link;
	while ((state == 2) && (status_ == htkOk) && is) {
	    switch (s.at(0)) {
	    case 'I':
		status_ = htkUnexpectedNode;
		break;
	    case 'J':
		if ((status_ = add(s, _props)) != htkOk)
		    break;
		if ((status_ = set(_props, _link)) != htkOk)
		    break;
		if ((status_ = link(_link)) != htkOk)
		    break;
		_props.clear();
		nextLine(is, s);
		break;
	    default:
		status_ = htkUnexpectedLine;
	    }
	}
	if (!checkStatus(status_))
	    return ConstWordLatticeRef();
    }
    {
	status_ = end();
	if (!checkStatus(status_))
	    return ConstWordLatticeRef();
    }

    Core::Ref<WordLattice> f(new WordLattice);
    f->setWordBoundaries(Core::Ref<WordBoundaries>(wordBoundaries_));
    f->setFsa(Fsa::ConstAutomatonRef(lattice_), WordLattice::totalFsa);
    ConstWordLatticeRef l = Lattice::composeMatching(lemmaPronunciationToHtkTransducerRef_, f);
    WordLattice *wordLattice = new WordLattice;
    wordLattice->setWordBoundaries(l->wordBoundaries());
    wordLattice->setFsa(
	Fsa::staticCopy(
	    Fsa::projectInput(
		l->part(WordLattice::totalFsa))),
	    WordLattice::totalFsa);

    lattice_ = 0;
    wordBoundaries_ = 0;
    timeOffset_ = Core::Type<f64>::min;
    states_.clear();
	return ConstWordLatticeRef(wordLattice);
}

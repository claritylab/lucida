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
#include <Core/StringUtilities.hh>
#include "AlphabetXml.hh"
#include "StorageXml.hh"

namespace Fsa {

    void StorageAutomatonXmlParser::startFsa(const Core::XmlAttributes atts) {
	const char *tmp = atts["type"];
	if (!tmp) {
	    parser()->warning("no type specified for fsa. assuming transducer.");
	    fsa_->setType(TypeTransducer);
	} else {
	    Core::Choice::Value type = TypeChoice[std::string(reinterpret_cast<const char*>(tmp))];
	    if (type != Core::Choice::IllegalValue)
		fsa_->setType(Type(type));
	}
	tmp = atts["semiring"];
	if (!tmp) {
	    parser()->warning("no semiring specified for fsa. assuming tropical semiring.");
	    fsa_->setSemiring(TropicalSemiring);
	} else {
	    Core::Choice::Value type = SemiringTypeChoice[std::string(reinterpret_cast<const char*>(tmp))];
	    if (type != Core::Choice::IllegalValue) fsa_->setSemiring(getSemiring(SemiringType(type)));
	    else {
		parser()->warning("unknown semiring specified for fsa. assuming tropical semiring.");
		fsa_->setSemiring(TropicalSemiring);
	    }
	}
	tmp = atts["initial"];
	if (!tmp) {
	    parser()->warning("no initial state id specified for fsa.");
	    fsa_->setInitialStateId(InvalidStateId);
	} else {
	    char *end;
	    StateId id = strtoll(tmp, &end, 0);
	    if (*end != '\0') parser()->error("invalid initial state id \"%s\"", tmp);
	    fsa_->setInitialStateId(id);
	}
    }

    void StorageAutomatonXmlParser::startState(const Core::XmlAttributes atts) {
	const char *idStr = atts["id"];
	if (idStr) {
	    char *end;
	    StateId id = strtoll((const char*) idStr, &end, 0);
	    if (*end != '\0') parser()->error("invalid state id \"%s\"", idStr);
	    else if (id > StateIdMask) parser()->error("state id too large (%d > %d)", id, StateIdMask);
	    state_ = new State(id);
	} else parser()->error("no state id specified");
    }
    void StorageAutomatonXmlParser::endState() {
	fsa_->setState(state_);
    }
    void StorageAutomatonXmlParser::startUser(const Core::XmlAttributes atts) {
	if (state_) state_->addTags(StateTagUser);
    }
    void StorageAutomatonXmlParser::startFinal(const Core::XmlAttributes atts) {
	if (state_) {
	    state_->addTags(StateTagFinal);
	    state_->weight_ = fsa_->semiring()->one();
	}
    }
    void StorageAutomatonXmlParser::startFinalWeight(const Core::XmlAttributes atts) {
	contentFinalWeight_.resize(0);
    }
    void StorageAutomatonXmlParser::endFinalWeight() {
	Core::stripWhitespace(contentFinalWeight_);
	state_->weight_ = fsa_->semiring()->fromString(contentFinalWeight_);
    }
    void StorageAutomatonXmlParser::charactersFinalWeight(const char *ch, int len) {
	contentFinalWeight_.append((char*)ch, len);
    }

    void StorageAutomatonXmlParser::startArc(const Core::XmlAttributes atts) {
	const char *idStr = atts["target"];
	if (idStr) {
	    char *end;
	    StateId id = strtoll((const char*) idStr, &end, 0);
	    if (*end != '\0') parser()->error("invalid target state id \"%s\"", idStr);
	    else if (id > StateIdMask) parser()->error("target id too large (%d > %d)", id, StateIdMask);
	    arc_ = state_->newArc();
	    arc_->target_ = id;
	    arc_->input_ = Epsilon;
	    arc_->output_ = Epsilon;
	} else parser()->error("no state id specified");
    }
    void StorageAutomatonXmlParser::charactersArc(const char *ch, int len) {
	content_.append((char*)(ch), len);
    }
    void StorageAutomatonXmlParser::startIn(const Core::XmlAttributes atts) {
	content_.resize(0);
    }
    void StorageAutomatonXmlParser::endIn() {
	Core::stripWhitespace(content_);
	if (!content_.empty()) {
	    if (content_[0] != '*') {
		char *end;
		LabelId id = strtoll((const char*)content_.c_str(), &end, 0);
		if (*end != '\0') parser()->error("invalid input label id \"%s\"", content_.c_str());
		arc_->input_ = id;
	    } else {
		LabelId id = fsa_->getInputAlphabet()->index(content_);
		if (id != InvalidLabelId) arc_->input_ = id;
		else parser()->error("unknown special label id \"%s\"", content_.c_str());
	    }
	    if (fsa_->type() == TypeAcceptor) arc_->output_ = arc_->input_;
	} else parser()->error("input label contains no content");
    }
    void StorageAutomatonXmlParser::startOut(const Core::XmlAttributes atts) {
	if (fsa_->type() == TypeAcceptor) parser()->error("acceptor must not have output labels");
	content_.resize(0);
    }
    void StorageAutomatonXmlParser::endOut() {
	Core::stripWhitespace(content_);
	if (!content_.empty()) {
	    if (content_[0] != '*') {
		char *end;
		LabelId id = strtoll((const char*)content_.c_str(), &end, 0);
		if (*end != '\0') parser()->error("invalid output label id \"%s\"", content_.c_str());
		arc_->output_ = id;
	    } else {
		LabelId id = fsa_->getOutputAlphabet()->index(content_);
		if (id != InvalidLabelId) arc_->output_ = id;
		else parser()->error("unknown special label id \"%s\"", content_.c_str());
	    }
	} else parser()->error("output label contains no content");
    }
    void StorageAutomatonXmlParser::startArcWeight(const Core::XmlAttributes atts) {
	content_.resize(0);
    }
    void StorageAutomatonXmlParser::endArcWeight() {
	Core::stripWhitespace(content_);
	arc_->weight_ = fsa_->semiring()->fromString(content_);
    }

    StorageAutomatonXmlParser::StorageAutomatonXmlParser(const Core::Configuration &c, StorageAutomaton *fsa) :
	Core::XmlSchemaParser(c), fsa_(fsa)
    {
	fsa_->setType(TypeTransducer);
	Core::Ref<StaticAlphabet> input(new StaticAlphabet), output(new StaticAlphabet);
	fsa_->setInputAlphabet(input);
	fsa_->setOutputAlphabet(output);
	setRoot(new Core::XmlMixedElementRelay
		("fsa", this, startHandler(&Self::startFsa), 0, 0,
		 XML_CHILD(new AlphabetXmlParser("input-alphabet", this, input)),
		 XML_CHILD(new AlphabetXmlParser("output-alphabet", this, output)),
		 XML_CHILD(new Core::XmlMixedElementRelay
			   ("state", this, startHandler(&Self::startState), endHandler(&Self::endState), 0,
			    XML_CHILD(new Core::XmlEmptyElementRelay("user", this, startHandler(&Self::startUser))),
			    XML_CHILD(new Core::XmlEmptyElementRelay("final", this, startHandler(&Self::startFinal))),
			    XML_CHILD(new Core::XmlMixedElementRelay
				      ("weight", this, startHandler(&Self::startFinalWeight),
				       endHandler(&Self::endFinalWeight), charactersHandler(&Self::charactersFinalWeight),
				       XML_NO_MORE_CHILDREN)),
			    XML_CHILD(new Core::XmlMixedElementRelay
				      ("arc", this, startHandler(&Self::startArc), 0, 0,
				       XML_CHILD(new Core::XmlMixedElementRelay
						 ("in", this, startHandler(&Self::startIn),
						  endHandler(&Self::endIn), charactersHandler(&Self::charactersArc),
						  XML_NO_MORE_CHILDREN)),
				       XML_CHILD(new Core::XmlMixedElementRelay
						 ("out", this, startHandler(&Self::startOut),
						  endHandler(&Self::endOut), charactersHandler(&Self::charactersArc),
						  XML_NO_MORE_CHILDREN)),
				       XML_CHILD(new Core::XmlMixedElementRelay
						 ("weight", this, startHandler(&Self::startArcWeight),
						  endHandler(&Self::endArcWeight), charactersHandler(&Self::charactersArc),
						  XML_NO_MORE_CHILDREN)),
				       XML_NO_MORE_CHILDREN)),
			    XML_NO_MORE_CHILDREN)),
		 XML_NO_MORE_CHILDREN));
    }

    bool StorageAutomatonXmlParser::parseString(const std::string &str) {
	return (Core::XmlSchemaParser::parseString(str.c_str()) == 0);
    }

    bool StorageAutomatonXmlParser::parseStream(std::istream &i) {
	return (Core::XmlSchemaParser::parseStream(i) == 0);
    }

    bool StorageAutomatonXmlParser::parseFile(const std::string &filename) {
	return (Core::XmlSchemaParser::parseFile(filename.c_str()) == 0);
    }

} // namespace Fsa

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
#ifndef _T_FSA_STORAGE_XML_HH
#define _T_FSA_STORAGE_XML_HH

#include <Core/StringUtilities.hh>
#include <Core/XmlBuilder.hh>
#include <Core/XmlParser.hh>
#include "tStorage.hh"
#include "AlphabetXml.hh"
#include "Types.hh"

namespace Ftl {

    template<class _Automaton>
    class StorageAutomatonXmlParser : public Core::XmlSchemaParser {
    private:
	typedef StorageAutomatonXmlParser<_Automaton> Self;
	typedef Core::XmlSchemaParser Precursor;
	typedef Resources<_Automaton> _Resources;
	typedef StorageAutomaton<_Automaton> _StorageAutomaton;
	typedef typename _Automaton::Weight _Weight;
	typedef typename _Automaton::Arc _Arc;
	typedef typename _Automaton::State _State;
	typedef typename _Automaton::Semiring _Semiring;
	typedef typename _Automaton::ConstSemiringRef _ConstSemiringRef;
    private:
	const _Resources &resources_;
	_StorageAutomaton *fsa_;
	_State *state_;
	_Arc* arc_;
	std::string content_;

    private:
	void startFsa(const Core::XmlAttributes atts) {
	    const char *tmp = atts["type"];
	    if (!tmp) {
		parser()->warning("no type specified. set type to transducer.");
		fsa_->setType(Fsa::TypeTransducer);
	    } else {
		// Core::Choice::Value type = Fsa::TypeChoice[std::string(reinterpret_cast<const char*>(tmp))];
		Core::Choice::Value type = Fsa::TypeChoice[std::string(tmp)];
		if (type != Core::Choice::IllegalValue)
		    fsa_->setType(Fsa::Type(type));
		else {
		    parser()->warning("unknown type \"%s\". set type to transducer.", tmp);
		    fsa_->setType(Fsa::TypeTransducer);
		}
	    }

	    _ConstSemiringRef semiring;
	    tmp = atts["semiring"];
	    if (tmp) {
		semiring = resources_.getSemiring(tmp);
		if (semiring) fsa_->setSemiring(semiring);
		else parser()->warning("%s semiring not found", tmp);
	    }
	    if (!(semiring)) {
		if (fsa_->semiring())
		    parser()->warning("semiring not specified or not found. use %s semiring.",
				      fsa_->semiring()->name().c_str());
		else
		    parser()->error("semiring not specified or not found. no semiring set.");
	    }

	    tmp = atts["initial"];
	    if (!tmp) {
		parser()->warning("no initial state id specified for fsa.");
		fsa_->setInitialStateId(Fsa::InvalidStateId);
	    } else {
		char *end;
		Fsa::StateId id = strtoll(tmp, &end, 0);
		if (*end != '\0') {
		    parser()->error("invalid initial state id \"%s\"", tmp);
		    fsa_->setInitialStateId(Fsa::InvalidStateId);
		} else
		    fsa_->setInitialStateId(id);
	    }
	}

	void startState(const Core::XmlAttributes atts) {
	    const char *idStr = atts["id"];
	    if (idStr) {
		char *end;
		Fsa::StateId id = strtoll((const char*) idStr, &end, 0);
		if (*end != '\0') parser()->error("invalid state id \"%s\"", idStr);
		else if (id > Fsa::StateIdMask) parser()->error("state id too large (%d > %d)", id, Fsa::StateIdMask);
		state_ = fsa_->createState(id);
	    } else parser()->error("no state id specified");
	}
	void endState() {
	    fsa_->setState(state_);
	}
	void startUser(const Core::XmlAttributes atts) {
	    if (state_) state_->addTags(Fsa::StateTagUser);
	}
	void startFinal(const Core::XmlAttributes atts) {
	    if (state_) {
		state_->addTags(Fsa::StateTagFinal);
		state_->weight_ = fsa_->semiring()->defaultWeight();
	    }
	}
	void startFinalWeight(const Core::XmlAttributes atts) {
	    content_.resize(0);
	}
	void endFinalWeight() {
	    Core::stripWhitespace(content_);
	    state_->weight_ = fsa_->semiring()->fromString(content_);
	}

	void startArc(const Core::XmlAttributes atts) {
	    const char *idStr = atts["target"];
	    if (idStr) {
		char *end;
		Fsa::StateId id = strtoll((const char*) idStr, &end, 0);
		if (*end != '\0') parser()->error("invalid state id \"%s\"", idStr);
		else if (id > Fsa::StateIdMask) parser()->error("state id too large (%d > %d)", id, Fsa::StateIdMask);
		arc_ = state_->newArc();
		arc_->target_ = id;
		arc_->input_ = Fsa::Epsilon;
		arc_->output_ = Fsa::Epsilon;
		arc_->weight_ = fsa_->semiring()->defaultWeight();
	    } else parser()->error("no state id specified");
	}
	void startIn(const Core::XmlAttributes atts) {
	    content_.resize(0);
	}
	void endIn() {
	    Core::stripWhitespace(content_);
	    if (!content_.empty()) {
		if (content_[0] != '*') {
		    char *end;
		    Fsa::LabelId id = strtoll((const char*)content_.c_str(), &end, 0);
		    if (*end != '\0') parser()->error("invalid input label id \"%s\"", content_.c_str());
		    arc_->input_ = id;
		} else {
		    Fsa::LabelId id = fsa_->getInputAlphabet()->index(content_);
		    if (id != Fsa::InvalidLabelId) arc_->input_ = id;
		    else parser()->error("unknown special label id \"%s\"", content_.c_str());
		}
		if (fsa_->type() == Fsa::TypeAcceptor) arc_->output_ = arc_->input_;
	    } else parser()->error("input label contains no content");
	}
	void startOut(const Core::XmlAttributes atts) {
	    if (fsa_->type() == Fsa::TypeAcceptor) parser()->error("acceptor must not have output labels");
	    content_.resize(0);
	}
	void endOut() {
	    Core::stripWhitespace(content_);
	    if (!content_.empty()) {
		if (content_[0] != '*') {
		    char *end;
		    Fsa::LabelId id = strtoll((const char*)content_.c_str(), &end, 0);
		    if (*end != '\0') parser()->error("invalid output label id \"%s\"", content_.c_str());
		    arc_->output_ = id;
		} else {
		    Fsa::LabelId id = fsa_->getOutputAlphabet()->index(content_);
		    if (id != Fsa::InvalidLabelId) arc_->output_ = id;
		    else parser()->error("unknown special label id \"%s\"", content_.c_str());
		}
	    } else parser()->error("output label contains no content");
	}
	void startArcWeight(const Core::XmlAttributes atts) {
	    content_.resize(0);
	}
	void endArcWeight() {
	    Core::stripWhitespace(content_);
	    arc_->weight_ = fsa_->semiring()->fromString(content_);
	}
	void charactersFsa(const char *ch, int len) {
	    content_.append((char*)(ch), len);
	}

    public:
	StorageAutomatonXmlParser(const _Resources &resources, _StorageAutomaton *fsa) :
	    Precursor(resources.getConfiguration()), resources_(resources), fsa_(fsa) {
	    fsa_->setType(Fsa::TypeTransducer);
	    Core::XmlMixedElement *arcElement = new Core::XmlMixedElementRelay
		("arc", this, startHandler(&Self::startArc), 0, 0,
		 XML_CHILD(new Core::XmlMixedElementRelay
			   ("in", this, startHandler(&Self::startIn),
			    endHandler(&Self::endIn), charactersHandler(&Self::charactersFsa),
			    XML_NO_MORE_CHILDREN)),
		 XML_CHILD(new Core::XmlMixedElementRelay
			   ("out", this, startHandler(&Self::startOut),
			    endHandler(&Self::endOut), charactersHandler(&Self::charactersFsa),
			    XML_NO_MORE_CHILDREN)),
		 XML_CHILD(new Core::XmlMixedElementRelay
			   ("weight", this, startHandler(&Self::startArcWeight),
			    endHandler(&Self::endArcWeight), charactersHandler(&Self::charactersFsa),
			    XML_NO_MORE_CHILDREN)),
		 XML_NO_MORE_CHILDREN);
	    Core::XmlMixedElement *stateElement = new Core::XmlMixedElementRelay
		("state", this, startHandler(&Self::startState), endHandler(&Self::endState), 0,
		 XML_CHILD(new Core::XmlEmptyElementRelay
			   ("user", this, startHandler(&Self::startUser))),
		 XML_CHILD(new Core::XmlEmptyElementRelay
			   ("final", this, startHandler(&Self::startFinal))),
		 XML_CHILD(new Core::XmlMixedElementRelay
			   ("weight", this, startHandler(&Self::startFinalWeight),
			    endHandler(&Self::endFinalWeight), charactersHandler(&Self::charactersFsa),
			    XML_NO_MORE_CHILDREN)),
		 arcElement,
		 XML_NO_MORE_CHILDREN);

	    if (fsa_->getInputAlphabet()) {
		if (fsa_->type() == Fsa::TypeTransducer) verify(fsa_->getOutputAlphabet());
		Core::XmlMixedElement *rootElement = new Core::XmlMixedElementRelay
		    ("fsa", this, startHandler(&Self::startFsa), 0, 0,
		     XML_CHILD(new Core::XmlIgnoreElement
			       ("input-alphabet", this)),
		     XML_CHILD(new Core::XmlIgnoreElement
			       ("output-alphabet", this)),
		     stateElement,
		     XML_NO_MORE_CHILDREN);
		setRoot(rootElement);
	    } else {
		Core::Ref<Fsa::StaticAlphabet> input(new Fsa::StaticAlphabet), output(new Fsa::StaticAlphabet);
		fsa_->setInputAlphabet(input);
		if (fsa_->type() == Fsa::TypeTransducer) fsa_->setOutputAlphabet(output);
		Core::XmlMixedElement *rootElement = new Core::XmlMixedElementRelay
		    ("fsa", this, startHandler(&Self::startFsa), 0, 0,
		     XML_CHILD(new Fsa::AlphabetXmlParser
			       ("input-alphabet", this, input)),
		     XML_CHILD(new Fsa::AlphabetXmlParser
			       ("output-alphabet", this, output)),
		     stateElement,
		     XML_NO_MORE_CHILDREN);
		setRoot(rootElement);
	    }
	}

	bool parseString(const std::string &str) {
	    return (Precursor::parseString(str.c_str()) == 0);
	}
	bool parseStream(std::istream &i) {
	    return (Precursor::parseStream(i) == 0);
	}
	bool parseFile(const std::string &filename) {
	    return (Precursor::parseFile(filename.c_str()) == 0);
	}
    };

} // namespace Ftl

#endif // _T_FSA_STORAGE_XML_HH

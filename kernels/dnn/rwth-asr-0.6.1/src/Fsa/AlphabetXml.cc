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
#include "AlphabetXml.hh"

namespace Fsa {

    void AlphabetXmlParser::symbolStart(const Core::XmlAttributes atts) {
	const char *indexStr = atts["index"];
	if (indexStr) {
	    char *end;
	    index_ = strtoll((const char*) indexStr, &end, 0);
	    if (*end != '\0') parser()->error("invalid index \"%s\"", indexStr);
	    else if ((index_ < 0) || (LabelId(index_) > Core::Type<LabelId>::max))
		parser()->error("index %ld out of range (%d..%d)", index_,
				Core::Type<LabelId>::min, Core::Type<LabelId>::max);
	} else parser()->error("no index specified");
	symbol_.resize(0);
	isDisambiguator_ = false;
    }

    void AlphabetXmlParser::symbolEnd() {
	Core::stripWhitespace(symbol_);
	if (!a_->symbol(index_).empty()) parser()->error("ignoring duplicate index %ld", long(index_));
	else a_->addIndexedSymbol(symbol_, index_, isDisambiguator_);
    }

    void AlphabetXmlParser::symbolDisambiguatorStart(const Core::XmlAttributes atts) {
	isDisambiguator_ = true;
    }

    void AlphabetXmlParser::alphabetCharacters(const char *ch, int len) {
	symbol_.append((char*)(ch), len);
    }

    AlphabetXmlParser::AlphabetXmlParser(
	const char *name, Core::XmlContext *context, Core::Ref<StaticAlphabet> a)
	:
	Core::XmlRegularElement(name, context), a_(a)
    {
	XmlElement *symbol = new Core::XmlMixedElementRelay
	    ("symbol", this, startHandler(&Self::symbolStart), endHandler(&Self::symbolEnd),
	     charactersHandler(&Self::alphabetCharacters),
	     XML_CHILD(new Core::XmlEmptyElementRelay
		       ("disambiguator", this, startHandler(&Self::symbolDisambiguatorStart))),
	     XML_NO_MORE_CHILDREN);
	addTransition(initial, initial, symbol);
	addFinalState(initial);
    }

} // namespace Fsa

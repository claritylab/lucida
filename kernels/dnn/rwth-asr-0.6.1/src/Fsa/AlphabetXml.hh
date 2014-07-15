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
#ifndef _FSA_ALPHABET_XML_HH
#define _FSA_ALPHABET_XML_HH

#include <Core/XmlBuilder.hh>
#include <Core/XmlParser.hh>
#include "Alphabet.hh"

namespace Fsa {

    /**
     * The Alphabet XML parser. This class is used when reading an automaton
     * from an XML formatted stream.
     */
    class AlphabetXmlParser : public Core::XmlRegularElement {
    private:
	typedef AlphabetXmlParser Self;
	Core::Ref<StaticAlphabet> a_;
	long index_;
	std::string symbol_;
	bool isDisambiguator_;

	void symbolStart(const Core::XmlAttributes atts);
	void symbolEnd();
	void symbolDisambiguatorStart(const Core::XmlAttributes atts);
	void alphabetCharacters(const char *ch, int len);

    public:
	AlphabetXmlParser(const char *name, Core::XmlContext *context, Core::Ref<StaticAlphabet> a);
    };

} // namespace Fsa

#endif // _FSA_ALPHABET_XML_HH

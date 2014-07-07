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
#ifndef _FSA_STORAGE_XML_HH
#define _FSA_STORAGE_XML_HH

#include <Core/XmlBuilder.hh>
#include <Core/XmlParser.hh>
#include "Storage.hh"

namespace Fsa {

    class StorageAutomatonXmlParser : public Core::XmlSchemaParser {
    private:
	typedef StorageAutomatonXmlParser Self;
	StorageAutomaton *fsa_;
	State *state_;
	Arc* arc_;
	std::string content_;
	std::string contentFinalWeight_;

	void startFsa(const Core::XmlAttributes atts);
	void startState(const Core::XmlAttributes atts);
	void endState();
	void startUser(const Core::XmlAttributes atts);
	void startFinal(const Core::XmlAttributes atts);
	void startFinalWeight(const Core::XmlAttributes atts);
	void endFinalWeight();
	void charactersFinalWeight(const char *ch, int len);
	void startArc(const Core::XmlAttributes atts);
	void charactersArc(const char *ch, int len);
	void startIn(const Core::XmlAttributes atts);
	void endIn();
	void startOut(const Core::XmlAttributes atts);
	void endOut();
	void startArcWeight(const Core::XmlAttributes atts);
	void endArcWeight();

    public:
	StorageAutomatonXmlParser(const Core::Configuration &c, StorageAutomaton *fsa);
	bool parseString(const std::string &str);
	bool parseStream(std::istream &i);
	bool parseFile(const std::string &filename);
    };

} // namespace Fsa

#endif // _FSA_STORAGE_XML_HH

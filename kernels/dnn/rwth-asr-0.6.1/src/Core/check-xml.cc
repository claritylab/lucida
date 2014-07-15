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
#include <iostream>
#include "Application.hh"
#include "XmlParser.hh"
#include "XmlBuilder.hh"
#include "Unicode.hh"

class EventDrivenParser :
    public Core::XmlParser
{
private:
    int state;
protected:
    virtual void startElement(const char *name, const Core::XmlAttributes atts) {
	const char *lang = atts["lang"] ;

	if (strcmp(name, "orth") == 0) {
	    if (lang && strcmp(lang, "de") == 0)
		state = 1;
	}
    }

    virtual void endElement(const char *name) {
	if (strcmp(name, "orth") == 0)
	    state = 0;
    }

    virtual void characters(const char *ch, int len) {
	if (state) {
	    log() << std::string(ch, len);
	}
    }

public:
    EventDrivenParser(const Core::Configuration &c) : Core::XmlParser(c) {
	state=0;
    }
};


class MySchemaParser :
    public Core::XmlSchemaParser
{
    typedef MySchemaParser Self;

    class OrthographyElement :
	public Core::XmlBuilderElement<std::string, Core::XmlMixedElement, Core::CreateStatic>
    {
	typedef Core::XmlBuilderElement<std::string, Core::XmlMixedElement, Core::CreateStatic> Predecessor;
    public:
	OrthographyElement(Core::XmlContext *_context, Handler _handler = 0) :
	    Predecessor("orth", _context, _handler)
	{
	    addChild(new Core::XmlIgnoreElement("noise",      this));
	    addChild(new Core::XmlIgnoreElement("hesitation", this));
	    flattenUnknownElements();
	}

	virtual void characters(const char *ch, int len) {
	    product_ += std::string(ch, len);
	}
    };

    void startRecording(const Core::XmlAttributes atts) {
	const char *audio = atts["audio"] ;
	if (audio)
	    log("working on ") << audio;
    }

    void orth(const std::string &s) {
	log() << s;
    }

public:
    MySchemaParser(const Core::Configuration &c) : Core::XmlSchemaParser(c) {
	Core::XmlElement *orth_element = new OrthographyElement(
	    this, OrthographyElement::handler(&Self::orth));

	Core::XmlMixedElement *segment_element = new Core::XmlMixedElementRelay(
	    "segment", this);
	segment_element->addChild(orth_element);
	segment_element->ignoreUnknownElements();

	Core::XmlMixedElement *recording_element = new Core::XmlMixedElementRelay(
	    "recording", this,
	    Core::XmlMixedElementRelay::startHandler(&Self::startRecording));
	recording_element->addChild(segment_element);
	recording_element->ignoreUnknownElements();

	Core::XmlMixedElement *corpus = new Core::XmlMixedElementRelay(
	    "corpus", this);
	corpus->addChild(recording_element);

	setRoot(corpus);
    }
};


class TestApplication :
    public Core::Application
{
public:
    virtual std::string getUsage() const {
	return "short program to test the XML parser\n";
    }

    int main(const std::vector<std::string> &arguments) {
	Core::XmlParser *parser = 0;

//        parser - new EventDrivenParser(select("parser"));
	parser = new MySchemaParser(select("parser"));

	for (unsigned int i = 0 ; i < arguments.size() ; ++i) {
	    parser->parseFile(arguments[i].c_str());
	    parser->parseFile(arguments[i].c_str());
	}

	delete parser;

	return 0;
    }
};

APPLICATION(TestApplication)

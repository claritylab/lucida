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
// $Id: Attributes.cc 9227 2013-11-29 14:47:07Z golik $

#include "Attributes.hh"

using namespace Flow;


Attributes::Parser::Parser(const Core::Configuration &c) :
    Core::XmlSchemaParser(c),
    attribs_(0)
{
    setRoot(new Core::XmlMixedElementRelay
	    ("flow-attributes", this,
	     0, 0, 0,
	     XML_CHILD(new Core::XmlEmptyElementRelay("flow-attribute", this, startHandler(&Self::startAttribute))),
	     XML_NO_MORE_CHILDREN));
}

void Attributes::Parser::startAttribute(const Core::XmlAttributes atts) {
    const char *name = atts["name"];
    if (!name) std::cerr << "no name specified for attribute";
    const char *value = atts["value"];
    if (!value) std::cerr << "no value specified for attribute";
    attribs_->set(std::string(name), std::string(value));
}


bool Attributes::Parser::buildFromString(Attributes &attribs, const std::string &str)  {
    attribs_ = &attribs;
    bool result = (parseString(str.c_str()) == 0);
    attribs_ = 0;
    return result;
}

bool Attributes::Parser::buildFromStream(Attributes &attribs, std::istream &i) {
    attribs_ = &attribs;
    bool result = (parseStream(i) == 0);
    attribs_ = 0;
    return result;
}

bool Attributes::Parser::buildFromFile(Attributes &attribs, const std::string &filename) {
    attribs_ = &attribs;
    bool result = (parseFile(filename.c_str()) == 0);
    attribs_ = 0;
    return result;
}

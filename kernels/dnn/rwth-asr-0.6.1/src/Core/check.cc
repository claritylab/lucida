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
// $Id: check.cc 7257 2009-07-10 12:17:00Z rybach $

/**
 * This program illustrates typical use of some SprintCore functions.
 */

#include "Application.hh"
#include "Channel.hh"
#include "Choice.hh"
#include "Component.hh"
#include "CompressedStream.hh"
#include "Configuration.hh"
#include "Parameter.hh"
#include "ProgressIndicator.hh"
#include "XmlStream.hh"
#include "Tokenizer.hh"
#include <string>

using namespace Core;


// ===========================================================================
// Component

class Tester :
    public Component
{
public:
    enum Flavour { vanilla, strawberry, chocolate };
    static const Choice flavourChoice;

    static const ParameterBool   paramBoolean;
    static const ParameterInt    paramInteter;
    static const ParameterFloat  paramFloat;
    static const ParameterString paramString;
    static const ParameterChoice paramFlavour;

private:
    bool b;
    s32 i;
    f32 f;
    std::string s;
    Flavour l;

    XmlChannel ch;
public:
    Tester(const Configuration& c) :
	Component(c),
	ch(config, "results")
    {
	// parameters should be read once
	b = paramBoolean(config);
	i = paramInteter(config);
	f = paramFloat(config);
	s = paramString(config);
	l = Flavour(paramFlavour(config));
    }

    void doIt(int x) {
	ch << i * f * x << "\n";
	log("flavour: ") << l;
	warning("This is a warning.") << " line " << __LINE__ ;
	error("This is an error.") << "line " << __LINE__ ;
    }
};

const Choice Tester::flavourChoice(
    "vanilla",    vanilla,
    "strawberry", strawberry,
    "chocolate",  chocolate,
    Core::Choice::endMark());

const ParameterBool   Tester::paramBoolean("bool",   "a boolean parameter");
const ParameterInt    Tester::paramInteter("int",    "an integer parameter between -10 and 10", 0, -10, 10);
const ParameterFloat  Tester::paramFloat  ("float",  "a real valued parameter");
const ParameterString Tester::paramString ("string", "a string parameter", "DEFAULT");
const ParameterChoice Tester::paramFlavour(
    "flavour", &flavourChoice, "a multiple choice paramter", Choice::IllegalValue);


// ===========================================================================
// Application

class TestApplication :
    public Application
{
public:
    std::string getUsage() const {
	return "short program to demonstrate features of the Core library";
    }

    int main(const std::vector<std::string> &arguments) {
	for (std::vector<std::string>::const_iterator itArg = arguments.begin(); itArg != arguments.end(); ++itArg) {
	    StringTokenizer tokenizer(*itArg, ".");
	    std::vector<std::string> components = tokenizer();
	    Configuration _config(config);
	    for (std::vector<std::string>::const_iterator itComponent = components.begin(); itComponent != components.end() - 1; ++itComponent)
		_config = Configuration(_config, *itComponent);
	    ParameterString param(components.back().c_str(), "", "n/a");
	    std::string result = param(_config);
	    std::cerr << *itArg << "=" << result << std::endl;

	}


/*
	Core::XmlOutputStream xw(new Core::CompressedOutputStream("/tmp/xm.gz"));
	xw << Core::XmlOpen("fsm")
	   << "HELLO WORLD"
	   << Core::XmlOpenComment() << "AAA --- BBB -- CCC ---- DDD" << Core::XmlCloseComment()
	   << Core::XmlClose("fsm");
*/
	return 0;
    }
};

APPLICATION(TestApplication)

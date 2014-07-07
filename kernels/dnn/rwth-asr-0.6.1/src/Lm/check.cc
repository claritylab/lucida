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

#include <Core/Application.hh>
#include <Bliss/Lexicon.hh>
#include <Bliss/CorpusDescription.hh>
#include <Fsa/Basic.hh>
#include <Fsa/Compose.hh>
#include <Fsa/Output.hh>
#include <vector>
#include <cmath>
#include <fstream>

#include "Module.hh"
#include "ClassLm.hh"
#include "IndexMap.hh"


class TestApplication : public Core::Application {
private:
  static const Core::ParameterString paramDumpLexiconFsa_;
  static const Core::ParameterString paramDumpLmFsa_;
  static const Core::ParameterString paramDrawLmFsa_;

public:
    virtual std::string getUsage() const { return "short program to test Lm features\n"; }
    TestApplication() : Core::Application() { setTitle("check"); }

    virtual int main(const std::vector<std::string> &arguments) {
	// load lexicon
	Bliss::LexiconRef lex = Bliss::Lexicon::create(select("lexicon"));
	if (!lex) criticalError("failed to load lexicon");

	// load language model
	Core::Ref<Lm::LanguageModel> lm = Lm::Module::instance().createLanguageModel(select("lm"), lex);
	if (!lm) criticalError("failed to initialize language model");

	// check for class language model
	Lm::ClassLm *classLm = dynamic_cast<Lm::ClassLm*>(lm.get());
	if (classLm) {
	    log("class lm found");
	    classLm->classMapping()->writeClasses(log());
	    Fsa::ConstAutomatonRef f = lm->getFsa();
	    Fsa::info(f, log(), true);
	}


	return 0;
    }

};

APPLICATION(TestApplication)

const Core::ParameterString TestApplication::paramDumpLexiconFsa_("dump-lexicon-fsa", "dump lexicon as fsa to file");
const Core::ParameterString TestApplication::paramDumpLmFsa_("dump-lm-fsa", "dump lm as fsa to file");
const Core::ParameterString TestApplication::paramDrawLmFsa_("draw-lm-fsa", "draw lm as fsa to file");

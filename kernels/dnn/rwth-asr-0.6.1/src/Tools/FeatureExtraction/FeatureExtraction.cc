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
#include <Modules.hh>
#include <Am/Module.hh>
#include <Audio/Module.hh>
#include <Core/Application.hh>
#include <Flow/Module.hh>
#include <Lm/Module.hh>
#include <Math/Module.hh>
#include <Mm/Module.hh>
#include <Signal/Module.hh>
#include <Speech/Module.hh>
#include <Flow/Registry.hh>
#include <cstdlib>
#include <unistd.h>
#ifdef MODULE_NN
#include <Nn/Module.hh>
#endif


class ExtractionApplication :
    public Core::Application
{
private:
    enum Format {noFormat, sietillFormat, mm2Format};
    static const Core::Choice choiceFormat;
    static const Core::ParameterChoice paramFormat;
    static const Core::ParameterBool  paramVerbose;
private:

    Speech::CorpusProcessor* createCorpusProcessor() {

	switch (paramFormat(config)) {
	default:
	    log("not storing features");
	    return new Speech::DataExtractor(config);
	    break;
	}
    }

public:

    ExtractionApplication() {
	INIT_MODULE(Am);
	INIT_MODULE(Audio);
	INIT_MODULE(Flow);
	INIT_MODULE(Lm);
	INIT_MODULE(Math);
	INIT_MODULE(Mm);
	INIT_MODULE(Signal);
	INIT_MODULE(Speech);
#ifdef MODULE_NN
	INIT_MODULE(Nn);
#endif
	setTitle("extraction");
    }

    int main(const std::vector<std::string> &arguments) {
	// dump useful debugging information
	if (paramVerbose(getConfiguration())) {
	    std::cout << "--- filter(s) ---" << std::endl;
	    Flow::Registry::instance().dumpFilters(std::cout);
	    std::cout << std::endl;
	    std::cout << "--- datatype(s) ---" << std::endl;
	    Flow::Registry::instance().dumpDatatypes(std::cout);
	    std::cout << std::endl;
	} else {
	    Speech::CorpusProcessor* corpusProcessor = createCorpusProcessor();

	    Speech::CorpusVisitor corpusVisitor(config);
	    corpusProcessor->signOn(corpusVisitor);
	    Bliss::CorpusDescription corpusDescription(select("corpus"));
	    corpusDescription.accept(&corpusVisitor);

	    delete corpusProcessor;
	}
	return EXIT_SUCCESS;
    }

};

APPLICATION(ExtractionApplication)

const Core::Choice ExtractionApplication::choiceFormat(
    "none",    noFormat,
    "sietill", sietillFormat,
    "mm2", mm2Format,
    Core::Choice::endMark());

const Core::ParameterChoice ExtractionApplication::paramFormat(
    "feature-format", &choiceFormat, "file format for generated features", noFormat);

const Core::ParameterBool   ExtractionApplication::paramVerbose(
    "verbose", "verbose mode", false);

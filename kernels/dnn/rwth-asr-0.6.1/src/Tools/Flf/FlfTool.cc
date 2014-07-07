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
#include <Core/Parameter.hh>
#include <Flf/Module.hh>
#include <Flf/CorpusProcessor.hh>
#include <Flf/Network.hh>
#include <Flf/NodeFactory.hh>
#include <Flf/Processor.hh>
#include <Flow/Module.hh>
#include <Lm/Module.hh>
#include <Math/Module.hh>
#include <Mm/Module.hh>
#include <Signal/Module.hh>
#include <Speech/Module.hh>

#ifdef MODULE_NN
#include <Nn/Module.hh>
#endif

class FlfTool : public Core::Application
{
private:
    virtual std::string getApplicationDescription() const {
	return
	    "The FLF lattice tool runs a lattice processing network\n"
	    "for a Bliss corpus or a sequence of batches depending\n"
	    "on the chosen nodes.\n"
	    "The network has to be provided as Sprint configuration file,\n"
	    "i.e. the use of the option \"--config\" is mandatory for\n"
	    "running a network.\n"
	    "\n"
	    "The following command line commands are supported:\n"
	    "  (none)         runs the network\n"
	    "  init           initializes the network, but does not\n"
	    "                 run it\n"
	    "  parse          parses the network, but neither\n"
	    "                 initializes nor runs it\n"
	    "  help           gives some general help on how setting\n"
	    "                 up a network\n"
	    "  help list      lists all nodes; help is available for\n"
	    "                 each node\n"
	    "  help NODE      gives help to the specific node\n";
    }

    void printHelp(std::ostream &os) {
	os << "Structure of a network:\n"
	   << "\n"
	   << "[*.network]\n"
	   << "initial-nodes    = INITIAL_NODE ...\n"
	   << "\n"
	   << "[*.network.INITIAL_NODE]\n"
	   << "type             = TYPE\n"
	   << "links            = PORT->TARGET_NODE:PORT ...\n"
	   << "...\n"
	   << "\n"
	   << "[*.network.TARGET_NODE]\n"
	   << "type             = TYPE\n"
	   << "...\n"
	   << "\n"
	   << "Common initial nodes are \"speech-segment\" and \"batch\",\n"
	   << "common final node is \"sink\".\n"
	   << "The links syntax can be shorten. The header \"PORT->\"\n"
	   << "can be skipped; source-port is set to 0. In addition or\n"
	   << "alternatively the tail \":PORT\" can be skipped;\n"
	   << "target-port is set to 0.\n"
	   << "\n"
	   << "Try \"help list\" to get a list of all available nodes.\n"
	   << "Try \"help NODE\" to get help for a specific node.\n"
	   << "\n";
    }

    Flf::Processor * createProcessor(Flf::Network *network) {
	Flf::Processor *processor = 0;
	if ((processor = Flf::CorpusProcessor::create(config, network)))
	    return processor;
	return new Flf::Processor(config, network);
    }

public:
    FlfTool() {
	INIT_MODULE(Flf);
	INIT_MODULE(Am);
	INIT_MODULE(Audio);
	INIT_MODULE(Flow);
	INIT_MODULE(Math);
	INIT_MODULE(Mm);
	INIT_MODULE(Lm);
	INIT_MODULE(Signal);
	INIT_MODULE(Speech);
#ifdef MODULE_NN
	INIT_MODULE(Nn);
#endif
	setTitle("flf-lattice-tool");
	setDefaultLoadConfigurationFile(false);
	setDefaultOutputXmlHeader(false);
    }

    int main(const std::vector<std::string> &arguments) {
	enum { Run, Init, Parse, Help } cmd = Run;
	Flf::Module::Instance &flfApp = Flf::Module::instance();
	if (!arguments.empty()) {
	    if      ((arguments.front() == "init") ||
		     (arguments.front() == "check")) cmd = Init;
	    else if  (arguments.front() == "parse")  cmd = Parse;
	    else if  (arguments.front() == "help")   cmd = Help;
	}
	switch (cmd) {
	case Run: case Init: case Parse: {
	    /*! @todo network, lexicon, processor should not be part of Flf::Module (?) */
	    flfApp.setNetwork(Flf::Network::createNetwork(select("network")));
	    if (cmd == Parse) {
		std::cout << "Network was successfully parsed:" << std::endl << std::endl;
		flfApp.network()->dump(std::cout);
		break;
	    }
	    log("Initialize network ...");
	    flfApp.init();
	    flfApp.setProcessor(createProcessor(flfApp.network()));
	    bool good = flfApp.processor()->init(arguments);
	    if (cmd == Init) {
		std::cout << "Network was successfully initialized:" << std::endl << std::endl;
		flfApp.processor()->crawler().dump(std::cout);
		break;
	    }
	    log("Process network...");
	    if (good) flfApp.processor()->run();
	    flfApp.processor()->finalize();
	} break;
	case Help: {
	    if (arguments.size() == 1) {
		printHelp(std::cout);
	    } else {
		Flf::NodeFactory factory;
		if (arguments[1] == "list") {
		    factory.dumpNodeList(std::cout);
		    std::cout << "Try \"help <node> to get help for a specific node.\n" << std::endl;
		} else {
		    for (u32 i = 1; i < arguments.size(); ++i)
			{ factory.dumpNodeDescription(std::cout, arguments[i]); std::cout << std::endl; }
		}
	    }
	} break; }

	delete flfApp.processor();
	delete flfApp.network();
	return EXIT_SUCCESS;
    }
};

APPLICATION(FlfTool)

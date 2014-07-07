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
#include <Core/Application.hh>
#include <Core/XmlStream.hh>
#include <Flow/Module.hh>
#include <Flow/Attributes.hh>
#include <Flow/Registry.hh>

class TestApplication : public Core::Application {
public:
    std::string getUsage() const {
	return "short program to test flow network\n";
    }

    int main(const std::vector<std::string> &arguments) {
	INIT_MODULE(Flow);

	std::cout << "--- filters ---" << std::endl;
	Flow::Registry::instance().dumpFilters(std::cout);
	std::cout << std::endl << "--- datatypes ---" << std::endl;
	Flow::Registry::instance().dumpDatatypes(std::cout);

	Flow::Attributes atts;
	atts.set("datatype", "generic-vector-f32");
	atts.set("sample-rate", "8000");
	atts.set("sample-size", "1");
	{
	    std::ofstream o("xxx");
	    Core::XmlWriter xw(o);
	    xw << atts;
	}
	Flow::Attributes atts2;
	Flow::Attributes::Parser parser(select("atts"));
	parser.buildFromFile(atts2, "xxx");
	Core::XmlWriter xw(std::cout);
	xw << atts2;

	return 0;
    }
};

APPLICATION(TestApplication)

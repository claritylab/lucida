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
#include <Core/XmlParser.hh>
#include <Core/XmlStream.hh>

#include "Lattice.hh"
#include "Semiring.hh"

#include <fstream>

// ===========================================================================
// Application

class MyApplication : public Core::Application {
public:
    std::string getUsage() const {
	return "...";
    }

    int main(const std::vector<std::string> &arguments) {
	std::ifstream fis(arguments[0].c_str());
	Flf::ConstSemiringRef semiring = Flf::Semiring::read(fis);

	std::cout << semiring->name() << std::endl;
	std::cout << std::endl;

	Core::XmlWriter xml(std::cout);
	semiring->write(xml);

	return 0;
    }

} app; // <- You have to create ONE instance of the application

APPLICATION(MyApplication)

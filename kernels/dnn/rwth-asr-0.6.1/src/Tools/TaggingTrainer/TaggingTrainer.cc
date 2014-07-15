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
/*
 * TaggingTrainer.cc
 *
 *  Created on: Jan 23, 2012
 *      Author: lehnen
 */

#include <Core/Application.hh>
#include <Tagging/Module.hh>

class TaggingTrainer : public Core::Application {
	public:
	TaggingTrainer() : Core::Application() {
		INIT_MODULE(Tagging);
		setTitle("tagging-trainer");
	}

	virtual std::string getUsage() const {
		return "tagging model manipulator";
	}

	virtual int main(const std::vector<std::string> &arguments) {
		Tagging::Module::instance().shortModelLifeCycle(select("feature-holder"));

		return EXIT_SUCCESS;
	}
};

APPLICATION(TaggingTrainer)

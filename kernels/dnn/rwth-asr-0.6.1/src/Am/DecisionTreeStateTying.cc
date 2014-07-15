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
#include "DecisionTreeStateTying.hh"
#include <Core/MD5.hh>

using namespace Am;

DecisionTreeStateTying::DecisionTreeStateTying(
    const Core::Configuration & config,
    ClassicStateModelRef stateModel) :
    Core::Component(config),
    Precursor(config, stateModel),
    tree_(config, stateModel),
    props_(0)
{
    std::string filename = paramFilename(config);
    if (!tree_.loadFromFile(filename)) {
		criticalError("failure while reading decision tree from \"%s\"", filename.c_str());
		return;
    }
    Core::MD5 md5;
    if (md5.updateFromFile(filename))
		dependency_.setValue(md5);
    else
		warning("could not derive md5 sum from file '%s'", filename.c_str());
	log("dependency value: %s", std::string(md5).c_str());

    props_ = new Properties(tree_.getMap());
    /*
    if (classifyDumpChannel_.isOpen())
		dumpStateTying(classifyDumpChannel_);
    */
}


DecisionTreeStateTying::~DecisionTreeStateTying()
{
    delete props_;
}

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
#include "ClassLabelWrapper.hh"
#include <Math/Module.hh>
#include <Math/Vector.hh>

using namespace Nn;

const Core::ParameterIntVector ClassLabelWrapper::paramDisregardClasses(
	"disregard-classes",
	"list of class indices that are disregarded in posterior calculation, e.g. states of mul-phoneme",
	",", 0);

const Core::ParameterString ClassLabelWrapper::paramSaveToFile(
	"save-to-file",	"save label information to this file","");

const Core::ParameterString ClassLabelWrapper::paramLoadFromFile(
	"load-from-file", "load label information from this file","");

const Core::ParameterInt ClassLabelWrapper::paramNumberOfClasses(
	"number-of-classes", "number of classes", 0);

ClassLabelWrapper::ClassLabelWrapper(const Core::Configuration& config, u32 nclasses) :
	Core::Component(config),
	nTargets_(0)
{
    std::string filename(paramLoadFromFile(config));
    if (filename != ""){
	if (!load(filename))
	    error("failed to read mapping from file: ") << filename;
    }
    else {
	log("generating network-output-to-class-index-mapping from config file");
	if (nclasses == 0)
	    nclasses = paramNumberOfClasses(config);
	if (nclasses == 0){
	    error("need to set configuration parameter 'number-of-classes'");
	}
	initMapping(nclasses);
	filename = paramSaveToFile(config);
	if (filename != "")
	    save(filename);
    }
}

/* initialize mapping_ from config*/
void ClassLabelWrapper::initMapping(u32 nclasses){
    mapping_.resize(nclasses);
    std::vector<s32> disregardedClasses = paramDisregardClasses(config);
    for (u32 c = 0; c < nclasses; c++) {
	bool disregardClass = std::find(disregardedClasses.begin(), disregardedClasses.end(), c) != disregardedClasses.end();
	if (!disregardClass){
	    mapping_.at(c) = nTargets_;
	    nTargets_++;
	}
	else{
	    mapping_.at(c) = -1;
	}
    }
    log("number of classes to accumulate: ") << nTargets_;
}

bool ClassLabelWrapper::load(const std::string &filename){
    log("loading network-output-to-class-index-mapping from file ") << filename;
    Math::Vector<s32> mapping;
    if (!Math::Module::instance().formats().read(filename, mapping))
	return false;
    mapping_ = mapping;
    std::map<s32,s32> tmpMap;
    for (u32 i = 0; i < mapping_.size(); i++){
	if (mapping_[i] != -1)
	    tmpMap[i] = mapping_[i];
    }
    nTargets_ = tmpMap.size();
    return true;
}

bool ClassLabelWrapper::save(const std::string &filename) const {
    log("saving network-output-to-class-index-mapping to file ") << filename;
    Math::Vector<s32> mapping(mapping_);
    if (!Math::Module::instance().formats().write(filename, mapping))
	return false;
    return true;
}


bool ClassLabelWrapper::isOneToOneMapping() const {
    std::map<u32,std::set<u32> > outputToEmissionMap;
    for (u32 classIndex = 0; classIndex < nClasses(); classIndex++){
	if (isClassToAccumulate(classIndex)){
	    u32 outputIndex = getOutputIndexFromClassIndex(classIndex);
	    outputToEmissionMap[outputIndex].insert(classIndex);
	}
    }

    bool isOneToOne = true;
    for (u32 outputIndex = 0; outputIndex < nClassesToAccumulate(); outputIndex++)
	isOneToOne = isOneToOne && (outputToEmissionMap[outputIndex].size() <= 1);

    return isOneToOne;
}

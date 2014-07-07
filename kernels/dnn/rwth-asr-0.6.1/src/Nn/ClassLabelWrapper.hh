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
#ifndef _CLASS_LABEL_WRAPPER_HH
#define _CLASS_LABEL_WRAPPER_HH

#include <map>
#include <vector>

#include <Core/Component.hh>

/* ClassLabelWrapper
 *
 * used for mapping between network outputs and class indices
 * mapping can be loaded from file or set via configuration
 *
 * all classes that are not disregarded are set to a value between 0 and #Classes - #disregardedClasses - 1
 * classes are assumed to be labeled 0,1,2,...
 */

namespace Nn {

class ClassLabelWrapper : public Core::Component {
//    typedef Mm::MixtureToMixtureMap Precursor;
protected:
    static const Core::ParameterIntVector paramDisregardClasses;
    static const Core::ParameterString paramLoadFromFile;
    static const Core::ParameterString paramSaveToFile;
    static const Core::ParameterInt paramNumberOfClasses;
protected:
    std::vector<s32> mapping_;
    u32 nTargets_;
public:
    ClassLabelWrapper(const Core::Configuration& config, u32 nClasses = 0);
    virtual ~ClassLabelWrapper() {}

    u32 nClasses() const { return mapping_.size(); };
    u32 nClassesToAccumulate() const { return nTargets_;}

    bool isClassToAccumulate(u32 s) const { return mapping_.at(s) != -1 ; }
    u32 getOutputIndexFromClassIndex(u32 s) const { return mapping_[s]; }
    bool isOneToOneMapping() const;
    bool load(const std::string &filename);
    bool save(const std::string &filename) const;
protected:
    void initMapping(u32 nClasses);
};


}

#endif

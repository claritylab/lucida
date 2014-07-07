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
#ifndef _NN_MODULE_HH
#define _NN_MODULE_HH

#include <Core/Factory.hh>
#include <Core/Singleton.hh>

#include <Flow/Module.hh>

namespace Core
{
    class FormatSet;
}


namespace Nn {

class Module_ {
private:
    Core::FormatSet *formats_;
public:
    Module_();
    ~Module_();
    enum { FeatureScorerTypeOffset = 0x300 };
    enum FeatureScorerType {
	nnOnDemanHybrid = FeatureScorerTypeOffset,
	nnFullHybrid = FeatureScorerTypeOffset + 1,
	nnPrecomputedHybrid = FeatureScorerTypeOffset + 2,
	nnBatchFeatureScorer = FeatureScorerTypeOffset + 3,
	nnCached = FeatureScorerTypeOffset + 4,
    };

    /** Set of file format class.
     */
    Core::FormatSet &formats();

};

typedef Core::SingletonHolder<Module_> Module;

}

#endif // _NN_MODULE_HH

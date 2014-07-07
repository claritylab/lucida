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
#ifndef _SPEECH_INITADAPTATIONTREE_HH
#define _SPEECH_INITADAPTATIONTREE_HH

#include <Core/ReferenceCounting.hh>
#include <Core/BinaryTree.hh>
#include <Legacy/DecisionTree.hh>
#include <Am/ClassicStateModel.hh>

namespace Speech{

    class InitAdaptationTree{

    private:
	const Legacy::PhoneticDecisionTree *mllrDectree_;

    public:

	InitAdaptationTree(Am::ClassicStateModelRef, const Core::Configuration&);
	//	InitAdaptationTree(const Am::AllophoneStateAlphabet &, const Core::Configuration&);
	~InitAdaptationTree();

	Core::Ref<Core::BinaryTree> adaptationTree();
	u32 classify(const Bliss::Phoneme::Id&) const;

    };
}
#endif // _SPEECH_INITADAPTATIONTREE_HH

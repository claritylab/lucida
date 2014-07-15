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
#include <Core/Assertions.hh>
#include "InitAdaptationTree.hh"


using namespace Speech;

InitAdaptationTree::InitAdaptationTree(
    //    const Am::AllophoneStateAlphabet &allophoneStateAlphabet, const Core::Configuration& config ) :
    Am::ClassicStateModelRef stateModel, const Core::Configuration& config ) :
    mllrDectree_(0){


    mllrDectree_ = new Legacy::PhoneticDecisionTree(config, stateModel);
    //    mllrDectree_ = new Legacy::PhoneticDecisionTree(config, allophoneStateAlphabet);
    ensure(mllrDectree_);
}

InitAdaptationTree::~InitAdaptationTree(){
    delete mllrDectree_;
}

Core::Ref<Core::BinaryTree> InitAdaptationTree::adaptationTree(){
    Core::Ref<Core::BinaryTree>
	tree(new Core::BinaryTree(mllrDectree_->treeStructure()));
    ensure(tree);
    return tree;
}


u32 InitAdaptationTree::classify(const Bliss::Phoneme::Id &p) const{

    Am::Allophone ap(p, Am::Allophone::isInitialPhone | Am::Allophone::isFinalPhone);

    return mllrDectree_->classify(Am::AllophoneState(ap, 0));
}

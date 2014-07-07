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
#include <Am/ClassicAcousticModel.hh>
#include <Core/Assertions.hh>
#include <Legacy/DecisionTree.hh>
#include "AdaptationTree.hh"
#include "DecisionTreeStateTying.hh"

using namespace Am;

const Core::ParameterInt AdaptationTree::paramNumberOfBaseClasses(
    "base-classes",
    "number of leafs in MLLR tree",
    70, 1);

AdaptationTree::AdaptationTree(
    const Core::Configuration& c,
    const Am::ClassicStateModelRef stateModel,
    Bliss::Phoneme::Id silencePhoneme) :
    Core::Component(c),
    treeDumpChannel_(config, "dump-tree" ) {
    numberOfBaseClasses_=paramNumberOfBaseClasses(config);
    log("number of base classe for MLLR adaptation: ") << numberOfBaseClasses_;

    switch (ClassicAcousticModel::paramType(config, ClassicAcousticModel::oldCartTying)) {
    case ClassicAcousticModel::oldCartTying:
	loadLegacyDecisionTree(config, stateModel, silencePhoneme);
	break;
    case ClassicAcousticModel::cartTying:
	loadDecisionTree(config, stateModel, silencePhoneme);
	break;
    default:
	criticalError("decision tree required");
    }
    if(treeDumpChannel_.isOpen())
	tree_->draw(treeDumpChannel_);
}

AdaptationTree::~AdaptationTree(){}

void AdaptationTree::loadLegacyDecisionTree(
    const Core::Configuration &config,
    const Am::ClassicStateModelRef stateModel,
    Bliss::Phoneme::Id silencePhoneme) {
    Legacy::PhoneticDecisionTree mllrDectree(config, stateModel);

    const Am::Allophone *siAllo = stateModel->allophoneAlphabet().allophone(
	Am::Allophone(silencePhoneme,
		      Am::Allophone::isInitialPhone | Am::Allophone::isFinalPhone));
    Am::AllophoneState siAlloState = stateModel->allophoneStateAlphabet().allophoneState(
	siAllo, 0);
    silenceMixtures_.insert(mllrDectree.classify(siAlloState));
    // log("Using separate MLLR adaptation for /%s/",
    //     acousticModel_->phonemeInventory()->phoneme(p)->symbol().c_str());

    tree_ = Core::ref(new Core::BinaryTree(mllrDectree.treeStructure()));
    ensure(tree_);
    leafIndex_ = tree_->prune(numberOfBaseClasses_);

    /*! \todo change if silence is handled correctly by dectree */
    (*leafIndex_).push_back(numberOfBaseClasses_);
    nLeafs_ = numberOfBaseClasses_ + 1;
    //end of part to be changed
}

void AdaptationTree::loadDecisionTree(
    const Core::Configuration &config,
    const Am::ClassicStateModelRef stateModel,
    Bliss::Phoneme::Id silencePhoneme) {
    Am::DecisionTreeStateTying cartStateTying(config, stateModel);

    const Am::Allophone *siAllo = stateModel->allophoneAlphabet().allophone(
	Am::Allophone(silencePhoneme,
		      Am::Allophone::isInitialPhone | Am::Allophone::isFinalPhone));
    Am::AllophoneState siAlloState = stateModel->allophoneStateAlphabet().allophoneState(
	siAllo, 0);
    Mm::MixtureIndex siMixtureId = cartStateTying.classify(siAlloState);
    silenceMixtures_.insert(siMixtureId);

    // Build tree structure as intermediate representation for getting a Core::BinaryTree
    // deprecated: use directly Cart::DecisionTree instead of Core::BinaryTree
    // deprecated: don't separate explicitly silence, rather verify that silence is in the pruned tree
    Core::BinaryTree::TreeStructure structure;
    {
	const Cart::BinaryTree::Node &root = cartStateTying.decisionTree().root();
	/*
	  First question separates silence,
	  i.e. the decision tree must have been built in the following way:
	  the first question asks the central phoneme for being the (unique, single-state) silence phoneme,
	  the left node (answer=YES) is a leaf, while the right node is splitted further
	*/
	verify(root.leftChild().isLeaf() && (root.leftChild().id() == siMixtureId));
	verify((root.leftChild().info<Cart::TrainingInformation>().order == 1)
	       && (root.rightChild().info<Cart::TrainingInformation>().order == 2));
	std::stack<const Cart::BinaryTree::Node *> S;
	S.push(&root.rightChild());
	while (!S.empty()) {
	    const Cart::BinaryTree::Node *node = S.top(); S.pop();
	    u32 order = node->info<Cart::TrainingInformation>().order - 2;
	    if (node->isLeaf()) {
		Mm::MixtureIndex leafNumber = node->id_;
		if (leafNumber > siMixtureId) --leafNumber;
		structure.push_back(Core::BinaryTree::TreeStructureEntry(order, leafNumber));
	    } else {
		structure.push_back(Core::BinaryTree::TreeStructureEntry(order, Core::BinaryTree::invalidId));
		S.push(&node->rightChild());
		S.push(&node->leftChild());
	    }
	}
    }
    tree_ = Core::ref(new Core::BinaryTree(structure));
    ensure(tree_);
    leafIndex_ = tree_->prune(numberOfBaseClasses_);

    /*! \todo change if silence is handled correctly by dectree */
    (*leafIndex_).push_back(numberOfBaseClasses_);
    nLeafs_ = numberOfBaseClasses_ + 1;
    //end of part to be changed
}

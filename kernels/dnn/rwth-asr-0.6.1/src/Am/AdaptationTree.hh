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
#ifndef _AM_ADAPTATION_TREE_HH
#define _AM_ADAPTATION_TREE_HH

#include <Core/ReferenceCounting.hh>
#include <Core/BinaryTree.hh>
#include <Am/ClassicStateModel.hh>
#include <Mm/Types.hh>

namespace Am{

    class AdaptationTree: public Core::Component, public Core::ReferenceCounted
    {
    private:
	Core::Ref<Core::BinaryTree> tree_;
	std::set<Mm::MixtureIndex> silenceMixtures_;
	u16 numberOfBaseClasses_;
	Core::BinaryTree::LeafIndexVector leafIndex_;
	u32 nLeafs_;

	static const Core::ParameterInt paramNumberOfBaseClasses;
	mutable Core::Channel treeDumpChannel_;
    private:
	void loadLegacyDecisionTree(
	    const Core::Configuration &config,
	    const Am::ClassicStateModelRef stateModel,
	    Bliss::Phoneme::Id silencePhoneme);
	void loadDecisionTree(
	    const Core::Configuration &config,
	    const Am::ClassicStateModelRef stateModel,
	    Bliss::Phoneme::Id silencePhoneme);
    public:
	AdaptationTree(const Core::Configuration&, const Am::ClassicStateModelRef, Bliss::Phoneme::Id);
	~AdaptationTree();

	Core::Ref<Core::BinaryTree> tree() const {return tree_;}
	std::set<Mm::MixtureIndex> silenceMixtures() const {return silenceMixtures_;}
	Core::BinaryTree::LeafIndexVector leafIndex() const {return leafIndex_;}
	u32 nLeafs() const {return nLeafs_;}
    };
}
#endif // _AM_ADAPTATION_TREE_HH

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
// $Id: DecisionTree.hh 6678 2007-10-30 13:05:28Z rybach $

#ifndef _LEGACY_DECISION_TREE_HH
#define _LEGACY_DECISION_TREE_HH

#include <Am/ClassicStateTying.hh>
#include <Core/BinaryTree.hh>
#include <Core/Component.hh>
#include <Core/Dependency.hh>


namespace Legacy {

    class PhoneticDecisionTreeBase :
	public virtual Core::Component
    {
    protected:
	enum BoundaryStyle { noPosDep, posDep, superPosDep };
	static const Core::Choice boundaryStyleChoice;
	static const Core::ParameterChoice paramBoundaryStyle;

	Core::Ref<const Bliss::PhonemeInventory> pi_;
	BoundaryStyle boundaryStyle_;
	PhoneticDecisionTreeBase(
	    const Core::Configuration&,
	    Core::Ref<const Bliss::PhonemeInventory>);
    };

    class PhoneticDecisionTree :
	public PhoneticDecisionTreeBase,
	public Am::ClassicStateTying
    {
    protected:
	Core::Dependency dependency_;

	struct LightTree;
	Bliss::PhonemeMap<signed char> phonemeMap_;
	std::vector<Bliss::Phoneme::Id> inversePhonemeMap_;
	LightTree *tree_;
	bool load(const char *filename);
	void makePhonemeMapping();
	s16 translateBoundaryFlag(u8) const;
	bool answerQuestion(short quest, short phonePosition,
			    const Am::Allophone &phone,
			    short state, short boundary) const;
	mutable Core::Channel classifyDumpChannel_;
	void checkCompatibility(const PhoneticDecisionTree& pdt2);
	void removeBeginAndEndBranches();
    public:
	static const Core::ParameterString paramFilename;
	static const Core::ParameterBool paramUseCentralStateClassesOnly;

	PhoneticDecisionTree(const Core::Configuration &,
			     Am::ClassicStateModelRef);
	virtual ~PhoneticDecisionTree();
	const Core::Dependency& getDependency() const { return dependency_; }
	virtual void getDependencies(Core::DependencySet &d) const { d.add(name(), getDependency()); }
	Core::Ref<const Bliss::PhonemeInventory> phonemeInventory() const { return pi_; }
	u32 maximumContextLength() const;
	virtual u32 nClasses() const;
	virtual u32 classify(const Am::AllophoneState&) const;
	void draw(std::ostream&) const;
	Core::BinaryTree::TreeStructure treeStructure() const;
	std::vector<std::set<u32> >  phonemeToMixtureIndices() const;
	typedef std::pair<u32, u32>  ClassPair;
	typedef std::vector<ClassPair>  ClassPairs;
	void addTree(const PhoneticDecisionTree& pdt2, ClassPairs& toBuild);
	void addTree(const PhoneticDecisionTree& pdt2) {
	    ClassPairs cp; addTree(pdt2, cp); }
    };

} // namespace Legacy

#endif // _LEGACY_DECISION_TREE_HH

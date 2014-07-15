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
#ifndef _AM_DECISION_TREE_STATE_TYING_HH
#define _AM_DECISION_TREE_STATE_TYING_HH

#include "ClassicStateTying.hh"
#include "ClassicDecisionTree.hh"

namespace Am
{
	class DecisionTreeStateTying :
		public ClassicStateTying {
	protected:
		typedef ClassicStateTying Precursor;

	private:
		PropertyMap map_;
		DecisionTree tree_;
		Properties * props_;
		Core::Dependency dependency_;

	public:
		DecisionTreeStateTying(
			const Core::Configuration & config,
			ClassicStateModelRef stateModel);

		~DecisionTreeStateTying();

		const PropertyMap & stateModelMap() const { return map_; }

		const DecisionTree & decisionTree() const { return tree_; }

		void set(const Conditions & cond) { props_->set(cond); }

		Mm::MixtureIndex nClasses() const { return Mm::MixtureIndex(tree_.nLeaves()) +1 ; }

		Mm::MixtureIndex classify(const AllophoneState &alloState) const {
			props_->set(alloState);
			return Mm::MixtureIndex(tree_.classify(*props_));
		}

		const Core::Dependency& getDependency() const { return dependency_; }

		virtual void getDependencies(Core::DependencySet &d) const { d.add(name(), getDependency()); }
	};
}

#endif // _AM_DECISION_TREE_STATE_TYING_HH

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
#ifndef _SPEECH_DECISION_TREE_TRAINER_HH
#define _SPEECH_DECISION_TREE_TRAINER_HH

#include <Am/AcousticModel.hh>
#include <Am/ClassicDecisionTree.hh>
#include <Cart/Properties.hh>
#include <Cart/DecisionTreeTrainer.hh>
#include <Mm/Types.hh>

#include <Speech/AcousticModelTrainer.hh>

namespace Speech {
    class FeatureAccumulator : public AcousticModelTrainer {
    protected:
		typedef AcousticModelTrainer Precursor;

    protected:
		Cart::ExampleList examples_;
		f64 nObs_;
		size_t nCols_;
		Cart::PropertyMapRef map_;

    protected:
		Am::AllophoneState allophoneState(Am::AllophoneStateIndex id) {
			return acousticModel()->allophoneStateAlphabet()->allophoneState(id);
		}
		Cart::Example & example(Am::AllophoneStateIndex id);

    public:
		FeatureAccumulator(const Core::Configuration & config);
		virtual ~FeatureAccumulator() {}

		const Cart::PropertyMap & map() { return *map_; }
		Cart::PropertyMapRef getMap() { return map_; }
		const Cart::ExampleList & examples() { return examples_; }

		virtual void processAlignedFeature(Core::Ref<const Feature> f, Am::AllophoneStateIndex id);
		virtual void processAlignedFeature(Core::Ref<const Feature> f, Am::AllophoneStateIndex id, Mm::Weight w);

		void write(std::ostream & out) const;
		void writeXml(Core::XmlWriter & xml) const;
    };


    // tying of mixtures
    class StateTyingDecisionTreeTrainer :
		public Cart::DecisionTreeTrainer {
    protected:
		typedef StateTyingDecisionTreeTrainer Self;
		typedef Cart::DecisionTreeTrainer Precursor;

    public:
		class LogLikelihoodGain : public Cart::Scorer {
			typedef Cart::Scorer Precursor;
		public:
			static const Core::ParameterFloat paramVarianceClipping;
		private:
			f64 minSigmaSquare_;
			mutable std::vector<f64> mu_;
			mutable std::vector<f64> sigmaSquare_;

		public:
			LogLikelihoodGain(const Core::Configuration &config);

			void write(std::ostream &os) const;

			// negated log-likelihood
			Cart::Score logLikelihood(Cart::ExamplePtrList::const_iterator begin, Cart::ExamplePtrList::const_iterator end) const;

			// compute a gain, not a distance, i.e. the higher the return score, the better
			Cart::Score operator()(
				const Cart::ExamplePtrRange &leftExamples, const Cart::ExamplePtrRange &rightExamples,
				const Cart::Score fatherLogLikelihood,
				Cart::Score &leftChildLogLikelihood, Cart::Score &rightChildLogLikelihood) const;
			void operator()(const Cart::ExamplePtrRange &examples, Cart::Score &score) const;
		};

    public:
		StateTyingDecisionTreeTrainer(const Core::Configuration & config);
    };


    // ============================================================================



} // namespace Speech

#endif // _SPEECH_DECISION_TREE_TRAINER_HH

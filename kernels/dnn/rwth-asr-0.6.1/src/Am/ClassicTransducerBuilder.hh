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
#ifndef _AM_CLASSIC_TRANSDUCER_BUILDER_HH
#define _AM_CLASSIC_TRANSDUCER_BUILDER_HH

#include "ClassicAcousticModel.hh"

namespace Am {

	class ClassicTransducerBuilder : public TransducerBuilder {
		typedef TransducerBuilder Precursor;
	protected: // configurable options
		u32 nDisambiguators_;
		bool shouldApplyTransitionModel_;
		bool acceptCoarticulatedSinglePronunciation_;
		enum {
			inputAcceptsAllophones,
			inputAcceptsAllophoneStates,
			inputAcceptsEmissionLabels
		} inputType_;
	public: // configurable options
		virtual void setDisambiguators(u32);

		virtual void selectAllophonesFromLexicon();
		virtual void selectAllAllophones();

		virtual void selectNonCoarticulatedSentenceBoundaries();
		virtual void selectCoarticulatedSinglePronunciation();

		virtual void selectAllophonesAsInput();
		virtual void selectAllophoneStatesAsInput();
		virtual void selectEmissionLabelsAsInput();

		virtual void selectFlatModel();
		virtual void selectTransitionModel();
		virtual void setSilencesAndNoises(const PronunciationList *);

	private: // internal
		Core::Ref<const ClassicAcousticModel> model_;
		Core::Ref<Fsa::StaticAutomaton> product_;
		Core::Ref<const AllophoneAlphabet> allophones_;
		Core::Ref<const AllophoneStateAlphabet> allophoneStates_;
		Core::Ref<const Bliss::PhonemeAlphabet> phonemes_;
		Core::Ref<const EmissionAlphabet> emissions_;

		const AllophoneAlphabet::AllophoneList *allophoneList_;

		const PronunciationList *silencesAndNoises_;

		enum PhoneBoundaryFlags {
			intraWord		   = 0,
			wordStart		   = 1,
			wordEnd			 = 2,
			nPhoneBoundaryFlags = 4
		};

		struct PhoneBoundaryStateDescriptor {
			Phonology::Context context;
			u8 flag; // PhoneBoundaryFlags
			bool isWordStart()	 const { return (flag & wordStart); }
			bool isWordEnd()	   const { return (flag & wordEnd); }
			bool isCoarticulated() const { return (!context.history.empty() && !context.future.empty()); }
			bool operator== (const PhoneBoundaryStateDescriptor &r) const {
				return (context == r.context) && (flag == r.flag);
			}
			struct Hash {
				Phonology::Context::Hash ch;
				u32 operator()(const PhoneBoundaryStateDescriptor &pbsd) const {
					return ch(pbsd.context) ^ (u32(pbsd.flag) << 13);
				}
			};
		};

		typedef Core::hash_map<
			PhoneBoundaryStateDescriptor,
			Fsa::State*,
			PhoneBoundaryStateDescriptor::Hash> PhoneBoundaryStates;
		PhoneBoundaryStates phoneBoundaryStates_;

		struct AllophoneSuffix {
			const ClassicHmmTopology *hmmTopology;
			AllophoneState allophoneState;
			int subState;
			Fsa::StateId target;
			mutable size_t hash_;
			AllophoneSuffix() : hash_(0) {}

			struct Hash {
				ClassicTransducerBuilder *model;
				Hash(ClassicTransducerBuilder *mm) : model(mm) {}
				size_t operator() (const AllophoneSuffix &as) const {
					if (!as.hash_) {
						as.hash_ =  model->hashSequence(as);
						if (!as.hash_) as.hash_ = 1;
					}
					return as.hash_;
				}
			};

			struct Equality {
				ClassicTransducerBuilder *model;
				Equality(ClassicTransducerBuilder *mm) : model(mm) {}
				bool operator() (const AllophoneSuffix &l, const AllophoneSuffix &r) const {
					return model->compareSequences(l, r) == 0;
				}
			};
		};
		size_t hashSequence(const AllophoneSuffix&);
		int compareSequences(const AllophoneSuffix&, const AllophoneSuffix&);
		typedef Core::hash_map<
			AllophoneSuffix,
			Fsa::StateId,
			AllophoneSuffix::Hash, AllophoneSuffix::Equality
			> AllophoneSuffixMap;
		AllophoneSuffixMap allophoneSuffixes_;

		class Statistics;
		Statistics *statistics_;

		Fsa::State *phoneBoundaryState(const PhoneBoundaryStateDescriptor&);
		void setupWordStart(Fsa::State*);
		void setupWordEnd(Fsa::State*, const PhoneBoundaryStateDescriptor&);
		Fsa::State *phoneStartState(const Allophone*);
		Fsa::State *phoneEndState(const Allophone*);
		Fsa::LabelId allophoneStateLabel(const AllophoneState&);

		void buildAllophone(const Allophone*);
		Fsa::State *buildAllophone(const Allophone*, Fsa::State *start);
		void buildAllophone(const Allophone*, Fsa::State *start, Fsa::State *end);
		void buildAllophoneStates(const Allophone*, Fsa::State *start, Fsa::State *end);
		void buildWordBoundaryLinks(Fsa::State*, Fsa::State*);
		void createEmptyTransducer();
		void buildPhoneLoop();
		Fsa::State* buildSilenceAndNoiseLoops(
			const Bliss::PhonemeInventory &, const Phonology &, Fsa::State *);
		void buildPronunciation(const Bliss::Coarticulated<Bliss::Pronunciation>&);
		void applyStateTying();
		void finalize();

	public:
		ClassicTransducerBuilder(Core::Ref<const ClassicAcousticModel>);
		virtual ~ClassicTransducerBuilder();

		/**
		 * Create an <xxx> to phoneme transducer for (almost)
		 * arbitrary phoneme sequences.
		 *
		 * Where <xxx> is
		 * allophones if selectAllophones() was called,
		 * allophone states if selectAllophoneStates() was called,
		 * emission labels if selectEmissions() was called.
		 *
		 * Call either selectAllAllophones() or
		 * selectAllophonesFromLexicon() before build() to choose the
		 * set of allophones represented by the resulting transducer.
		 *
		 * If you need disambiguators as word separators, call
		 * setDisambiguators().
		 *
		 * Call selectCoarticulatedSinglePronunciation() to create
		 * transducer which accepts only a singel pronunciation with
		 * possible coarticulations at the boundaries.
		 */
		virtual Fsa::ConstAutomatonRef createPhonemeLoopTransducer();

		/**
		 * Create an <xxx> to phoneme transducer for a specific pronunciation.
		 * For options see createPhonemeLoopTransducer().
		 */
		virtual Fsa::ConstAutomatonRef createPronunciationTransducer(
			const Bliss::Coarticulated<Bliss::Pronunciation>&);

		/**
		 * Create an <xxx> to allophone transducer for (almost)
		 * arbitrary allophone sequences.
		 * For options see createPhonemeLoopTransducer().
		 */
		virtual Fsa::ConstAutomatonRef createAllophoneLoopTransducer();

		/**
		 * Apply transition model to a flat automaton, i.e. add loop and skip transitions.
		 */
		virtual Fsa::ConstAutomatonRef applyTransitionModel(Fsa::ConstAutomatonRef);

		/**
		 * maps sequences of allophone states or emission indexes to allophones.
		 */
		virtual Fsa::ConstAutomatonRef createEmissionLoopTransducer(bool transitionModel);

		/**
		 * context dependency transducer.
		 * maps sequences of allophones to sequences of phonemes
		 */
		virtual Fsa::ConstAutomatonRef createMinimizedContextDependencyTransducer(Fsa::LabelId initialPhoneAndSilenceOffset);

	};

} // namespace Am

#endif //_AM_CLASSIC_TRANSDUCER_BUILDER_HH

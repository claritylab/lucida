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
#ifndef _AM_ACOUSTIC_MODEL_HH
#define _AM_ACOUSTIC_MODEL_HH

#include "ClassicStateModel.hh"
#include "ClassicStateTying.hh"
#include "TransitionModel.hh"
#include "ClassicHmmTopologySet.hh"
#include <Core/Parameter.hh>
#include <Core/Dependency.hh>
#include <Mc/Component.hh>
#include <Mm/ScaledFeatureScorer.hh>
#include <Speech/CorpusVisitor.hh>

namespace Am {

    class TransducerBuilder;

    /** Interface class for acoustic models
     *  Concepts and Future Plans:
     *  -An acoustic model consist of:
     *    -Emission probalility set. Interface: Mm::FeatureScorer
     *    -Tying of allophone states concerning the emission probabilities. Interface: emissionIndex(...)
     *    -Transition probalility set (only preliminary implementaion)
     *    -Tying of transition probalility concerning transitions. (Not implemented yet.)
     *    -HMM topology. Preliminary interface: hmmTopology(...)
     *  Further functionalities
     *    -Create allophone state to phoneme transducer.
     *       Interface createAllophoneStateToPhonemeTransducer
     *    -Deliver the phoneme inventory. Interface phonemeInventory().
     *    -Deliver the allophone state alphabet. Interface allophoneStateAlphabet().
     *    -Deliver a phonology object. (Needs refinements.) Interface phonology().
     *    -Deliver the silence identifier. (Not sure if necessary.) Interface silence().
     *    -Delivers if across-word model is enabled. (Not sure if necessary.)
     *       Interface isAcrossWordModelEnabled().
     */
    class AcousticModel :
	public Mc::Component,
	public Core::ReferenceCounted
    {
    private:
	mutable Core::WeakRef<const EmissionAlphabet> emissionAlphabet_;
    public:
	typedef u32 Mode;
	static const Mode complete;
	static const Mode noEmissions;
	static const Mode noStateTying;
	static const Mode noStateTransition;

	typedef Mm::MixtureIndex EmissionIndex;
	typedef int StateTransitionIndex;

    public:
	AcousticModel(const Core::Configuration &c) : Core::Component(c), Mc::Component(c) {}
	virtual ~AcousticModel() {}

	virtual void load(Mode mode = complete) = 0;

	virtual void getDependencies(Core::DependencySet &) const = 0;

	virtual Core::Ref<TransducerBuilder> createTransducerBuilder() const = 0;

	virtual Core::Ref<const Bliss::PhonemeInventory> phonemeInventory() const = 0;
	virtual Core::Ref<const AllophoneAlphabet> allophoneAlphabet() const = 0;
	virtual Core::Ref<const AllophoneStateAlphabet> allophoneStateAlphabet() const = 0;
	virtual Core::Ref<const Phonology> phonology() const = 0;
	virtual Bliss::Phoneme::Id silence() const = 0;
	virtual AllophoneStateIndex silenceAllophoneStateIndex() const = 0;

	virtual Core::Ref<Mm::AbstractMixtureSet> mixtureSet() = 0;

	virtual Core::Ref<const Mm::ScaledFeatureScorer> featureScorer() = 0;
	virtual Core::Ref<Mm::ScaledFeatureScorer> mutableFeatureScorer() = 0;
	virtual bool setFeatureScorer(Core::Ref<Mm::ScaledFeatureScorer>) = 0;

	virtual EmissionIndex emissionIndex(AllophoneState) const = 0;
	virtual EmissionIndex emissionIndex(AllophoneStateIndex) const = 0;
	virtual EmissionIndex nEmissions() const = 0;
	Core::Ref<const EmissionAlphabet> emissionAlphabet() const;
	/*! @todo Concept of transition tying not complete yet! */
	virtual StateTransitionIndex nStateTransitions() const = 0;
	virtual const StateTransitionModel* stateTransition(StateTransitionIndex) const = 0;
	virtual StateTransitionIndex stateTransitionIndex(AllophoneState, s8 subState = 0) const = 0;
	virtual StateTransitionIndex stateTransitionIndex(AllophoneStateIndex, s8 subState = 0) const = 0;

	virtual Core::Ref<const ClassicHmmTopologySet> hmmTopologySet() const = 0;
	virtual const ClassicHmmTopology* hmmTopology(Bliss::Phoneme::Id phoneme) const = 0;
	virtual bool isAcrossWordModelEnabled() const = 0;

	/** Check if feature source is compatible with the acoustic model. */
	bool isCompatible(const Mm::FeatureDescription &description) const;
	/** Check if feature source is almost compatible with the acoustic model */
	bool isWeakCompatible(const Mm::FeatureDescription& description) const;

	/** Override this function to add triggers to the corpus visitor */
	virtual void signOn(Speech::CorpusVisitor &corpusVisitor) {};

	/** Override this function to trigger manually (needed in 'Flow') */
	virtual bool setKey(const std::string &key);
    };

    typedef Core::Ref<const AcousticModel> AcousticModelRef;

    /**
     * Create transducers for the AcousticModel.
     *
     * This class is merely an interface stub for
     * ClassicTransducerBuilder.  Maybe there will be different
     * builders (corresponding to different types of models) in the
     * future, and then this class will become the abstract interface
     * to them.  For now, just ignore it and refer to
     * ClassicTransducerBuilder for stuff that actually does things.
     */

    class TransducerBuilder :
	public Core::ReferenceCounted
    {
    public:
	typedef std::vector<const Bliss::Pronunciation*> PronunciationList;
    protected:
	TransducerBuilder();
    public:
	virtual ~TransducerBuilder();

	virtual void setDisambiguators(u32) = 0;

	virtual void selectAllophonesFromLexicon() = 0;
	virtual void selectAllAllophones() = 0;

	virtual void selectAllophonesAsInput() = 0;
	virtual void selectAllophoneStatesAsInput() = 0;
	virtual void selectEmissionLabelsAsInput() = 0;

	virtual void selectFlatModel() = 0;
	virtual void selectTransitionModel() = 0;
	virtual void setSilencesAndNoises(const PronunciationList *) = 0;

	/**
	 *  Transducer should accept only coarticulated sequence of
	 *  pronunciation with non-coarticulated sequence boundaries.
	 */
	virtual void selectNonCoarticulatedSentenceBoundaries() = 0;

	/**
	 *  Transducer accept only a single pronunciation with
	 *  possible coarticulation at the boundaries.
	 */
	virtual void selectCoarticulatedSinglePronunciation() = 0;

	virtual Fsa::ConstAutomatonRef createPhonemeLoopTransducer() = 0;
	virtual Fsa::ConstAutomatonRef createPronunciationTransducer(
	    const Bliss::Coarticulated<Bliss::Pronunciation>&) = 0;
	virtual Fsa::ConstAutomatonRef createAllophoneLoopTransducer() = 0;

	virtual Fsa::ConstAutomatonRef applyTransitionModel(Fsa::ConstAutomatonRef) = 0;

	virtual Fsa::ConstAutomatonRef createEmissionLoopTransducer(bool transitionModel) = 0;
	virtual Fsa::ConstAutomatonRef createMinimizedContextDependencyTransducer(Fsa::LabelId initialPhoneAndSilenceOffset) = 0;


    };

} // namespace Am

#endif //_AM_ACOUSTIC_MODEL_HH

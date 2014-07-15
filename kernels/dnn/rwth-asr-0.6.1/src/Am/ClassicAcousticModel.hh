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
#ifndef _AM_CLASSIC_ACOUSTIC_MODEL_HH
#define _AM_CLASSIC_ACOUSTIC_MODEL_HH

#include <Core/Parameter.hh>
#include <Am/AcousticModel.hh>
#include <Am/ClassicStateModel.hh>
#include <Am/ClassicStateTying.hh>
#include <Am/ClassicHmmTopologySet.hh>
#include <Mm/FeatureScorer.hh>

namespace Am {

    /** ClassicAcousticModel
     */
    class ClassicAcousticModel : public AcousticModel {
    public:
	enum StateTyingType {
	    noTying = 0,
	    monophoneTying,
	    lutTying,
	    cartTying,
	    oldCartTying};
	static const Core::Choice choiceStateTying;
	static const Core::ParameterChoice paramType;

    protected:
	friend class ClassicTransducerBuilder;

	Bliss::LexiconRef lexiconRef_;

	Bliss::Phoneme::Id silence_;
	void determineSilencePhoneme();

	AllophoneStateIndex silenceAllophoneStateIndex_;
	void determineSilenceAllophoneStateIndex();

	ClassicStateModelRef stateModelRef_;
	bool loadStateModel();

	ClassicStateTyingRef stateTyingRef_;
	bool loadStateTying();

	Core::Ref<ScaledTransitionModel> transitionModel_;
	bool loadTransitionModel();

	Core::Ref<Mm::ScaledFeatureScorer> featureScorer_;

	Core::Ref<Mm::AbstractMixtureSet> mixtureSet_;
	bool loadMixtureSet();
	bool openMixtureSet();

    public:
	ClassicAcousticModel(const Core::Configuration &, Bliss::LexiconRef);
	virtual ~ClassicAcousticModel();

	virtual void load(Mode mode = complete);

	virtual void getDependencies(Core::DependencySet &dependencies) const;

	virtual Core::Ref<TransducerBuilder> createTransducerBuilder() const;

	Bliss::LexiconRef lexicon() const { return lexiconRef_; }
	virtual Core::Ref<const Bliss::PhonemeInventory> phonemeInventory() const {
	    return lexiconRef_->phonemeInventory();
	}

	virtual Bliss::Phoneme::Id silence() const { return silence_; }
	virtual AllophoneStateIndex silenceAllophoneStateIndex() const { return silenceAllophoneStateIndex_; }

	ClassicStateModelRef stateModel() const {
	    return stateModelRef_;
	}
	ClassicHmmTopologySetRef hmmTopologySet() const {
	    return stateModelRef_->getHmmTopologySet();
	}
	virtual ConstPhonologyRef phonology() const {
	    return stateModelRef_->getPhonology();
	}
	virtual ConstAllophoneAlphabetRef allophoneAlphabet() const {
	    return stateModelRef_->getAllophoneAlphabet();
	}
	virtual ConstAllophoneStateAlphabetRef allophoneStateAlphabet() const {
	    return stateModelRef_->getAllophoneStateAlphabet();
	}

	virtual Core::Ref<Mm::AbstractMixtureSet> mixtureSet();

	virtual Core::Ref<const Mm::ScaledFeatureScorer> featureScorer();
	virtual Core::Ref<Mm::ScaledFeatureScorer> mutableFeatureScorer();
	virtual bool setFeatureScorer(Core::Ref<Mm::ScaledFeatureScorer>);

	ClassicStateTyingRef stateTying() const {
	    return stateTyingRef_;
	}

	virtual EmissionIndex emissionIndex(AllophoneState phone) const {
	    return stateTyingRef_->classify(phone);
	}
	virtual EmissionIndex emissionIndex(AllophoneStateIndex e) const {
	    return stateTyingRef_->classifyIndex(e);
	}
	virtual EmissionIndex nEmissions() const;

	virtual StateTransitionIndex nStateTransitions() const { return transitionModel_->nModels(); }
	virtual const StateTransitionModel* stateTransition(StateTransitionIndex i) const {
	    return (*transitionModel_)[i];
	}
	virtual StateTransitionIndex stateTransitionIndex(AllophoneState phone, s8 subState = 0) const {
	    if (allophoneStateAlphabet()->isSilence(phone)) return TransitionModel::silence;
	    else return transitionModel_->classify(phone, subState);
	}
	virtual StateTransitionIndex stateTransitionIndex(AllophoneStateIndex e, s8 subState = 0) const {
	    if (e == silenceAllophoneStateIndex()) return TransitionModel::silence;
	    else return transitionModel_->classifyIndex(e, subState);
	}

	virtual const ClassicHmmTopology* hmmTopology(Bliss::Phoneme::Id phoneme) const {
	    return stateModelRef_->hmmTopologySet().get(phoneme);
	}
	virtual bool isAcrossWordModelEnabled() const {
	    return stateModelRef_->hmmTopologySet().isAcrossWordModelEnabled();
	}

	virtual void distributeScaleUpdate(const Mc::ScaleUpdate &scaleUpdate);

    };

} // namespace Am

#endif //_AM_CLASSIC_ACOUSTIC_MODEL_HH

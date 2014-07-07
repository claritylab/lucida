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
#ifndef _AM_TRANSITION_MODEL_HH
#define _AM_TRANSITION_MODEL_HH

#include <Mc/Component.hh>
#include <Mm/Types.hh>
#include "ClassicStateTying.hh"

namespace Am {

    /**
     * State transition probabilities (aka time distortion penalties)
     */

    class StateTransitionModel : public Core::Configurable {
    public:
	typedef Mm::Score Score;

	enum TransitionType {
	    loop = 0, forward = 1, skip = 2, exit = 3,
	    nTransitionTypes
	};
	static const Core::ParameterFloat paramScores[nTransitionTypes];
    private:
	Score tdps_[nTransitionTypes];
    public:
	StateTransitionModel(const Core::Configuration&);
	void set(int i, Score s) {
	    require_(0 <= i && i < nTransitionTypes);
	    tdps_[i] = s;
	}
	void load(Score scale = 1);
	void load(Score scale, const Mm::Scales &scores);
	void clear();
	Score operator[] (int i) const {
	    require_(0 <= i && i < nTransitionTypes);
	    return tdps_[i];
	}
	StateTransitionModel& operator+=(const StateTransitionModel &m);

	void getDependencies(Core::DependencySet &dependencies) const;
	void dump(Core::XmlWriter &) const;
    };

    /**
     * Model of the transition aspect of HMMs.
     * The TransitionModel takes care of "dynamic time warping" by
     * adding loop and skip transitions to the emission sequence
     * automaton.
     * @see correponsing Wiki page
     */
    class TransitionModel : public virtual Core::Component
    {
    public:
	enum StateType {
	    entryM1, entryM2, silence, phone0, phone1,
	    nStateTypes
	};
	class Applicator;
    public:
	enum TyingType { global, globalPlusNonWord, cart };
	static Core::Choice choiceTyingType;
	static Core::ParameterChoice paramTyingType;
	static Core::ParameterString paramTdpValuesFile;
    protected:
	std::vector<StateTransitionModel*> transitionModels_;
    protected:
	void dump(Core::XmlWriter &) const;
    public:
	TransitionModel(const Core::Configuration &);
	virtual ~TransitionModel();

	bool load(Mc::Scale scale);
	void clear();

	/** Verifies and corrects the different state transitions.
	 *  @return is false if correction was necessary.
	 */
	bool correct();
	u32 nModels() const { return transitionModels_.size(); }
	const StateTransitionModel *operator[] (int i) const {
	    require_(0 <= i && i < (int)transitionModels_.size() && transitionModels_[i] != 0);
	    return transitionModels_[i];
	}

	virtual void getDependencies(Core::DependencySet &dependencies) const;

	TransitionModel &operator+=(const TransitionModel &m);

	Fsa::ConstAutomatonRef apply(
	    Fsa::ConstAutomatonRef in,
	    Fsa::LabelId silenceLabel,
	    bool applyExitTransitionToFinalStates) const;

	virtual StateType classify(AllophoneState phone, s8 subState = 0) const = 0;
	virtual StateType classifyIndex(AllophoneStateIndex e, s8 subState = 0) const = 0;

	// factory
	static TransitionModel* createTransitionModel(const Core::Configuration &, ClassicStateModelRef);
    };

    class GlobalTransitionModel : public TransitionModel
    {
	typedef	TransitionModel Precursor;
    public:
	GlobalTransitionModel(const Core::Configuration &);

	virtual StateType classify(AllophoneState phone, s8 subState = 0) const {
	    return (StateType)(TransitionModel::phone0 + subState);
	}
	virtual StateType classifyIndex(AllophoneStateIndex e, s8 subState = 0) const {
	    return (StateType)(TransitionModel::phone0 + subState);
	}
    };

    /**
     * Uses separate tdps for a set of non-word phones (i.e. noise).
     *
     * Tdps are set using the selector nonword-0 and nonword-1 for
     * substate 0 and 1 respectively.
     */
    class NonWordAwareTransitionModel : public GlobalTransitionModel
    {
	typedef GlobalTransitionModel Precursor;
	static const Core::ParameterStringVector paramNonWordPhones;
    public:
	NonWordAwareTransitionModel(const Core::Configuration &c, ClassicStateModelRef stateModel);
	bool isNonWord(AllophoneStateIndex ai) const {
	    return nonWordStates_.find(ai) != nonWordStates_.end();
	}
	bool isNonWord(AllophoneState state) const {
	    return isNonWord(stateModel_->allophoneStateAlphabet().index(state));
	}

	virtual StateType classify(AllophoneState phone, s8 subState = 0) const {
	    return (StateType)(TransitionModel::phone0 + 2 * static_cast<u32>(isNonWord(phone)) + subState);
	}
	virtual StateType classifyIndex(AllophoneStateIndex e, s8 subState = 0) const {
	    return (StateType)(TransitionModel::phone0 + 2 * static_cast<u32>(isNonWord(e)) + subState);
	}
    protected:
	Core::hash_set<AllophoneStateIndex> nonWordStates_;
	ClassicStateModelRef stateModel_;
    };

    class CartTransitionModel : public TransitionModel
    {
	typedef	TransitionModel Precursor;
    protected:
	ClassicStateTyingRef stateTying_;
	u8 nSubStates_;
    public:
	CartTransitionModel(const Core::Configuration &, ClassicStateModelRef);

	virtual StateType classify(AllophoneState phone, s8 subState = 0) const {
	    return (StateType)(stateTying_->classify(phone) * nSubStates_ + TransitionModel::silence + subState);
	}
	virtual StateType classifyIndex(AllophoneStateIndex e, s8 subState = 0) const {
	    return (StateType)(stateTying_->classifyIndex(e) * nSubStates_ + TransitionModel::silence + subState);
	}
    };

    /** Log-linear scaling of transition model */
    class ScaledTransitionModel :
	public Core::ReferenceCounted,
	public Mc::Component
    {
    protected:
	TransitionModel *transitionModel_;
    public:
	ScaledTransitionModel(const Core::Configuration &, ClassicStateModelRef);
	virtual ~ScaledTransitionModel();

	u32 nModels() const { return transitionModel_->nModels(); }
	const StateTransitionModel *operator[] (int i) const { return (*transitionModel_)[i]; }
	virtual bool load() { return transitionModel_->load(scale()); }
	virtual void distributeScaleUpdate(const Mc::ScaleUpdate &scaleUpdate) { load(); }
	virtual void getDependencies(Core::DependencySet &dependencies) const {
	    transitionModel_->getDependencies(dependencies);
	}
	ScaledTransitionModel &operator+=(const ScaledTransitionModel &m) {
	    (*transitionModel_) += (*m.transitionModel_);
	    return *this;
	}
	Fsa::ConstAutomatonRef apply(
	    Fsa::ConstAutomatonRef in,
	    Fsa::LabelId silenceLabel,
	    bool applyExitTransitionToFinalStates) const {
	    return transitionModel_->apply(in, silenceLabel, applyExitTransitionToFinalStates);
	}
	TransitionModel::StateType classify(AllophoneState phone, s8 subState = 0) const {
	    return transitionModel_->classify(phone, subState);
	}
	TransitionModel::StateType classifyIndex(AllophoneStateIndex e, s8 subState = 0) const {
	    return transitionModel_->classifyIndex(e, subState);
	}
    };

    /** Log-linear combination of transition models */
    class CombinedTransitionModel : public ScaledTransitionModel {
	typedef ScaledTransitionModel Precursor;
    private:
	std::vector<Core::Ref<ScaledTransitionModel> > transitionModels_;
    public:
	CombinedTransitionModel(const Core::Configuration &,
				const std::vector<Core::Ref<ScaledTransitionModel> > &,
				ClassicStateModelRef);
	virtual ~CombinedTransitionModel();

	virtual bool load();
	virtual void distributeScaleUpdate(const Mc::ScaleUpdate &scaleUpdate);
    };
}

#endif //_AM_TRANSITION_MODEL_HH

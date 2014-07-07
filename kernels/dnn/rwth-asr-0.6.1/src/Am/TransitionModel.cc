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
#include <Modules.hh>
#include "TransitionModel.hh"
#include <stack>
#include <Core/ProgressIndicator.hh>
#include <Fsa/Dfs.hh>
#include <Fsa/Static.hh>
#include <Math/Utilities.hh>

using namespace Am;

// ===========================================================================
const Core::ParameterFloat StateTransitionModel::paramScores[nTransitionTypes] = {
    Core::ParameterFloat("loop",    "negative logarithm of probability for loop transition",     3.0),
    Core::ParameterFloat("forward", "negative logarithm of probability for forward transition",  0.0),
    Core::ParameterFloat("skip",    "negative logarithm of probability for skip transition",     3.0),
    Core::ParameterFloat("exit",    "negative logarithm of probability for word end transition", 0.0),
};

StateTransitionModel::StateTransitionModel(const Core::Configuration &c) :
    Core::Configurable(c)
{
    clear();
}

void StateTransitionModel::load(Score scale)
{
    for (int i = 0; i < nTransitionTypes; ++ i) {
	tdps_[i] = scale * paramScores[i](config);
	tdps_[i] = Core::clip(tdps_[i]);
	ensure(!Math::isnan(tdps_[i]));
    }
}

void StateTransitionModel::load(Score scale, const Mm::Scales &scores)
{
    for (int i = 0; i < nTransitionTypes; ++ i) {
	tdps_[i] = scale * scores[i];
	tdps_[i] = Core::clip(tdps_[i]);
	ensure(!Math::isnan(tdps_[i]));
    }
}

void StateTransitionModel::clear()
{
    for (int i = 0; i < nTransitionTypes; ++ i) {
	tdps_[i] = 0;
    }
}

StateTransitionModel& StateTransitionModel::operator+=(const StateTransitionModel &m)
{
    for (int i = 0; i < nTransitionTypes; ++i) {
	tdps_[i] = Core::clip(tdps_[i] + m.tdps_[i]);
	ensure(!Math::isnan(tdps_[i]));
    }
    return *this;
}

void StateTransitionModel::getDependencies(Core::DependencySet &dependencies) const
{
    std::string value;
    for (int i = 0; i < nTransitionTypes; ++i) {
	value += Core::form("%s=%f", paramScores[i].name().c_str(), tdps_[i]);
	if (i + 1 < nTransitionTypes) value += "; ";
    }
    dependencies.add(name(), value);
}

void StateTransitionModel::dump(Core::XmlWriter &o) const
{
    o << Core::XmlOpen(name())
	+ Core::XmlAttribute(paramScores[0].name(), tdps_[0])
	+ Core::XmlAttribute(paramScores[1].name(), tdps_[1])
	+ Core::XmlAttribute(paramScores[2].name(), tdps_[2])
	+ Core::XmlAttribute(paramScores[3].name(), tdps_[3]);
    o << Core::XmlClose(name());
}

// ===========================================================================
TransitionModel::TransitionModel(const Core::Configuration &c) :
    Core::Component(c)
{}

TransitionModel::~TransitionModel()
{
    for (u32 i = 0; i < transitionModels_.size(); ++ i) {
	delete transitionModels_[i];
    }
}

void TransitionModel::dump(Core::XmlWriter &o) const
{
    for (u32 t = 0; t < transitionModels_.size(); ++ t) {
	if (transitionModels_[t]) {
	    transitionModels_[t]->dump(o);
	}
    }
}

bool TransitionModel::load(Mc::Scale scale)
{
    const std::string tdpValuesFile = paramTdpValuesFile(config);
    if (tdpValuesFile.empty()) {
	for (u32 i = 0; i < transitionModels_.size(); ++ i) {
	    if (transitionModels_[i]) {
		transitionModels_[i]->load(scale);
	    }
	}
    } else {
#if 1
	criticalError("cannot load tdp values from file. Module MM_ADVANCED is not available");
#endif
    }
    correct();
    return true;
}

void TransitionModel::clear()
{
    for (u32 i = 0; i < transitionModels_.size(); ++ i) {
	if (transitionModels_[i]) {
	    transitionModels_[i]->clear();
	}
    }
}

TransitionModel &TransitionModel::operator+=(const TransitionModel &m)
{
    for (u32 i = 0; i < transitionModels_.size(); ++ i)
	(*transitionModels_[i]) += (*m.transitionModels_[i]);
    return *this;
}

bool TransitionModel::correct()
{
    bool result = true;
    for (int t = entryM1; t <= entryM2; ++ t) {
	if (Core::isSignificantlyLess((*transitionModels_[t])[StateTransitionModel::loop],
				      Core::Type<StateTransitionModel::Score>::max)) {
	    result = false;
	    warning("Changing loop probability for entry state to zero, was: %f",
		    exp(-(*transitionModels_[t])[StateTransitionModel::loop]));
	    transitionModels_[t]->set(StateTransitionModel::loop,
				      Core::Type<StateTransitionModel::Score>::max);
	}
    }
    return result;
}

void TransitionModel::getDependencies(Core::DependencySet &dependencies) const
{
    Core::DependencySet d;
    for (u32 i = 0; i < transitionModels_.size(); ++i) {
	if (transitionModels_[i]) {
	    transitionModels_[i]->getDependencies(d);
	}
    }
    dependencies.add(name(), d);
}

/**
 * Apply TransitionModel to a flat automaton.
 *
 * What is does: The purpose if this algorithm is to add loop and skip
 * transitions to a "flat" automaton.  (Flat meaning that it does not
 * contain loops and skips.)  The emission labels, i.e. the labels
 * that are repeated or skipped are on the input side of the
 * automaton, while the output labels will be unchanged.
 *
 * This can be viewed as a specialized compose algorithm for the
 * time-distortion transducer (left) and a given automaton (right).
 * If you read on, you will discover that considerable care must be
 * taken to creating compact results.
 *
 * How it works: The state space is expanded so that we remember the
 * most recent emission label, this is called "left state" in the
 * following.  "Right state" refers to the corresponding state in the
 * original automaton.  This expansion is necessary to provide the
 * loop transitions.  The representation of the left state is rather
 * verbose.  It consists of a mask stating which kinds of transition
 * are possible, a reference to the state's transition model, and of
 * course the most recent emission label.  In fact only a small number
 * of combinations of the possible values are actually used.  (One
 * could slim down the data structure to represent only the valid
 * combinations.  However priority was given to clarity and
 * maintainability of the code, over the small increase in efficiency.)
 * The function isStateLegitimate() specifies which potential states
 * can be used.  It is good to make these constraints as tight as
 * possible in order to ensure the result automaton does not contain
 * unnecessary states.
 *
 * The most recent emission label may be empty (Fsa::Epsilon).  We
 * call this a "discharged" state.  This happens for three reasons: 1)
 * At the word start no emission label has been consumed.  2) After
 * processing an input epsilon arc.  3) In some situations we
 * deliberately forget the emission label (see below).
 *
 * In the expanded state space, loop transitions are simple to
 * implement.  (In "discharged" states they are not allowed.)
 * Concerning the other (forward, exit and skip) transitions, there is
 * a little twist: When a right state has multiple incoming and
 * outgoing arcs, we choose to first discharge the recent-most
 * emission label by going to an appropriate left state via an epsilon
 * transition.  The alternative would be to avoid the epsilon
 * transition and directly connect to all successor states.  However,
 * in practice this would dramatically increase the total number of
 * arcs needed.  So discharging is the preferable alternative.  The
 * discharged state can be thought of as the state when we have
 * decided to leave the current state, but not yet chosen where to go
 * to.  As mentioned before, the appropriate set of transition weights
 * is recorded, so that we know what to do when we forward or skip
 * from the discharged state.
 *
 * Concerning skips: In general, a skip consists of two transitions:
 * First an epsilon transition goes to an intermediate state that
 * allows a forward only, and then another transition leads to the
 * target state.  In "favorable" situations this is optimized into a
 * single transitions (skip optimization).  If you have read so far,
 * you are certainly able to figure out what these favorable
 * conditions are.
 *
 * As you noticed, there is some freedom in designing the discharge
 * transitions.  It turns out that compact results can be obtained by
 * combining the forward discharge with the intermediate skip states,
 * and to combine exit and loop discharge states.
 *
 * Any disambiguator label is interpreted as a word boundary and is
 * given the following special treatment: No loop transition, since
 * the word boundary cannot be repeated.  No skip transitions: The word
 * boundary cannot be skipped and the final state before the boundary
 * cannot be skipped.  The latter is done for consistency with
 * WordConditionedTreeSearch.
 *
 * Once the state space is constructed as describe above, it is
 * relatively straight forward to figure out, which transition weight
 * (aka time distortion penalty or TDP) must be applied to which arc.
 * Unfortunately the current scheme is not able to distinguish phone-1
 * from phone-2 states.  This will require additional state space
 * expansion by counting repetitive emission labels.  Alternatively,
 * and probably simpler, we change the labels to allow the distinction
 * between phone-1 and phone-2.
 *
 * Todo:
 * - An even better condition for discharging would be:
 *   (number of DIFFERENT incoming arc labels > 1) && (number of outgoing arcs > 1)
 *   Presently it is
 *   (number of incoming arcs > 1) && (number of outgoing arcs > 1).
 * - Implement proper distinction between between phone-1 and phone-2 states.
 * - Reconsider the transition model.  (see discussion in Wiki)
 * - It might be nice to convert this to lazy evaluation (should
 *   be straight forward).
 **/

class TransitionModel::Applicator {
private:
    friend class TransitionModel;

    const TransitionModel &transitionModel_;
    Fsa::LabelId silenceLabel_;
    Fsa::ConstAutomatonRef in_;
    Core::Ref<Fsa::StaticAutomaton> t_;
    Fsa::ConstAlphabetRef alphabet_;
    bool applyExitTransitionToFinalStates_;

    // -----------------------------------------------------------------------
    class StateDegrees : public Fsa::DfsState {
    public:
	enum Direction {
	    incoming = 0,
	    outgoing = 2
	};
	enum Type {
	    emitting = 0,
	    epsilon  = 4,
	    disambiguating = 8
	};
	enum {
	    none = 0x00,
	    one  = 0x01,
	    many = 0x03
	};

	class Degree {
	    u16 flags_;
	public:
	    Degree() : flags_(0) {}
	    void add(Direction direction, Type type) {
		u32 shift = direction + type;
		if (flags_ & (one << shift))
		    flags_ |= (many << shift);
		else
		    flags_ |= (one << shift);
	    };
	    u8 operator() (Direction direction, Type type) const {
		u32 shift = direction + type;
		return (flags_ >> shift) & 0x03;
	    };
	};
    private:
	Fsa::ConstAlphabetRef alphabet_;
	Core::Vector<Degree> degrees_;
    public:
	void exploreArc(Fsa::ConstStateRef from, const Fsa::Arc &arc) {
	    degrees_.grow(from->id(),   Degree());
	    degrees_.grow(arc.target(), Degree());
	    Type type
		= (arc.input() == Fsa::Epsilon) ? epsilon
		: (alphabet_->isDisambiguator(arc.input())) ? disambiguating
		: emitting;
	    degrees_[from->id()  ].add(outgoing, type);
	    degrees_[arc.target()].add(incoming, type);
	}

	virtual void exploreTreeArc(Fsa::ConstStateRef from, const Fsa::Arc &arc) {
	    exploreArc(from, arc);
}
	virtual void exploreNonTreeArc(Fsa::ConstStateRef from, const Fsa::Arc &arc) {
	    exploreArc(from, arc);
}

	StateDegrees(Fsa::ConstAutomatonRef ff, Fsa::ConstAlphabetRef aa) :
	    Fsa::DfsState(ff), alphabet_(aa)
	{}

	const Degree &operator[](Fsa::StateId ii) const {
	    return degrees_[ii];
	}
    };

    StateDegrees *rightStateDegrees_;

    // -----------------------------------------------------------------------

    struct State {
	typedef u8 Mask;
	Mask mask;             /**< bitmask of allowed transitions (1 << TransitionType) */
	Fsa::LabelId emission; /**< recent most emission */
	StateType weights;     /**< transition model to apply */
	static const Mask allowLoop    = (1 << StateTransitionModel::loop);
	static const Mask allowForward = (1 << StateTransitionModel::forward);
	static const Mask allowSkip    = (1 << StateTransitionModel::skip);
	static const Mask allowExit    = (1 << StateTransitionModel::exit);
	static const Mask isFinal      = (1 << StateTransitionModel::nTransitionTypes);

	static const StateType noWeights = (StateType)-1;

	Fsa::StateId right;

	State(Mask m, Fsa::LabelId e, StateType t, Fsa::StateId r) :
	    mask(m), emission(e), weights(t), right(r) {}

	struct Equality {
	    bool operator() (const State &ll, const State &rr) const {
		return (ll.emission == rr.emission)
		    && (ll.mask == rr.mask)
		    && (ll.weights == rr.weights)
		    && (ll.right == rr.right);
	    }
	};
	struct Hash {
	    size_t operator() (const State &s) const {
		return (((((size_t(s.right) << 12) ^ size_t(s.emission)) << 2) ^ size_t(s.weights)) << 2) ^ size_t(0x0f ^ s.mask);
	    }
	};
    };

    struct StackItem : State {
	Fsa::StateRef result;
	StackItem(const State &state, Fsa::StateRef _result) :
	    State(state), result(_result) {}
    };

    typedef std::stack<StackItem> StateStack;
    StateStack todo_;
    typedef Core::hash_map<State, Fsa::StateId, State::Hash, State::Equality> StateMap;
    StateMap states_;

    bool isStateLegitimate(const State &s) const {
	if (alphabet_->isDisambiguator(s.emission))
	    return false;
	// word start state
	if ((s.mask == (State::allowForward | State::allowSkip | State::allowExit | State::isFinal)) &&
	    (s.weights == TransitionModel::entryM1) &&
	    (s.emission == Fsa::Epsilon))
	    return true;
	// normal emitting state
	if ((s.mask == (State::allowForward | State::allowLoop | State::allowSkip | State::allowExit | State::isFinal)) &&
	    (s.weights >= TransitionModel::silence) &&
	    (s.emission != Fsa::Epsilon))
	    return true;
	// discharged forward and intermediate skip state
	if ((s.mask == (State::allowForward)) &&
	    (s.weights == State::noWeights) &&
	    (s.emission == Fsa::Epsilon))
	    return true;
	// discharged skip and exit state
	if ((s.mask == (State::allowSkip | State::allowExit)) &&
	    (s.weights != State::noWeights) &&
	    (s.emission == Fsa::Epsilon))
	    return true;
	// post-epsilon state
	if ((s.mask == (State::allowForward | State::allowSkip | State::allowExit | State::isFinal)) &&
	    (s.weights >= TransitionModel::silence) &&
	    (s.emission == Fsa::Epsilon))
	    return true;
	return false;
    }

    Fsa::StateId getStateId(State::Mask m, Fsa::LabelId e, StateType t, Fsa::StateId r) {
	State s(m, e, t, r);
	StateMap::iterator i = states_.find(s);
	if (i == states_.end()) {
#if 1
	    verify(isStateLegitimate(s));
#endif
	    StackItem si(s, createState(s));
	    i = states_.insert(std::make_pair(s, si.result->id())).first;
	    todo_.push(si);
	}
	return i->second;
    }

    Core::Ref<Fsa::State> createState(const State &s) {
	Core::Ref<Fsa::State> result = Core::ref(t_->newState());
	Fsa::ConstStateRef sr;
	bool isFinal =
	    (s.mask & State::isFinal) &&
	    ((sr = in_->getState(s.right))->isFinal());
	if (isFinal) {
	    Fsa::Weight w(sr->weight_);
	    if (applyExitTransitionToFinalStates_)
		w = t_->semiring()->extend(w, weight(s, StateTransitionModel::exit));
	    result->setFinal(w);
	}
	return result;
    }

    Fsa::Weight weight(const State &s, StateTransitionModel::TransitionType type) const {
	if (s.weights == State::noWeights) {
	    return Fsa::Weight(0.0);
	} else {
	    return Fsa::Weight((*transitionModel_[s.weights])[type]);
	}
    }

    /**
     * \todo proper distinction between phone-1 and phone-2 states
     */
    StateType stateType(Fsa::LabelId emission) const {
	if (emission == silenceLabel_) {
	    return TransitionModel::silence;
	} else {
	    if (dynamic_cast<const CartTransitionModel*>(&transitionModel_)) {
		return dynamic_cast<const EmissionAlphabet*>(alphabet_.get()) ?
		    (StateType)emission : transitionModel_.classifyIndex(emission);
	    } else {
		return TransitionModel::phone0;
	    }
	}
    }

    void doEpsilon(const StackItem &current, const Fsa::Arc *ra) {
	require(ra->input() == Fsa::Epsilon);
	current.result->newArc(
	    getStateId(
		current.mask & ~(State::allowLoop),
		Fsa::Epsilon,
		current.weights,
		ra->target()),
	    ra->weight(),
	    Fsa::Epsilon, ra->output());
    }

    void doForward(const StackItem &current, const Fsa::Arc *ra) {
	require(!alphabet_->isDisambiguator(ra->input()));
	require(ra->input() != Fsa::Epsilon);
	current.result->newArc(
	    getStateId(
		State::allowLoop | State::allowForward | State::allowSkip | State::allowExit | State::isFinal,
		ra->input(),
		stateType(ra->input()),
		ra->target()),
	    t_->semiring()->extend(ra->weight(), weight(current, StateTransitionModel::forward)),
	    ra->input(), ra->output());
    }

    void doLoop(const StackItem &current) {
	require(current.emission != Fsa::Epsilon);
	current.result->newArc(
	    current.result->id(),
	    weight(current, StateTransitionModel::loop),
	    current.emission, Fsa::Epsilon);
    }

    void doSkip(const StackItem &current, const Fsa::Arc* ra) {
	require(!alphabet_->isDisambiguator(ra->input()));
	require(ra->input() != Fsa::Epsilon);

	StateDegrees::Degree targetDegree = (*rightStateDegrees_)[ra->target()];
	bool wouldSkipToDeadEnd = (
	    ( targetDegree(StateDegrees::outgoing, StateDegrees::emitting) +
	      targetDegree(StateDegrees::outgoing, StateDegrees::epsilon ) ) == 0);
	if (wouldSkipToDeadEnd) return;

	Fsa::Weight w = weight(current, StateTransitionModel::skip);
	if (t_->semiring()->compare(w, t_->semiring()->max()) >= 0) return;

	bool isEligbleForSkipOptimization = (
	    (targetDegree(StateDegrees::outgoing, StateDegrees::disambiguating) == 0) &&
	    (targetDegree(StateDegrees::outgoing, StateDegrees::epsilon)        == 0) &&
	    (targetDegree(StateDegrees::outgoing, StateDegrees::emitting)       == 1));
	Fsa::ConstStateRef rat;
	const Fsa::Arc *ras = 0;
	if (isEligbleForSkipOptimization) {
	    rat = in_->getState(ra->target());
	    verify(rat->nArcs() == 1);
	    ras = &*rat->begin();
	    if (ras->output() != Fsa::Epsilon)
		isEligbleForSkipOptimization = false;
	}

	Fsa::Arc *ca = current.result->newArc();
	ca->weight_ = t_->semiring()->extend(ra->weight(), w);
	if (isEligbleForSkipOptimization) {
	    verify(ras->input() != Fsa::Epsilon);
	    verify(!alphabet_->isDisambiguator(ras->input()));
	    ca->target_ = getStateId(
		State::allowLoop | State::allowForward | State::allowSkip | State::allowExit | State::isFinal,
		ras->input(),
		stateType(ras->input()),
		ras->target());
	    ca->weight_ = t_->semiring()->extend(ca->weight_, ras->weight());
	    ca->input_ = ras->input();
	} else {
	    ca->target_ = getStateId(
		State::allowForward,
		Fsa::Epsilon,
		State::noWeights,
		ra->target());
	    ca->input_ = Fsa::Epsilon;
	}
	ca->output_ = ra->output();
    }

    void doExit(const StackItem &current, const Fsa::Arc *ra) {
	require(alphabet_->isDisambiguator(ra->input()));
	verify(!applyExitTransitionToFinalStates_);
	current.result->newArc(
	    getStateId(
		State::allowForward | State::allowSkip | State::allowExit | State::isFinal,
		Fsa::Epsilon,
		TransitionModel::entryM1,
		ra->target()),
	    t_->semiring()->extend(ra->weight(), weight(current, StateTransitionModel::exit)),
	    ra->input(), ra->output());
    }

    void doDischarge(const StackItem &current) {
	current.result->newArc(
	    getStateId(
		State::allowForward,
		Fsa::Epsilon,
		State::noWeights,
		current.right),
	    weight(current, StateTransitionModel::forward),
	    Fsa::Epsilon, Fsa::Epsilon);
	current.result->newArc(
	    getStateId(
		State::allowSkip | State::allowExit,
		Fsa::Epsilon,
		current.weights,
		current.right),
	    t_->semiring()->one(),
	    Fsa::Epsilon, Fsa::Epsilon);
    }

public:
    Applicator(const TransitionModel &tm) :
	transitionModel_(tm),
	silenceLabel_(Fsa::InvalidLabelId),
	applyExitTransitionToFinalStates_(false)
    {}
    Fsa::ConstAutomatonRef apply(Fsa::ConstAutomatonRef in);
};

Fsa::ConstAutomatonRef TransitionModel::Applicator::apply(Fsa::ConstAutomatonRef input) {
    require(alphabet_);
    in_ = input;

    Core::ProgressIndicator pi("applying transition model", "states");
    rightStateDegrees_ = new StateDegrees(in_, alphabet_);
    rightStateDegrees_->dfs(&pi);

    t_ = Core::ref(new Fsa::StaticAutomaton);
    t_->setType(input->type());
    t_->setSemiring(input->semiring());
    t_->setInputAlphabet(input->getInputAlphabet());
    t_->setOutputAlphabet(input->getOutputAlphabet());

    Fsa::StateId initial = getStateId(
	State::allowForward | State::allowSkip | State::allowExit | State::isFinal,
	Fsa::Epsilon,
	TransitionModel::entryM1,
	in_->initialStateId());
    t_->setInitialStateId(initial);
    pi.start();
    while (!todo_.empty()) {
	const StackItem current(todo_.top()); todo_.pop();
	Fsa::ConstStateRef currentRight = in_->getState(current.right);

	StateDegrees::Degree degree = (*rightStateDegrees_)[current.right];
	bool shouldDischarge =
	    (degree(StateDegrees::incoming, StateDegrees::emitting) == StateDegrees::many) &&
	    ( (degree(StateDegrees::outgoing, StateDegrees::emitting) == StateDegrees::many) ||
	      (degree(StateDegrees::outgoing, StateDegrees::disambiguating) == StateDegrees::many) );

//	std::cerr << Core::form("expand: mask=%x\temission=%d\tweights=%d\tright=%zd\n", current.mask, current.emission, current.weights, current.right); // DEBUG

	if (current.mask & State::allowLoop)
	    doLoop(current);
	if (current.emission != Fsa::Epsilon && shouldDischarge) {
	    doDischarge(current);
	} else {
	    for (Fsa::State::const_iterator ra = currentRight->begin(); ra != currentRight->end(); ++ra) {
		if (ra->input() == Fsa::Epsilon) {
		    doEpsilon(current, &*ra);
		} else if (alphabet_->isDisambiguator(ra->input())) {
		    if (current.mask & State::allowExit)
			doExit(current, &*ra);
		} else {
		    if (current.mask & State::allowForward)
			doForward(current, &*ra);
		    if (current.mask & State::allowSkip)
			doSkip(current, &*ra);
		}
	    }
	}
	pi.notify(t_->size());
    }
    pi.finish();
    delete rightStateDegrees_;
    in_.reset();
    Fsa::removeInvalidArcsInPlace(t_);
    Fsa::trimInPlace(t_);
    Fsa::ConstAutomatonRef result = t_;
    t_.reset();
    return result;
}

Fsa::ConstAutomatonRef TransitionModel::apply(
    Fsa::ConstAutomatonRef in,
    Fsa::LabelId silenceLabel,
    bool applyExitTransitionToFinalStates) const
{
    Applicator ap(*this);

    ap.alphabet_ = in->getInputAlphabet();
    ap.silenceLabel_ = silenceLabel;
    ap.applyExitTransitionToFinalStates_ = applyExitTransitionToFinalStates;

    return ap.apply(in);
}

// ===========================================================================
GlobalTransitionModel::GlobalTransitionModel(const Core::Configuration &c) :
    Core::Component(c),
    Precursor(c)
{
    transitionModels_.resize(nStateTypes);
    transitionModels_[entryM1] = new StateTransitionModel(select("entry-m1"));
    transitionModels_[entryM2] = new StateTransitionModel(select("entry-m2"));
    transitionModels_[silence] = new StateTransitionModel(select("silence"));
    transitionModels_[phone0] = new StateTransitionModel(select("state-0"));
    transitionModels_[phone1] = new StateTransitionModel(select("state-1"));
}

// ===========================================================================
const Core::ParameterStringVector NonWordAwareTransitionModel::paramNonWordPhones(
    "nonword-phones", "Non-word (noise) phone symbols with separate tdps.\
		       Wildcards can be used at boundaries to select multiple phonemes.", ",");

NonWordAwareTransitionModel::NonWordAwareTransitionModel(
    const Core::Configuration &c,
    ClassicStateModelRef stateModel) :
    Core::Component(c),
    Precursor(c),
    stateModel_(stateModel)
{
    std::vector<std::string> nonWordPhones = paramNonWordPhones(config);
    Bliss::PhonemeInventoryRef pi = stateModel_->phonology().getPhonemeInventory();
    const AllophoneAlphabet &allophones = stateModel_->allophoneAlphabet();
    for (std::vector<std::string>::const_iterator s = nonWordPhones.begin(); s != nonWordPhones.end(); ++s) {
	std::vector<std::string> selection;
	std::set<Bliss::Phoneme::Id> selectedPhones = pi->parseSelection(*s);
	for(std::set<Bliss::Phoneme::Id>::iterator phoneIt = selectedPhones.begin(); phoneIt != selectedPhones.end(); ++phoneIt)
	{
	    Allophone allo(*phoneIt, Allophone::isInitialPhone | Allophone::isFinalPhone);
	    if (allophones.isSilence(allo))
		continue;
	    const Allophone *allophone = allophones.allophone(allophones.index(allo));
	log("using nonword tdps for allophone %s", allophones.toString(*allophone).c_str());
	const ClassicHmmTopology *hmmTopology = stateModel_->hmmTopology(allophone);
	const u32 nStates = hmmTopology->nPhoneStates();
	for (u32 state = 0; state < nStates; ++state) {
	    const AllophoneStateIndex si = stateModel_->allophoneStateAlphabet().index(allophone, state);
	    nonWordStates_.insert(si);
	    }
	}
    }
    if (nonWordStates_.empty()) {
	warning("no non-word phone defined");
    }
    transitionModels_.push_back(new StateTransitionModel(select("nonword-0")));
    transitionModels_.push_back(new StateTransitionModel(select("nonword-1")));
}

// ===========================================================================
CartTransitionModel::CartTransitionModel(
    const Core::Configuration &c,
    ClassicStateModelRef stateModel)
    :
    Core::Component(c),
    Precursor(c),
    nSubStates_(1)
{
    stateTying_ =
	ClassicStateTying::createClassicStateTyingRef(
	    select("state-tying"),
	    stateModel);
    if (stateTying_->hasFatalErrors()) {
	criticalError("failed to initialize state tying.");
	stateTying_.reset();
    }

    verify(nSubStates_ == 1);

    transitionModels_.resize(silence + (stateTying_->nClasses() * nSubStates_), 0);
    transitionModels_[entryM1] = new StateTransitionModel(select("entry-m1"));
    transitionModels_[entryM2] = new StateTransitionModel(select("entry-m2"));
    transitionModels_[silence] = new StateTransitionModel(select("silence"));
    // silence is assumed to have index=0 but is ignored
    for (u32 s = 1; s < stateTying_->nClasses(); ++ s) {
	verify(!transitionModels_[silence + s]);
	const std::string selection = Core::form("state-%d-0", s);
	transitionModels_[silence + s] = new StateTransitionModel(select(selection));
    }
}

// ===========================================================================
Core::Choice TransitionModel::choiceTyingType(
    "global", global,
    "cart", cart,
    "global-and-nonword", globalPlusNonWord,
    Core::Choice::endMark());

Core::ParameterChoice TransitionModel::paramTyingType(
    "tying-type",
    &choiceTyingType,
    "type of tying scheme",
    global);

/**
 * This solution is supported because the parameter
 * mechanism cannot efficiently handle a large number
 * of transition models, e.g. one for each CART label!
 */
Core::ParameterString TransitionModel::paramTdpValuesFile(
    "file",
    "file with tdp values, overwrites paramScores()",
    "");

TransitionModel* TransitionModel::createTransitionModel(
    const Core::Configuration &configuration,
    ClassicStateModelRef stateModel)
{
    switch (TransitionModel::paramTyingType(configuration)) {
    case TransitionModel::global:
	return new GlobalTransitionModel(configuration);
	break;
    case TransitionModel::globalPlusNonWord:
	return new NonWordAwareTransitionModel(configuration, stateModel);
	break;
    case TransitionModel::cart:
	CartTransitionModel *result = new CartTransitionModel(configuration, stateModel);
	return result;
	break;
    }
    return 0;
}

// ===========================================================================
ScaledTransitionModel::ScaledTransitionModel(
    const Core::Configuration &c,
    ClassicStateModelRef stateModel)
    :
    Core::Component(c), Mc::Component(c), transitionModel_(0)
{
    transitionModel_ = TransitionModel::createTransitionModel(c, stateModel);
}

ScaledTransitionModel::~ScaledTransitionModel()
{
    delete transitionModel_;
}

// ===========================================================================
CombinedTransitionModel::CombinedTransitionModel(
    const Core::Configuration &c,
    const std::vector<Core::Ref<ScaledTransitionModel> > &transitionModels,
    ClassicStateModelRef stateModel) :
    Core::Component(c),
    Precursor(c, stateModel),
    transitionModels_(transitionModels)
{
    for(u32 i = 0; i < transitionModels_.size(); ++ i)
	transitionModels_[i]->setParentScale(scale());
}

CombinedTransitionModel::~CombinedTransitionModel()
{}

bool CombinedTransitionModel::load()
{
    transitionModel_->clear();
    bool result = true;
    for(u32 i = 0; i < transitionModels_.size(); ++ i) {
	if (transitionModels_[i]->load())
	    (*this) += (*transitionModels_[i]);
	else
	    result = false;
    }
    transitionModel_->correct();
    return result;
}

void CombinedTransitionModel::distributeScaleUpdate(const Mc::ScaleUpdate &scaleUpdate)
{
    transitionModel_->clear();
    for(u32 i = 0; i < transitionModels_.size(); ++ i) {
	transitionModels_[i]->updateScales(scaleUpdate);
	(*this) += (*transitionModels_[i]);
    }
    transitionModel_->correct();
}

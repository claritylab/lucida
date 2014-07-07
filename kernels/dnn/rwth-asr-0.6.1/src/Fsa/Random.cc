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
#include "Random.hh"

namespace Fsa {

    class RandomPathAutomaton : public SlaveAutomaton {
    public:
	f32 samplingWeight_;
	u32 maximumSize_;

    private:
	struct Step {
	    /** state in original automaton */
	    Fsa::StateId state;

	    /** number of arc taken from state
	     * -1 indicates accepting as final state
	     * -2 indicates we ran into a dead end.
	     *    (Yes, using trim() before random() is a good idea.)
	     */
	    s32 arc;
	};

	mutable Core::Vector<Step> steps_;

	void nextStep(StateId s) const {
	    Step step;
	    step.state = s;
	    ConstStateRef origState = fsa_->getState(step.state);
	    s32 nChoices = origState->nArcs();
	    if (origState->isFinal()) nChoices += 1;
	    if (maximumSize_ && (steps_.size() + 1 >= maximumSize_)) {
		// maximum length exceeded
		step.arc = -3;
	    } else if (nChoices < 1) {
		// reached dead end of original automaton
		step.arc = -2;
	    } else {
		if (samplingWeight_ != 0.0) {
		    std::vector<f64> probs;
		    if (origState->isFinal())
			probs.push_back(exp( - f32(origState->weight_) * samplingWeight_));
		    for (State::const_iterator aa = origState->begin(); aa != origState->end(); ++aa)
			probs.push_back(exp( - f32(aa->weight()) * samplingWeight_));
		    verify(nChoices == s32(probs.size()));
		    for (s32 ii = 1; ii < nChoices; ++ii)
			probs[ii] += probs[ii-1];
		    f64 rand = drand48() * probs.back();
		    step.arc = std::upper_bound(probs.begin(), probs.end(), rand) - probs.begin();
		    verify(rand < probs[step.arc]);
		    verify(((step.arc == 0) ? 0.0 : probs[step.arc-1]) <= rand);
		} else {
		    step.arc = lrand48() % nChoices;
		}
		if (origState->isFinal()) step.arc -= 1;
		ensure(step.arc <  s32(origState->nArcs()));
		ensure(step.arc >= (origState->isFinal() ? -1 : 0));
	    }
	    steps_.push_back(step);
	}

    public:
	RandomPathAutomaton(ConstAutomatonRef f) :
	    SlaveAutomaton(f),
	    samplingWeight_(0.0),
	    maximumSize_(0)
	{
	    setProperties(PropertyStorage | PropertyCached, PropertyNone);
	    addProperties(PropertySorted);
	    addProperties(PropertyLinear | PropertyAcyclic);
	    nextStep(fsa_->initialStateId());
	}

	virtual StateId initialStateId() const {
	    return 0;
	}

	virtual ConstStateRef getState(StateId s) const {
	    require(s < steps_.size());
	    const Step &step(steps_[s]);
	    ConstStateRef origState = fsa_->getState(step.state);

	    State *state = new State(s);
	    if (step.arc >= 0) {
		State::const_iterator origArc = origState->begin() + step.arc;
		state->newArc(s + 1,
			      origArc->weight(),
			      origArc->input(),
			      origArc->output());
		if (s + 1 >= steps_.size())
		    nextStep(origArc->target());
	    } else if (step.arc == -1) {
		verify(origState->isFinal());
		state->setFinal(origState->weight_);
	    }
	    return ConstStateRef(state);
	}

	virtual std::string describe() const {
	    return "random(" + fsa_->describe() + ")";
	}

	virtual void dumpState(Fsa::StateId s, std::ostream &os) const {
	    if (s < steps_.size()) {
		os << "(";
		fsa_->dumpState(steps_[s].state, os);
		os << ")";
	    } else {
		os << "unknown";
	    }
	}
    };

    ConstAutomatonRef random(
	ConstAutomatonRef f,
	f32 weight,
	u32 maximumSize)
    {
	RandomPathAutomaton *result = new RandomPathAutomaton(f);
	result->samplingWeight_ = weight;
	result->maximumSize_ = maximumSize;
	return ConstAutomatonRef(result);
    };

} // namespace Fsa

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
#include "Arithmetic.hh"
#include "Basic.hh"
#include "Best.hh"
#include "Cache.hh"
#include "Compose.hh"
#include "Dfs.hh"
#include "Hash.hh"
#include "Levenshtein.hh"
#include "Minimize.hh"
#include "Project.hh"
#include "Sort.hh"
#include <Core/Vector.hh>
#include "RemoveEpsilons.hh"

namespace Fsa {

#if 1
    class LevenshteinAutomaton : public Automaton {
    private:
	ConstStateRef state_;
	ConstAlphabetRef inputAlphabet_, outputAlphabet_;
    public:
	LevenshteinAutomaton(ConstAlphabetRef ref, ConstAlphabetRef test, f32 delCost, f32 insCost, f32 subCost, f32 corCost) :
	    inputAlphabet_(ref), outputAlphabet_(test) {
	    setProperties(PropertyStorage, PropertyStorage);

	    AlphabetMapping mapping;
	    mapAlphabet(inputAlphabet_, outputAlphabet_, mapping, false);

	    State *sp = new State(0, StateTagFinal, Weight(0.0));
	    for (Alphabet::const_iterator i = inputAlphabet_->begin(); i != inputAlphabet_->end(); ++i) {
		if (mapping[LabelId(i)] != InvalidLabelId)
		    sp->newArc(0, Weight(corCost), LabelId(i), mapping[LabelId(i)]); // match
		sp->newArc(0, Weight(delCost), LabelId(i), Epsilon); // deletion
		for (Alphabet::const_iterator j = outputAlphabet_->begin(); j != outputAlphabet_->end(); ++j)
		    if (mapping[LabelId(i)] != j) sp->newArc(0, Weight(subCost), LabelId(i), LabelId(j)); // substitution
	    }
	    for (Alphabet::const_iterator i = outputAlphabet_->begin(); i != outputAlphabet_->end(); ++i)
		sp->newArc(0, Weight(insCost), Epsilon, LabelId(i)); // insertion
	    state_ = ConstStateRef(sp);
	}
	virtual ~LevenshteinAutomaton() {}

	virtual Type type() const { return TypeTransducer; }
	virtual ConstSemiringRef semiring() const { return TropicalSemiring; }
	virtual StateId initialStateId() const { return state_->id(); }
	virtual ConstAlphabetRef getInputAlphabet() const { return inputAlphabet_; }
	virtual ConstAlphabetRef getOutputAlphabet() const { return outputAlphabet_; }
	virtual ConstStateRef getState(StateId s) const { return state_; }
	virtual void releaseState(StateId s) const {}
	virtual std::string describe() const { return "LevenshteinAutomaton"; }
    };

    ConstAutomatonRef levenshtein(ConstAutomatonRef ref, ConstAutomatonRef test, f32 delCost, f32 insCost, f32 subCost, f32 corCost) {
	LevenshteinAutomaton *tmp = new LevenshteinAutomaton(ref->getOutputAlphabet(), test->getInputAlphabet(), delCost, insCost, subCost, corCost);
	//return ConstAutomatonRef(tmp);
	return composeMatching(composeMatching(ref, ConstAutomatonRef(tmp)), test);
    }
#endif

    class LevenshteinDfsState : public DfsState {
    private:
	LevenshteinInfo info_;
    public:
	LevenshteinDfsState(ConstAutomatonRef f) : DfsState(f) {
	    info_.del_ = info_.ins_ = info_.sub_ = info_.total_ = 0;
	}
	virtual void discoverState(ConstStateRef sp) {
	    for (State::const_iterator a = sp->begin(); a != sp->end(); ++a) {
		if (a->input() == Epsilon) info_.del_++;
		else {
		    if (a->output() == Epsilon) info_.ins_++;
		    else if (a->input() != a->output()) info_.sub_++;
		    info_.total_++;
		}
	    }
	}
	LevenshteinInfo info() const { return info_; }
    };

    LevenshteinInfo levenshteinInfo(ConstAutomatonRef levensh) {
	LevenshteinDfsState s(best(levensh));
	s.dfs();
	return s.info();
    }

} // namespace Fsa

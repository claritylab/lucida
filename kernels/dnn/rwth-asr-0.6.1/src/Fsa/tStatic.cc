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
#include "tStatic.hh"

namespace Ftl {
    template<class _Automaton>
    StaticAutomaton<_Automaton>::~StaticAutomaton() {
	clear();
    }

    template<class _Automaton>
    void StaticAutomaton<_Automaton>::clear() {
	states_.erase(states_.begin(), states_.end());
    }

    template<class _Automaton>
    bool StaticAutomaton<_Automaton>::hasState(Fsa::StateId sid) const {
	if (sid < states_.size()) return bool(states_[sid]);
	else return false;
    }

    template<class _Automaton>
    Core::Ref<typename _Automaton::State> StaticAutomaton<_Automaton>::state(Fsa::StateId s) {
	if (s < states_.size()) return states_[s];
	return Core::Ref<typename _Automaton::State>();
    }

    template<class _Automaton>
    typename _Automaton::State* StaticAutomaton<_Automaton>::newState(Fsa::StateTag tags) {
	setState(new typename _Automaton::State(states_.size(), tags));
	return states_.back().get();
    }

    template<class _Automaton>
    typename _Automaton::State* StaticAutomaton<_Automaton>::newState
    (Fsa::StateId tags, const typename _Automaton::Weight &weight) {
	setState(new typename _Automaton::State(states_.size(), tags, weight));
	return states_.back().get();
    }

    template<class _Automaton>
    typename _Automaton::State* StaticAutomaton<_Automaton>::newFinalState
    (const typename _Automaton::Weight &weight) {
	setState(new typename _Automaton::State(states_.size(), Fsa::StateTagFinal, weight));
	return states_.back().get();
    }

    template<class _Automaton>
    void StaticAutomaton<_Automaton>::setStateFinal
    (typename _Automaton::State *s, const typename _Automaton::Weight &finalWeight) {
	s->setFinal(finalWeight);
    }

    template<class _Automaton>
    void StaticAutomaton<_Automaton>::setStateFinal
    (typename _Automaton::State *s) {
	s->setFinal(this->semiring()->one());
    }

    template<class _Automaton>
    void StaticAutomaton<_Automaton>::setState(typename _Automaton::State *sp) {
	if (!sp) return;
	states_.grow(sp->id());
	states_[sp->id()] = Core::Ref<typename _Automaton::State>(sp);
    }

    template<class _Automaton>
    void StaticAutomaton<_Automaton>::deleteState(typename Fsa::StateId si) {
	require(si <= states_.size());
	states_[si].reset();
    }

    template<class _Automaton>
    Core::Ref<const typename _Automaton::State> StaticAutomaton<_Automaton>::getState(Fsa::StateId s) const {
	if (s < states_.size()) return states_[s];
	return Core::Ref<const typename _Automaton::State>();
    }

    template<class _Automaton>
    void StaticAutomaton<_Automaton>::normalize() {
	Fsa::StateId *map = new Fsa::StateId[states_.size()];
	bool targetStatesNeedMapping = false, isLinear = true;
	Fsa::StateId s, _s, first = Fsa::InvalidStateId;
	for (s = _s = 0; s < states_.size(); ++s) {
	    map[s] = Fsa::InvalidStateId;
	    if (states_[s]) {
		if (first == Fsa::InvalidStateId) first = s;
		states_[_s] = states_[s];
		states_[_s]->setId(_s);
		if (states_[s]->nArcs() > 1) isLinear = false;
		for (typename _Automaton::State::iterator a = states_[_s]->begin();
		     a != states_[_s]->end(); ++a) {
		    if ((a->target() >= states_.size()) || (!states_[a->target()]) ||
			((a->target() < s) && (map[a->target()] == Fsa::InvalidStateId)))
			std::cerr << "arc at state " << s << " points to non-existent target state "
				  << a->target() << std::endl;
		    if (this->type() == Fsa::TypeAcceptor) a->setOutput(a->input());
		}
		map[s] = _s++;
	    }
	}
	Fsa::StateId initial = this->initialStateId();
	if ((initial != Fsa::InvalidStateId) && (initial != 0)) {
	    Core::Ref<typename _Automaton::State> tmp = states_[map[initial]];
	    states_[map[initial]] = states_[0];
	    states_[0] = tmp;
	    states_[map[initial]]->setId(map[initial]);
	    states_[0]->setId(0);
	    map[first] = map[initial];
	    map[initial] = 0;
	    targetStatesNeedMapping = true;
	    initial = 0;
	}
	this->setInitialStateId(initial);
	if (_s < states_.size()) targetStatesNeedMapping = true;
	if (targetStatesNeedMapping)
	    for (s = 0; s < _s; ++s)
		for (typename _Automaton::State::iterator a = states_[s]->begin();
		     a != states_[s]->end(); ++a)
		    a->setTarget(map[a->target()]);
	states_.resize(_s);
	states_.minimize();
	delete[] map;
	if (isLinear)
	    this->addProperties(Fsa::PropertyLinear);
    }

    template<class _Automaton>
    std::pair<size_t, size_t> StaticAutomaton<_Automaton>::getStateAndArcMemoryUsed() const {
	size_t states = 0, arcs = 0;
	for (typename Core::Vector<Core::Ref<typename _Automaton::State> >::const_iterator sp = states_.begin();
	     sp != states_.end(); ++sp)
	    if (*sp) {
		states += sizeof(typename _Automaton::State);
		arcs += sizeof(typename _Automaton::Arc) * (*sp)->nArcs();
	    }
	return std::make_pair(states, arcs);
    }

    template<class _Automaton>
    size_t StaticAutomaton<_Automaton>::getMemoryUsed() const {
	if (!memoryUsed_) {
	    size_t size = 0;
	    if (Precursor::input_)
		size += Precursor::input_->getMemoryUsed();
	    if (Precursor::output_ && Precursor::output_ != Precursor::input_)
		size += Precursor::output_->getMemoryUsed();
	    std::pair<size_t, size_t> memory = getStateAndArcMemoryUsed();
	    memoryUsed_ = size + memory.first + memory.second;
	}
	return memoryUsed_;
    }

    template<class _Automaton>
    void StaticAutomaton<_Automaton>::dumpMemoryUsage(Core::XmlWriter &o) const {
	std::pair<size_t, size_t> memory = getStateAndArcMemoryUsed();
	o << Core::XmlOpen("static")
	  << Core::XmlFull("states", memory.first)
	  << Core::XmlFull("arcs", memory.second)
	  << Core::XmlFull("total", getMemoryUsed())
	  << Core::XmlClose("static");
    }

    template<class _Automaton>
    void StaticAutomaton<_Automaton>::compact(Fsa::StateMap &mapping) {
	mapping.clear();
	mapping.grow(states_.size() - 1, Fsa::InvalidStateId);
	Fsa::StateId s, _s;
	for (s = _s = 0; s < states_.size(); ++s)
	    if (states_[s]) {
		states_[_s] = states_[s];
		states_[_s]->setId(_s);
		mapping[s] = _s++;
	    }
	states_.resize(_s);
	Precursor::initial_ = mapping[Precursor::initial_];
	for (typename Core::Vector<Core::Ref<typename _Automaton::State> >::iterator s = states_.begin();
	     s != states_.end(); ++s)
	    for (typename _Automaton::State::iterator a = (*s)->begin(); a != (*s)->end(); ++a)
		a->setTarget(mapping[a->target()]);
    }
} // namespace Ftl

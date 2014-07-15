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
#include <Core/Hash.hh>
#include "tLinear.hh"
#include "tProperties.hh"

namespace Ftl {
    template<class _Automaton>
    bool isLinear(typename _Automaton::ConstRef la) {
	Fsa::StateId si = la->initialStateId();
	if (si == Fsa::InvalidStateId) return true;
	Core::hash_set<Fsa::StateId> seen;
	seen.insert(si);
	typename _Automaton::ConstStateRef s = la->getState(si);
	while (!s->isFinal()) {
	    if (s->nArcs() != 1) return false;
	    si = s->begin()->target();
	    if (!seen.insert(si).second) return false;
	    s = la->getState(si);
	}
	return (s->nArcs() == 0);
    }

    template<class _Automaton>
    void getLinearInput(typename _Automaton::ConstRef la, std::vector<Fsa::LabelId> &result) {
	require(hasProperties<_Automaton>(la, Fsa::PropertyLinear));
	//	require(!isEmpty<_Automaton>(la));
	typename _Automaton::ConstStateRef s = la->getState(la->initialStateId());
	while (!s->isFinal()) {
	    typename _Automaton::State::const_iterator a = s->begin();
	    if ((a->input() != Fsa::Epsilon) && (a->input() < Fsa::LastLabelId))
		result.push_back(a->input());
	    s = la->getState(a->target());
	}
    }

    template<class _Automaton>
    void getLinearOutput(typename _Automaton::ConstRef la, std::vector<Fsa::LabelId> &result) {
	require(hasProperties<_Automaton>(la, Fsa::PropertyLinear));
	//	require(!isEmpty<_Automaton>(la));
	typename _Automaton::ConstStateRef s = la->getState(la->initialStateId());
	while (!s->isFinal()) {
	    typename _Automaton::State::const_iterator a = s->begin();
	    if ((a->output() != Fsa::Epsilon) && (a->output() < Fsa::LastLabelId))
		result.push_back(a->output());
	    s = la->getState(a->target());
	}
    }

    template<class _Automaton>
    typename _Automaton::Weight getLinearWeight(typename _Automaton::ConstRef la) {
	require(hasProperties<_Automaton>(la, Fsa::PropertyLinear));
	//	require(!isEmpty<_Automaton>(la));
	typename _Automaton::Weight result = la->semiring()->one();
	typename _Automaton::ConstStateRef s = la->getState(la->initialStateId());
	while (!s->isFinal()) {
	    typename _Automaton::State::const_iterator a = s->begin();
	    result = la->semiring()->extend(result, a->weight());
	    s = la->getState(a->target());
	}
	return la->semiring()->extend(result, s->weight_);
    }

} // namespace Ftl

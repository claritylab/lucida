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
#include "Types.hh"
#include "tAccessible.hh"
#include "tDfs.hh"
#include "tStaticAlgorithm.hh"

namespace Ftl {

    template<class _Automaton>
    void removeNonAccessibleStates(Core::Ref<StaticAutomaton<_Automaton> > s) {
	DfsState<_Automaton> v(s);
	v.dfs();
	for (Fsa::StateId sid = 0; sid < s->size(); ++sid)
	    if (v.color(sid) != DfsState<_Automaton>::Black) s->deleteState(sid);
    }

    template<class _Automaton>
    void trimInPlace(Core::Ref<StaticAutomaton<_Automaton> > sa) {
	CoaccessibleDfsState<_Automaton> vv(sa);
	vv.dfs();
	// not necesserarily true: we want to trim because of the violation of that property ...
	// verify(vv.maxStateId() == sa->maxStateId());
	verify(vv.maxStateId() <= sa->maxStateId());
	for (Fsa::StateId si = 0; si < sa->size(); ++si) {
//	    std::cerr << si << '\t' << vv.flags().size() << std::endl; // DEBUG
	    typename _Automaton::State *st = sa->fastState(si);
	    if (st) {
		if (vv.flags()[si] & CoaccessibleDfsState<_Automaton>::FlagCoaccessible) {
		    typename _Automaton::State::iterator out = st->begin();
		    for (typename _Automaton::State::iterator in = st->begin(); in != st->end(); ++in) {
			if (vv.flags()[in->target()] & CoaccessibleDfsState<_Automaton>::FlagCoaccessible)
			    *out++ = *in;
		    }
		    st->truncate(out);
		} else {
		    sa->deleteState(si);
		}
	    }
	}
	if (!sa->fastState(sa->initialStateId()))
	    sa->setInitialStateId(Fsa::InvalidStateId);
    }

    class IsInvalidArc {
    private:
	const Fsa::Weight invalid_;
	const Fsa::Weight zero_;
    public:
	IsInvalidArc(Fsa::ConstSemiringRef sr) : invalid_(sr->invalid()), zero_(sr->zero()) {}
	bool operator()(const Fsa::Arc &a) {
	    return (a.weight() == invalid_) or (a.weight() == zero_);
	}
    };

    template<class _Automaton>
    void removeInvalidArcsInPlace(Core::Ref<StaticAutomaton<_Automaton> > sa) {
	IsInvalidArc pred(sa->semiring());
	for (Fsa::StateId si = 0; si < sa->maxStateId(); ++ si) {
	    sa->fastState(si)->remove(pred);
	}
    }

} // namespace Ftl

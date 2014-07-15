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
#include "Stack.hh"

#include "tDfs.hh"

namespace Ftl {
    template<class _Automaton>
    void DfsState<_Automaton>::dfs(Core::ProgressIndicator *progress) {
	Fsa::Stack<Fsa::StateId> S;
	Fsa::StateId initial = fsa_->initialStateId();
	Fsa::StateId init = initial | White;
	info_.erase(info_.begin(), info_.end());
	if (initial != Fsa::InvalidStateId) {
	    info_.grow(initial, init);
	    S.push(initial);
	}
	if (progress) progress->start(info_.size());
	while (!S.isEmpty()) {
	    Fsa::StateId s = S.top();
	    if (color(s) == White) {
		setColor(s, Gray);
		_ConstStateRef sp = fsa_->getState(s);
		if (sp) {
		    discoverState(sp);
		    for (typename _State::const_iterator a = sp->begin(); a != sp->end(); ++a) {
			info_.grow(a->target(), init);
			if (progress) progress->setTotal(a->target());
			if (color(a->target()) == White) {
			    setPredecessor(a->target(), s);
			    exploreTreeArc(sp, *a);
			    S.push(a->target());
			} else exploreNonTreeArc(sp, *a);
		    }
		} else std::cerr << "unreachable state " << s << " encountered during dfs" << std::endl;
	    } else {
		if (color(s) == Gray) {
		    setColor(s, Black);
		    finishState(s);
		    if (progress) progress->notify();
		}
		S.pop();
	    }
	}
	finish();
	if (progress) progress->finish();
    }


    template<class _Automaton>
    void DfsState<_Automaton>::recursiveDfs(Core::ProgressIndicator *progress) {
	Fsa::Stack<Fsa::StateId> S;
	Fsa::Stack<size_t> A;
	Fsa::StateId init = fsa_->initialStateId() | White;
	info_.erase(info_.begin(), info_.end());
	if (fsa_->initialStateId() != Fsa::InvalidStateId) {
	    info_.grow(fsa_->initialStateId(), init);
	    S.push(fsa_->initialStateId());
	}
	if (progress) progress->start(info_.size());
	while (!S.isEmpty()) {
	    Fsa::StateId s = S.top();
	    _ConstStateRef sp = fsa_->getState(s);
	    if (!sp) {
		std::cerr << "unreachable state " << s << " encountered during dfs" << std::endl;
		S.pop();
		continue;
	    }
	    if (color(s) == White) {
		setColor(s, Gray);
		discoverState(sp);
		A.push(0);
	    }
	    require(S.size() == A.size());
	    size_t a = A.top(); A.pop();
	    if (a != 0)
		finishArc(sp, *(sp->begin() + a-1));
	    if (a != sp->nArcs()) {
		typename _State::const_iterator ai = sp->begin() + a;
		info_.grow(ai->target(), init);
		if (progress) progress->setTotal(ai->target());
		if (color(ai->target()) == White) {
		    exploreTreeArc(sp, *ai);
		    setPredecessor(ai->target(), s);
		    S.push(ai->target());
		} else {
		    exploreNonTreeArc(sp, *ai);
		}
		A.push(++a);
	    } else {
		setColor(s, Black);
		finishState(s);
		if (progress) progress->notify();
		S.pop();
	    }
	}
	finish();
	if (progress) progress->finish();
    }

} // namespace Ftl

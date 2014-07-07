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
#include <Core/ProgressIndicator.hh>
#include "tAutomaton.hh"
#include "tDfs.hh"
#include "tSort.hh"

namespace Ftl {
    template<class _Automaton>
    class SortByArcAutomaton : public ModifyAutomaton<_Automaton> {
	typedef ModifyAutomaton<_Automaton> Precursor;
    public:
	typedef typename _Automaton::State _State;
	typedef typename _Automaton::ConstRef _ConstAutomatonRef;
    public:
	SortByArcAutomaton(_ConstAutomatonRef f) : Precursor(f) {
	    this->setProperties(Fsa::PropertySorted, Fsa::PropertySortedByArc);
	}
	virtual void modifyState(_State *sp) const { sp->sort(byArc<_Automaton>(this->semiring())); }
	virtual std::string describe() const {
	    return "sort(" + Precursor::fsa_->describe() + ",SortTypeByArc)";
	}
    };

    template<class _Automaton>
    class SortByInputAutomaton : public ModifyAutomaton<_Automaton> {
	typedef ModifyAutomaton<_Automaton> Precursor;
    public:
	typedef typename _Automaton::State _State;
	typedef typename _Automaton::ConstRef _ConstAutomatonRef;
    public:
	SortByInputAutomaton(_ConstAutomatonRef f) : Precursor(f) {
	    this->setProperties(Fsa::PropertySorted, Fsa::PropertySortedByInput);
	}
	virtual void modifyState(_State *sp) const { sp->sort(byInput<_Automaton>()); }
	virtual std::string describe() const {
	    return "sort(" + Precursor::fsa_->describe() + ",SortTypeByInput)";
	}
    };

    template<class _Automaton>
    class SortByInputAndOutputAutomaton : public ModifyAutomaton<_Automaton> {
	typedef ModifyAutomaton<_Automaton> Precursor;
    public:
	typedef typename _Automaton::State _State;
	typedef typename _Automaton::ConstRef _ConstAutomatonRef;
    public:
	SortByInputAndOutputAutomaton(_ConstAutomatonRef f) : Precursor(f) {
	    this->setProperties(Fsa::PropertySorted, Fsa::PropertySortedByInputAndOutput);
	}
	virtual void modifyState(_State *sp) const { sp->sort(byInputAndOutput<_Automaton>()); }
	virtual std::string describe() const {
	    return "sort(" + Precursor::fsa_->describe() + ",SortTypeByInputAndOutput)";
	}
    };

    template<class _Automaton>
    class SortByInputAndTargetAutomaton : public ModifyAutomaton<_Automaton> {
	typedef ModifyAutomaton<_Automaton> Precursor;
    public:
	typedef typename _Automaton::State _State;
	typedef typename _Automaton::ConstRef _ConstAutomatonRef;
    public:
	SortByInputAndTargetAutomaton(_ConstAutomatonRef f) : Precursor(f) {
	    this->setProperties(Fsa::PropertySorted, Fsa::PropertySortedByInput);
	}
	virtual void modifyState(_State *sp) const { sp->sort(byInputAndTarget<_Automaton>()); }
	virtual std::string describe() const {
	    return "sort(" + Precursor::fsa_->describe() + ",SortTypeByInputAndTarget)";
	}
    };

    template<class _Automaton>
    class SortByInputAndOutputAndTargetAutomaton : public ModifyAutomaton<_Automaton> {
	typedef ModifyAutomaton<_Automaton> Precursor;
    public:
	typedef typename _Automaton::State _State;
	typedef typename _Automaton::ConstRef _ConstAutomatonRef;
    public:
	SortByInputAndOutputAndTargetAutomaton(_ConstAutomatonRef f) : Precursor(f) {
	    this->setProperties(Fsa::PropertySorted, Fsa::PropertySortedByInputAndOutputAndTarget);
	}
	virtual void modifyState(_State *sp) const { sp->sort(byInputAndOutputAndTarget<_Automaton>()); }
	virtual std::string describe() const {
	    return "sort(" + Precursor::fsa_->describe() + ",SortTypeByInputAndOutputAndTarget)";
	}
    };

    template<class _Automaton>
    class SortByOutputAutomaton : public ModifyAutomaton<_Automaton> {
	typedef ModifyAutomaton<_Automaton> Precursor;
    public:
	typedef typename _Automaton::State _State;
	typedef typename _Automaton::ConstRef _ConstAutomatonRef;
    public:
	SortByOutputAutomaton(_ConstAutomatonRef f) : Precursor(f) {
	    this->setProperties(Fsa::PropertySorted, Fsa::PropertySortedByOutput);
	}
	virtual void modifyState(_State *sp) const { sp->sort(byOutput<_Automaton>()); }
	virtual std::string describe() const {
	    return "sort(" + Precursor::fsa_->describe() + ",SortTypeByOutput)";
	}
    };

    template<class _Automaton>
    class SortByWeightAutomaton : public ModifyAutomaton<_Automaton> {
	typedef ModifyAutomaton<_Automaton> Precursor;
    public:
	typedef typename _Automaton::State _State;
	typedef typename _Automaton::ConstRef _ConstAutomatonRef;
    public:
	SortByWeightAutomaton(_ConstAutomatonRef f) : Precursor(f) {
	    this->setProperties(Fsa::PropertySorted, Fsa::PropertySortedByWeight);
	}
	virtual void modifyState(_State *sp) const { sp->sort(byWeight<_Automaton>(this->semiring())); }
	virtual std::string describe() const {
	    return "sort(" + Precursor::fsa_->describe() + ",SortTypeByWeight)";
	}
    };

    template<class _Automaton>
    typename _Automaton::ConstRef sort(typename _Automaton::ConstRef f, Fsa::SortType type) {
	switch (type) {
	case Fsa::SortTypeNone:
	    return f;
	case Fsa::SortTypeByArc:
	    if (f->hasProperty(Fsa::PropertySortedByArc)) return f;
	    else return typename _Automaton::ConstRef(new SortByArcAutomaton<_Automaton>(f));
	case Fsa::SortTypeByInput:
	    if (f->hasProperty(Fsa::PropertySortedByArc) ||
		f->hasProperty(Fsa::PropertySortedByInput) ||
		f->hasProperty(Fsa::PropertySortedByInputAndTarget) ||
		f->hasProperty(Fsa::PropertySortedByInputAndOutput) ||
		f->hasProperty(Fsa::PropertySortedByInputAndOutputAndTarget))
		return f;
	    else return typename _Automaton::ConstRef(new SortByInputAutomaton<_Automaton>(f));
	case Fsa::SortTypeByInputAndOutput:
	    if (f->hasProperty(Fsa::PropertySortedByArc) ||
		f->hasProperty(Fsa::PropertySortedByInputAndOutput) ||
		f->hasProperty(Fsa::PropertySortedByInputAndOutputAndTarget))
		return f;
	    else return typename _Automaton::ConstRef(new SortByInputAndOutputAutomaton<_Automaton>(f));
	case Fsa::SortTypeByInputAndTarget:
	    if (f->hasProperty(Fsa::PropertySortedByInputAndTarget)) return f;
	    else return typename _Automaton::ConstRef(new SortByInputAndTargetAutomaton<_Automaton>(f));
	case Fsa::SortTypeByInputAndOutputAndTarget:
	    if (f->hasProperty(Fsa::PropertySortedByInputAndOutputAndTarget)) return f;
	    else return typename _Automaton::ConstRef(new SortByInputAndOutputAndTargetAutomaton<_Automaton>(f));
	case Fsa::SortTypeByOutput:
	    if (f->hasProperty(Fsa::PropertySortedByOutput)) return f;
	    else return typename _Automaton::ConstRef(new SortByOutputAutomaton<_Automaton>(f));
	case Fsa::SortTypeByWeight:
	    if (f->hasProperty(Fsa::PropertySortedByWeight)) return f;
	    else return typename _Automaton::ConstRef(new SortByWeightAutomaton<_Automaton>(f));
	default:
	    defect();
	}
	return f;
    }

    /**
     * TopologicallySortDfsState
     * Helper class for the topologicallySort function.
     * time_ always represents the finishing time of the next state to finish
     * isCyclic_ is set to true if the automaton contains cycles (even trivial ones)
     */
    template<class _Automaton>
    class TopologicallySortDfsState : public DfsState<_Automaton> {
	typedef DfsState<_Automaton> Precursor;
    public:
	typedef typename _Automaton::Arc _Arc;
	typedef typename _Automaton::ConstStateRef _ConstStateRef;
	typedef typename _Automaton::ConstRef _ConstAutomatonRef;
    public:
	Fsa::StateMap map_;
	Fsa::StateId time_;
	bool isCyclic_;
    public:
	TopologicallySortDfsState(_ConstAutomatonRef f) : Precursor(f), time_(0), isCyclic_(false) {}
	virtual void finishState(Fsa::StateId s) {
	    map_.grow(s, Fsa::InvalidStateId);
	    map_[s] = time_++;
	}
	virtual void exploreNonTreeArc(_ConstStateRef from, const _Arc &a) {
	    if (Precursor::color(a.target()) == Precursor::Gray) isCyclic_ = true;
	}
    };

    template<class _Automaton>
    Fsa::StateMap topologicallySort(typename _Automaton::ConstRef f, bool progress) {
	Core::ProgressIndicator *p = 0;
	if (progress) p = new Core::ProgressIndicator("counting", "states");
	TopologicallySortDfsState<_Automaton> v(f);
	v.dfs(p);
	if (p) delete p;
	--v.time_;
	if (v.isCyclic_) v.map_.resize(0); // return empty map in case of cyclic automaton
	// states in decreasing finishing times of dfs define topological order
	for (Fsa::StateId i = 0; i < v.map_.size(); ++i)
	    if (v.map_[i] != Fsa::InvalidStateId) v.map_[i] = v.time_ - v.map_[i];
	return v.map_;
    }

    template<class _Automaton>
    bool isAcyclic(typename _Automaton::ConstRef f) {
	TopologicallySortDfsState<_Automaton> v(f);
	v.dfs();
	return !v.isCyclic_;
    }

} // namespace Ftl

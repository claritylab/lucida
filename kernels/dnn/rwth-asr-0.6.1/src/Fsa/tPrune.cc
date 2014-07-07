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
#include "tAutomaton.hh"
#include "tDfs.hh"
#include "tPrune.hh"
#include "tRational.hh"
#include "tSssp.hh"
#include <Core/Vector.hh>

namespace Ftl {

    /**
     * pruning based on posterior scores still has some problems with floating point
     * accuracy (some single arcs may not be pruned, but trimming afterwards helps)
     **/
    template<class _Automaton>
    class PosteriorPruneAutomaton : public SlaveAutomaton<_Automaton>, public DfsState<_Automaton> {
    private:
	typedef SlaveAutomaton<_Automaton> Precursor;

    public:
	typedef typename _Automaton::Weight _Weight;
	typedef typename _Automaton::State _State;
	typedef typename _Automaton::ConstStateRef _ConstStateRef;
	typedef typename _Automaton::ConstRef _ConstAutomatonRef;
	typedef typename _Automaton::ConstSemiringRef _ConstSemiringRef;
	typedef StatePotentials<_Weight>  _StatePotentials;

    private:
    /// minWeight_: minimum posterior
	_Weight threshold_, minWeight_;
//      _StatePotentials forward_, backward_;
	_StatePotentials *fw_,*bw_;
	const _StatePotentials &forward_, &backward_;
	bool relative_;

    private:
	void setMinWeight(const _Weight &threshold) {
	    if (relative_) {
		minWeight_ = this->semiring()->max();
		this->dfs();
		minWeight_ = this->semiring()->extend(minWeight_, threshold);
	    } else {
		minWeight_ = this->semiring()->extend(backward_[Precursor::fsa_->initialStateId()], threshold);
	    }
	}

    public:
	PosteriorPruneAutomaton(_ConstAutomatonRef f, const _Weight &threshold, bool relative) :
	    Precursor(f), DfsState<_Automaton>(f), threshold_(threshold), fw_(new _StatePotentials),
	    bw_(new _StatePotentials),forward_(*fw_), backward_(*bw_), relative_(relative)
	{
	    this->setProperties(Fsa::PropertyStorage | Fsa::PropertyCached, Fsa::PropertyNone);
	    Fsa::StateId initial = f->initialStateId();
	    if (initial != Fsa::InvalidStateId) {
		*fw_ = sssp<_Automaton>(f);
		*bw_ = sssp<_Automaton>(transpose<_Automaton>(f));
		setMinWeight(threshold);
	    }
	}

	PosteriorPruneAutomaton(_ConstAutomatonRef f, const _Weight &threshold,const _StatePotentials &fw,const _StatePotentials& bw, bool relative) :
	    Precursor(f), DfsState<_Automaton>(f), threshold_(threshold), fw_(0),bw_(0),forward_(fw),backward_(bw), relative_(relative)
	    {
	    this->setProperties(Fsa::PropertyStorage | Fsa::PropertyCached, Fsa::PropertyNone);
	    setMinWeight(threshold);
	}

	virtual ~PosteriorPruneAutomaton() {
	    if(fw_) { delete fw_; }
	    if(bw_) { delete bw_; }
	}

	virtual void discoverState(_ConstStateRef sp) {
	    _ConstSemiringRef semiring = DfsState<_Automaton>::fsa_->semiring();
	    for (typename _State::const_iterator a = sp->begin(); a != sp->end(); ++a) {
		_Weight w = semiring->extend(a->weight(), backward_[a->target()]);
		w = semiring->extend(forward_[sp->id()], w);
		if (semiring->compare(w, minWeight_) < 0) minWeight_ = w;
	    }
	}
	virtual _ConstStateRef getState(Fsa::StateId s) const {
	    if (s < forward_.size()) {
		_ConstStateRef _s = Precursor::fsa_->getState(s);
		_State *sp = new _State(_s->id(), _s->tags(), _s->weight_);
		_ConstSemiringRef semiring = Precursor::fsa_->semiring();
		for (typename _State::const_iterator a = _s->begin(); a != _s->end(); ++a) {
		    _Weight w = semiring->extend(a->weight(), backward_[a->target()]);
		    w = semiring->extend(forward_[s], w);
		    if (semiring->compare(w, minWeight_) <= 0) *sp->newArc() = *a;
		}
		sp->minimize();
		return _ConstStateRef(sp);
	    }
	    return _ConstStateRef();
	}
	virtual size_t getMemoryUsed() const {
	    return Precursor::fsa_->getMemoryUsed() + 2 * sizeof(typename Precursor::Weight) + forward_.getMemoryUsed() +
		backward_.getMemoryUsed();
	}
	virtual std::string describe() const {
	    return Core::form("prunePosterior(%s,%s,%s)", Precursor::fsa_->describe().c_str(),
			      this->semiring()->asString(threshold_).c_str(),
			      relative_ ? "relative" : "absolute");
	}
    };

    template<class _Automaton>
    typename _Automaton::ConstRef prunePosterior
    (typename _Automaton::ConstRef f, const typename _Automaton::Weight &threshold, bool relative) {
	return typename _Automaton::ConstRef(new PosteriorPruneAutomaton<_Automaton>(f, threshold, relative));
    }

    template<class _Automaton>
    typename _Automaton::ConstRef prunePosterior
    (typename _Automaton::ConstRef f, const typename _Automaton::Weight &threshold,
     const StatePotentials<typename _Automaton::Weight> &fw, const StatePotentials<typename _Automaton::Weight>& bw, bool relative) {
	return typename _Automaton::ConstRef(new PosteriorPruneAutomaton<_Automaton>(f, threshold, fw, bw, relative));
    }

    /*
     * for the first state of a synchronous slice that is being extend, we extend
     * all states of the previous slice and determine the pruning threshold
     * => pruning threshold for each slice
     * => state potentials up to current slice
     * => state-to-slice mapping
     * what about epsilon arcs?
     */
    template<class _Automaton>
    class SyncPruneAutomaton : public SlaveAutomaton<_Automaton> {
    private:
	typedef SlaveAutomaton<_Automaton> Precursor;

    public:
	typedef typename _Automaton::Weight _Weight;
	typedef typename _Automaton::State _State;
	typedef typename _Automaton::ConstStateRef _ConstStateRef;
	typedef typename _Automaton::ConstRef _ConstAutomatonRef;
	typedef typename _Automaton::ConstSemiringRef _ConstSemiringRef;

    private:
	_Weight threshold_, maxWeight_;
	mutable Core::Vector<Fsa::StateId> slice_; // index: state
	mutable Core::Vector<_Weight> potentials_; // index: state
	mutable Core::Vector<_Weight> minPotentials_; // index: slice
	mutable Core::Vector<Fsa::StateId> sliceStates_;
    public:
	SyncPruneAutomaton(_ConstAutomatonRef f, const _Weight &threshold) :
	    Precursor(f), threshold_(threshold), maxWeight_(Precursor::fsa_->semiring()->max())
	{
	    this->setProperties(Fsa::PropertyStorage | Fsa::PropertyCached, Fsa::PropertyNone);
	    Fsa::StateId initial = f->initialStateId();
	    slice_.grow(initial, Fsa::InvalidStateId);
	    slice_[initial] = 0;
	    potentials_.grow(initial, f->semiring()->one());
	    minPotentials_.push_back(threshold_);
	    sliceStates_.push_back(initial);
	}

	void calculateSlice() const {
	    // extend all states, calculate minimum weight
	    std::vector<Fsa::StateId> states;
	    _ConstSemiringRef semiring = Precursor::fsa_->semiring();
	    _Weight minWeight = maxWeight_, worst = maxWeight_;
	    while (!sliceStates_.empty()) {
		Core::Vector<Fsa::StateId> epsilonStates;
		for (Core::Vector<Fsa::StateId>::const_iterator s = sliceStates_.begin(); s != sliceStates_.end(); ++s) {
		    _ConstStateRef sp = Precursor::fsa_->getState(*s);
		    for (typename _State::const_iterator a = sp->begin(); a != sp->end(); ++a) {
			_Weight w = semiring->extend(potentials_[*s], a->weight());
			if (semiring->compare(w, worst) < 0) {
			    potentials_.grow(a->target(), maxWeight_);
			    if (a->input_ != Fsa::Epsilon) {
				if (semiring->compare(w, potentials_[a->target()]) < 0) {
				    if (semiring->compare(w, minWeight) < 0) {
					minWeight = w;
					worst = semiring->extend(minWeight, threshold_);
				    }
				    if (semiring->compare(potentials_[a->target()], maxWeight_) == 0)
					states.push_back(a->target());
				    potentials_[a->target()] = w;
				}
			    } else if (semiring->compare(w, potentials_[a->target()]) < 0) {
				epsilonStates.push_back(a->target());
				potentials_[a->target()] = w;
				slice_.grow(a->target(), Fsa::InvalidStateId);
				slice_[a->target()] = minPotentials_.size();
			    }
			}
		    }
		}
		sliceStates_.swap(epsilonStates);
	    }

	    // prune target states with threshold
	    minPotentials_.push_back(worst);
	    sliceStates_.clear();
	    for (std::vector<Fsa::StateId>::iterator i = states.begin(); i != states.end(); ++i) {
		slice_.grow(*i, Fsa::InvalidStateId);
		if (semiring->compare(potentials_[*i], worst) < 0) {
		    sliceStates_.push_back(*i);
		    slice_[*i] = minPotentials_.size() - 1;
		} else potentials_[*i] = maxWeight_;
	    }
	}
	virtual _ConstStateRef getState(Fsa::StateId s) const {
	    if (s < slice_.size()) {
		if (slice_[s] >= minPotentials_.size() - 1) calculateSlice();
		_ConstStateRef _sp = Precursor::fsa_->getState(s);
		// we assume calculateSlice() creates entries in slice_ and potentials_ for each target state
		_State *sp = new _State(_sp->id(), _sp->tags(), _sp->weight_);
		_ConstSemiringRef semiring = Precursor::fsa_->semiring();
		for (typename _State::const_iterator a = _sp->begin(); a != _sp->end(); ++a)
		    if ((a->target() < slice_.size()) && (slice_[a->target()] != Fsa::InvalidStateId) &&
			(semiring->compare(semiring->extend(potentials_[s], a->weight()), minPotentials_[slice_[a->target()]]) < 0))
			*sp->newArc() = *a;
		sp->minimize();
		return _ConstStateRef(sp);
	    }
	    return _ConstStateRef();
	}
	virtual size_t getMemoryUsed() const {
	    return Precursor::fsa_->getMemoryUsed() +
		sizeof(Fsa::LabelId) + 2 * sizeof(typename Precursor::Weight) + slice_.getMemoryUsed() + potentials_.getMemoryUsed() +
		minPotentials_.getMemoryUsed() + sliceStates_.getMemoryUsed();
	}
	virtual std::string describe() const {
	    return Core::form("pruneSync(%s,%s)", Precursor::fsa_->describe().c_str(), this->semiring()->asString(threshold_).c_str());
	}
    };

    template<class _Automaton>
    typename _Automaton::ConstRef pruneSync
    (typename _Automaton::ConstRef f, const typename _Automaton::Weight &threshold) {
	return typename _Automaton::ConstRef(new SyncPruneAutomaton<_Automaton>(f, threshold));
    }

} // namespace Ftl

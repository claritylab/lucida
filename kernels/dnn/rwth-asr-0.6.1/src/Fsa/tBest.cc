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
#include <Core/Assertions.hh>
#include <Core/Hash.hh>
#include <Core/PriorityQueue.hh>
#include <Core/XmlStream.hh>

#include "tAutomaton.hh"
#include "tBest.hh"
#include "tCache.hh"
#include "tDeterminize.hh"
#include "tRational.hh"
#include "tRemoveEpsilons.hh"
#include "Types.hh"
#include <Core/Vector.hh>

namespace Ftl {

    template<class _Automaton>
    class BestAutomaton : public SlaveAutomaton<_Automaton> {
    private:
	typedef SlaveAutomaton<_Automaton> Precursor;

    public:
	typedef typename _Automaton::Weight _Weight;
	typedef typename _Automaton::State _State;
	typedef typename _Automaton::ConstStateRef _ConstStateRef;
	typedef typename _Automaton::ConstRef _ConstAutomatonRef;
	typedef StatePotentials<_Weight> _StatePotentials;

    private:
	_StatePotentials potentials_;

    public:
	BestAutomaton(_ConstAutomatonRef f) :
	    Precursor(f), potentials_(sssp<_Automaton>(transpose<_Automaton>(f, false)))
	{
	    this->setProperties(Fsa::PropertyStorage | Fsa::PropertyCached, Fsa::PropertyNone);
	    this->addProperties(Fsa::PropertySorted);
	    this->addProperties(Fsa::PropertyLinear | Fsa::PropertyAcyclic);
	}
	BestAutomaton(_ConstAutomatonRef f, const _StatePotentials &backward) :
	    Precursor(f), potentials_(backward)
	{
	    this->setProperties(Fsa::PropertyStorage | Fsa::PropertyCached, Fsa::PropertyNone);
	    this->addProperties(Fsa::PropertySorted);
	    this->addProperties(Fsa::PropertyLinear | Fsa::PropertyAcyclic);
	}

	virtual Fsa::StateId initialStateId() const {
	    if (potentials_.size() > 0) return Precursor::fsa_->initialStateId();
	    return Fsa::InvalidStateId;
	}

	/**
	 * \warning best() does not work when there are non-trivial zero-weight loops.
	 */
	virtual _ConstStateRef getState(Fsa::StateId s) const {
	    _ConstStateRef _sp = Precursor::fsa_->getState(s);
	    _State *sp = new _State(_sp->id(), _sp->tags(), _sp->weight_);
	    typename _State::const_iterator bestArc = _sp->end();
	    _Weight minWeight = Precursor::fsa_->semiring()->max();
	    for (typename _State::const_iterator a = _sp->begin(); a != _sp->end(); ++a) {
		if (a->target() == s) continue;
		_Weight w = Precursor::fsa_->semiring()->extend(a->weight_, potentials_[a->target()]);
		if (Precursor::fsa_->semiring()->compare(w, minWeight) < 0) {
		    minWeight = w;
		    bestArc = a;
		}
	    }
	    if (sp->isFinal()) {
		if (Precursor::fsa_->semiring()->compare(sp->weight_, minWeight) < 0) {
		    minWeight = sp->weight_;
		    bestArc = _sp->end();
		}
	    }
	    if (bestArc !=  _sp->end()) {
		sp->unsetFinal();
		*sp->newArc() = *bestArc;
	    }
	    return _ConstStateRef(sp);
	}
	virtual std::string describe() const { return "best(" + Precursor::fsa_->describe() + ")"; }
    };


    /**
     * \todo make bestscore more efficient
     */
    template<class _Automaton>
    typename _Automaton::Weight bestscore(typename _Automaton::ConstRef f) {
	typename _Automaton::ConstRef transposed = transpose<_Automaton>(f);
	if (f->initialStateId() != Fsa::InvalidStateId &&
	    transposed->initialStateId() != Fsa::InvalidStateId) {
	    StatePotentials<typename _Automaton::Weight> backward = sssp<_Automaton>(transposed);
	    return backward[f->initialStateId()];
	}
	return f->semiring()->invalid();
    }

    template<class _Automaton>
    typename _Automaton::ConstRef best(typename _Automaton::ConstRef f) {
	return typename _Automaton::ConstRef(new BestAutomaton<_Automaton>(f));
    }

    template<class _Automaton>
    typename _Automaton::ConstRef best
    (typename _Automaton::ConstRef f, const StatePotentials<typename _Automaton::Weight> &backward) {
	return typename _Automaton::ConstRef(new BestAutomaton<_Automaton>(f, backward));
    }


    // traces
    struct Trace {
    public:
	Fsa::LabelId input_, output_;
	u32 count_, predecessor_;
    public:
	Trace(
	    Fsa::LabelId input = Fsa::InvalidLabelId,
	    Fsa::LabelId output = Fsa::InvalidLabelId,
	    u32 predecessor = 0) :
	    input_(input), output_(output), count_(1), predecessor_(predecessor) {}
    };

    class TraceCache : public Core::Vector<Trace> {
    public:
	typedef u32 Index;
    private:
	Index freelist_;
    public:
	TraceCache(size_t n) : freelist_(0) {
	    reserve(n + 1);
	    for (size_t t = size(); t <= n; ++t) push_back(Trace());
	}
	Index newTrace(const Trace &trace) {
	    ++(*this)[trace.predecessor_].count_;
	    if (freelist_) {
		Index t = freelist_;
		freelist_ = (*this)[freelist_].predecessor_;
		(*this)[t] = trace;
		return t;
	    } else {
		push_back(trace);
		return size() - 1;
	    }
	}
	void deleteTrace(Index t) {
	    if (--(*this)[t].count_ == 0) {
		(*this)[t].predecessor_ = freelist_;
		freelist_ = t;
	    }
	}
    };

    // n best traces
    template<class _Automaton>
    class NBestTraces {
	typedef Core::Vector<typename _Automaton::Weight> Precursor;
    public:
	typedef typename _Automaton::Weight _Weight;
	typedef typename _Automaton::State _State;
	typedef typename _Automaton::ConstStateRef _ConstStateRef;
	typedef typename _Automaton::ConstRef _ConstAutomatonRef;
	typedef typename _Automaton::ConstSemiringRef _ConstSemiringRef;
	typedef StatePotentials<_Weight> _StatePotentials;

    private:
	struct Hyp {
	    Fsa::StateId state_;
	    _Weight potential_;
	    u32 trace_;
	    Hyp() {}
	    Hyp(Fsa::StateId state, _Weight potential, u32 trace) :
		state_(state), potential_(potential), trace_(trace) {}
	};

	struct PriorityFunction {
	    _ConstSemiringRef semiring_;
	    const _StatePotentials &potentials_;
	    PriorityFunction(_ConstSemiringRef semiring, const _StatePotentials &potentials) :
		semiring_(semiring), potentials_(potentials) {}
	    bool operator() (const Hyp &h1, const Hyp &h2) const {
		return (semiring_->compare(semiring_->extend(h1.potential_, potentials_[h1.state_]),
					   semiring_->extend(h2.potential_, potentials_[h2.state_])) < 0);
	    }
	};

    private:
	TraceCache traces_;
	Core::Vector<_Weight> finals_;

    public:
	NBestTraces(_ConstAutomatonRef f, size_t n, bool bestSequences) : traces_(n), finals_() {
	    // check for tropical semiring (or for at least a semiring with property 'ordered')
	    if (bestSequences) {
		if (f->type() & Fsa::TypeAcceptor)
		    f = cache<_Automaton>(removeEpsilons<_Automaton>(f));
		f = determinize<_Automaton>(f);
	    }
	    _ConstAutomatonRef f2 = transpose<_Automaton>(f); // also generates a single final state
	    _StatePotentials potentials(sssp<_Automaton>(f, false));
	    _ConstSemiringRef semiring = f2->semiring();
	    Core::PriorityQueue<Hyp, PriorityFunction> S(PriorityFunction(semiring, potentials));
	    Core::Vector<size_t> r_;

	    finals_.reserve(n + 1);
	    finals_.push_back(semiring->one());
	    S.insert(Hyp(f2->initialStateId(), semiring->one(), traces_.newTrace(Trace())));
	    while (!S.empty()) {
		Hyp h = S.top();
		S.pop();
		_ConstStateRef sp = f2->getState(h.state_);
		if (sp->isFinal()) {
		    traces_[finals_.size()] = traces_[h.trace_];
		    finals_.push_back(semiring->extend(h.potential_, sp->weight_));
		    if (finals_.size() == n + 1) break; // stop after n final traces
		}
		// there can only be n distinct paths through a single state
		r_.grow(h.state_, 0);
		if (++r_[h.state_] <= n)
		    for (typename _State::const_iterator a = sp->begin(); a != sp->end(); ++a)
			S.insert(Hyp(a->target(), semiring->extend(h.potential_, a->weight()),
				     traces_.newTrace(Trace(a->input(), a->output(), h.trace_))));
		traces_.deleteTrace(h.trace_);
	    }
	}
	size_t nTraces() const {
	    return traces_.size();
	}
	const Trace & trace(size_t n) const {
	    require(n < traces_.size()); return traces_[n];
	}
	size_t size() const {
	    return finals_.size() - 1;
	}
	const _Weight & weight(size_t n) const {
	    require(n < finals_.size()); return finals_[n];
	}
	size_t getMemoryUsed() const {
	    return traces_.getMemoryUsed() + finals_.getMemoryUsed();
	}
	void dumpMemoryUsage(Core::XmlWriter &o) const {
	    o << Core::XmlFull("traces", traces_.getMemoryUsed())
	      << Core::XmlFull("weights", finals_.getMemoryUsed());
	}
    };

    /*
     * 0:    initial state
     * 1..n: start states for sentences
     *
     * ideas:
     * - restrict queue to exactly n hyptheses at a time (corresponding to n paths)
     * - use reference counting for traces?
     * - property of traces: exactly one predecessor
     * - attach path weight to first arc
     */
    template<class _Automaton>
    class NBestAutomaton : public SlaveAutomaton<_Automaton> {
    private:
	typedef SlaveAutomaton<_Automaton> Precursor;

    public:
	typedef typename _Automaton::Weight _Weight;
	typedef typename _Automaton::State _State;
	typedef typename _Automaton::ConstStateRef _ConstStateRef;
	typedef typename _Automaton::ConstRef _ConstAutomatonRef;

    private:
	NBestTraces<_Automaton> nBestTraces_;
	size_t n_;

    public:
	NBestAutomaton(_ConstAutomatonRef f, size_t n, bool bestSequences) :
	    Precursor(f),
	    nBestTraces_(f, n, bestSequences) {
	    n_ = std::min(n, nBestTraces_.size());
	    this->setProperties(Fsa::PropertyStorage | Fsa::PropertyCached, Fsa::PropertyNone);
	    this->addProperties(Fsa::PropertySortedByInput);
	    this->addProperties(Fsa::PropertyAcyclic);
	}
	virtual std::string describe() const { return "nbest(" + Precursor::fsa_->describe() + ")"; }

	virtual Fsa::StateId initialStateId() const { return 0; }
	virtual _ConstStateRef getState(Fsa::StateId s) const {
	    _State *sp = 0;
	    if (s == 0) {
		sp = new _State(0);
		for (size_t n = 1; n <= n_; ++n)
		    sp->newArc(n, nBestTraces_.weight(n), Fsa::Epsilon);
		return _ConstStateRef(sp);
	    } else if (s < nBestTraces_.nTraces()) {
		const Trace &trace = nBestTraces_.trace(s);
		if (trace.predecessor_) {
		    sp = new _State(s);
		    sp->newArc(trace.predecessor_, Precursor::fsa_->semiring()->one(), trace.input_, trace.output_);
		} else sp = new _State(s, Fsa::StateTagFinal, Precursor::fsa_->semiring()->one());
	    }
	    return _ConstStateRef(sp);
	}
	virtual size_t getMemoryUsed() const {
	    return Precursor::fsa_->getMemoryUsed() + nBestTraces_.getMemoryUsed();
	}
	virtual void dumpMemoryUsage(Core::XmlWriter &o) const {
	    o << Core::XmlOpen("nbest");
	    Precursor::fsa_->dumpMemoryUsage(o);
	    nBestTraces_.dumpMemoryUsage(o);
	    o << Core::XmlClose("nbest");
	}
    };

    template<class _Automaton>
    typename _Automaton::ConstRef nbest(typename _Automaton::ConstRef f, size_t n, bool bestSequences) {
	return typename _Automaton::ConstRef(new NBestAutomaton<_Automaton>(f, n, bestSequences));
    }

    // =======================================================================

    template<class _Automaton>
    class FirstBestAutomaton : public SlaveAutomaton<_Automaton> {
    private:
	typedef SlaveAutomaton<_Automaton> Precursor;

    public:
	typedef typename _Automaton::Weight _Weight;
	typedef typename _Automaton::State _State;
	typedef typename _Automaton::ConstStateRef _ConstStateRef;
	typedef typename _Automaton::ConstRef _ConstAutomatonRef;
	typedef typename _Automaton::ConstSemiringRef _ConstSemiringRef;

    private:
	struct Step {
	    Fsa::StateId state;
	    s32 arc;
	    Step(Fsa::StateId _state, s32 _arc) : state(_state), arc(_arc) {}
	};

	struct Trace : Step {
	    typedef u32 Id;
	    Id back;
	    Trace(Id _back, Fsa::StateId _state, s32 _arc) :
		Step(_state, _arc), back(_back) {}
	};
	typedef typename Trace::Id TraceId;

	class TracePool :
	    public std::vector<Trace>
	{
	public:
	    u32 newTrace(TraceId _back, Fsa::StateId _state, s32 _arc) {
		std::vector<Trace>::push_back(Trace(_back, _state, _arc));
		return this->size() - 1;
	    }
	    TracePool() {
		std::vector<Trace>::push_back(Trace(0, Fsa::InvalidLabelId, 0));
	    }
	};

	struct Hyp {
	    Fsa::StateId state;
	    _Weight weight;
	    TraceId trace;

	    struct Key {
		Fsa::StateId operator() (const Hyp &hyp) const { return hyp.state; }
	    };

	    struct Priority {
		_ConstSemiringRef semiring_;
		Priority(_ConstSemiringRef sr) : semiring_(sr) {}

		bool operator() (const Hyp &h1, const Hyp &h2) const {
		    return semiring_->compare(h1.weight, h2.weight) < 0;
		}
	    };
	};

	TraceId search(TracePool &traces) {
	    typename Hyp::Priority precedes(this->semiring());
	    Core::TracedPriorityQueue<Hyp, Fsa::StateId, typename Hyp::Key, typename Hyp::Priority>
		stack(precedes);
	    Core::hash_map<Fsa::StateId, _Weight> closed;

	    Hyp newHyp;
	    newHyp.state = Precursor::fsa_->initialStateId();
	    newHyp.weight = this->semiring()->one();
	    newHyp.trace = 0;
	    stack.insert(newHyp);

	    while (!stack.empty()) {
		Hyp current = stack.top();
		if (current.state & Fsa::StateTagFinal)
		    break;

		verify(closed.find(current.state) == closed.end());
		closed[current.state] = current.weight;

		stack.pop();

		_ConstStateRef state = Precursor::fsa_->getState(current.state);
		for (typename _State::const_iterator aa = state->begin(); aa != state->end(); ++aa) {
		    newHyp.state  = aa->target();
		    newHyp.weight = this->semiring()->extend(current.weight, aa->weight());
		    if (closed.find(newHyp.state) != closed.end()) {
//			std::cerr << __LINE__ << "\t" << newHyp.state << "\t" << f32(newHyp.weight) << "\t" << f32(closed[newHyp.state]) << std::endl; // DEBUG
//                      Assertion is true, but dangerous due to limited accuracy.
//			verify(semiring()->compare(newHyp.weight, closed[newHyp.state]) >= 0);
		    } else {
			newHyp.trace = traces.newTrace(current.trace, current.state, aa - state->begin());
			stack.insertOrRelax(newHyp);
		    }
		}
		if (state->isFinal()) {
		    newHyp.state  = current.state | Fsa::StateTagFinal;
		    newHyp.weight = this->semiring()->extend(current.weight, state->weight_);
		    if (closed.find(newHyp.state) != closed.end()) {
//			std::cerr << __LINE__ << "\t" << newHyp.state << "\t" << f32(newHyp.weight) << "\t" << f32(closed[newHyp.state]) << std::endl; // DEBUG
//                      Assertion is true, but dangerous due to limited accuracy.
//			verify(semiring()->compare(newHyp.weight, closed[newHyp.state]) >= 0);
		    } else {
			newHyp.trace = traces.newTrace(current.trace, current.state, -1);
			stack.insertOrRelax(newHyp);
		    }
		}
	    }

	    if (stack.empty())
		return 0;

	    return stack.top().trace;
	}

	Core::hash_map<Fsa::StateId, u32> path_;
	void traceback(TracePool &traces, TraceId trace) {
	    while (trace) {
		path_[traces[trace].state] = traces[trace].arc;
		trace = traces[trace].back;
	    }
	}

    public:
	FirstBestAutomaton(_ConstAutomatonRef ff) :
	    Precursor(ff)
	{
	    this->setProperties(Fsa::PropertyStorage | Fsa::PropertyCached, Fsa::PropertyNone);
	    this->addProperties(Fsa::PropertySorted);
	    this->addProperties(Fsa::PropertyLinear | Fsa::PropertyAcyclic);
	    TracePool traces;
	    traceback(traces, search(traces));
	}

	virtual std::string describe() const { return "firstbest(" + Precursor::fsa_->describe() + ")"; }

	virtual _ConstStateRef getState(Fsa::StateId si) const {
	    require(path_.find(si) != path_.end());
	    s32 arc = path_.find(si)->second;
	    _ConstStateRef origState = Precursor::fsa_->getState(si);

	    _State *state = new _State(origState->id(), origState->tags(), origState->weight_);
	    if (arc >= 0) {
		typename _State::const_iterator origArc = origState->begin() + arc;
		*state->newArc() = *origArc;
		state->unsetFinal();
	    }
	    return _ConstStateRef(state);
	}


    };

    template<class _Automaton>
    typename _Automaton::ConstRef firstbest(typename _Automaton::ConstRef f) {
	return typename _Automaton::ConstRef(new FirstBestAutomaton<_Automaton>(f));
    }

} // namespace Ftl

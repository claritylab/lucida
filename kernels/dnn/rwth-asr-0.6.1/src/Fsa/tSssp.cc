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
#include "tInfo.hh"
#include "tProperties.hh"
#include "tRational.hh"
#include "tSemiring.hh"
#include "tSort.hh"
#include "tSssp.hh"
#include <Core/Vector.hh>

namespace Ftl {

	/**
	 * queue:
	 * - end of queue is defined as self-referencing state id
	 * - default implementation is a stack
	 **/
	class SsspQueue {
	protected:
		Fsa::StateId head_, tail_, n_;
		Core::Vector<Fsa::StateId> next_;
	public:
		SsspQueue() :
			head_(Fsa::InvalidStateId), tail_(Fsa::InvalidStateId), n_(0) {}
		SsspQueue(Fsa::StateId maxStateId) :
			head_(Fsa::InvalidStateId), tail_(Fsa::InvalidStateId), n_(0) {
			next_.grow(maxStateId, Fsa::InvalidStateId);
		}

		virtual ~SsspQueue() {}

		virtual std::string name() const {
			return "stack";
		}
		bool empty() const {
			return head_ == Fsa::InvalidStateId;
		}
		Fsa::StateId dequeue() {
			require_(!empty());
			Fsa::StateId s = head_;
			head_ = next_[s]; // unlink head state
			if (head_ == s)
				// end of queue
				head_ = tail_ = Fsa::InvalidStateId;
			next_[s] = Fsa::InvalidStateId;
			--n_;
			return s;
		}
		virtual void enqueue(Fsa::StateId s) {
			if (next_[s] == Fsa::InvalidStateId) {
				if (head_ != Fsa::InvalidStateId)
					next_[s] = head_;
				else
					next_[s] = s;
				head_ = s;
				++n_;
			}
		}
		Fsa::StateId size() const {
			return n_;
		}
		Fsa::StateId maxStateId() const {
			return (next_.size() == 0 ? Fsa::InvalidStateId : next_.size() - 1);
		}
	};

	class FifoSsspQueue : public SsspQueue {
	public:
		FifoSsspQueue(Fsa::StateId maxStateId) :
			SsspQueue(maxStateId) {}

		virtual ~FifoSsspQueue() {}

		std::string name() const {
			return "queue";
		}
		void enqueue(Fsa::StateId s) {
			if (next_[s] == Fsa::InvalidStateId) {
				next_[s] = s;
				if (head_ == Fsa::InvalidStateId)
					head_ = s;
				else
					next_[tail_] = s;
				tail_ = s;
				++n_;
			}
		}
	};

	class TopologicalSsspQueue : public SsspQueue {
	private:
		Fsa::StateMap s2t_;
	public:
		TopologicalSsspQueue(const Fsa::StateMap &s2t) :
			SsspQueue(s2t.size() - 1), s2t_(s2t) {}

		virtual ~TopologicalSsspQueue() {}

		std::string name() const {
			return "topo";
		}
		void enqueue(Fsa::StateId s) {
			if (next_[s] == Fsa::InvalidStateId) {
				// ensure: for all queue neighbours i, j we have s2t[i] < s2t[j]
				Fsa::StateId i = head_, j = Fsa::InvalidStateId, st = s2t_[s];
				for (; j != i; i = next_[i])
					if (s2t_[i] < st)
						j = i;
					else
						break;
				if (j == Fsa::InvalidStateId) {
					if (i == Fsa::InvalidStateId)
						next_[s] = s;
					else
						next_[s] = head_;
					head_ = s;
				} else {
					if (next_[j] == j)
						next_[s] = s;
					else
						next_[s] = next_[j];
					next_[j] = s;
				}
				++n_;
			}
		}
	};

	template<class _Automaton, class Queue>
	StatePotentials<typename _Automaton::Weight> ssspLoop(
			Queue &q, typename _Automaton::ConstRef f, Fsa::StateId start,
			const SsspArcFilter<_Automaton> &arcFilter, bool progress) {
		typedef typename _Automaton::Weight _Weight;
		typedef typename _Automaton::State _State;
		typedef typename _Automaton::ConstStateRef _ConstStateRef;
		typedef typename _Automaton::ConstSemiringRef _ConstSemiringRef;
		typedef typename _Automaton::Weight _Weight;
		typedef StatePotentials<_Weight> _StatePotentials;

		Fsa::StateId maxStateId = q.maxStateId();
		if (maxStateId == Fsa::InvalidStateId) {
			Fsa::AutomatonCounts counts = count<_Automaton>(f, progress);
			maxStateId = counts.maxStateId_;
		}
		if ((start > maxStateId) || (maxStateId == Fsa::InvalidStateId))
			return _StatePotentials();
		_ConstSemiringRef semiring = f->semiring();
		// d[q] denotes the tentative shortest distance from the source s to q (cite from [Mohri])
		_StatePotentials d(maxStateId + 1, semiring->zero());
		// r[q] keeps track of the sum of the weights +-added to d[q] since the last queue extraction of q (cite from [Mohri])
		_StatePotentials r(maxStateId + 1, semiring->zero());
		d[start] = r[start] = semiring->one();

		q.enqueue(start);
		Core::ProgressIndicator *p = 0;
		if (progress) {
			p = new Core::ProgressIndicator("sssp(" + q.name() + ")", "states");
			p->start();
		}
		while (!q.empty()) {
			Fsa::StateId s = q.dequeue();
			_ConstStateRef sp = f->getState(s);
			_Weight R = r[s];
			r[s] = semiring->zero();
			for (typename _State::const_iterator a = sp->begin(); a	!= sp->end(); ++a) {
				if (arcFilter(*a)) {
					// d[arc->target()] = d[arc->target()] + (R * arc->weight())
					// r[arc->target()] = r[arc->target()] + (R * arc->weight())
					_Weight extended = semiring->extend(R, a->weight());
					_Weight w = semiring->collect(d[a->target()], extended);
					if (semiring->compare(d[a->target()], w) != 0) {
						d[a->target()] = w;
						r[a->target()] = semiring->collect(r[a->target()], extended);
						q.enqueue(a->target());
					}
				}
			}
			if (p) p->notify(q.size(), q.maxStateId() - 1);
		}
		if (p) {
			p->finish();
			delete p;
		}
		// although in the original paper by Mohri and Riley, uncommenting the following
		// gives wrong initial and final weights when pushing:
		// d[start] = f->semiring()->one();
		return d;
	}
	;

	template<class _Automaton> StatePotentials<typename _Automaton::Weight> sssp(
			typename _Automaton::ConstRef f, Fsa::StateId start,
			const SsspArcFilter<_Automaton> &arcFilter, bool progress) {
		if (hasProperties<_Automaton>(f, Fsa::PropertyAcyclic)) {
			Fsa::StateMap m = topologicallySort<_Automaton>(f);
			TopologicalSsspQueue q(m);
			if (q.maxStateId() != Fsa::InvalidStateId)
				return ssspLoop<_Automaton, TopologicalSsspQueue>(q, f, start,
						arcFilter, progress);

		}
		// in case of a wrongly tagged automaton we gently fall back to a standard filo queue
		Fsa::AutomatonCounts counts = count<_Automaton>(f, progress);
		FifoSsspQueue q(counts.maxStateId_);
		return ssspLoop<_Automaton, FifoSsspQueue>(q, f, start, arcFilter, progress);
	}

	template<class _Automaton>
	StatePotentials<typename _Automaton::Weight> sssp(typename _Automaton::ConstRef f, bool progress) {
		return sssp<_Automaton>(f, f->initialStateId(), SsspArcFilter<_Automaton>(), progress);
	}


    template<class _Automaton>
    StatePotentials<typename _Automaton::Weight> ssspBackward(typename _Automaton::ConstRef f, const SsspArcFilter<_Automaton> &arcFilter, bool progress) {
		typedef typename _Automaton::Weight _Weight;
		typedef typename _Automaton::State _State;
		typedef typename _Automaton::ConstStateRef _ConstStateRef;
		typedef typename _Automaton::ConstSemiringRef _ConstSemiringRef;
		typedef StatePotentials<_Weight> _StatePotentials;

		Fsa::AutomatonCounts counts = count<_Automaton>(f, progress);
		Fsa::StateId maxStateId = counts.maxStateId_;
		if (maxStateId == Fsa::InvalidStateId) return _StatePotentials();

		// initalize with final weights
		_ConstSemiringRef semiring = f->semiring();
		_StatePotentials d(maxStateId + 1, semiring->zero());
		_StatePotentials r(maxStateId + 1, semiring->zero());
		std::vector<bool> changed(maxStateId + 1, false), oldChanged(maxStateId + 1, false);
		for (size_t s = 0; s <= maxStateId; ++s) {
			_ConstStateRef sp = f->getState(s);
			if ((sp) && (sp->isFinal())) {
				d[s] = r[s] = sp->weight();
				oldChanged[s] = true;
			}
		}

		Core::ProgressIndicator *p = 0;
		if (progress) {
			p = new Core::ProgressIndicator("sssp (backward)", "states");
			p->start();
		}
		bool potentialsHaveChanged = true;
		while (potentialsHaveChanged) {
			potentialsHaveChanged = false;
			for (Fsa::StateId s = 0; s <= maxStateId; ++s) {
				_ConstStateRef sp = f->getState(s);
				if (sp) {
					bool targetHasChanged = false;
					for (typename _State::const_iterator a = sp->begin(); a != sp->end(); ++a)
						if (oldChanged[a->target()] && arcFilter(*a)) {
							targetHasChanged = true;
							break;
						}
					if (targetHasChanged) {
						_Weight D = d[s], R = semiring->zero();
						for (typename _State::const_iterator a = sp->begin(); a != sp->end(); ++a)
							if (arcFilter(*a)) {
								D = semiring->collect(D, semiring->extend(a->weight(), r[a->target()]));
								R = semiring->collect(R, semiring->extend(a->weight(), r[a->target()]));
							}
						if (semiring->compare(d[s], D) != 0) {
							d[s] = D;
							r[s] = R;
							changed[s] = true;
							potentialsHaveChanged = true;
						}
					}
					if (p) p->notify();
				}
			}
			std::fill(oldChanged.begin(), oldChanged.end(), false);
			std::swap(changed, oldChanged);
		}
		if (p) {
			p->finish();
			delete p;
		}
		return d;
    }


    template<class _Automaton>
    StatePotentials<typename _Automaton::Weight> ssspBackward(typename _Automaton::ConstRef f, bool progress) {
		return ssspBackward<_Automaton>(f, SsspArcFilter<_Automaton>(), progress);
    }



	// together with the final weight correction above we also changed the calculation of the
	// final weights in the two automata below. the uncommented code is the original one from the
	// literature. residual initial weights are ignored.
	template<class _Automaton> class PushToFinalAutomaton :
			public ModifyAutomaton<_Automaton> {
		typedef ModifyAutomaton<_Automaton> Precursor;
	public:
		typedef typename _Automaton::Weight _Weight;
		typedef typename _Automaton::State _State;
		typedef typename _Automaton::ConstRef _ConstAutomatonRef;
	protected:
		StatePotentials<_Weight> potentials_;
	Core::Ref<const typename _Automaton::Semiring> semiring_;
	Fsa::StateId initialStateId_;

	public:
		PushToFinalAutomaton(_ConstAutomatonRef f, bool progress) :
			Precursor(f),
			semiring_(f->semiring()),
			initialStateId_(f->initialStateId())
		{
			potentials_ = sssp(f, progress);
		}
		virtual void modifyState(_State *sp) const {
			Fsa::StateId s = sp->id();
			for (typename _State::iterator a = sp->begin(); a != sp->end(); ++a)
				a->weight_ = semiring_->extend(semiring_->extend(potentials_[s], a->weight()), semiring_->invert(potentials_[a->target()]));
			if (s == initialStateId_)
				sp->weight_ = semiring_->one(); //extend(sp->weight_, potentials_[s]);
			if (sp->isFinal())
				sp->weight_ = semiring_->extend(semiring_->extend(potentials_[s], sp->weight_), semiring_->invert(potentials_[initialStateId_]));
			//if (sp->isFinal()) sp->weight_ = semiring()->extend(sp->weight_, semiring()->invert(potentials_[s]));
		}
		virtual std::string describe() const {
			return "pushToFinal(" + Precursor::fsa_->describe() + ")";
		}
	};

	template<class _Automaton> typename _Automaton::ConstRef pushToFinal(
			typename _Automaton::ConstRef f, bool progress) {
		return typename _Automaton::ConstRef(new PushToFinalAutomaton<_Automaton>(f, progress));
	}

	template<class _Automaton> class PushToInitialAutomaton :
			public ModifyAutomaton<_Automaton> {
		typedef ModifyAutomaton<_Automaton> Precursor;
	public:
		typedef typename _Automaton::Weight _Weight;
		typedef typename _Automaton::State _State;
		typedef typename _Automaton::ConstRef _ConstAutomatonRef;
	protected:
		Core::Ref<const typename _Automaton::Semiring> semiring_;
		Fsa::StateId initialStateId_;
		StatePotentials<_Weight> potentials_;
	public:
		PushToInitialAutomaton(_ConstAutomatonRef f, bool progress) :
			Precursor(f),
			semiring_(f->semiring()),
			initialStateId_(f->initialStateId())
		{
			potentials_ = sssp<_Automaton>(transpose<_Automaton>(f), progress);
			/*! @todo test ssspBackward:
			 *  potentials_ = ssspBackward<_Automaton>(f, progress);
			 */
		}
		PushToInitialAutomaton(_ConstAutomatonRef f, const StatePotentials<typename _Automaton::Weight> &potentials, bool progress) :
				Precursor(f),
				semiring_(f->semiring()),
				initialStateId_(f->initialStateId()),
				potentials_(potentials)
		{}
		virtual void modifyState(_State *sp) const {
			Fsa::StateId s = sp->id();
			for (typename _State::iterator a = sp->begin(); a != sp->end(); ++a) {
				a->weight_ = semiring_->extend(semiring_->invert(potentials_[s]), semiring_->extend(a->weight(), potentials_[a->target()]));
			}
			if (s == initialStateId_)
				sp->weight_ = semiring_->one(); //extend(sp->weight_, potentials_[s]);
			if (sp->isFinal())
				sp->weight_ = semiring_->extend(semiring_->extend(sp->weight_,
						potentials_[initialStateId_]), semiring_->invert(potentials_[s]));
			//if (sp->isFinal()) sp->weight_ = semiring()->extend(semiring()->invert(potentials_[s]), sp->weight_);
		}
		virtual std::string describe() const {
			return "pushToInitial(" + Precursor::fsa_->describe() + ")";
		}
	};

	template<class _Automaton> typename _Automaton::ConstRef pushToInitial(
			typename _Automaton::ConstRef f, bool progress) {
		return typename _Automaton::ConstRef(new PushToInitialAutomaton<_Automaton>(f, progress));
	}

	template<class _Automaton> class PosteriorAutomaton :
			public ModifyAutomaton<_Automaton> {
		typedef ModifyAutomaton<_Automaton> Precursor;
	public:
		typedef typename _Automaton::Weight _Weight;
		typedef typename _Automaton::State _State;
		typedef typename _Automaton::ConstRef _ConstAutomatonRef;
		typedef StatePotentials<_Weight> _StatePotentials;
	private:
		_StatePotentials forwardPotentials_;
		_StatePotentials backwardPotentials_;
		_Weight totalInv_;

	public:
		PosteriorAutomaton(_ConstAutomatonRef f) :
			Precursor(f) {
			forwardPotentials_ = sssp<_Automaton>(f);
			backwardPotentials_ = sssp<_Automaton>(transpose<_Automaton>(f));
			totalInv_ = this->semiring()->invert(backwardPotentials_[f->initialStateId()]);
		}
		PosteriorAutomaton(_ConstAutomatonRef f,
				const _StatePotentials &forward) :
			Precursor(f), forwardPotentials_(forward) {
			backwardPotentials_ = sssp<_Automaton>(transpose<_Automaton>(f));
			totalInv_ = this->semiring()->invert(backwardPotentials_[f->initialStateId()]);
		}
		virtual void modifyState(_State *sp) const {
			Fsa::StateId s = sp->id();
			for (typename _State::iterator a = sp->begin(); a != sp->end(); ++a) {
				a->weight_ = this->semiring()->extend(a->weight(), backwardPotentials_[a->target()]);
				a->weight_ = this->semiring()->extend(forwardPotentials_[s], a->weight());
				a->weight_ = this->semiring()->extend(totalInv_, a->weight());
			}
		}
		const _Weight& totalInv() const {
			return totalInv_;
		}
		virtual std::string describe() const {
			return "posterior(" + Precursor::fsa_->describe() + ")";
		}
	};

	template<class _Automaton> typename _Automaton::ConstRef posterior(
			typename _Automaton::ConstRef f) {
		return typename _Automaton::ConstRef(new PosteriorAutomaton<_Automaton>(f));
	}

	template<class _Automaton> typename _Automaton::ConstRef posterior(
			typename _Automaton::ConstRef f,
			const StatePotentials<typename _Automaton::Weight> &forward) {
		return typename _Automaton::ConstRef(new PosteriorAutomaton<_Automaton>(f, forward));
	}

	template<class _Automaton> typename _Automaton::ConstRef posterior(
			typename _Automaton::ConstRef f,
			typename _Automaton::Weight &totalInv) {
		PosteriorAutomaton<_Automaton> *p=new PosteriorAutomaton<_Automaton>(f);
		totalInv=p->totalInv();
		return typename _Automaton::ConstRef(p);
	}

	/**
	 * CountPathsAutomaton
	 *
	 * Sets the semiring to CountPathSemiring, all arc
	 * weights to 1 and adds a super final state to the automaton in
	 * order to do sssp in a single forward pass.
	 * @param f the automaton to count paths for
	 */
	typedef size_t Count;
	typedef Semiring<Count> CountSemiring;
	typedef Automaton<CountSemiring> CountAutomaton;

	class CountPathsSemiring_ : public CountSemiring {
	private:
		Count infinity() const {
			return Core::Type<Count>::max;
		}
	public:
		virtual std::string name() const {
			return "count-paths";
		}
		virtual Count create() const {
			return 0;
		}
		virtual Count clone(const Count &a) const {
			return a;
		}
		virtual Count invalid() const {
			return Count(Core::Type<Count>::min);
		}
		virtual Count zero() const {
			return 0;
		}
		virtual Count one() const {
			return 1;
		}
		virtual Count max() const {
			return infinity();
		}
		virtual Count extend(const Count &a, const Count &b) const {
			if (a == infinity())
				return infinity();
			if (b == infinity())
				return infinity();
			if (infinity() / std::max(a, b) < std::min(a, b))
				return infinity();
			return a * b;
		}
		virtual Count collect(const Count &a, const Count &b) const {
			if (a == infinity())
				return infinity();
			if (b == infinity())
				return infinity();
			if (infinity() - a < b)
				return infinity();
			return a + b;
		}
		virtual Count invert(const Count &a) const {
			return -a;
		}
		virtual int compare(const Count &a, const Count &b) const {
			return a < b ? -1 : (a > b ? 1 : 0);
		}
		virtual size_t hash(const Count &a) const {
			return size_t(a);
		}
		virtual bool isDefault(const Count &a) const {
			return a == defaultWeight();
		}

		virtual bool read(Count &a, Core::BinaryInputStream &i) const {
			f64 tmp;
			i >> tmp;
			a = Count(tmp);
			return i;
		}
		virtual bool write(const Count &a, Core::BinaryOutputStream &o) const {
			if (a == infinity())
				o << "inf";
			return o << f64(a);
			return o;
		}
		virtual Count fromString(const std::string &str) const {
			size_t tmp;
			if (!Core::strconv(str, tmp))
				std::cerr << "'" << str << "' is not a valid " << name()
						<< " semiring value." << std::endl;
			return Count(tmp);
		}
		virtual std::string asString(const Count &a) const {
			if (a == infinity())
				return "inf";
			return Core::form("%zd", size_t(a));
		}
		virtual void compress(Core::Vector<u8> &stream, const Count &a) const {
			std::cerr << "method \"compress()\" is not supported" << std::endl;
		}
		virtual Count uncompress(Core::Vector<u8>::const_iterator &) const {
			std::cerr << "method \"uncompress()\" is not supported"
					<< std::endl;
			return invalid();
		}
		virtual size_t compressedSize() const {
			std::cerr << "method \"compressedSize()\" is not supported"
					<< std::endl;
			return 0;
		}
		virtual std::string describe(const Count &a, Fsa::Hint hints = 0) const {
			return asString(a);
		}
	};

	template<class _SrcAutomaton> class CountPathsAutomaton :
			public WrapperAutomaton<_SrcAutomaton, CountAutomaton> {
		typedef WrapperAutomaton<_SrcAutomaton, CountAutomaton> Precursor;
	public:
		typedef typename CountAutomaton::State _State;
		typedef typename CountAutomaton::ConstStateRef _ConstStateRef;
		typedef typename CountAutomaton::ConstSemiringRef _ConstSemiringRef;
	private:
		static _ConstSemiringRef semiring_;
		_ConstStateRef superFinalState_;
	public:
		CountPathsAutomaton(typename _SrcAutomaton::ConstRef f) :
			Precursor(f) {
			if (!(semiring_))
				semiring_ = _ConstSemiringRef(new CountPathsSemiring_());
			superFinalState_ = _ConstStateRef(new _State(0, Fsa::StateTagFinal, semiring()->one()));
		}
		virtual _ConstSemiringRef semiring() const {
			return semiring_;
		}
		virtual Fsa::StateId initialStateId() const {
			return Precursor::fsa_->initialStateId() + 1;
		}
		virtual _ConstStateRef getState(Fsa::StateId s) const {
			if (s == 0)
				return superFinalState_;
			typename _SrcAutomaton::ConstStateRef fromSp =
					Precursor::fsa_->getState(s - 1);
			_State *sp = new _State(fromSp->id(), fromSp->tags());
			sp->setId(s);
			for (typename _SrcAutomaton::State::const_iterator a =
					fromSp->begin(); a != fromSp->end(); ++a)
				sp->newArc(a->target() + 1, semiring()->one(), a->input(), a->output());
			if (sp->isFinal()) {
				sp->setTags(sp->tags() & ~Fsa::StateTagFinal);
				sp->newArc(0, semiring()->one(), Fsa::Epsilon);
			}
			return _ConstStateRef(sp);
		}
		virtual std::string describe() const {
			return "countPaths(" + Precursor::fsa_->describe() + ")";
		}
	};

	template<class _SrcAutomaton> typename CountAutomaton::ConstSemiringRef
			CountPathsAutomaton<_SrcAutomaton>::semiring_ =
					CountAutomaton::ConstSemiringRef();

	template<class _SrcAutomaton> size_t countPaths(
			typename _SrcAutomaton::ConstRef f) {
		CountAutomaton::ConstRef tmp =
				CountAutomaton::ConstRef(new CountPathsAutomaton<_SrcAutomaton>(f));
		StatePotentials<Count> potentials = sssp<CountAutomaton>(tmp);
		return size_t(potentials[0]); // potential at super final state
	}
} // namespace Ftl

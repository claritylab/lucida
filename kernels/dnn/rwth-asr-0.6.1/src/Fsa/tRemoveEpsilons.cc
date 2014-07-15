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
#include <iostream>
#include <set>
#include <Core/PriorityQueue.hh>
#include "tAutomaton.hh"
#include "tCache.hh"
#include "tProperties.hh"
#include "tRemoveEpsilons.hh"
#include "tSort.hh"
#include "tSssp.hh"
#include "Stack.hh"
#include "Types.hh"
#include <Core/Vector.hh>

namespace Ftl {

	/**
	 * remove epsilon (input) transitions
	 *
	 * use invert() if you wish to remove epsilon output transitions instead.
	 * does not work for cyclic automata where the semiring is not k-closed. from:
	 *
	 *   "Generic Epsilon-Removal Algorithm for Weighted Automata"
	 **/
	template<class _Automaton> class RemoveEpsilons :
			public ModifyAutomaton<_Automaton> {
	private:
		typedef ModifyAutomaton<_Automaton> Precursor;

	public:
		typedef typename _Automaton::Weight _Weight;
		typedef typename _Automaton::Arc _Arc;
		typedef typename _Automaton::State _State;
		typedef typename _Automaton::Semiring _Semiring;
		typedef Core::Ref<const _State> _ConstStateRef;
		typedef Core::Ref<const _Automaton> _ConstAutomatonRef;
		typedef Core::Ref<const _Semiring> _ConstSemiringRef;

	private:
		typedef std::pair<Fsa::StateId, _Weight> EpsPath;
		struct EpsPathCmp {
			_ConstSemiringRef semiring_;
			EpsPathCmp(_ConstSemiringRef semiring) :
				semiring_(semiring) {
			}
			bool operator()(const EpsPath &a, const EpsPath &b) const {
				if (a.first == b.first)
					return semiring_->compare(a.second, b.second) < 0;
				else
					return (a.first < b.first);
			}
		};

	public:
		RemoveEpsilons(_ConstAutomatonRef f) :
			Precursor(cache<_Automaton>(sort<_Automaton>(f,
					Fsa::SortTypeByInputAndTarget))) {
			if (f->type() != Fsa::TypeAcceptor)
				std::cerr
						<< "epsilon-removal does only work for acceptors at the moment"
						<< std::endl;
		}

		virtual void modifyState(_State *sp) const {
			// save all epsilon arc path weights
			EpsPathCmp epsPathCmp(this->semiring());
			std::set<EpsPath, EpsPathCmp> closure(epsPathCmp);
			StatePotentials<_Weight> distances;
			bool distancesCalculated = false;
			Fsa::Stack<Fsa::StateId> S;
			S.push_back(sp->id());
			Core::Vector<u8> done;
			done.grow(sp->id(), 0);
			done[sp->id()] = 1;
			while (!S.empty()) {
				Fsa::StateId s = S.pop();
				_ConstStateRef _sp = Precursor::fsa_->getState(s);
				for (typename _State::const_iterator a = _sp->begin(); a
						!= _sp->end(); ++a) {
					done.grow(a->target(), 0);
					if ((a->input() == Fsa::Epsilon) && (!done[a->target()])) {
						if (!distancesCalculated) {
							distances = sssp(Precursor::fsa_, sp->id(),
									IsInputLabel<_Automaton>(Fsa::Epsilon));
							distancesCalculated = true;
						}
						done[a->target()] = 1;
						closure.insert(EpsPath(a->target(),
								distances[a->target()]));
						S.push(a->target());
					}
				}
			}
			// remove epsilon arcs
			sp->remove(IsInputLabel<_Automaton>(Fsa::Epsilon));
			// project epsilon arc weight to target states
			if (!Precursor::fsa_->hasProperty(Fsa::PropertySortedByInput))
				sp->sort(byArc<_Automaton>(this->semiring()));
			for (typename std::set<EpsPath, EpsPathCmp>::const_iterator qw =
					closure.begin(); qw != closure.end(); ++qw) {
				_ConstStateRef _sp = Precursor::fsa_->getState(qw->first);
				for (typename _State::const_iterator a = _sp->begin(); a
						!= _sp->end(); ++a) {
					if (a->input() != Fsa::Epsilon) {
						_Arc tmp = *a;
						tmp.weight_ = Precursor::fsa_->semiring()->extend(qw->second, a->weight_);
						typename _State::iterator pos = sp->lower_bound(tmp,
								byArc<_Automaton>(this->semiring()));
						if ((pos == sp->end()) || (pos->input() != a->input())
								|| (pos->target() != a->target()))
							sp->insert(pos, tmp);
						else
							pos->weight_ = Precursor::fsa_->semiring()->collect(pos->weight_, tmp.weight_);
					}
				}
				if (_sp->isFinal()) {
					_Weight w = Precursor::fsa_->semiring()->extend(qw->second, _sp->weight_);
					if (sp->isFinal())
						sp->weight_ = Precursor::fsa_->semiring()->collect(sp->weight_, w);
					else
						sp->setFinal(w);
				}
			}
		}
		virtual std::string describe() const {
			return "removeEpsilons(" + Precursor::fsa_->describe() + ")";
		}
	};

	template<class _Automaton> Core::Ref<const _Automaton> removeEpsilons(
			Core::Ref<const _Automaton> f) {
		return Core::Ref<const _Automaton>(new RemoveEpsilons<_Automaton>(f));
	}

	/**
	 * remove "simple" epsilon arcs in a transducer
	 */
	template<class _Automaton> class SimpleRemoveEpsilonsAutomaton :
			public ModifyAutomaton<_Automaton> {
	private:
		typedef ModifyAutomaton<_Automaton> Precursor;

	public:
		typedef typename _Automaton::Arc Arc;
		typedef typename _Automaton::State State;
		typedef typename _Automaton::Semiring Semiring;
		typedef Core::Ref<const State> ConstStateRef;
		typedef Core::Ref<const _Automaton> ConstAutomatonRef;
		typedef Core::Ref<const Semiring> ConstSemiringRef;

	private:
		class IsEpsilonArc {
		private:
			ConstSemiringRef semiring_;

		public:
			IsEpsilonArc(ConstSemiringRef semiring) :
				semiring_(semiring) {
			}
			bool operator()(const Arc &a) const {
				return (a.input() == Fsa::Epsilon) && (a.output()
						== Fsa::Epsilon) && (semiring_->compare(a.weight(),
						semiring_->one()) == 0);
			}
		};
		ConstSemiringRef semiring_;
		Core::Vector<u32> inDegree_;

	public:
		SimpleRemoveEpsilonsAutomaton(ConstAutomatonRef f) :
			ModifyAutomaton<_Automaton>(f) {
			semiring_ = f->semiring();
			inDegree_ = inDegree<_Automaton>(f, false);
		}
		virtual void modifyState(State *sp) const {
			// skip epsilon arcs to non-final states with in-degree = out-degree = 1
			for (typename State::iterator a = sp->begin(); a != sp->end(); ++a)
				if (inDegree_[a->target()] == 1) {
					ConstStateRef _sp = Precursor::fsa_->getState(a->target());
					if ((!_sp->isFinal()) && (_sp->nArcs() == 1)) {
						typename State::const_iterator _a = _sp->begin();
						if ((_a->input() == Fsa::Epsilon) && (_a->output()
								== Fsa::Epsilon)) {
							a->target_ = _a->target_;
							a->weight_ = semiring_->extend(a->weight(),
									_a->weight());
						}
					}
				}

			std::vector<Fsa::StateId> mergeStates;
			Fsa::Stack<Fsa::StateId> S;
			S.push(sp->id());
			while (!S.empty()) {
				Fsa::StateId s = S.pop();
				ConstStateRef _sp = Precursor::fsa_->getState(s);
				for (typename State::const_iterator a = _sp->begin(); a
						!= _sp->end(); ++a)
					if ((a->input() == Fsa::Epsilon) && (a->output()
							== Fsa::Epsilon) && (Precursor::fsa_->semiring()->compare(a->weight(), Precursor::fsa_->semiring()->one()) == 0)) {
						mergeStates.push_back(a->target());
						S.push_back(a->target());
					}
			}
			sp->remove(IsEpsilonArc(Precursor::fsa_->semiring()));
			for (std::vector<Fsa::StateId>::const_iterator i =
					mergeStates.begin(); i != mergeStates.end(); ++i) {
				ConstStateRef _sp = Precursor::fsa_->getState(*i);
				for (typename State::const_iterator a = _sp->begin(); a
						!= _sp->end(); ++a)
					if ((a->input() != Fsa::Epsilon) || (a->output()
							!= Fsa::Epsilon) || (Precursor::fsa_->semiring()->compare(a->weight(), Precursor::fsa_->semiring()->one()) != 0))
						*sp->newArc() = *a;
			}
		}
		virtual std::string describe() const {
			return "simpleRemoveEpsilons(" + Precursor::fsa_->describe() + ")";
		}
	};

	template<class _Automaton> Core::Ref<const _Automaton> removeSimpleEpsilonArcs(
			Core::Ref<const _Automaton> f) {
		return Core::Ref<const _Automaton>(new SimpleRemoveEpsilonsAutomaton<_Automaton>(f));
	}

} // namespace Ftl

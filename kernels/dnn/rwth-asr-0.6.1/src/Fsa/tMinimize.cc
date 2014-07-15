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

#include "tCache.hh"
#include "tDeterminize.hh"
#include "tMinimize.hh"
#include "tRational.hh"
#include "tSort.hh"
#include "tSssp.hh"
#include "Stack.hh"


namespace Ftl {

	template<class A> class StateMappedAutomaton : public SlaveAutomaton<A> {
		typedef SlaveAutomaton<A> Precursor;
public:
		typedef typename A::State _State;
		typedef typename A::ConstStateRef _ConstStateRef;
		typedef typename A::ConstRef _ConstAutomatonRef;

protected:
		Fsa::StateMap map_;

		StateMappedAutomaton() {
		}
		StateMappedAutomaton(_ConstAutomatonRef f) :
			Precursor(f) {
		}
		StateMappedAutomaton(_ConstAutomatonRef f, const Fsa::StateMap &map) :
			Precursor(f), map_(map) {
		}

public:
		virtual Fsa::StateId initialStateId() const {
			Fsa::StateId s = Precursor::fsa_->initialStateId();
			if ((s == Fsa::InvalidStateId) || (s >= map_.size()))
				return Fsa::InvalidStateId;
			return map_[s] & Fsa::StateIdMask;
		}
		virtual _ConstStateRef getState(Fsa::StateId s) const {
			_State *sp = new _State(*Precursor::fsa_->getState(s));
			for (typename _State::iterator a = sp->begin(), b = sp->begin(); a
					!= sp->end(); ++a)
				if (a->target() < map_.size()) {
					a->target_ = map_[a->target()] & Fsa::StateIdMask;
					*(b++) = *a;
				}
			return _ConstStateRef(sp);
		}
		virtual size_t getMemoryUsed() const {
			return Precursor::fsa_->getMemoryUsed() + map_.getMemoryUsed();
		}
		virtual std::string describe() const {
			return "state-mapped(" + Precursor::fsa_->describe() + ")";
		}
	};

	/*
	 * Regular equivalence set construction for automata. Treats all
	 * automata as unweighted acceptors and compares arcs as a whole.
	 * Slight modification for weights: states need to be compared
	 * using their potential.
	 */
	template<class A> class MinimizeAutomaton : public StateMappedAutomaton<A> {
		typedef StateMappedAutomaton<A> Precursor;
	public:
		typedef typename A::State _State;
		typedef typename A::Weight _Weight;
		typedef typename A::ConstStateRef _ConstStateRef;
		typedef typename A::ConstRef _ConstAutomatonRef;

	private:
		typedef s32 ClassId;
		struct ClassEntry {
			Fsa::StateId id_;
			ClassId class_;
			ClassEntry(Fsa::StateId id, ClassId c) :
				id_(id), class_(c) {
			}
			bool operator<(const ClassEntry &e) const {
				return class_ < e.class_;
			}
		};

		typedef std::vector<ClassId> ClassMap;
		class HashTargetMappedAndLabels {
		private:
			bool transducer_;
			_ConstAutomatonRef fsa_;
			const ClassMap &classMap_;
			typename A::ConstSemiringRef semiring_;
			const StatePotentials<_Weight> &statePotentials_;
		public:
			HashTargetMappedAndLabels(_ConstAutomatonRef f,
					const ClassMap &classMap,
					const StatePotentials<_Weight> &statePotentials) :
				transducer_(f->type() == Fsa::TypeTransducer), fsa_(f),
						classMap_(classMap), semiring_(f->semiring()),
						statePotentials_(statePotentials) {}
			size_t operator()(Fsa::StateId s) const {
				const _ConstStateRef sp = fsa_->getState(s);
				size_t key = 100003 * semiring_->hash(statePotentials_[s])
						+ sp->nArcs();
				for (typename A::State::const_iterator a = sp->begin();
						a != sp->end(); ++a) {
					key = 100003 * key + classMap_[a->target()];
					key = 100003 * key + a->input();
					if (transducer_)
						key = 100003 * key + a->output();
				}
				return key;
			}
		};
		class EqualTargetMappedAndLabels {
		private:
			_ConstAutomatonRef fsa_;
			const ClassMap &classMap_;
			typename A::ConstSemiringRef semiring_;
			const StatePotentials<_Weight> &statePotentials_;
		public:
			EqualTargetMappedAndLabels(_ConstAutomatonRef f,
					const ClassMap &classMap,
					const StatePotentials<_Weight> &statePotentials) :
				fsa_(f), classMap_(classMap), semiring_(f->semiring()),
						statePotentials_(statePotentials) {
			}
			bool operator()(Fsa::StateId as, Fsa::StateId bs) const {
				if (semiring_->compare(statePotentials_[as],
						statePotentials_[bs]) != 0)
					return false;
				const _ConstStateRef a = fsa_->getState(as), b = fsa_->getState(bs);
				if (a->nArcs() != b->nArcs())
					return false;
				for (typename A::State::const_iterator aa = a->begin(), ba = b->begin();
						aa != a->end(); ++aa, ++ba)
					if ((classMap_[aa->target()] != classMap_[ba->target()]) || (aa->input() != ba->input())
							|| (aa->output() != ba->output())
							|| (semiring_->compare(aa->weight(), ba->weight())
									!= 0))
						return false;
				return true;
			}
		};
		typedef std::vector<ClassEntry> EquivalenceSet;

		_ConstAutomatonRef minimize(_ConstAutomatonRef f,
				Fsa::OptimizationHint hint, bool progress) {
			// 1. get state potentials
			StatePotentials<_Weight> statePotentials = ssspBackward<A>(f,
					progress);
			Fsa::StateId maxStateId = statePotentials.size() - 1;

			// 2. prepare automaton
			bool transducer = (f->type() == Fsa::TypeTransducer);
			if (transducer)
				f = sort<A>(f, Fsa::SortTypeByInputAndOutput);
			else
				f = sort<A>(f, Fsa::SortTypeByInput);
			f = cache<A>(f);

			// 3. maintain two tables of fixed size:
			//    - the equivalence set: all active states sorted by the class id (start with 2 classes: final/non-final)
			//    - a direct map of all states onto their class id
			EquivalenceSet es, nes;
			ClassMap classMap(maxStateId + 1, -1);
			Core::Vector<size_t> classSize(2, 0);
			for (size_t s = 0; s <= maxStateId; ++s) {
				typename A::ConstStateRef state = f->getState(s);
				classSize[classMap[s] = state->isFinal() ? 1 : 0]++;
				if (state)
					es.push_back(ClassEntry(s, classMap[s]));
			}
			sort(es.begin(), es.end());
			nes.reserve(es.size());

			// progress indicator for split loop
			Core::ProgressIndicator *p = 0;
			if (progress) {
				p = new Core::ProgressIndicator("splitting equiv. sets", "states");
				p->start();
			}

			// 4. split equivalence set
			// assume that allocated memory of classMap doesn't change
			bool didSplit = true;
			//ClassId nextSingletonId = -2;
			HashTargetMappedAndLabels hashTargetMappedAndLabels(f, classMap,
					statePotentials);
			EqualTargetMappedAndLabels equalTargetMappedAndLabels(f, classMap,
					statePotentials);
			while (didSplit) {
				// iterate over all classes (they are sorted by class id) and build new equivalence set
				nes.clear();
				didSplit = false;
				ClassId nClasses = 0;
				for (typename EquivalenceSet::const_iterator s = es.begin(); s
						!= es.end();) {
					ClassId c = s->class_;
					if (classSize[c] == 1) { // singleton classes can't be split anymore
						//classMap[(s++)->id_] = -(nextSingletonId++);
						nes.push_back(ClassEntry((s++)->id_, nClasses++));
						if (p)
							p->notify(); // report progress
					} else {
						// The best size for the following hash table is of course a prime number, which may be costly to
						// calculate. The following is already an optimization over previous versions:
						//
						// 1. A sorted vector for each class where splitted classes get merged again. Performed badly, because
						//    either the sorted vector used ConstStateRefs which was inefficient memory-wise or state ids which
						//    was painfully slow in case of packed automata.
						//
						// 2. A static global hash table which performed badly, because class sizes continously shrink and
						//    clearing the big same sized hash table all the time becomes a big burden.
						size_t startOfNewSets = nes.size();
						Fsa::Hash<Fsa::StateId, HashTargetMappedAndLabels, EqualTargetMappedAndLabels>
								states(hashTargetMappedAndLabels,
										equalTargetMappedAndLabels,
										classSize[c]);
						for (; (s != es.end()) && (s->class_ == c); ++s) {
							nes.push_back(ClassEntry(s->id_, nClasses
									+ states.insert(s->id_)));
							if (p)
								p->notify(); // report progress
						}
						if (states.size() > 1) {
							// just another trick:
							// 1. we only need to sort sets if we know there was more than 1 class
							// 2. overall complexity of this is smaller: sum(N_i)*log(N_i) <= sum(N_i)*log(sum(N_i))
							// 3. sorting works more local thus better for CPU cache
							std::sort(nes.begin() + startOfNewSets, nes.end());
							didSplit = true;
						}
						nClasses += states.size();
					}
				}

				// swap equivalence sets and update class map & sizes
				std::swap(es, nes);
				Core::Vector<size_t> newClassSize(nClasses, 0);
				for (typename EquivalenceSet::const_iterator s = es.begin(); s
						!= es.end(); ++s) {
					classMap[s->id_] = s->class_;
					newClassSize[s->class_]++;
				}
				std::swap(classSize, newClassSize);
			}
			if (p) {
				p->finish();
				delete p;
			}

			// 5. convert equivalence set into state id map
			Precursor::map_.resize(maxStateId + 1);
			for (size_t s = 0; s <= maxStateId; ++s)
				Precursor::map_[s] = s;
			for (typename EquivalenceSet::const_iterator s = es.begin(); s
					!= es.end();) {
				ClassId c = s->class_;
				Fsa::StateId rep = s->id_;
				for (++s; (s != es.end()) && (s->class_ == c); ++s)
					Precursor::map_[s->id_] = rep;
			}

			return f;
		}

	public:
		MinimizeAutomaton(_ConstAutomatonRef f, Fsa::OptimizationHint hint,
				bool progress) {
			Precursor::setFsa(minimize(f, hint, progress));
		}
		virtual std::string describe() const {
			return "minimize(" + Precursor::fsa_->describe() + ")";
		}
	};

	template<class A> typename A::ConstRef minimize(typename A::ConstRef f,
			bool progress) {
		return typename A::ConstRef(new MinimizeAutomaton<A>(f, Fsa::OptimizationHintSpeed, progress));
	}
	template<class A> typename A::ConstRef minimize(typename A::ConstRef f,
			Fsa::OptimizationHint hint, bool progress) {
		return typename A::ConstRef(new MinimizeAutomaton<A>(f, hint, progress));
	}
	template<class A> typename A::ConstRef minimizeSimple(typename A::ConstRef f) {
	    return Ftl::determinize<A>(Ftl::transpose<A>(Ftl::determinize<A>(Ftl::transpose<A>(f))));
	}

} // namespace Ftl

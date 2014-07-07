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
/*
 * tSssp4SpecialSymbols.cc
 *
 *  Created on: Mar 2, 2011
 *      Author: lehnen
 */

#include "Stack.hh"
#include "tSort.hh"
#include <list>
#include "tDfs.hh"
#include <Core/Utility.hh>
#include "Semiring64.hh"
#include "Resources.hh"
#include "tOutput.hh"
#include <Core/Application.hh>

namespace Ftl {
	namespace sssp4SpecialSymbolsHelper {
		template<class _Automaton> bool stateHasFailArc(typename _Automaton::ConstStateRef state) {
			typedef typename _Automaton::State::const_reverse_iterator _ArcIterator;
			for (_ArcIterator a = state->rbegin(); a!=state->rend(); ++a) {
				if ((a->input() < Fsa::LastLabelId) and (a->output() < Fsa::LastLabelId))
					return false;
				if ((a->input() == Fsa::Failure) or (a->output() == Fsa::Failure))
					return true;
			}
			return false;
		}

		template <typename _fromWeight, typename _toWeight>
		_toWeight convertWeight(_fromWeight w) {
			return _toWeight((f32)w);
		}

		template <>
		Fsa::Weight convertWeight<Fsa::Weight, Fsa::Weight>(Fsa::Weight w) {
			return w;
		}

		template <typename _toSemiring>
		_toSemiring convertSemiring(Fsa::ConstSemiringRef semiring) {
			return semiring;
		}

		template <>
		Fsa::ConstSemiring64Ref convertSemiring<Fsa::ConstSemiring64Ref>(Fsa::ConstSemiringRef semiring) {
			if (semiring == Fsa::LogSemiring)
				return Fsa::LogSemiring64;
			else
				defect();
		}

		template<class _leftArc, class _rightArc>
		struct byInputAndOutput : public std::binary_function<_leftArc, _rightArc, bool> {
			static bool cmp(const _leftArc &a, const _rightArc &b) {
				if (a.input()  < b.input())  return true;
				if (a.input()  > b.input())  return false;
				return a.output() < b.output();
			}
		};

		template <typename _ConstSemiringRef> bool isTropicalSemiring(_ConstSemiringRef semiring) {
			return false;
		}

		template <> bool isTropicalSemiring<Fsa::ConstSemiringRef>(Fsa::ConstSemiringRef semiring) {
			return (semiring == Fsa::TropicalSemiring);
		}

		template <typename _Weight> std::string weight2str(const _Weight &w) {
			std::ostringstream buffer;
			buffer << (f32)w;
			return buffer.str();
		}

		template <> std::string weight2str<Fsa::Weight64>(const Fsa::Weight64 &w) {
			std::ostringstream buffer;
			buffer.precision(30);
			buffer << (f64)w;
			return buffer.str();
		}
	}

	/**
	 * A backward single source shortest distance for fsas with Failure transitions
	 *
	 * TODO:
	 * - check if sssp is correct for cycles in automaton
	 * Assumptions:
	 * - only one fail arc per state
	 * - fail states do not include more arcs than parents of fail states
	 */
	template<class _Automaton, class _Semiring=typename _Automaton::Semiring, class _Weight=typename _Automaton::Weight>
	class SsspBackward4SpecialSymbols : public DfsState<_Automaton> {
		typedef SsspBackward4SpecialSymbols<_Automaton, _Semiring, _Weight> _SsspBackward4SpecialSymbols;
		typedef DfsState<_Automaton> Precursor;
		typedef typename _Automaton::Arc _Arc;
		typedef typename _Automaton::State _State;
		typedef typename _Automaton::ConstStateRef _ConstStateRef;
		typedef typename _Automaton::Weight _AutomataWeight;
		typedef typename _Semiring::ConstRef _ConstSemiringRef;
		typedef typename _Semiring::Accumulator _Accumulator;
		typedef StatePotentials<_Weight> _StatePotentials;
		typedef std::list<const _Arc*> DirectHits;
		typedef byInputAndOutput<_Automaton> Sorter;
		Sorter sorter_;

		Fsa::Stack<Fsa::StateId> processedStates_;
		_StatePotentials potential_, failPotential_;
		_ConstSemiringRef semiring_;

		public:
		SsspBackward4SpecialSymbols(Fsa::ConstAutomatonRef f) :
			Precursor(sort<_Automaton>(f, Fsa::SortTypeByInputAndOutput)),
			semiring_(sssp4SpecialSymbolsHelper::convertSemiring<_ConstSemiringRef>(f->semiring()))
			{
			if(not f->hasProperty(Fsa::PropertyAcyclic))
				Core::Application::us()->warning("SsspBackward4SpecialSymbols: automaton do not have the property 'acyclic'");
			this->dfs();
		}

	    const Fsa::Stack<Fsa::StateId>& getProcessedStates() const { return processedStates_; }
	    inline _Weight getPotential(Fsa::StateId s) const { return potential_[s]; }
	    inline _Weight getFailPotential(Fsa::StateId s) const { return failPotential_[s]; }
	    bool empty() const { return potential_.empty(); }
	    _Weight totalFlow() const {
		if (this->empty() or this->fsa_->initialStateId()==Fsa::InvalidStateId)
			return semiring_->zero();
		return potential_[this->fsa_->initialStateId()];
	}

		private:
		struct AddArc {
			inline static void regularArcWithoutDirectHits(const _Arc *arc, _Accumulator *collector, _SsspBackward4SpecialSymbols *owner) {
				Fsa::StateId ts = arc->target();
				collector->feed(owner->semiring_->extend(sssp4SpecialSymbolsHelper::convertWeight<_AutomataWeight,_Weight>(arc->weight()), owner->potential_[ts]));
			}

			inline static void regularArcWithDirectHits(const _Arc *arc, DirectHits &directHits, typename DirectHits::iterator &darc, _Accumulator *collector, _SsspBackward4SpecialSymbols *owner) {
				// while: darc->output < farc->output
				while ((darc != directHits.end()) and owner->sorter_(*(*darc), *arc)) {
					++darc;
				}
				// now: darc->output >= farc->output

				// if: darc->output > farc->output
				// <=>: directHits has _not_ arc
				if ((darc == directHits.end()) or owner->sorter_(*arc, *(*darc))) {
					darc = directHits.insert(darc, arc);
					regularArcWithoutDirectHits(&(*arc), collector, owner);
				}
			}

			inline static _Weight failArc(const _Arc *arc, DirectHits &directHits, _Accumulator *collector, const bool firstLevel, _SsspBackward4SpecialSymbols *owner){
				Fsa::StateId ts = arc->target();
				_Weight failWeight, update;
				failWeight=owner->processState<AddArc>(ts, directHits);
				update=owner->semiring_->extend(
						sssp4SpecialSymbolsHelper::convertWeight<_AutomataWeight,_Weight>(arc->weight()),
						failWeight);
				collector->feed(update);
				return failWeight;
			}
		};

		// as the trick does not work, this struct is useless and equal to AddArc...
		struct AddArcTropical : AddArc {
			inline static _Weight failArc(const _Arc *arc, DirectHits &directHits, _Accumulator *collector, const bool firstLevel, _SsspBackward4SpecialSymbols *owner){
				Fsa::StateId ts = arc->target();
				_Weight failWeight, update;
					failWeight=owner->processState<AddArcTropical>(ts, directHits);
					update=owner->semiring_->extend(
							sssp4SpecialSymbolsHelper::convertWeight<_AutomataWeight,_Weight>(arc->weight()),
							failWeight);

					collector->feed(update);

				return failWeight;
			}
		};

		struct AddArcNegCollect : AddArc {
			inline static void regularArcWithDirectHits(const _Arc *arc, DirectHits &directHits, typename DirectHits::iterator &darc, _Accumulator *collector, _SsspBackward4SpecialSymbols *owner) {
				// while: darc->output < farc->output
				while ((darc != directHits.end()) and owner->sorter_(*(*darc), *arc)) {
					++darc;
				}
				// now: darc->output >= farc->output

				// directHits has arc
				if ((darc != directHits.end()) and (not owner->sorter_(*arc, *(*darc)))) {
					AddArc::regularArcWithoutDirectHits(&(*arc), collector, owner);
				}
			}

			inline static _Weight failArc(const _Arc *arc, DirectHits &directHits, _Accumulator *collector, const bool firstLevel, _SsspBackward4SpecialSymbols *owner){
				Fsa::StateId ts = arc->target();
				_Weight failWeight;
				_Weight allFailWeights=owner->potential_[ts];
				failWeight=owner->semiring_->invCollect(allFailWeights, owner->processState<AddArcNegCollect>(ts, directHits));
				if (firstLevel) {
					_Weight update=owner->semiring_->extend(
							sssp4SpecialSymbolsHelper::convertWeight<_AutomataWeight,_Weight>(arc->weight()),
							failWeight);
					collector->feed(update);
				}

				return failWeight;
			}
		};

	    template <class _AddArc> _Weight processState(const Fsa::StateId s, DirectHits &directHits) {
		_Accumulator *collector = semiring_->getCollector(semiring_->zero());
		_ConstStateRef state = this->fsa_->getState(s);
		bool oldDirectHitsEmpty=directHits.empty();

		if (not sssp4SpecialSymbolsHelper::stateHasFailArc<_Automaton>(state))
			if (oldDirectHitsEmpty)
				for (typename _State::const_iterator arc = state->begin(); arc != state->end(); ++arc) {
					_AddArc::regularArcWithoutDirectHits(&(*arc), collector, this);
				}
			else {
				typename DirectHits::iterator darc = directHits.begin();
				for (typename _State::const_iterator arc = state->begin(); arc != state->end(); ++arc)
					_AddArc::regularArcWithDirectHits(&(*arc), directHits, darc, collector, this);
			}
		else {
			bool noNoneFailArc=true;
			typename DirectHits::iterator darc = directHits.begin();

			for (typename _State::const_iterator arc = state->begin(); arc != state->end(); ++arc) {
				if (noNoneFailArc or not (arc->output() == Fsa::Failure or arc->input() == Fsa::Failure)) {
					noNoneFailArc=false;

					if (oldDirectHitsEmpty) {
						_AddArc::regularArcWithoutDirectHits(&(*arc), collector, this);
						directHits.push_back(&(*arc));
					} else {
						_AddArc::regularArcWithDirectHits(&(*arc), directHits, darc, collector, this);
					}
				} else {
					verify(arc->output() == arc->input()); // assume that we only have arcs with fail on both sides
					_Weight failWeight=_AddArc::failArc(&(*arc), directHits, collector, oldDirectHitsEmpty, this);
					//std::cout << "processState(" << s << ") oldDirectHitsEmpty=" << oldDirectHitsEmpty << " failWeight=" << (f32)failWeight << std::endl;
					// only first processState in recursion update fail potential
					if (oldDirectHitsEmpty)
						failPotential_.set(s, failWeight);
				} // end else Failure
			} // end for arc
		}

		if (state->isFinal()) {
			collector->feed(semiring_->extend(sssp4SpecialSymbolsHelper::convertWeight<_AutomataWeight,_Weight>(state->weight()), semiring_->one()));
		}

		_Weight result = collector->get();
		delete collector;
		return result;
	    }

		protected:
	    void finishState(Fsa::StateId s) {
		processedStates_.push(s);
		DirectHits directHits;
		_Weight potential;
		if (sssp4SpecialSymbolsHelper::isTropicalSemiring<_ConstSemiringRef>(semiring_))
			potential=processState<AddArcTropical>(s, directHits);
		else if (semiring_->hasInvCollect())
			potential=processState<AddArcNegCollect>(s, directHits);
		else
			potential=processState<AddArc>(s, directHits);
		//std::cout << "finishState(" << s << ") potential=" << sssp64Helper::weight2str(potential) << std::endl;
		potential_.set(s, potential);
	    }
	};

	template<class _Automaton, class _Semiring=typename _Automaton::Semiring, class _Weight=typename _Automaton::Weight>
	class SsspForward4SpecialSymbols {
		typedef SsspForward4SpecialSymbols<_Automaton, _Semiring, _Weight> _SsspForward4SpecialSymbols;
		typedef typename _Automaton::ConstRef _ConstAutomatonRef;
		typedef typename _Automaton::State _State;
		typedef typename _Automaton::ConstStateRef _ConstStateRef;
		typedef typename _Automaton::Arc _Arc;
		typedef typename _Automaton::Weight _AutomataWeight;
		typedef typename _Semiring::ConstRef _ConstSemiringRef;
		typedef typename _Semiring::Accumulator _Accumulator;
		typedef std::list<const _Arc*> DirectHits;
		typedef byInputAndOutput<_Automaton> Sorter;
		typedef Fsa::Stack<Fsa::StateId> _Stack;
		public:
		typedef _Weight Weight;

		private:
		Sorter sorter_;
		_ConstAutomatonRef fsa_;
		_ConstSemiringRef semiring_;
		_Weight totalFlow_;

		public:
		class ForwardPotential {
			private:
			_ConstSemiringRef semiring_;
			_Weight forwardPotential_;
			_Accumulator *forwardAccumulator_;
			typedef Arc<_Weight> _FPArc;
			typedef sssp4SpecialSymbolsHelper::byInputAndOutput<_Arc, _FPArc> RSorter;
			typedef sssp4SpecialSymbolsHelper::byInputAndOutput<_FPArc, _Arc> LSorter;
			typedef typename std::pair<_FPArc,_Accumulator*> _ArcAccumulatorPair;
			typedef typename std::list<_ArcAccumulatorPair> _ForwardAccumulators;
			_ForwardAccumulators *forwardAccumulators_;
			mutable typename _ForwardAccumulators::iterator forwardAccumulatorsPos_;

			private:
			ForwardPotential() { defect(); }

			public:
			ForwardPotential(_ConstSemiringRef semiring) :
				semiring_(semiring),
				forwardPotential_(semiring_->zero()),
				forwardAccumulator_(NULL),
				forwardAccumulators_(NULL)
				{
				}

			ForwardPotential(_ConstSemiringRef semiring, _Weight weight) :
				semiring_(semiring),
				forwardPotential_(weight),
				forwardAccumulator_(NULL),
				forwardAccumulators_(NULL)
				{
				}

			/**
			* copy constructor, forwardAccumulatorsPos_ is not copied,
			* Warning: only the pointers of the accumulators are copied
			*/
			ForwardPotential(const ForwardPotential &fp) :
				semiring_(fp.semiring_),
				forwardPotential_(fp.forwardPotential_),
				forwardAccumulator_(NULL),
				forwardAccumulators_(NULL)
				{
				require(fp.forwardAccumulator_ == NULL);
				require(fp.forwardAccumulators_ == NULL);
				}

			~ForwardPotential() {
				delete forwardAccumulator_;
				if (forwardAccumulators_ != NULL) {
					for (forwardAccumulatorsPos_=forwardAccumulators_->begin(); forwardAccumulatorsPos_!=forwardAccumulators_->end(); ++forwardAccumulatorsPos_) {
						delete forwardAccumulatorsPos_->second;
					}
					delete forwardAccumulators_;
				}
			}

			private:

			/**
			* move forwardAccumulatorsPos_ to given arc
			* if arc is not found forwardAccumulatorsPos_ is
			* at position where arc should be included
			* return: true if arc was found
			*/
			bool walk2(const _Arc &arc, const Sorter &sorter) const {
				if (RSorter::cmp(arc, forwardAccumulators_->front().first)) {
					forwardAccumulatorsPos_=forwardAccumulators_->begin();
					return false;
				}

				if (LSorter::cmp(forwardAccumulators_->back().first, arc)) {
					forwardAccumulatorsPos_=forwardAccumulators_->end();
					return false;
				}

				if (forwardAccumulatorsPos_ == forwardAccumulators_->end())
					--forwardAccumulatorsPos_;

				// while forwardPotentialsPos > arc
				while ((forwardAccumulatorsPos_ != forwardAccumulators_->begin()) and (RSorter::cmp(arc, forwardAccumulatorsPos_->first))) {
					--forwardAccumulatorsPos_;
				}
				// while forwardPotentialsPos < arc
				while (LSorter::cmp(forwardAccumulatorsPos_->first, arc)) {
					++forwardAccumulatorsPos_;
					if (forwardAccumulatorsPos_ == forwardAccumulators_->end()) {
						return false;
					}
				}
				return not (RSorter::cmp(arc, forwardAccumulatorsPos_->first));
			}
			public:

			void feedForwardPotential(const _Weight &w) {
				if (forwardAccumulator_ == NULL) {
					forwardAccumulator_ = semiring_->getCollector(forwardPotential_);
				}
				forwardAccumulator_->feed(w);
			}

			void feedForwardPotential(const _Arc &arc, const Sorter &sorter, const _Weight &w) {
				if (forwardAccumulators_==NULL) {
					forwardAccumulators_ = new _ForwardAccumulators();
					forwardAccumulatorsPos_ = forwardAccumulators_->begin();
					_FPArc forwardArc(arc.target(), semiring_->zero(), arc.input(), arc.output());
					_ArcAccumulatorPair pair(forwardArc, semiring_->getCollector(w));
					forwardAccumulatorsPos_ = forwardAccumulators_->insert(forwardAccumulatorsPos_, pair);
				} else {
					if (walk2(arc, sorter)) {
						// forwardPotentials_ has arc
						_ArcAccumulatorPair &pair=*forwardAccumulatorsPos_;
						verify(pair.first.output() == arc.output());
						if (pair.second == NULL)
							pair.second = semiring_->getCollector(pair.first.weight());
						pair.second->feed(w);
					} else {
						// no element arc in forwardPotentials_
						_FPArc forwardArc(arc.target(), semiring_->zero(), arc.input(), arc.output());
						_ArcAccumulatorPair pair(forwardArc, semiring_->getCollector(w));
						forwardAccumulatorsPos_ = forwardAccumulators_->insert(forwardAccumulatorsPos_, pair);
					}
				}
			}

			private:
			void saveForwardAccumulator() {
				if (forwardAccumulator_ != NULL) {
					forwardPotential_ = forwardAccumulator_->get();
					delete forwardAccumulator_;
					forwardAccumulator_ = NULL;
				}
			}

			public:
			void saveForwardAccumulators() {
				saveForwardAccumulator();
				if (forwardAccumulators_ != NULL) {
					for (forwardAccumulatorsPos_=forwardAccumulators_->begin(); forwardAccumulatorsPos_!=forwardAccumulators_->end(); ++forwardAccumulatorsPos_) {
						_ArcAccumulatorPair &pair=*forwardAccumulatorsPos_;
						if (pair.second != NULL) {
							pair.first.setWeight(pair.second->get());
							delete pair.second;
							pair.second = NULL;
						}
					}
				}
			}

			_Weight getForwardPotential(const _Arc &arc, const Sorter &sorter) const {
				require(forwardAccumulator_ == NULL);
				if ((forwardAccumulators_ != NULL) and (walk2(arc, sorter))) {
					require(forwardAccumulatorsPos_->first.output() == arc.output());
					require(forwardAccumulatorsPos_->second == NULL);
					if (not semiring_->hasInvCollect())
						return semiring_->collect(forwardPotential_, forwardAccumulatorsPos_->first.weight());
					else
						return semiring_->invCollect(forwardPotential_, forwardAccumulatorsPos_->first.weight());
				} else {
					return forwardPotential_;
				}
			}

			_Weight getForwardPotential() const {
				require(forwardAccumulator_ == NULL);
				return forwardPotential_;
			}
		};
		typedef Core::Vector<ForwardPotential*> _StatePotentialsForwardFail;

		private:
		_StatePotentialsForwardFail forwardPotentials_;

		inline ForwardPotential& getForwardPotential(const std::size_t &index) {
			forwardPotentials_.grow(index, NULL);
			ForwardPotential *&fp=forwardPotentials_[index];
			if (fp == NULL)
				fp = new ForwardPotential(semiring_);
			return *fp;
		}

		inline const ForwardPotential& getFinalizedForwardPotential(const std::size_t &index) {
			require(forwardPotentials_.size() > index);
			ForwardPotential *fp=forwardPotentials_[index];
			verify(fp != NULL);
			fp->saveForwardAccumulators();
			return *fp;
		}

		private:
		struct AddArc {
			inline static void regularArcWithoutDirectHits(const _Arc *arc, const _Weight &sourceForwardPotential, ForwardPotential &fpot, _SsspForward4SpecialSymbols *owner) {
				Fsa::StateId ts = arc->target();
				_Weight arcWeight = sssp4SpecialSymbolsHelper::convertWeight<_AutomataWeight,_Weight>(arc->weight());
				_Weight newWeight = owner->semiring_->extend(arcWeight, sourceForwardPotential);
				owner->getForwardPotential(ts).feedForwardPotential(newWeight);
			}

			/// we are at a fail state (at least one arc to this state is a fail arc)
			inline static void regularArcWithDirectHits(const _Arc *arc, DirectHits &directHits, typename DirectHits::iterator &darc, const _Weight &sourceForwardPotential, ForwardPotential &fpot, _SsspForward4SpecialSymbols *owner) {
				// while: darc->output < farc->output
				while ((darc != directHits.end()) and owner->sorter_(*(*darc), *arc))
					++darc;
				// now: darc->output >= farc->output

				// if: darc->output > farc->output
				// <=>: directHits has not arc
				if ((darc == directHits.end()) or owner->sorter_(*arc, *(*darc))) {
					regularArcWithoutDirectHits(arc, sourceForwardPotential, fpot, owner);
					darc = directHits.insert(darc, arc);
					/* note for this arc that it has gotten the source forward potential
					 * from the caller of processState()
					 */
					fpot.feedForwardPotential(*arc, owner->sorter_, sourceForwardPotential);
				}
			}

			inline static void failArc(const _Arc *arc, DirectHits &directHits, const _Weight &sourceForwardPotential, const bool firstLevel, _SsspForward4SpecialSymbols *owner) {
				Fsa::StateId ts = arc->target();
				owner->processState<AddArc>(owner->fsa_->getState(ts), directHits, owner->semiring_->extend(sssp4SpecialSymbolsHelper::convertWeight<_AutomataWeight,_Weight>(arc->weight()), sourceForwardPotential));
			}
		};

		struct AddArcNegCollect {
			inline static void regularArcWithoutDirectHits(const _Arc *arc, const _Weight &sourceForwardPotential, ForwardPotential &fpot, _SsspForward4SpecialSymbols *owner) {
				Fsa::StateId ts = arc->target();
				_Weight projection = owner->semiring_->extend(sssp4SpecialSymbolsHelper::convertWeight<_AutomataWeight,_Weight>(arc->weight()), fpot.getForwardPotential(*arc, owner->sorter_));
				owner->getForwardPotential(ts).feedForwardPotential(projection);
			}

			inline static void regularArcWithDirectHits(const _Arc *arc, DirectHits &directHits, typename DirectHits::iterator &darc, const _Weight &sourceForwardPotential, ForwardPotential &fpot, _SsspForward4SpecialSymbols *owner) {
				// while: darc->output < farc->output
				while ((darc != directHits.end()) and owner->sorter_(*(*darc), *arc))
					++darc;
				// now: darc->output >= farc->output

				// if: darc->output > farc->output
				// <=>: directHits has not arc
				if ((darc != directHits.end()) and (not owner->sorter_(*arc, *(*darc)))) {
					/* note for this arc that it has gotten the source forward potential
					 * from the caller of processState()
					 */
					fpot.feedForwardPotential(*arc, owner->sorter_, sourceForwardPotential);
				}
			}

			inline static void failArc(const _Arc *arc, DirectHits &directHits, const _Weight &sourceForwardPotential, const bool firstLevel, _SsspForward4SpecialSymbols *owner) {
				Fsa::StateId ts = arc->target();
				if (firstLevel) {
					_Weight totalFailWeight =  owner->semiring_->extend(sssp4SpecialSymbolsHelper::convertWeight<_AutomataWeight,_Weight>(arc->weight()), sourceForwardPotential);
					owner->getForwardPotential(ts).feedForwardPotential(totalFailWeight);
					owner->processState<AddArcNegCollect>(owner->fsa_->getState(ts), directHits, totalFailWeight);
				} // else stop recursion
			}
		};

		template <class _AddArc> void processState(_ConstStateRef state, DirectHits &directHits, _Weight sourceForwardPotential) {
			Fsa::StateId s = state->id();

			bool oldDirectHitsEmpty=directHits.empty();
			ForwardPotential &fpot = this->getForwardPotential(s);

			if (not sssp4SpecialSymbolsHelper::stateHasFailArc<_Automaton>(state))
				if (oldDirectHitsEmpty) {
					for (typename _State::const_iterator arc = state->begin(); arc != state->end(); ++arc)
						_AddArc::regularArcWithoutDirectHits(&(*arc), sourceForwardPotential, fpot, this);
				} else {
					typename DirectHits::iterator darc = directHits.begin();
					for (typename _State::const_iterator arc = state->begin(); arc != state->end(); ++arc)
						_AddArc::regularArcWithDirectHits(&(*arc), directHits, darc, sourceForwardPotential, fpot, this);
				}
			else { // state has fail arc
				bool noNoneFailArc=true;
				typename DirectHits::iterator darc = directHits.begin();

				for (typename _State::const_iterator arc = state->begin(); arc != state->end(); ++arc) {
					if (noNoneFailArc or not (arc->output() == Fsa::Failure or arc->input() == Fsa::Failure)) {
						noNoneFailArc=false;

						if (oldDirectHitsEmpty) {
							_AddArc::regularArcWithoutDirectHits(&(*arc), sourceForwardPotential, fpot, this);
							directHits.push_back(&(*arc));
						} else {
							_AddArc::regularArcWithDirectHits(&(*arc), directHits, darc, sourceForwardPotential, fpot, this);
						}
					} else {
						verify(arc->output() == arc->input()); // assume that we only have arcs with fail on both sides
						_AddArc::failArc(&(*arc), directHits, sourceForwardPotential, oldDirectHitsEmpty, this);
					} // end else Failure
				} // end arc
			}
		}

		void finishStates(const _Stack states) {
			Fsa::StateId initialStateId = this->fsa_->initialStateId();
			if (initialStateId == Fsa::InvalidStateId)
				return;
			_Accumulator *fwdFlowAcc = semiring_->getCollector();
			Fsa::StateId firstState = *(states.begin());
			require(firstState == initialStateId);

			this->getForwardPotential(initialStateId).feedForwardPotential(semiring_->one());

			for (_Stack::const_iterator s=states.begin(); s!=states.end(); ++s) {
				const ForwardPotential &fpot = this->getFinalizedForwardPotential(*s);
				DirectHits directHits;
				_ConstStateRef state = this->fsa_->getState(*s);
				_Weight fStatePot = fpot.getForwardPotential();
				if (semiring_->hasInvCollect())
					processState<AddArcNegCollect>(state, directHits, fStatePot);
				else
					processState<AddArc>(state, directHits, fStatePot);

				if (state->isFinal()) {
					//std::cout << "Forward: final state " << *s << " fStatePot=" << (f32)fStatePot << " state->weight()=" << (f32)state->weight() << std::endl;
					//std::cout << " before fwdFlowAcc->get()=" << (f32)fwdFlowAcc->get() << " feed(" << (f32)semiring_->extend(fStatePot, sssp4SpecialSymbolsHelper::convertWeight<_AutomataWeight,_Weight>(state->weight())) << ")" << std::endl;
					fwdFlowAcc->feed(semiring_->extend(fStatePot, sssp4SpecialSymbolsHelper::convertWeight<_AutomataWeight,_Weight>(state->weight())));
					//std::cout << " after fwdFlowAcc->get()=" << (f32)fwdFlowAcc->get() << std::endl;
				}
			}

			for (_Stack::const_iterator s=states.begin(); s!=states.end(); ++s) {
#if 1
				this->getFinalizedForwardPotential(*s);
#endif
			}

			totalFlow_ = fwdFlowAcc->get();
			delete fwdFlowAcc;
		}

		public:
		SsspForward4SpecialSymbols(_ConstAutomatonRef f, const _Stack states) :
			fsa_(sort<_Automaton>(f, Fsa::SortTypeByInputAndOutput)),
			semiring_(sssp4SpecialSymbolsHelper::convertSemiring<_ConstSemiringRef>(this->fsa_->semiring())),
			totalFlow_(semiring_->zero())
		{
			if(not f->hasProperty(Fsa::PropertyAcyclic))
				Core::Application::us()->warning("SsspForward4SpecialSymbols: automaton does not have the property 'acyclic'");
			finishStates(states);
		}

		~SsspForward4SpecialSymbols() {
			for (typename _StatePotentialsForwardFail::iterator p = forwardPotentials_.begin(); p!=forwardPotentials_.end(); ++p)
				delete *p;
		}

		_Weight totalFlow() const { return totalFlow_; }
		inline const ForwardPotential &forwardPotential(Fsa::StateId s) const { return *forwardPotentials_[s]; }
		bool empty() const { return forwardPotentials_.empty(); }
	};

	template<class _Automaton, class _SsspForward4SpecialSymbols=SsspForward4SpecialSymbols<_Automaton>, class _SsspBackward4SpecialSymbols=SsspBackward4SpecialSymbols<_Automaton> >
	class PosteriorAutomaton4SpecialSymbols : public ModifyAutomaton<_Automaton> {
		typedef ModifyAutomaton<_Automaton> Precursor;
		public:
		typedef typename _Automaton::ConstRef _ConstAutomatonRef;
		typedef typename _Automaton::Weight _Weight;
		typedef typename _Automaton::State _State;
		typedef typename _Automaton::ConstStateRef _ConstStateRef;
		typedef typename _Automaton::Arc _Arc;
		typedef typename _Automaton::Semiring _Semiring;
		typedef typename _Automaton::ConstSemiringRef _ConstSemiringRef;
		typedef typename _Automaton::Semiring::Accumulator _Accumulator;
		typedef typename _SsspForward4SpecialSymbols::ForwardPotential _ForwardPotential;
		typedef std::list<const _Arc*> DirectHits;
		typedef byInputAndOutput<_Automaton> Sorter;
		typedef typename _SsspForward4SpecialSymbols::Weight _SsspWeight;

		private:
		Sorter sorter_;
		_ConstSemiringRef semiring_;
		const _SsspBackward4SpecialSymbols ssspBackward4SpecialSymbols_;
		const _SsspForward4SpecialSymbols ssspForward4SpecialSymbols_;
		_Weight totalInv_;

		public:
		PosteriorAutomaton4SpecialSymbols(_ConstAutomatonRef f, s32 tol) :
			Precursor(sort<_Automaton>(f, Fsa::SortTypeByInputAndOutput)),
			semiring_(this->fsa_->semiring()),
			ssspBackward4SpecialSymbols_(this->fsa_),
			ssspForward4SpecialSymbols_(this->fsa_, ssspBackward4SpecialSymbols_.getProcessedStates()),
			totalInv_(this->semiring()->invert(sssp4SpecialSymbolsHelper::convertWeight<_SsspWeight,_Weight>(ssspBackward4SpecialSymbols_.totalFlow())))
			{
			if(not f->hasProperty(Fsa::PropertyAcyclic)) {
				Core::Application::us()->warning("PosteriorAutomaton4SpecialSymbols: automaton does not have the property 'acyclic'");
			}
			const _Weight fwdFlow = sssp4SpecialSymbolsHelper::convertWeight<_SsspWeight,_Weight>(ssspForward4SpecialSymbols_.totalFlow());
			const _Weight bwdFlow = sssp4SpecialSymbolsHelper::convertWeight<_SsspWeight,_Weight>(ssspBackward4SpecialSymbols_.totalFlow());
			if (!Core::isAlmostEqual((f32)fwdFlow, (f32)bwdFlow, tol)) {
				std::cerr << "forward-total-flow: " << (f32)fwdFlow
				<< " | backward-total-flow: " << (f32)bwdFlow << std::endl;
			}
		}

		PosteriorAutomaton4SpecialSymbols(_ConstAutomatonRef f) :
			Precursor(sort<_Automaton>(f, Fsa::SortTypeByInputAndOutput)),
			semiring_(this->fsa_->semiring()),
			ssspBackward4SpecialSymbols_(this->fsa_),
			ssspForward4SpecialSymbols_(this->fsa_, ssspBackward4SpecialSymbols_.getProcessedStates()),
			totalInv_(this->semiring()->invert(ssspBackward4SpecialSymbols_.getPotential(this->fsa_->initialStateId())))
			{
			if(not f->hasProperty(Fsa::PropertyAcyclic)) {
				Core::Application::us()->warning("PosteriorAutomaton4SpecialSymbols: automaton does not have the property 'acyclic'");
			}
		}

		const _Weight& totalInv() const {
			return this->totalInv_;
		}

		virtual void modifyState(_State *sp) const {
			Fsa::StateId s = sp->id();
			//std::cout << "State s=" << s << std::endl;
			bool noNoneFailArc=true;
			const _ForwardPotential &fpot(ssspForward4SpecialSymbols_.forwardPotential(s));

			for (typename _State::iterator a = sp->begin(); a != sp->end(); ++a) {
				Fsa::StateId ts = a->target();
				//std::cout << "Arc " << a->input() << ":" << a->output() << "->" << ts << "/" << (f32)a->weight() << ": ";
				_Weight forwardPot=fpot.getForwardPotential(*a, sorter_);
				_Weight backwardPot;
				if (noNoneFailArc or not (a->output() == Fsa::Failure or a->input() == Fsa::Failure)) {
					backwardPot=ssspBackward4SpecialSymbols_.getPotential(ts);
					noNoneFailArc=false;
				} else {
					verify(a->output() == a->input());
					backwardPot=ssspBackward4SpecialSymbols_.getFailPotential(s);
				}
				_Accumulator *extender(semiring_->getExtender(a->weight()));
				extender->feed(forwardPot);
				extender->feed(backwardPot);
				extender->feed(totalInv_);
				a->weight_ = extender->get();
				//std::cout << "forward: " << (f32)forwardPot << " backward: " << (f32)backwardPot << " totalInv: " << (f32)totalInv_ << " result: " << (f32)a->weight_ << std::endl;
				delete extender;
			}
		}

		virtual std::string describe() const {
			return "posterior4SpecialSymbols(" + Precursor::fsa_->describe() + ")";
		}
	};

	template<class _Automaton> typename _Automaton::ConstRef posterior4SpecialSymbols(typename _Automaton::ConstRef f) {
		return typename _Automaton::ConstRef(new PosteriorAutomaton4SpecialSymbols<_Automaton>(f));
	}

	template<class _Automaton> typename _Automaton::ConstRef posterior4SpecialSymbols(typename _Automaton::ConstRef f, typename _Automaton::Weight &totalInv, s32 tol) {
		typedef PosteriorAutomaton4SpecialSymbols<_Automaton> _PosteriorAutomaton4SpecialSymbols;
		_PosteriorAutomaton4SpecialSymbols *result = new PosteriorAutomaton4SpecialSymbols<_Automaton>(f, tol);
		totalInv = result->totalInv();

		return typename _Automaton::ConstRef(result);
	}

	template<class _Automaton, class _SsspSemiring, class _SsspWeight> typename _Automaton::ConstRef posterior4SpecialSymbols(typename _Automaton::ConstRef f, typename _Automaton::Weight &totalInv, s32 tol) {
		typedef SsspForward4SpecialSymbols<_Automaton, _SsspSemiring, _SsspWeight> _SsspForward4SpecialSymbols;
		typedef SsspBackward4SpecialSymbols<_Automaton, _SsspSemiring, _SsspWeight> _SsspBackward4SpecialSymbols;
		typedef PosteriorAutomaton4SpecialSymbols<_Automaton, _SsspForward4SpecialSymbols, _SsspBackward4SpecialSymbols> _PosteriorAutomaton4SpecialSymbols;
		_PosteriorAutomaton4SpecialSymbols *result = new _PosteriorAutomaton4SpecialSymbols(f, tol);
		totalInv = result->totalInv();

		return typename _Automaton::ConstRef(result);
	}

	/**
	 * A best automaton which is able to work with FAIL arcs
	 *
	 * In the resulting automaton the FAIL arcs have been removed
	 */
	template<class _Automaton>
	class BestAutomaton4SpecialSymbols : public SlaveAutomaton<_Automaton> {
		private:
		typedef SlaveAutomaton<_Automaton> Precursor;

		public:
		typedef typename _Automaton::Weight _Weight;
		typedef typename _Automaton::State _State;
		typedef typename _Automaton::Arc _Arc;
		typedef typename _Automaton::ConstStateRef _ConstStateRef;
		typedef typename _Automaton::ConstRef _ConstAutomatonRef;
		typedef std::list<const _Arc*> DirectHits;
		typedef byInputAndOutput<_Automaton> Sorter;

		private:
		const SsspBackward4SpecialSymbols<_Automaton> ssspBackward4SpecialSymbols_;
		Sorter sorter_;

		public:
		BestAutomaton4SpecialSymbols(_ConstAutomatonRef f) :
			Precursor(sort<_Automaton>(f, Fsa::SortTypeByInputAndOutput)),
			ssspBackward4SpecialSymbols_(this->fsa_)
			{
			this->setProperties(Fsa::PropertyStorage | Fsa::PropertyCached, Fsa::PropertyNone);
			this->addProperties(Fsa::PropertySorted);
			this->addProperties(Fsa::PropertyLinear | Fsa::PropertyAcyclic);
			}

		virtual Fsa::StateId initialStateId() const {
			if (ssspBackward4SpecialSymbols_.empty())
				return Fsa::InvalidStateId;
			else
				return Precursor::fsa_->initialStateId();
		}

		private:
		bool directHistsHasArc(typename _State::const_iterator &arc, DirectHits &directHits, typename DirectHits::iterator &darc) const {
			// while: darc->output < farc->output
			while ((darc != directHits.end()) and sorter_(*(*darc), *arc)) {
				++darc;
			}
			// now: darc->output >= farc->output

			// if: darc->output > farc->output
			// <=>: directHits has not arc
			return not ((darc == directHits.end()) or sorter_(*arc, *(*darc)));
		}

		bool getBestArc(_Arc &bestArc, Fsa::StateId s, Fsa::Weight &failWeight, DirectHits &directHits) const {
			_ConstStateRef _sp = Precursor::fsa_->getState(s);
			bool bestArcFound = false;
			_Weight minWeight = Precursor::fsa_->semiring()->max();
			bool oldDirectHitsEmpty=directHits.empty();
			bool hasFailArc=sssp4SpecialSymbolsHelper::stateHasFailArc<_Automaton>(_sp);
			typename DirectHits::iterator darc = directHits.begin();
			bool noNoneFailArc=true;

			for (typename _State::const_iterator a = _sp->begin(); a != _sp->end(); ++a) {
				Fsa::StateId ts = a->target();
				if (ts == s) continue;
				if (a->output() == Fsa::Failure or a->input() == Fsa::Failure) {
					verify(a->output() == a->input()); // assume that we only have arcs with fail on both sides
					_Weight potential;
					if (noNoneFailArc)
						potential = ssspBackward4SpecialSymbols_.getPotential(ts);
					else
						potential = ssspBackward4SpecialSymbols_.getFailPotential(s);
					_Weight w = Precursor::fsa_->semiring()->extend(a->weight(), potential);
					if (Precursor::fsa_->semiring()->compare(w, minWeight) < 0) {
						minWeight = w;
						Fsa::Weight additionalFailWeight = Precursor::fsa_->semiring()->one();
						bool result = getBestArc(bestArc, ts, additionalFailWeight, directHits);
						failWeight = Precursor::fsa_->semiring()->extend(a->weight(), additionalFailWeight);
						return result;
					}
				} else {
					noNoneFailArc=false;
					if (not oldDirectHitsEmpty) {
						if (directHistsHasArc(a, directHits, darc))
							continue;
						else
							darc = directHits.insert(darc, &(*a));
					}
					if (hasFailArc) directHits.push_back(&(*a));
					_Weight w = Precursor::fsa_->semiring()->extend(a->weight(), ssspBackward4SpecialSymbols_.getPotential(ts));
					if (Precursor::fsa_->semiring()->compare(w, minWeight) < 0) {
						minWeight = w;
						bestArc = *a;
						bestArcFound = true;
					}
				}
			}



			if (_sp->isFinal()) {
				if (Precursor::fsa_->semiring()->compare(_sp->weight_, minWeight) < 0) {
					minWeight = _sp->weight_;
					bestArcFound = false;
				}
			}

			return bestArcFound;
		}

		public:
		/**
		* \warning best4SpecialSymbols() does not work when there are non-trivial zero-weight loops.
		*/
		virtual _ConstStateRef getState(Fsa::StateId s) const {
			_ConstStateRef _sp = Precursor::fsa_->getState(s);
			_State *sp = new _State(_sp->id(), _sp->tags(), _sp->weight_);
			_Arc bestArc;
			DirectHits directHits;
			Fsa::Weight additionalFailWeight = Precursor::fsa_->semiring()->one();
			if (getBestArc(bestArc, s, additionalFailWeight, directHits)) {
				sp->unsetFinal();
				sp->newArc(bestArc.target(), Precursor::fsa_->semiring()->extend(additionalFailWeight, bestArc.weight()), bestArc.input(), bestArc.output());
			}
			return _ConstStateRef(sp);
		}
		virtual std::string describe() const { return "best4SpecialSymbols(" + Precursor::fsa_->describe() + ")"; }
	};


	template<class _Automaton>
	typename _Automaton::ConstRef best4SpecialSymbols(typename _Automaton::ConstRef f) {
		return typename _Automaton::ConstRef(new BestAutomaton4SpecialSymbols<_Automaton>(f));
	}

	/**
	 * A FAIL arc removal automaton
	 *
	 * Warning: automata could become huge, after this operation.
	 * In the resulting automaton the FAIL arcs have been removed
	 */
	template<class _Automaton>
	class RemoveFailure4SpecialSymbols : public SlaveAutomaton<_Automaton> {
		private:
		typedef SlaveAutomaton<_Automaton> Precursor;

		public:
		typedef typename _Automaton::Weight _Weight;
		typedef typename _Automaton::State _State;
		typedef typename _Automaton::Arc _Arc;
		typedef typename _Automaton::ConstStateRef _ConstStateRef;
		typedef typename _Automaton::ConstRef _ConstAutomatonRef;
		typedef std::list<const _Arc*> DirectHits;
		typedef byInputAndOutput<_Automaton> Sorter;

		private:
		Sorter sorter_;

		public:
		RemoveFailure4SpecialSymbols(_ConstAutomatonRef f) :
			Precursor(sort<_Automaton>(f, Fsa::SortTypeByInputAndOutput))
		{
			this->setProperties(Fsa::PropertyStorage | Fsa::PropertyCached | Fsa::PropertySorted, Fsa::PropertyNone);
		}

		virtual Fsa::StateId initialStateId() const {
			return Precursor::fsa_->initialStateId();
		}

		private:
		bool directHistsHasArc(typename _State::const_iterator &arc, DirectHits &directHits, typename DirectHits::iterator &darc) const {
			// while: darc->output < farc->output
			while ((darc != directHits.end()) and sorter_(*(*darc), *arc)) {
				++darc;
			}
			// now: darc->output >= farc->output

			// if: darc->output > farc->output
			// <=>: directHits has not arc
			return not ((darc == directHits.end()) or sorter_(*arc, *(*darc)));
		}

		void addArcs(_State *newState, Fsa::StateId s, Fsa::Weight weight, DirectHits &directHits) const {
			_ConstStateRef _sp = Precursor::fsa_->getState(s);
			bool oldDirectHitsEmpty=directHits.empty();
			bool hasFailArc=sssp4SpecialSymbolsHelper::stateHasFailArc<_Automaton>(_sp);
			typename DirectHits::iterator darc = directHits.begin();

			for (typename _State::const_iterator a = _sp->begin(); a != _sp->end(); ++a) {
				if (a->output() == Fsa::Failure or a->input() == Fsa::Failure) {
					if(a->output() != a->input()) {
						// assume that we only have arcs with fail on both sides
						write<_Automaton>(Fsa::getResources(), Precursor::fsa_, "removeFail-input.fsa.gz", Fsa::storeAll);
						std::cerr << "removeFailure4SpecialSymbols assumes that we only have arcs with fail on both sides. Input was saved in removeFail-input.fsa.gz. State id is " << s << std::endl;
						defect();
					}
					Fsa::StateId ts = a->target();
					if (ts == s) continue;

					addArcs(newState, ts, Precursor::fsa_->semiring()->extend(weight, a->weight()), directHits);
				} else {
					if (not oldDirectHitsEmpty) {
						if (directHistsHasArc(a, directHits, darc))
							continue;
						else
							darc = directHits.insert(darc, &(*a));
					}
					if (hasFailArc) directHits.push_back(&(*a));
					newState->newArc(a->target(), Precursor::fsa_->semiring()->extend(weight, a->weight()), a->input(), a->output());
				}
			}
		}

		public:
		/**
		 *
		 */
		virtual _ConstStateRef getState(Fsa::StateId s) const {
			_ConstStateRef _sp = Precursor::fsa_->getState(s);
			_State *sp = new _State(_sp->id(), _sp->tags(), _sp->weight_);
			_Arc bestArc;
			DirectHits directHits;
			addArcs(sp, s, Precursor::fsa_->semiring()->one(), directHits);
			return _ConstStateRef(sp);
		}
		virtual std::string describe() const { return "removeFailure4SpecialSymbols(" + Precursor::fsa_->describe() + ")"; }
	};


	template<class _Automaton>
	typename _Automaton::ConstRef removeFailure4SpecialSymbols(typename _Automaton::ConstRef f) {
		return typename _Automaton::ConstRef(new RemoveFailure4SpecialSymbols<_Automaton>(f));
	}

	template<class _Automaton, class _SsspForward4SpecialSymbols=SsspForward4SpecialSymbols<_Automaton>, class _SsspBackward4SpecialSymbols=SsspBackward4SpecialSymbols<_Automaton> >
	class PrunePosteriorAutomaton4SpecialSymbols : public SlaveAutomaton<_Automaton>, public DfsState<_Automaton> {
		typedef SlaveAutomaton<_Automaton> Precursor;
		public:
		typedef typename _Automaton::ConstRef _ConstAutomatonRef;
		typedef typename _Automaton::Weight _Weight;
		typedef typename _Automaton::State _State;
		typedef typename _Automaton::ConstStateRef _ConstStateRef;
		typedef typename _Automaton::Arc _Arc;
		typedef typename _Automaton::Semiring _Semiring;
		typedef typename _Automaton::ConstSemiringRef _ConstSemiringRef;
		typedef typename _Automaton::Semiring::Accumulator _Accumulator;
		typedef typename _SsspForward4SpecialSymbols::ForwardPotential _ForwardPotential;
		typedef std::list<const _Arc*> DirectHits;
		typedef byInputAndOutput<_Automaton> Sorter;
		typedef typename _SsspForward4SpecialSymbols::Weight _SsspWeight;

		private:
		Sorter sorter_;
		_ConstSemiringRef semiring_;
		const _SsspBackward4SpecialSymbols ssspBackward4SpecialSymbols_;
		const _SsspForward4SpecialSymbols ssspForward4SpecialSymbols_;
		/// minWeight_: minimum posterior
		const _Weight threshold_;
		_Weight minWeight_;
		const bool relative_;
		//const _Weight totalInv_;

		private:
		void setMinWeight() {
			if (relative_) {
				minWeight_ = semiring_->max();
				this->dfs();
				minWeight_ = semiring_->extend(minWeight_, threshold_);
			} else {
				minWeight_ = semiring_->extend(ssspBackward4SpecialSymbols_.getPotential(Precursor::fsa_->initialStateId()), threshold_);
			}
		}

		public:
		PrunePosteriorAutomaton4SpecialSymbols(_ConstAutomatonRef f, const _Weight threshold, bool relative) :
			Precursor(sort<_Automaton>(f, Fsa::SortTypeByInputAndOutput)),
			DfsState<_Automaton>(Precursor::fsa_),
			semiring_(Precursor::fsa_->semiring()),
			ssspBackward4SpecialSymbols_(Precursor::fsa_),
			ssspForward4SpecialSymbols_(Precursor::fsa_, ssspBackward4SpecialSymbols_.getProcessedStates()),
			threshold_(threshold), relative_(relative) //,
			//totalInv_(semiring_->invert(ssspBackward4SpecialSymbols_.getPotential(this->fsa_->initialStateId())))
		{
			if(not f->hasProperty(Fsa::PropertyAcyclic)) {
				Core::Application::us()->warning("PrunePosteriorAutomaton4SpecialSymbols: automaton does not have the property 'acyclic'");
			}
			this->setProperties(Fsa::PropertyStorage | Fsa::PropertyCached, Fsa::PropertyNone);
			Fsa::StateId initial = f->initialStateId();
			if (initial != Fsa::InvalidStateId) {
				setMinWeight();
			}
		}

		virtual void discoverState(_ConstStateRef sp) {
			Fsa::StateId s = sp->id();
			bool noNoneFailArc=true;
			const _ForwardPotential &fpot(ssspForward4SpecialSymbols_.forwardPotential(s));

			for (typename _State::const_iterator a = sp->begin(); a != sp->end(); ++a) {
				Fsa::StateId ts = a->target();
				_Weight forwardPot=fpot.getForwardPotential(*a, sorter_);
				_Weight backwardPot;
				if (noNoneFailArc or not (a->output() == Fsa::Failure or a->input() == Fsa::Failure)) {
					backwardPot=ssspBackward4SpecialSymbols_.getPotential(ts);
					noNoneFailArc=false;
				} else {
					verify(a->output() == a->input());
					backwardPot=ssspBackward4SpecialSymbols_.getFailPotential(s);
				}
				_Accumulator *extender(semiring_->getExtender(a->weight()));
				extender->feed(backwardPot);
				extender->feed(forwardPot);

				_Weight w = extender->get();
				delete extender;

				if (semiring_->compare(w, minWeight_) < 0)
					minWeight_ = w;
			}
		}

		virtual _ConstStateRef getState(Fsa::StateId stateId) const {
			_ConstStateRef oldState = Precursor::fsa_->getState(stateId);
			_State *newState = new _State(oldState->id(), oldState->tags(), oldState->weight_);
			bool noNoneFailArc=true;
			const _ForwardPotential &fpot(ssspForward4SpecialSymbols_.forwardPotential(stateId));

			for (typename _State::const_iterator a = oldState->begin(); a != oldState->end(); ++a) {
				Fsa::StateId ts = a->target();
				_Weight forwardPot=fpot.getForwardPotential(*a, sorter_);
				_Weight backwardPot;
				if (noNoneFailArc or not (a->output() == Fsa::Failure or a->input() == Fsa::Failure)) {
					backwardPot=ssspBackward4SpecialSymbols_.getPotential(ts);
					noNoneFailArc=false;
				} else {
					verify(a->output() == a->input());
					backwardPot=ssspBackward4SpecialSymbols_.getFailPotential(stateId);
				}
				_Accumulator *extender(semiring_->getExtender(a->weight()));
				extender->feed(backwardPot);
				extender->feed(forwardPot);
				_Weight w = extender->get();
				delete extender;

				if (semiring_->compare(w, minWeight_) <= 0)
					*newState->newArc() = *a;
			}
			newState->minimize();
			return _ConstStateRef(newState);
		}

		virtual std::string describe() const {
			return Core::form("posteriorPrune4SpecialSymbols(%s,%s,%s)", Precursor::fsa_->describe().c_str(),
				      semiring_->asString(threshold_).c_str(),
				      relative_ ? "relative" : "absolute");
		}
	};

	template<class _Automaton>
	typename _Automaton::ConstRef prunePosterior4SpecialSymbols(typename _Automaton::ConstRef f, const typename _Automaton::Weight &threshold, bool relative) {
		typedef PrunePosteriorAutomaton4SpecialSymbols<_Automaton> _PrunePosteriorAutomaton4SpecialSymbols;
		_PrunePosteriorAutomaton4SpecialSymbols *result = new PrunePosteriorAutomaton4SpecialSymbols<_Automaton>(f, threshold, relative);
		return typename _Automaton::ConstRef(result);
	}

	template<class _Automaton, class _SsspSemiring, class _SsspWeight> typename _Automaton::ConstRef prunePosterior4SpecialSymbols(typename _Automaton::ConstRef f, const typename _Automaton::Weight &threshold, bool relative) {
		typedef SsspForward4SpecialSymbols<_Automaton, _SsspSemiring, _SsspWeight> _SsspForward4SpecialSymbols;
		typedef SsspBackward4SpecialSymbols<_Automaton, _SsspSemiring, _SsspWeight> _SsspBackward4SpecialSymbols;
		typedef PrunePosteriorAutomaton4SpecialSymbols<_Automaton, _SsspForward4SpecialSymbols, _SsspBackward4SpecialSymbols> _PrunePosteriorAutomaton4SpecialSymbols;
		_PrunePosteriorAutomaton4SpecialSymbols *result = new _PrunePosteriorAutomaton4SpecialSymbols(f, threshold, relative);
		return typename _Automaton::ConstRef(result);
	}

} // namespace Ftl

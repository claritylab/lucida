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
#ifndef _T_FSA_BASIC_HH
#define _T_FSA_BASIC_HH

#include <Core/ReferenceCounting.hh>
#include "Types.hh"
#include "tAutomaton.hh"
#include "Mapping.hh"

namespace Ftl {
    /**
     * Normalize the state ids of an automaton. With the "destructive"
     * algorithms, e.g. trimming, pruning, it happens that the sequence of
     * state ids get holes. As some other algorithms allocate vectors based
     * on state ids as index this may result in poor memory efficiency.
     * This function removes all holes in state ids on-demand by renumbering
     * them in the order they are requested. We found out empirically
     * that mapping the indices on-demand is quicker and more memory
     * efficient than using hash tables instead of vectors.
     * Category: on-demand
     * @param f the input automaton
     * @return returns an automaton with all state ids being normalized
     **/
    template<class _Automaton>
    typename _Automaton::ConstRef normalize(typename _Automaton::ConstRef f);

    /**
     * Mapping returning the state's id in the original automaton.
     **/
    template<class _Automaton>
    Fsa::ConstMappingRef mapNormalized(typename _Automaton::ConstRef f);

    /**
     * Keeps only states that are both accessible and coaccessible.
     * Category: on-demand with precalculations
     * Complexity: O(V + E) for the precalculations
     * @param f input automaton
     * @param progress set to true if function should show a progress indicator
     *    on current tty
     * @return returns an automaton with non-accessible and non-coaccessible
     *    states being removed
     **/
    template<class _Automaton>
    typename _Automaton::ConstRef trim(typename _Automaton::ConstRef f, bool progress = false);

    /**
     * Returns a partial automaton, i.e. an automaton where the initial state
     * is moved to different one. This is helpful in order to extract linear
     * automata from n-best lists.
     * Category: on-demand
     * @param f input automaton
     * @param initial the id of the new initial state
     * @return returns an automaton with the new initial state
     **/
    template<class _Automaton>
    typename _Automaton::ConstRef partial(typename _Automaton::ConstRef f, Fsa::StateId initialStateOfPartialAutomaton);

    template<class _Automaton>
    typename _Automaton::ConstRef partial(typename _Automaton::ConstRef f, Fsa::StateId initialStateOfPartialAutomaton, typename _Automaton::Weight additionalFinalWeight);

    /**
     * Same as partial but preserving the scores for n-best lists.
     * Category: on-demand
     * @param f input automaton
     * @param initial the id of the new initial state
     * @return returns an automaton with the new initial state
     **/
    template<class _Automaton>
    typename _Automaton::ConstRef hypothesis(typename _Automaton::ConstRef f, Fsa::StateId n);

    /**
     * Changes the semiring of an automaton.
     * Only the semiring is exchanges, the binary representation of
     * weight is unchanged.  There is no check whether this gives
     * meaningful results.  At the moment you are only able to change
     * between log and tropical semiring.
     * Category: on-demand
     * @param f input automaton
     * @param semiring the new semiring
     * @return returns an automaton over the new semiring
     **/
    template<class _Automaton>
    typename _Automaton::ConstRef changeSemiring(typename _Automaton::ConstRef f, typename _Automaton::ConstSemiringRef semiring);

    /**
     * Changes the semiring and the set of an automaton.
     * Thus, it is necessary to define a mapping between the two sets.
     * Category: on-demand
     * @param f input automaton
     * @param semiring the new semiring determinizing the new set
     * @param map a functor defining a mapping from the old to the new set
     * @return returns an automaton over the new semiring
     **/
    template<class _AutomatonFrom, class _AutomatonTo, class _Mapping>
    Core::Ref<const _AutomatonTo>
    convert(Core::Ref<const _AutomatonFrom> f, Core::Ref<const typename _AutomatonTo::Semiring> semiring, const _Mapping &map = _Mapping());

    template<class _AutomatonFrom, class _Semiring, class _Mapping>
    typename Core::Ref<const Automaton<State<Arc<typename _Semiring::Weight> > > >
    changeSemiringAndSet(Core::Ref<const _AutomatonFrom> f, Core::Ref<const _Semiring> semiring, const _Mapping &map = _Mapping()) {
	return convert<_AutomatonFrom, Automaton<State<Arc<typename _Semiring::Weight> > >, _Mapping>(f, semiring, map);
    }
} // namespace Ftl

#include "tBasic.cc"

#endif // _T_FSA_BASIC_HH

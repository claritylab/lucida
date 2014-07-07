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
#ifndef _T_FSA_COMPOSE_HH
#define _T_FSA_COMPOSE_HH

#include "Mapping.hh"

namespace Ftl {
    /**
     * Calculates the composition result of two input transducers
     * using the matching epsilon filter. This is not guaranteed to do
     * the same than what the AT&T FSM library matching composition
     * filter does although we haven't seen a case yet where results
     * were different.
     * @param fl "left" automaton
     * @param fr "right" automaton
     * @param reportUnknowns tell the user (on stdout) wether mapping
     *   of alphabet symbols reveiled unknowns in the "right" automaton
     * @return the result automaton of the composition (an acceptor in
     *   the case of two input acceptors, a transducer in all other
     *   cases)
     **/
    template<class _Automaton>
    typename _Automaton::ConstRef composeMatching
    (typename _Automaton::ConstRef fl, typename _Automaton::ConstRef fr, bool reportUnknowns = true);

    /**
     * Calculates the composition result of two input transducers
     * using a so-called sequencing epsilon filter. This is not
     * guaranteed to do the same than what the AT&T FSM library
     * sequencing composition filter does although we haven't seen a
     * case yet where results were different.
     * @param fl "left" automaton
     * @param fr "right" automaton
     * @param reportUnknowns tell the user (on stdout) wether mapping
     *   of alphabet symbols reveiled unknowns in the "right" automaton
     * @return the result automaton of the composition (an acceptor in
     *   the case of two input acceptors, a transducer in all other
     *   cases)
     **/
    template<class _Automaton>
    typename _Automaton::ConstRef composeSequencing
    (typename _Automaton::ConstRef fl, typename _Automaton::ConstRef fr, bool reportUnknowns = true);

    /**
     * Calculates an automaton that represents a language equal to the
     * difference of the two input languages L = L_l \ L_r.
     * @param fl "left" automaton
     * @param fr "right" automaton
     * @return the result automaton of the difference operation
     **/
    template<class _Automaton>
    typename _Automaton::ConstRef difference
    (typename _Automaton::ConstRef fl, typename _Automaton::ConstRef fr);

    /**
     * Mapping returning the state's id in the left original automaton.
     **/
    template<class _Automaton>
    Fsa::ConstMappingRef mapToLeft(typename _Automaton::ConstRef f);

    /**
     * Mapping returning the state's id in the right original automaton.
     **/
    template<class _Automaton>
    Fsa::ConstMappingRef mapToRight(typename _Automaton::ConstRef f);

} // namespace Ftl

#include "tCompose.cc"

#endif // _T_FSA_COMPOSE_HH

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
#ifndef _T_FSA_RATIONAL_HH
#define _T_FSA_RATIONAL_HH

#include "Alphabet.hh"
#include <Core/Vector.hh>
#include "Mapping.hh"

namespace Ftl {
    template<class _Automaton>
    typename _Automaton::ConstRef identity(Fsa::ConstAlphabetRef, typename _Automaton::ConstSemiringRef);
    template<class _Automaton>
    typename _Automaton::ConstRef closure(typename _Automaton::ConstRef f);
    template<class _Automaton>
    typename _Automaton::ConstRef kleeneClosure(typename _Automaton::ConstRef f);
    template<class _Automaton>
    typename _Automaton::ConstRef complement(typename _Automaton::ConstRef f);
    template<class _Automaton>
    typename _Automaton::ConstRef concat(const Core::Vector<typename _Automaton::ConstRef> &f);
    template<class _Automaton>
    typename _Automaton::ConstRef concat(typename _Automaton::ConstRef, typename _Automaton::ConstRef);
    template<class _Automaton>
    typename _Automaton::ConstRef unite(const Core::Vector<typename _Automaton::ConstRef> &f,
					const Core::Vector<typename _Automaton::Weight> &initialWeights
					= Core::Vector<typename _Automaton::Weight>());
    template<class _Automaton>
    typename _Automaton::ConstRef unite(typename _Automaton::ConstRef, typename _Automaton::ConstRef);
    /**
     * Mapping returning the state's id in the respective original sub-automaton.
     **/
    template<class _Automaton>
    Fsa::ConstMappingRef mapToSubAutomaton(typename _Automaton::ConstRef f, u32 subAutomaton);
    /**
       * Fuse initial states.  The result automaton will contain the
       * union of the states of the source automata, except for their
       * initial states.  The initial state of all source automata will
       * be merged into a single initial state.  For certain (but
       * common) cases this is equivalent to closure(unite()), but does
       * not produce the nasty epsilon loops.
       */
    template<class _Automaton>
    typename _Automaton::ConstRef fuse(const Core::Vector<typename _Automaton::ConstRef> &f);
    template<class _Automaton>
    typename _Automaton::ConstRef fuse(typename _Automaton::ConstRef, typename _Automaton::ConstRef);

    template<class _Automaton>
    typename _Automaton::ConstRef transpose(typename _Automaton::ConstRef f, bool progress = false);
} // namespace Ftl

#include "tRational.cc"

#endif // _T_FSA_RATIONAL_HH

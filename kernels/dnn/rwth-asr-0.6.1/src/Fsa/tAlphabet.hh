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
#ifndef _T_FSA_ALPHABET_HH
#define _T_FSA_ALPHABET_HH

#include "Alphabet.hh"
#include "AlphabetUtility.hh"
#include "Types.hh"

namespace Ftl {
    /*
     * Cases:
     * 1. Alphabets are identical (not just equal)
     *    => orignal automaton is returned
     * 2. Alphabet mapping is identity (same set of symbols and same indices)
     *    => returned SlaveAutomaton has new alphabet but does not actually do anything
     * 3. Mapping is partial identity (surjective) (same indices, but some symbols missing but)
     *    => only remove arcs from automaton
     *    (type changes; properties stay the same)
     * 4. Mapping is partial => remove arcs from automaton and map indices
     *    (type/properties of automaton change)
     * 5. Mapping is complete => only map indices
     *    (type/properties of automaton change)
     */

    /**
     * Maps the input alphabet of the input automaton and calculates the
     * mapping to the target alphabet beforehand.
     * Category: on-demand with precalculations
     * @param f the input automaton
     * @param alphabet the alphabet to map to
     * @param reportUnknowns maximum number of unknown symbols that should be reported
     **/
    template<class _Automaton>
    typename _Automaton::ConstRef mapInput(typename _Automaton::ConstRef f, Fsa::ConstAlphabetRef alphabet, u32 reportUnknowns = 10);

    /**
     * Maps the output alphabet of the input automaton and calculates the
     * mapping to the target alphabet beforehand.
     * Category: on-demand with precalculations
     * @param f the input automaton
     * @param alphabet the alphabet to map to
     * @param reportUnknowns maximum number of unknown symbols that should be reported
     **/
    template<class _Automaton>
    typename _Automaton::ConstRef mapOutput(typename _Automaton::ConstRef f, Fsa::ConstAlphabetRef alphabet, u32 reportUnknowns = 10);

    /**
     * When the given automaton is an acceptor or has the same input
     * and output alphabets, maps the input and output labels.  This
     * is more efficient than calling both mapInput() and mapOutput(),
     * and in the case of accetors, this prevents the type from being
     * changed to transducer.
     * Category: on-demand with precalculations
     * @param f the input automaton
     * @param alphabet the alphabet to map to
     * @param reportUnknowns maximum number of unknown symbols that should be reported
     **/
    template<class _Automaton>
    typename _Automaton::ConstRef mapInputOutput(typename _Automaton::ConstRef f, Fsa::ConstAlphabetRef, u32 reportUnknowns = 10);


    /**
     * Maps the input alphabet of the input automaton using an existing
     * mapping.  Note, that the mapping must fits the alphabet
     * of the input automaton.
     * Category: on-demand
     * @param f the input automaton
     * @param alphabet the alphabet to map to
     **/
    template<class _Automaton>
    typename _Automaton::ConstRef mapInput(typename _Automaton::ConstRef f, const Fsa::AlphabetMapping&);

    /**
     * Maps the output alphabet of the input automaton using an
     * existing mapping.  Note, that the mapping must fits the alphabet
     * of the input automaton.
     * Category: on-demand
     * @param f the input automaton
     * @param alphabet the alphabet to map to
     **/
    template<class _Automaton>
    typename _Automaton::ConstRef mapOutput(typename _Automaton::ConstRef f, const Fsa::AlphabetMapping&);


    /**
     * When the given automaton is an acceptor or has the same input
     * and output alphabets, maps the input and output labels.  This
     * is more efficient than calling both mapInput() and mapOutput(),
     * and in the case of accetors, this prevents the type from being
     * changed to transducer.
     * Category: on-demand
     * @param f the input automaton
     * @param alphabet the alphabet to map to
     **/
    template<class _Automaton>
    typename _Automaton::ConstRef mapInputOutput(typename _Automaton::ConstRef, const Fsa::AlphabetMapping&);
} // namespace Ftl

#include "tAlphabet.cc"

#endif //_T_FSA_ALPHABET_HH

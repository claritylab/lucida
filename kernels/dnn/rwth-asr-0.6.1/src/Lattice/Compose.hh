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
#ifndef _LATTICE_COMPOSE_HH
#define _LATTICE_COMPOSE_HH

#include "Lattice.hh"
#include <Fsa/Automaton.hh>

namespace Lm {
    class ScaledLanguageModel;
}

namespace Lattice {

    /** remember that for the intersection of a word lattice with an automaton
     *  the automaton needs to be deterministic to avoid path duplicating
     *  important for word posterior probabilities
     */
    ConstWordLatticeRef composeMatching(
	Fsa::ConstAutomatonRef left,
	ConstWordLatticeRef right,
	bool reportUnknowns = true);

    ConstWordLatticeRef composeMatching(
	ConstWordLatticeRef left,
	Fsa::ConstAutomatonRef right,
	bool reportUnknowns = true);

    /**
     *  Corresponds to Lm::compose but the word boundaries are restored.
     *  Advantage: no sentence hypotheses are duplicated by construction
     *  of the Lm::compose and, thus, no determinisation is needed beforehand.
     *  See Lm/Compose.hh for further comments.
     */
    ConstWordLatticeRef composeLm(
	ConstWordLatticeRef left,
	Core::Ref<const Lm::ScaledLanguageModel> right,
	f32 pronunciationScale);

} // namespace Lattice

#endif // _LATTICE_COMPOSE_HH

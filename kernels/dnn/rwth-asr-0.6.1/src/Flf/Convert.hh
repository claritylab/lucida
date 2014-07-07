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
#ifndef _FLF_CONVERT_HH
#define _FLF_CONVERT_HH

#include <Fsa/Automaton.hh>

#include "FlfCore/Lattice.hh"
#include "Lexicon.hh"
#include "Network.hh"

namespace Flf {

    /**
     * dim i -> dim+offest
     **/
    ConstLatticeRef offsetSemiring(ConstLatticeRef l, ConstSemiringRef targetSemiring, ScoreId offset = 0);


    /**
     * normalize semiring:
     * apply scales to scores, set scales and keys to defaults (i.e. scales to 1.0)
     **/
    ConstLatticeRef normalizeSemiring(ConstLatticeRef l, bool normalizeNames = true);


    /**
     * change semiring (and set)
     **/
    NodeRef createChangeSemiringNode(const std::string &name, const Core::Configuration &config);


    /**
     * project source semiring to target semiring
     **/
    NodeRef createProjectSemiringNode(const std::string &name, const Core::Configuration &config);


    /**
     * from fsa to flf, incl. alphabet mapping (and symbol insertion)
     **/
    ConstLatticeRef fromFsa(Fsa::ConstAutomatonRef f, ConstSemiringRef semiring, ScoresRef defaultScore, ScoreId id,
			    Lexicon::AlphabetId inputAlphabetId, Lexicon::AlphabetId outputAlphabetId = Lexicon::UnknownAlphabetId);
    ConstLatticeRef fromUnweightedFsa(Fsa::ConstAutomatonRef f, ConstSemiringRef semiring, ScoresRef constScore,
				      Lexicon::AlphabetId inputAlphabetId, Lexicon::AlphabetId outputAlphabetId = Lexicon::UnknownAlphabetId);

    /**
     * string to lattice
     **/
    NodeRef createStringConverterNode(const std::string &name, const Core::Configuration &config);

} // namespace Flf

#endif // _FLF_CONVERT_HH

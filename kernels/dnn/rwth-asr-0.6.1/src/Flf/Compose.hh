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
#ifndef _FLF_COMPOSE_HH
#define _FLF_COMPOSE_HH

#include <Fsa/Automaton.hh>

#include "FlfCore/Lattice.hh"
#include "Network.hh"

namespace Flf {

    /**
     * - set weight of arcs and states in l to l->semiring()->one() (or to semiring->one(), if a semiring is given)
     * - remove word boundaries
     *
     * application: intersections, i.e. composition of a weighted and an unweighted transducer
     **/
    ConstLatticeRef unweight(ConstLatticeRef l, ConstSemiringRef semiring = ConstSemiringRef());


    /**
     * compose two lattices
     **/
    ConstLatticeRef composeMatching(ConstLatticeRef l, ConstLatticeRef r);
    NodeRef createComposeMatchingNode(const std::string &name, const Core::Configuration &config);

    ConstLatticeRef composeSequencing(ConstLatticeRef l, ConstLatticeRef r);
    NodeRef createComposeSequencingNode(const std::string &name, const Core::Configuration &config);


    /**
     * difference between two lattices
     **/
    ConstLatticeRef differ(ConstLatticeRef l, ConstLatticeRef r);
    NodeRef createDifferenceNode(const std::string &name, const Core::Configuration &config);


    /**
     * intersection of two lattices;
     * extend or append scores
     **/
    ConstLatticeRef intersect(ConstLatticeRef l, ConstLatticeRef r, bool appendScores = false);
    NodeRef createIntersectionNode(const std::string &name, const Core::Configuration &config);


    /**
     * composition of lattice and fsa
     **/
    ConstLatticeRef composeWithFsa(ConstLatticeRef l, Fsa::ConstAutomatonRef f, Score scale);
    ConstLatticeRef composeWithFsa(ConstLatticeRef l, Fsa::ConstAutomatonRef f, ScoreId id, Score scale = Semiring::DefaultScale);
    NodeRef createComposeWithFsaNode(const std::string &name, const Core::Configuration &config);

} // namespace Flf

#endif // _FLF_COMPOSE_HH

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
#ifndef _FLF_CORE_BASIC_HH
#define _FLF_CORE_BASIC_HH

#include <Fsa/hSort.hh>
#include <Fsa/Automaton.hh>

#include "Lattice.hh"

namespace Flf {
    /**
     * Replace semiring
     *
     * Rescale semiring: new semiring is old semiring, where scales and/or key names are changed
     * Change semiring: old and new semiring must have same size
     * Project semiring: size(projection-matrix) = length(new semiring) x length(old semiring)
     *
    **/
    ConstLatticeRef rescale(ConstLatticeRef l, const ScoreList &scales, const KeyList &keys = KeyList());
    ConstLatticeRef rescale(ConstLatticeRef l, ScoreId id, Score scale, const Key &key = Semiring::UndefinedKey);
    ConstLatticeRef changeSemiring(ConstLatticeRef l, ConstSemiringRef targetSemiring);
    typedef std::vector<ScoreList> ProjectionMatrix;
    ConstLatticeRef projectSemiring(ConstLatticeRef l, ConstSemiringRef targetSemiring, const ProjectionMatrix &);


    /**
     * Fsa to Flf
     *
     **/
    ConstLatticeRef fromFsa(Fsa::ConstAutomatonRef f, ConstSemiringRef semiring, ScoreId id, ScoresRef defaultScore = ScoresRef());
    ConstLatticeRef fromFsaVector(const std::vector<Fsa::ConstAutomatonRef> &fsas, ConstSemiringRef semiring);
    ConstLatticeRef fromUnweightedFsa(Fsa::ConstAutomatonRef f, ConstSemiringRef semiring, ScoresRef defaultScore = ScoresRef());


    /**
     * Flf to Fsa
     *
     **/
    Fsa::ConstAutomatonRef toFsa(ConstLatticeRef l, const ScoreList &scales = ScoreList());
    Fsa::ConstAutomatonRef toFsa(ConstLatticeRef l, ScoreId id, bool scaled = false);
    std::vector<Fsa::ConstAutomatonRef> toFsaVector(ConstLatticeRef l, bool scaled = false);
    Fsa::ConstAutomatonRef toUnweightedFsa(ConstLatticeRef l, const Fsa::Weight &defaultWeight = Fsa::Weight(Semiring::One));


    /**
     * Topological/Chronological Order
     *
     * sort in topological order: order    -> state id
     * find topological order   : state id -> order, or
     *
     * if l has no topological order, i.e. l is cyclic, the
     * resulting map is empty
     *
     * if a chronological exists, that does not violates the
     * topological order, then the returned chronological
     * order is as well a topological order (many algorithm
     * require acyclic lattices that doe not violate the time
     * contraint; for those lattices the property is given).
     *
     * if you need both, topological sort and order, calculate
     * first sort and order is calculated fast by making use
     * of the topological sort
     *
     **/
    ConstStateMapRef sortTopologically(ConstLatticeRef l);
    ConstStateMapRef findTopologicalOrder(ConstLatticeRef l);
    ConstStateMapRef sortChronologically(ConstLatticeRef l);

    /**
     * The states of the resulting lattice are enumerated by
     * increasing topological order, i.e. state id = topological order
     **/
    ConstLatticeRef sortByTopologicalOrder(ConstLatticeRef l);


    /**
     * Properties
     *
     * getProperties computes missing acyclic/linear property.
     *
     **/
    bool isAcyclic(ConstLatticeRef l);
    /*
      Causes link erro. Why???
    bool isLinear(ConstLatticeRef l);
    */
    /*
      Causes link erro. Why???
    Fsa::Property getProperties(ConstLatticeRef l, Fsa::Property properties = Fsa::PropertyAll);
    */

    /**
     * Miscellaneous functions
     *
     **/
    ConstLatticeRef partial(ConstLatticeRef l, Fsa::StateId initial);
    ConstLatticeRef sort(ConstLatticeRef l, Fsa::SortType type);
    ConstLatticeRef trim(ConstLatticeRef l, bool progress = false);
    void trimInPlace(StaticLatticeRef l);

    /**
     * Append
     *
     * appends the scores of two topologically equivalent (down to state numbering!) lattices;
     * scores are concatenated, i.e. new semiring is the concatentation
     **/
    ConstLatticeRef appendScores(ConstLatticeRef l1, ConstLatticeRef l2, ConstSemiringRef semiring = ConstSemiringRef());

} // namespace Flf

#endif // _FLF_CORE_BASIC_HH

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
#ifndef _LATTICE_BASIC_HH
#define _LATTICE_BASIC_HH

#include "Lattice.hh"
#include <Fsa/Sssp.hh>

namespace Lattice {

    template <class Modifier>
    ConstWordLatticeRef apply(
	ConstWordLatticeRef lattice, Modifier &m, bool copyWordBoundaries = true)
    {
	if (lattice) {
	    WordLattice *l = new WordLattice;
	    if (copyWordBoundaries)
	    l->setWordBoundaries(lattice->wordBoundaries());
	    for (size_t i = 0; i < lattice->nParts(); ++ i) {
		l->setFsa(m.modify(lattice->part(i)),
			  lattice->name(i));
	    }
	    return ConstWordLatticeRef(l);
	}
	return ConstWordLatticeRef();
    }

    ConstWordLatticeRef normalize(ConstWordLatticeRef);
    ConstWordLatticeRef trim(ConstWordLatticeRef l, bool progress = false);
    ConstWordLatticeRef partial(ConstWordLatticeRef l, Fsa::StateId initial);
    ConstWordLatticeRef partial(ConstWordLatticeRef l, Fsa::StateId initial, Fsa::Weight additionalFinalWeight);
    ConstWordLatticeRef changeSemiring(ConstWordLatticeRef, Fsa::ConstSemiringRef);

    bool isEmpty(ConstWordLatticeRef l);

    /**
     * number of arcs per timeframe
     */
    f32 density(ConstWordLatticeRef);
    std::pair<u32, u32> densityInfo(ConstWordLatticeRef);

    typedef Fsa::SsspArcFilter Predicate;
    class IsInputLabel : public Predicate
    {
    public:
	typedef Core::hash_set<Fsa::LabelId> LabelSet;
    private:
	const LabelSet &labels_;
    public:
	IsInputLabel(const LabelSet &labels) : labels_(labels) {}
	virtual bool operator() (const Fsa::Arc &a) const {
	    return labels_.find(a.input()) != labels_.end();
	}
    };

    u32 nArcs(ConstWordLatticeRef, Predicate);
    std::pair<Fsa::Weight,Fsa::Weight> minMaxWeights(ConstWordLatticeRef);
}

#endif // _LATTICE_BASIC_HH

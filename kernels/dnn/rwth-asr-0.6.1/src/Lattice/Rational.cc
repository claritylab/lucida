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
#include "Rational.hh"
#include <Fsa/Basic.hh>
#include <Fsa/Rational.hh>
#include "Morphism.hh"

namespace Lattice {

    ConstWordLatticeRef transpose(ConstWordLatticeRef l, bool progress, WordBoundary final)
    {
	require(l->wordBoundaries());
	WordLattice *result = 0;
	for (size_t i = 0; i < l->nParts(); ++ i) {
	    Fsa::ConstAutomatonRef transposed = Fsa::transpose(l->part(i), progress);
	    if (!result) {
		if (!final.valid()) {
		    final = l->wordBoundaries()->getFinalWordBoundary();
		}
		Core::Ref<WordBoundaries> wordBoundaries(new WordBoundaries(*(l->wordBoundaries())));
		wordBoundaries->set(transposed->initialStateId(), final);
		result = new WordLattice();
		result->setWordBoundaries(wordBoundaries);
	    }
	    result->setFsa(transposed, l->name(i));
	}
	return ConstWordLatticeRef(result);
    }

    ConstWordLatticeRef unite(ConstWordLatticeRef l1, ConstWordLatticeRef l2)
    {
	Core::Vector<ConstWordLatticeRef> lattices(2);
	lattices[0] = l1;
	lattices[1] = l2;
	return unite(lattices);
    }

    ConstWordLatticeRef unite(const Core::Vector<ConstWordLatticeRef> &lattices)
    {
	if (lattices.size() > 0) {
	    bool hasWordBoundaries = true;
	    const u32 nParts = lattices.front()->nParts();
	    Core::Vector<Core::Ref<const WordBoundaries> > wordBoundaries(lattices.size());
	    Core::Vector<Fsa::ConstMappingRef> morphisms(lattices.size());
	    Core::Ref<WordLattice> result(new WordLattice());
	    for (size_t i = 0; i < nParts; ++ i) {
		Core::Vector<Fsa::ConstAutomatonRef> tmp(lattices.size());
		const std::string latticeName = lattices.front()->name(i);
		for (u32 n = 0; n < lattices.size(); ++ n) {
		    require(lattices[n]->name(i) == latticeName);
		    tmp[n] = lattices[n]->part(i);
		}
		result->setFsa(Fsa::unite(tmp), latticeName);
		for (u32 n = 0; n < lattices.size(); ++ n) {
		    if (lattices[n]->wordBoundaries()) {
			wordBoundaries[n] = lattices[n]->wordBoundaries();
		    } else {
			hasWordBoundaries = false;
		    }
		    morphisms[n] = Fsa::mapToSubAutomaton(result->part(latticeName), n);
		}
	    }
	    if (hasWordBoundaries) {
		result->setWordBoundaries(Core::ref(new WordBoundaries()));
		return resolveNaryMorphism(result, wordBoundaries, morphisms);
	    } else {
		return result;
	    }
	}
	return ConstWordLatticeRef();
    }

} // namespace Lattice

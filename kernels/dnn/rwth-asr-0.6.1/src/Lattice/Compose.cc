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
#include "Compose.hh"
#include "Morphism.hh"
#include <Fsa/Basic.hh>
#include <Fsa/Compose.hh>
#include <Lm/Compose.hh>

namespace Lattice {

    ConstWordLatticeRef composeMatching(
	Fsa::ConstAutomatonRef left,
	ConstWordLatticeRef right,
	bool reportUnknowns)
    {
	Core::Ref<WordLattice> wordLattice(new WordLattice);
	Fsa::ConstAutomatonRef composed;
	for (size_t i = 0; i < right->nParts(); ++i) {
	    composed = Fsa::composeMatching(
		left,
		right->part(i),
		reportUnknowns);
	    Fsa::ConstAutomatonRef f = Fsa::trim(composed);
	    wordLattice->setFsa(f, right->name(i));
	}
	if (right->wordBoundaries()) {
	    wordLattice->setWordBoundaries(Core::ref(new WordBoundaries));
	    return resolveMorphism(
		wordLattice,
		right->wordBoundaries(),
		Fsa::mapToRight(composed));
	} else {
	    return wordLattice;
	}
    }

    ConstWordLatticeRef composeMatching(
	ConstWordLatticeRef left,
	Fsa::ConstAutomatonRef right,
	bool reportUnknowns)
    {
	Core::Ref<WordLattice> wordLattice(new WordLattice);
	Fsa::ConstAutomatonRef composed;
	for (size_t i = 0; i < left->nParts(); ++i) {
	    composed = Fsa::composeMatching(
		left->part(i),
		right,
		reportUnknowns);
	    Fsa::ConstAutomatonRef f = Fsa::trim(composed);
	    wordLattice->setFsa(f, left->name(i));
	}
	if (left->wordBoundaries()) {
	    wordLattice->setWordBoundaries(Core::ref(new WordBoundaries));
	    return resolveMorphism(
		wordLattice,
		left->wordBoundaries(),
		Fsa::mapToLeft(composed));
	} else {
	    return wordLattice;
	}
    }

    ConstWordLatticeRef composeLm(
	ConstWordLatticeRef left,
	Core::Ref<const Lm::ScaledLanguageModel> right,
	f32 pronunciationScale)
    {
	require(left->wordBoundaries());
	Core::Ref<WordLattice> wordLattice(new WordLattice);
	wordLattice->setWordBoundaries(Core::ref(new WordBoundaries));
	if (left->nParts() == 0) return wordLattice;
	Core::Ref<const Lm::ComposeAutomaton> compose =
	    Lm::composePron(left->part(0), right, pronunciationScale);
	wordLattice->setFsa(compose, left->name(0));
	for (size_t i = 1; i < left->nParts(); ++i) {
	    compose = Lm::composePron(left->part(i), right, pronunciationScale);
	    wordLattice->setFsa(compose, left->name(i));
	}
	return resolveMorphism(ConstWordLatticeRef(wordLattice),
			       left->wordBoundaries(),
			       compose->mapToLeft());
    }

} // namespace Lattice

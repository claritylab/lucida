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
#include "Morphism.hh"
#include <Fsa/Automaton.hh>
#include <Core/Assertions.hh>

namespace Lattice {

    // MorphismAutomaton: base class
    class MorphismAutomaton : public Fsa::SlaveAutomaton
    {
    protected:
	mutable Core::Ref<WordBoundaries> mappedWordBoundaries_;
    public:
	MorphismAutomaton(Fsa::ConstAutomatonRef, Core::Ref<WordBoundaries>);
	virtual ~MorphismAutomaton() {}

	virtual std::string describe() const { return fsa_->describe(); }
    };

    MorphismAutomaton::MorphismAutomaton(
	Fsa::ConstAutomatonRef fsa,
	Core::Ref<WordBoundaries> mappedWordBoundaries)
	:
	Fsa::SlaveAutomaton(fsa),
	mappedWordBoundaries_(mappedWordBoundaries)
    {}

    // UnaryMorphismAutomaton: unary
    class UnaryMorphismAutomaton : public MorphismAutomaton
    {
	typedef MorphismAutomaton Precursor;
    protected:
	Core::Ref<const WordBoundaries> wordBoundaries_;
	Fsa::ConstMappingRef morphism_;
    public:
	UnaryMorphismAutomaton(Fsa::ConstAutomatonRef,
			  Core::Ref<const WordBoundaries>,
			  Fsa::ConstMappingRef,
			  Core::Ref<WordBoundaries>);
	virtual ~UnaryMorphismAutomaton() {}

	virtual Fsa::ConstStateRef getState(Fsa::StateId s) const;
    };

    UnaryMorphismAutomaton::UnaryMorphismAutomaton(
	Fsa::ConstAutomatonRef fsa,
	Core::Ref<const WordBoundaries> wordBoundaries,
	Fsa::ConstMappingRef morphism,
	Core::Ref<WordBoundaries> mappedWordBoundaries)
	:
	Precursor(fsa, mappedWordBoundaries),
	wordBoundaries_(wordBoundaries),
	morphism_(morphism)
    {}

    Fsa::ConstStateRef UnaryMorphismAutomaton::getState(Fsa::StateId s) const
    {
	mappedWordBoundaries_->set(s, (*wordBoundaries_)[morphism_->map(s)]);
	return Fsa::SlaveAutomaton::getState(s);
    }

    ConstWordLatticeRef resolveMorphism(
	ConstWordLatticeRef l,
	Core::Ref<const WordBoundaries> wordBoundaries,
	Fsa::ConstMappingRef morphism)
    {
	Core::Ref<WordBoundaries> _wordBoundaries(new WordBoundaries());
	WordLattice *result = new WordLattice;
	result->setWordBoundaries(_wordBoundaries);
	for (size_t i = 0; i < l->nParts(); ++i) {
	    MorphismAutomaton *f = new UnaryMorphismAutomaton(
		l->part(i), wordBoundaries, morphism, _wordBoundaries);
	    result->setFsa(Fsa::ConstAutomatonRef(f), l->name(i));
	}
	return ConstWordLatticeRef(result);
    }

    class NaryMorphismAutomaton : public MorphismAutomaton
    {
	typedef MorphismAutomaton Precursor;
    protected:
	Core::Vector<Core::Ref<const WordBoundaries> > wordBoundaries_;
	Core::Vector<Fsa::ConstMappingRef> morphisms_;
    public:
	NaryMorphismAutomaton(
	    Fsa::ConstAutomatonRef,
	    const Core::Vector<Core::Ref<const WordBoundaries> > &,
	    const Core::Vector<Fsa::ConstMappingRef> &,
	    Core::Ref<WordBoundaries>);
	virtual ~NaryMorphismAutomaton() {}

	virtual Fsa::ConstStateRef getState(Fsa::StateId s) const;
    };

    NaryMorphismAutomaton::NaryMorphismAutomaton(
	Fsa::ConstAutomatonRef fsa,
	const Core::Vector<Core::Ref<const WordBoundaries> > &wordBoundaries,
	const Core::Vector<Fsa::ConstMappingRef> &morphisms,
	Core::Ref<WordBoundaries> mappedWordBoundaries)
	:
	Precursor(fsa, mappedWordBoundaries),
	wordBoundaries_(wordBoundaries),
	morphisms_(morphisms)
    {}

    Fsa::ConstStateRef NaryMorphismAutomaton::getState(Fsa::StateId s) const
    {
	if ((s >= mappedWordBoundaries_->size()) or !(*mappedWordBoundaries_)[s].valid()) {
	    if (s == fsa_->initialStateId()) {
		mappedWordBoundaries_->set(s, WordBoundary(0, WordBoundary::Transit()));
	    } else {
		Fsa::StateId subS = Fsa::InvalidStateId;
		u32 n = 0;
		while (n < morphisms_.size()) {
		    subS = morphisms_[n]->map(s);
		    if (subS != Fsa::InvalidStateId) {
			break;
		    }
		    ++ n;
		}
		verify(subS != Fsa::InvalidStateId);
		mappedWordBoundaries_->set(s, (*wordBoundaries_[n])[subS]);
	    }
	}
	return fsa_->getState(s);
    }

    ConstWordLatticeRef resolveNaryMorphism(
	ConstWordLatticeRef l,
	const Core::Vector<Core::Ref<const WordBoundaries> > &wordBoundaries,
	const Core::Vector<Fsa::ConstMappingRef> &morphisms)
    {
	Core::Ref<WordBoundaries> _wordBoundaries(new WordBoundaries());
	WordLattice *result = new WordLattice();
	result->setWordBoundaries(_wordBoundaries);
	for (size_t i = 0; i < l->nParts(); ++ i) {
	    MorphismAutomaton *f = new NaryMorphismAutomaton(
		l->part(i), wordBoundaries, morphisms, _wordBoundaries);
	    result->setFsa(Fsa::ConstAutomatonRef(f), l->name(i));
	}
	return ConstWordLatticeRef(result);
    }

} // namespace Lattice

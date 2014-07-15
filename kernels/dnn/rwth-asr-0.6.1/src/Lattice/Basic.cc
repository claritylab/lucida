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
#include "Basic.hh"
#include "Morphism.hh"
#include <Fsa/Basic.hh>
#include <Core/Vector.hh>
#include <Core/Types.hh>

namespace Lattice {

    ConstWordLatticeRef normalize(ConstWordLatticeRef lattice)
    {
	if (lattice) {
	    WordLattice *normalized = new WordLattice;
	    Fsa::ConstAutomatonRef n;
	    for (size_t i = 0; i < lattice->nParts(); ++ i) {
		n = Fsa::normalize(lattice->part(i));
		normalized->setFsa(Fsa::staticCopy(n), lattice->name(i));
	    }
	    if (lattice->wordBoundaries()) {
		return resolveMorphism(ConstWordLatticeRef(normalized),
				       lattice->wordBoundaries(),
				       Fsa::mapNormalized(n));
	    } else {
		return ConstWordLatticeRef(normalized);
	    }
	}
	return ConstWordLatticeRef();
    }

    struct Trimmer
    {
	bool progress;
	Trimmer(bool _progress) : progress(_progress) {}
	Fsa::ConstAutomatonRef modify(Fsa::ConstAutomatonRef fsa) {
	    return Fsa::trim(fsa, progress);
	}
    };

    ConstWordLatticeRef trim(
	ConstWordLatticeRef lattice, bool progress)
    {
	Trimmer t(progress);
	return apply(lattice, t);
    }

    struct Cutter
    {
	Fsa::StateId initial;
	Fsa::Weight finalWeight;
	Cutter(Fsa::StateId _initial, Fsa::Weight _finalWeight) :
	    initial(_initial), finalWeight(_finalWeight) {}
	Fsa::ConstAutomatonRef modify(Fsa::ConstAutomatonRef fsa) {
	    return Fsa::partial(fsa, initial, finalWeight);
	}
    };

    ConstWordLatticeRef partial(
	ConstWordLatticeRef lattice, Fsa::StateId initial)
    {
	Cutter c(initial, lattice->part(0)->semiring()->one());
	return apply(lattice, c);
    }

    ConstWordLatticeRef partial(
	ConstWordLatticeRef lattice, Fsa::StateId initial, Fsa::Weight additionalFinalWeight)
    {
	Cutter c(initial, additionalFinalWeight);
	return apply(lattice, c);
    }

    struct SemiringChanger
    {
	Fsa::ConstSemiringRef semiring;
	SemiringChanger(Fsa::ConstSemiringRef sr) : semiring(sr) {}
	Fsa::ConstAutomatonRef modify(Fsa::ConstAutomatonRef fsa) {
	    return Fsa::changeSemiring(fsa, semiring);
	}
    };

    ConstWordLatticeRef changeSemiring(
	ConstWordLatticeRef lattice, Fsa::ConstSemiringRef semiring)
    {
	SemiringChanger c(semiring);
	return apply(lattice, c);
    }

    bool isEmpty(ConstWordLatticeRef lattice)
    {
	if (!lattice or (lattice->nParts() == 0)) {
	    return true;
	} else {
	    return Fsa::isEmpty(lattice->part(0));
	}
    }

    class DensityDfsState : public DfsState
    {
    private:
	u32 nArcs_;
	Speech::TimeframeIndex minTimeframeIndex_, maxTimeframeIndex_;
	f32 density_;
    public:
	DensityDfsState(ConstWordLatticeRef wordLattice) :
	    DfsState(wordLattice),
	    nArcs_(0),
	    minTimeframeIndex_(Core::Type<Speech::TimeframeIndex>::max),
	    maxTimeframeIndex_(Core::Type<Speech::TimeframeIndex>::min) {}
	virtual ~DensityDfsState() {}

	virtual void discoverState(Fsa::ConstStateRef sp) {
	    const Speech::TimeframeIndex tbeg = wordBoundaries_->time(sp->id());
	    if (tbeg != Speech::InvalidTimeframeIndex) {
		minTimeframeIndex_ = std::min(minTimeframeIndex_, tbeg);
		maxTimeframeIndex_ = std::max(maxTimeframeIndex_, tbeg);
	    }
	    for (Fsa::State::const_iterator a = sp->begin(); a != sp->end(); ++ a) {
		if (a->input() != Fsa::Epsilon) {
		    const Speech::TimeframeIndex tend =
			wordBoundaries_->time(
			    fsa_->getState(a->target())->id());
		    nArcs_ += tend - tbeg;
		}
	    }
	}

	virtual void finish() {
	    density_ = (nTimeframes() > 0) ? (f32) nArcs() / nTimeframes() : 0;
	}

	f32 density() const { return density_; }
	u32 nTimeframes() const {
	    return maxTimeframeIndex_ - minTimeframeIndex_;
	}
	u32 nArcs() const { return nArcs_; }
    };

    f32 density(ConstWordLatticeRef wordLattice)
    {
	DensityDfsState s(wordLattice);
	s.dfs();
	return s.density();
    }

    std::pair<u32, u32> densityInfo(ConstWordLatticeRef wordLattice)
    {
	DensityDfsState s(wordLattice);
	s.dfs();
	return std::make_pair(s.nTimeframes(), s.nArcs());
    }

    class LatticeCounts : public DfsState {
    public:
	Predicate pred;
	u32 nArcs;
    public:
	LatticeCounts(ConstWordLatticeRef lattice, Predicate p) :
	    DfsState(lattice), pred(p), nArcs(0) {}

	virtual void discoverState(Fsa::ConstStateRef sp) {
	    nArcs += std::count_if(sp->begin(), sp->end(), pred);
	}
    };

    u32 nArcs(ConstWordLatticeRef lattice, Predicate pred)
    {
	LatticeCounts counts(lattice, pred);
	counts.dfs();
	return counts.nArcs;
    }

    class LatticeWeights : public DfsState {
    public:
	std::pair<Fsa::Weight,Fsa::Weight> minMax;
    private:
	Fsa::Weight min(const Fsa::Weight &a, const Fsa::Weight &b) const {
	    return fsa_->semiring()->compare(a, b) < 0 ? a : b;
	}
	Fsa::Weight max(const Fsa::Weight &a, const Fsa::Weight &b) const {
	    return fsa_->semiring()->compare(a, b) > 0 ? a : b;
	}
    public:
	LatticeWeights(ConstWordLatticeRef lattice) :
	    DfsState(lattice),
	    minMax(std::make_pair(Fsa::Weight(Core::Type<f32>::max), Fsa::Weight(Core::Type<f32>::min))) {}

	virtual void discoverState(Fsa::ConstStateRef sp) {
	    for (Fsa::State::const_iterator a = sp->begin(); a != sp->end(); ++ a) {
		minMax.first = min(minMax.first, a->weight());
		minMax.second = max(minMax.second, a->weight());
	    }
	    if (sp->isFinal()) {
		minMax.first = min(minMax.first, sp->weight_);
		minMax.second = max(minMax.second, sp->weight_);
	    }
	}
    };

    std::pair<Fsa::Weight,Fsa::Weight> minMaxWeights(ConstWordLatticeRef lattice)
    {
	LatticeWeights weights(lattice);
	weights.dfs();
	return weights.minMax;
    }

} // namespace Lattice

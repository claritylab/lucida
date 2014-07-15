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
#include "Arithmetic.hh"
#include "Posterior.hh"
#include <Fsa/Arithmetic.hh>
#include <Fsa/Basic.hh>
#include <Fsa/Dfs.hh>
#include <Fsa/Prune.hh>
#include <Fsa/Sssp.hh>
#include <Fsa/Sssp4SpecialSymbols.hh>
#include <Core/Utility.hh>

#include <Fsa/Output.hh>

namespace Lattice {

    ConstWordLatticeRef posterior(ConstWordLatticeRef lattice, Fsa::Weight &totalInv, s32 tol)
    {
	WordLattice *result = new WordLattice();
	result->setWordBoundaries(lattice->wordBoundaries());
	result->setFsa(Fsa::posterior64(lattice->mainPart(), totalInv, tol), "posterior");
	return ConstWordLatticeRef(result);
    }

    ConstWordLatticeRef posterior(ConstWordLatticeRef lattice, f64 &totalInv, s32 tol)
    {
	WordLattice *result = new WordLattice();
	result->setWordBoundaries(lattice->wordBoundaries());
	result->setFsa(Fsa::posterior64(lattice->mainPart(), totalInv, tol), "posterior");
	return ConstWordLatticeRef(result);
    }

    ConstWordLatticeRef posterior(ConstWordLatticeRef lattice, s32 tol)
    {
	Fsa::Weight totalInv;
	return posterior(lattice, totalInv, tol);
    }

    class CheckPosteriorDfsState : public DfsState
    {
    private:
	class Accumulators : public std::vector<Fsa::Accumulator*>
	{
	    typedef std::vector<Fsa::Accumulator*> Precursor;
	public:
	    Accumulators(size_t n) : Precursor(n, 0) {}
	    ~Accumulators() {
		for (iterator it = begin(); it != end(); ++ it) {
		    delete *it;
		}
	    }
	};
    private:
	Accumulators accumulators_;
	s32 tolerance_;
    public:
	CheckPosteriorDfsState(ConstWordLatticeRef, s32);
	virtual ~CheckPosteriorDfsState() {}

	virtual void discoverState(Fsa::ConstStateRef sp);
	virtual void finish();
    };

    CheckPosteriorDfsState::CheckPosteriorDfsState(
	ConstWordLatticeRef lattice, s32 tolerance) :
	DfsState(lattice),
	accumulators_(lattice->maximumTime()),
	tolerance_(tolerance)
    {
	require(fsa_->semiring() == Fsa::LogSemiring);
    }

    void CheckPosteriorDfsState::discoverState(Fsa::ConstStateRef sp)
    {
	const Speech::TimeframeIndex tbeg = wordBoundaries_->time(sp->id());
	for (Fsa::State::const_iterator a = sp->begin(); a != sp->end(); ++ a) {
	    const Speech::TimeframeIndex tend = wordBoundaries_->time(fsa_->getState(a->target())->id());
	    for (Speech::TimeframeIndex t = tbeg; t < tend; ++ t) {
		if (!accumulators_[t]) {
		    accumulators_[t] = fsa_->semiring()->getCollector(a->weight());
		} else {
		    accumulators_[t]->feed(a->weight());
		}
	    }
	}
    }

    void CheckPosteriorDfsState::finish()
    {
	f32 maxDeviation = 0;
	for (Speech::TimeframeIndex t = 0; t < accumulators_.size() - 1; ++ t) {
	    const f32 sum = exp(-f32(accumulators_[t]->get()));
	    const f32 dev = Core::abs(sum - 1);
	    maxDeviation = maxDeviation < dev ? dev : maxDeviation;
	}
	verify(Core::isAlmostEqualUlp(maxDeviation, 0, tolerance_));
    }

    ConstWordLatticeRef checkPosterior(ConstWordLatticeRef lattice, s32 tolerance)
    {
	CheckPosteriorDfsState s(lattice, tolerance);
	s.dfs();
	return lattice;
    }

    ConstWordLatticeRef prune(ConstWordLatticeRef lattice, const Fsa::Weight &threshold, bool relative, bool backward, bool hasFailArcs)
    {
	bool hasInitialState = (lattice->mainPart()->initialStateId() != Fsa::InvalidStateId);
	if (f32(threshold) < Core::Type<f32>::max) {
	    WordLattice *result = new WordLattice;
	    result->setWordBoundaries(lattice->wordBoundaries());
	    if (backward) {
		if (hasFailArcs) {
			result->setFsa(Fsa::posteriorPrune4SpecialSymbols(lattice->mainPart(), threshold, relative), lattice->mainName());
		} else
			result->setFsa(Fsa::prunePosterior(lattice->mainPart(), threshold, relative), lattice->mainName());
	    } else
		result->setFsa(Fsa::pruneSync(lattice->mainPart(), threshold), lattice->mainName());
	    if (result->mainPart()->initialStateId() == Fsa::InvalidStateId and hasInitialState)
		Core::Application::us()->warning("During pruning the automaton became empty. result->mainPart()->initialStateId() == Fsa::InvalidStateId");
	    return ConstWordLatticeRef(result);
	}
	return lattice;
    }

} // namespace Lattice

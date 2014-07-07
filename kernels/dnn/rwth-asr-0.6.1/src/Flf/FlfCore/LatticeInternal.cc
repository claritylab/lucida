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
#include "LatticeInternal.hh"

namespace Ftl {

    // -------------------------------------------------------------------------
    template<>
    void WrapperAutomaton<Fsa::Automaton, Flf::Lattice>::setMaster(Fsa::ConstAutomatonRef f) {}

    template<>
    void WrapperAutomaton<Flf::Lattice, Flf::Lattice>::setMaster(Flf::ConstLatticeRef f) {
	setBoundaries(f->getBoundaries());
    }

    template<>
    void SlaveAutomaton<Flf::Lattice>::setMaster(Flf::ConstLatticeRef f) {
	setBoundaries(f->getBoundaries());
    }
    // -------------------------------------------------------------------------

}


namespace Flf {

    // -------------------------------------------------------------------------
    class RescoreLattice::StateRescorer {
    protected:
	const RescoreLattice &l;
	const Lattice &predecessor;
	const Semiring &semiring;
    public:
	StateRescorer(const RescoreLattice *l) : l(*l), predecessor(*l->fsa_), semiring(*l->semiring()) {}
	virtual ~StateRescorer() {}
	virtual ConstStateRef operator() (Fsa::StateId sid) const = 0;
    };

    class CloningStateRescorer : public RescoreLattice::StateRescorer {
	typedef RescoreLattice::StateRescorer Precursor;
    public:
	CloningStateRescorer(const RescoreLattice *l) : Precursor(l) {}
	virtual ConstStateRef operator() (Fsa::StateId sid) const {
	    State *sp = clone(semiring, predecessor.getState(sid));
	    l.rescore(sp);
	    return ConstStateRef(sp);
	}
    };

    class CachingInPlaceStateRescorer : public RescoreLattice::StateRescorer {
	typedef RescoreLattice::StateRescorer Precursor;
    private:
	mutable Core::Vector<ConstStateRef> cache_;
    public:
	CachingInPlaceStateRescorer(const RescoreLattice *l) : Precursor(l) {}
	virtual ConstStateRef operator() (Fsa::StateId sid) const {
	    cache_.grow(sid);
	    ConstStateRef sr = cache_[sid];
	    if (!sr) {
		sr = cache_[sid] = predecessor.getState(sid);
		l.rescore(const_cast<State*>(sr.get()));
	    }
	    return sr;
	}
    };

    class InPlaceStateRescorer : public RescoreLattice::StateRescorer {
	typedef RescoreLattice::StateRescorer Precursor;
    public:
	InPlaceStateRescorer(const RescoreLattice *l) : Precursor(l) {}
	virtual ConstStateRef operator() (Fsa::StateId sid) const {
	    ConstStateRef sr = predecessor.getState(sid);
	    l.rescore(const_cast<State*>(sr.get()));
	    return sr;
	}
    };

    RescoreLattice::RescoreLattice(ConstLatticeRef l, RescoreMode rescoreMode) : Precursor(l) {
	switch (rescoreMode) {
	case RescoreModeClone:
	    rescore_ = new CloningStateRescorer(this);
	    break;
	case RescoreModeInPlaceCache:
	    rescore_ = new CachingInPlaceStateRescorer(this);
	    break;
	case RescoreModeInPlace:
	    rescore_ = new InPlaceStateRescorer(this);
	    break;
	default:
	    rescore_ = 0;
	    defect();
	}
    }

    RescoreLattice::~RescoreLattice() {
	delete rescore_;
    }

    ConstStateRef RescoreLattice::getState(Fsa::StateId sid) const {
	return (*rescore_)(sid);
    }
    // -------------------------------------------------------------------------

} // namespace Flf

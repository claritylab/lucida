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
#ifndef _FLF_CORE_LATTICE_INTERNAL_HH
#define _FLF_CORE_LATTICE_INTERNAL_HH

#include <Fsa/tAutomaton.hh>
#include <Fsa/Automaton.hh>

#include "Types.hh"
#include "Lattice.hh"

namespace Ftl {

    // -------------------------------------------------------------------------
    /**
     * Specialisations for flf specific behavior
     **/
    template<>
    void WrapperAutomaton<Fsa::Automaton, Flf::Lattice>::setMaster(Fsa::ConstAutomatonRef fsa);

    template<>
    void WrapperAutomaton<Flf::Lattice, Flf::Lattice>::setMaster(Flf::ConstLatticeRef fsa);

    template<>
    void SlaveAutomaton<Flf::Lattice>::setMaster(Flf::ConstLatticeRef fsa);
    // -------------------------------------------------------------------------

}


namespace Flf {

    // -------------------------------------------------------------------------
    /**
     * Wrapper lattice/automaton;
     * generic fsa-to-flf and flf-to-fsa.
     *
     **/
    typedef Ftl::WrapperAutomaton<Fsa::Automaton, Lattice> FsaToFlfWrapperLattice;

    typedef Ftl::WrapperAutomaton<Lattice, Lattice> FlfToFlfWrapperLattice;


    /**
     * Slave lattice;
     * specialisation of wrapper lattice, generic flf-to-flf.
     *
     **/
    typedef Ftl::SlaveAutomaton<Lattice> SlaveLattice;


    /**
     * Modify lattice;
     * specialisation of slave lattice, structure preserving flf-to-flf.
     *
     **/
    typedef Ftl::ModifyAutomaton<Lattice> ModifyLattice;


    /**
     * Rescore lattice;
     * specialisation of modify lattice, structure preserving flf-to-flf.
     *
     * Framework for modifying scores, but not touching the structure,
     * see ModifyState.
     *
     * For each state the method "rescore" is called.
     *
    **/
    class RescoreLattice : public SlaveLattice {
	typedef SlaveLattice Precursor;
    public:
	class StateRescorer;
	friend class StateRescorer;
    private:
	const StateRescorer *rescore_;
    public:
	RescoreLattice(ConstLatticeRef l, RescoreMode rescoreMode);
	virtual ~RescoreLattice();
	virtual ConstStateRef getState(Fsa::StateId sid) const;
	virtual void rescore(State *sp) const = 0;
    };
    // -------------------------------------------------------------------------


    // -------------------------------------------------------------------------
    inline State * clone(const Semiring &semiring, ConstStateRef sr) {
	State *sp = new State(*sr);
	if (sp->isFinal())
	    sp->weight_ = semiring.clone(sp->weight_);
	for (State::iterator a = sp->begin(); a != sp->end(); ++a)
	    a->weight_ = semiring.clone(a->weight_);
	return sp;
    }
    // -------------------------------------------------------------------------

} // namespace Flf
#endif // _FLF_CORE_LATTICE_INTERNAL_HH

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
#ifndef _FLF_CORE_TRAVERSE_HH
#define _FLF_CORE_TRAVERSE_HH

#include <Fsa/tDfs.hh>
#include "Lattice.hh"

namespace Flf {

    /**
     * Traverse lattice using dfs, see Fsa/tDfs.hh
     **/
    typedef Ftl::DfsState<Lattice> DfsState;


    /**
     * Traverse a lattice.
     *
     * traverseDfs:
     *  Traverse lattice using the dfs algorithm.
     * traverseInTopologicalOrder:
     *  Traverse lattice in topological order; lattice has to be acyclic.
     * traverse:
     *   Use fastes way to traverse lattice, i.e.
     *   if pre-calculated topological order exists, use traverseInTopologicalOrder,
     *   else, use traverseDfs.
     *
     **/
    class TraverseState {
    protected:
	ConstLatticeRef l;
    protected:
	virtual void exploreState(ConstStateRef sr) {}
	virtual void exploreArc(ConstStateRef from, const Arc &a) {}
    public:
	TraverseState(ConstLatticeRef l) : l(l) {}
	virtual ~TraverseState() {}
	void traverseDfs();
	void traverseInTopologicalOrder();
	virtual void traverse();
    };

} // namespace Flf

#endif // _FLF_CORE_TRAVERSE_HH

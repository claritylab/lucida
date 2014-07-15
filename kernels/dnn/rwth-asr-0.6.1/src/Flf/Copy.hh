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
#ifndef _FLF_COPY_HH
#define _FLF_COPY_HH

#include "FlfCore/Lattice.hh"
#include "Network.hh"

namespace Flf {

    /**
     * Behaviour of copy and deepCopy:
     * - l is traversed exactly once, i.e. getState is called exactly once for every accessible state
     * Behaviour of std::pair<bool,bool> copyXXX(ConstLatticeRef l,
     *                                           StaticLattice *staticLattice,
     *                                           StaticBoundaries *staticBoundaries);
     * - staticLattice and staticBoundaries are cleared; the boundaries object of staticLattice is not touched
     * - if !l then (false,false) is returned
     * - else if boundaries of l are invalid or staticBoundaries is null, then (true,false) is returned
     * - else (true,true) is returned
     **/
    /**
	 * Persistent:
	 *  Copy state references, i.e. the states are made persistent
	 *  -> used for random or repeated state access (and insito state modifications)
     * Copy:
     *  Copy states, make weights persistent, i.e. states are copied, but not the weights
	 *  -> used for topology modifications (and insito weight modifications)
     * Deep Copy:
     *  Copy states and weights, i.e. states as well as any weight are copied
	 *  -> used for weight (and topology) modifications
     **/
    std::pair<bool,bool> persistent(ConstLatticeRef l, StaticLattice *staticLattice, StaticBoundaries *staticBoundaries);
    std::pair<bool,bool> copy(ConstLatticeRef l, StaticLattice *staticLattice, StaticBoundaries *staticBoundaries);
    std::pair<bool,bool> deepCopy(ConstLatticeRef l, StaticLattice *staticLattice, StaticBoundaries *staticBoundaries);
    ConstLatticeRef persistent(ConstLatticeRef l);
    ConstLatticeRef copy(ConstLatticeRef l);
    ConstLatticeRef deepCopy(ConstLatticeRef l);


    /**
     * Normalize state numbering,
     * Copy,
     * copy states, but no weights
     * Deep Copy,
     * copy states and weights
     **/
    std::pair<bool,bool> normalizeCopy(ConstLatticeRef l, StaticLattice *staticLattice, StaticBoundaries *staticBoundaries);
    std::pair<bool,bool> normalizeDeepCopy(ConstLatticeRef l, StaticLattice *staticLattice, StaticBoundaries *staticBoundaries);
    ConstLatticeRef normalizeCopy(ConstLatticeRef l);
    ConstLatticeRef normalizeDeepCopy(ConstLatticeRef l);


    /**
     * Behaviour of copyBoundaries:
     * - l is traversed exactly once and only boundaries associated with accessible states are copied
     * Behaviour of bool copyBoundaries(ConstLatticeRef l,
     *                                  StaticBoundaries *staticBoundaries);
     * - staticBoundaries are cleared; the boundaries object of l is not touched
     * - if !l or if boundaries of l are invalid then false is returned
     * - else true is returned
     * Behaviour of ConstLatticeRef copyBoundaries(ConstLatticeRef l):
     * - the boundaries of l are copied and set at l and l is returned, i.e. l is modified!
     **/
    bool copyBoundaries(ConstLatticeRef l, StaticBoundaries *staticBoundaries);
    ConstLatticeRef copyBoundaries(ConstLatticeRef l);


    NodeRef createCopyNode(const std::string &name, const Core::Configuration &config);

} // namespace Flf

#endif // _FLF_COPY_HH

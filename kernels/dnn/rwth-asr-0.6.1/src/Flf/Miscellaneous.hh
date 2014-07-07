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
#ifndef _FLF_MISCELLANEOUS_HH
#define _FLF_MISCELLANEOUS_HH

#include "FlfCore/Lattice.hh"
#include "Network.hh"

namespace Flf {

    /**
     * Fits the lattice into [begin, end]:
     * - begin and end are relative to the lattices initial state
     * - fitting lattice has exactly one initial state with time 0 and one
     *   final state with time end-begin, i.e. all pathes have the same length
     * - the time of all states but the final state is less than end (and >= 0)
     * - for each path the in origianl lattice there exist a path with
     *   the same score in the fitting lattice; those are the only pathes
     *   in the fitting lattice (i.e. according to the begin and end times
     *   words at the beginning or end of a path might be discarded).
     * - optional: the last arc carries the sentence-end-symbol
     * - if begin and end time are not given, then begin is 0 and end time
     *   is the max time over all states.
     **/
    ConstLatticeRef fit(ConstLatticeRef l, bool forceSentenceEndLabels = false);
    ConstLatticeRef fit(ConstLatticeRef l, s32 startTime, s32 endTime, bool forceSentenceEndLabels = false);
    NodeRef createFitLatticeNode(const std::string &name, const Core::Configuration &config);


    /**
     * Remove arcs that
     * - close a cycle
     * - have an invalid label id
     * - have an invalid or zero score at one dimension
     **/
    StaticLatticeRef cleanUp(ConstLatticeRef l);
    NodeRef createCleanUpNode(const std::string &name, const Core::Configuration &config);

} // namespace Flf

#endif // _FLF_MISCELLANEOUS_HH

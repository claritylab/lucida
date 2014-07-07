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
#ifndef _LATTICE_POSTERIOR_HH
#define _LATTICE_POSTERIOR_HH

#include "Lattice.hh"

namespace Lattice {

    ConstWordLatticeRef posterior(ConstWordLatticeRef, Fsa::Weight &totalInv, s32 tol = 100);
    ConstWordLatticeRef posterior(ConstWordLatticeRef, f64 &totalInv, s32 tol = 100);
    ConstWordLatticeRef posterior(ConstWordLatticeRef, s32 tol = 100);
    ConstWordLatticeRef checkPosterior(ConstWordLatticeRef, s32 tolerance = 981668463);
    ConstWordLatticeRef prune(ConstWordLatticeRef, const Fsa::Weight &threshold, bool relative = true, bool backward = true, bool hasFailArcs = false);

}

#endif // _LATTICE_POSTERIOR_HH

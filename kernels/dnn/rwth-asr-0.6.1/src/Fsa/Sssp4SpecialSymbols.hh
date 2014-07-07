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
/*
 * Sssp4SpecialSymbols.hh
 *
 *  Created on: Mar 4, 2011
 *      Author: lehnen
 */

#ifndef _FSA_SSSP4SPECIALSYMBOLS_HH_
#define _FSA_SSSP4SPECIALSYMBOLS_HH_

#include "hSssp.hh"
#include "Automaton.hh"
#include "Types.hh"

namespace Fsa {
    typedef Ftl::StatePotentials<Weight> StatePotentials;

    /**
     * Super posterior
     *
     * Features:
     * 	- FAIL arcs
     *  - efficient implementations for Tropical and LogSemiring
     *  - 64 bit precision for LogSemiring
     *  - general implementation for every Semiring
     *
     * Assumptions:
     *  - only one FAIL arc per state
     *  - FAIL state include all arcs their parents have (weight is not important)
     *  - no cycles
     *
     * Drawbacks:
     *  - very hard to read :-(
     */
    ConstAutomatonRef posterior4SpecialSymbols(ConstAutomatonRef f);

    /**
     * same as posterior4SpecialSymbols, but return total flow of automaton
     * and prints a warning if the tolerance tol is exceeded.
     */
    ConstAutomatonRef posterior4SpecialSymbols(ConstAutomatonRef f, Weight &totalInv, s32 tol = 100);

    /**
     * Super best
     *
     * same features/assumptions as posterior4SpecialSymbols
     */
    ConstAutomatonRef best4SpecialSymbols(ConstAutomatonRef f);

    ConstAutomatonRef removeFailure4SpecialSymbols(ConstAutomatonRef f);

    ConstAutomatonRef posteriorPrune4SpecialSymbols(ConstAutomatonRef f, const Weight &threshold, bool relative = true);
}

#endif /* _FSA_SSSP4SPECIALSYMBOLS_HH_ */

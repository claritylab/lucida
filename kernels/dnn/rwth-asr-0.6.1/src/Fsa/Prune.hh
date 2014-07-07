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
#ifndef _FSA_PRUNE_HH
#define _FSA_PRUNE_HH

#include "Automaton.hh"
#include "Sssp.hh"

namespace Fsa {
	/**
	 * Pruning based on posteriors. If the posterior of an arc is below the minimum posterior of the automaton, it is omitted.
	 * Minimum is defined as minimum of all arcs extended by the threshold if relative==true,
	 * else minimum is defined as the backward potential of the initial state extended by the threshold.
	 *
	 * prunePosterior(ConstAutomatonRef, const Weight &threshold, bool relative = true) is used by command "prune" of the Fsa tool and the Lattice Processor.
	 */
    ConstAutomatonRef prunePosterior(ConstAutomatonRef f, const Weight &threshold, bool relative = true);
    ConstAutomatonRef prunePosterior(ConstAutomatonRef f, const Weight &threshold,const StatePotentials &fw, const StatePotentials& bw, bool relative = true);
    ConstAutomatonRef pruneSync(ConstAutomatonRef f, const Weight &threshold);
} // namespace Fsa

#endif // _FSA_PRUNE_HH

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
 * Sssp4SpecialSymbols.cc
 *
 *  Created on: Mar 4, 2011
 *      Author: lehnen
 */

#include "Sssp4SpecialSymbols.hh"
#include "tSssp4SpecialSymbols.hh"
#include "Semiring64.hh"

namespace Fsa {


	ConstAutomatonRef posterior4SpecialSymbols(ConstAutomatonRef f) {
		return Ftl::posterior4SpecialSymbols<Automaton>(f);
	}

	ConstAutomatonRef posterior4SpecialSymbols(ConstAutomatonRef f, Weight &totalInv, s32 tol) {
		if (f->semiring() != Fsa::LogSemiring)
			return Ftl::posterior4SpecialSymbols<Automaton>(f, totalInv, tol);
		else
			return Ftl::posterior4SpecialSymbols<Automaton, Semiring64, Weight64>(f, totalInv, tol);
	}

	ConstAutomatonRef best4SpecialSymbols(ConstAutomatonRef f) {
		return Ftl::best4SpecialSymbols<Automaton>(f);
	}

	ConstAutomatonRef removeFailure4SpecialSymbols(ConstAutomatonRef f) {
		return Ftl::removeFailure4SpecialSymbols<Automaton>(f);
	}

	ConstAutomatonRef posteriorPrune4SpecialSymbols(ConstAutomatonRef f, const Weight &threshold, bool relative) {
		if (f->semiring() != Fsa::LogSemiring)
			return Ftl::prunePosterior4SpecialSymbols<Automaton>(f, threshold, relative);
		else
			return Ftl::prunePosterior4SpecialSymbols<Automaton, Semiring64, Weight64>(f, threshold, relative);
	}
}

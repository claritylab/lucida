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
#include "RemoveEpsilons.hh"
#include <Fsa/RemoveEpsilons.hh>
#include <Fsa/Sssp4SpecialSymbols.hh>
#include "Basic.hh"

namespace Lattice {

    struct Remover
    {
	Remover() {}
	Fsa::ConstAutomatonRef modify(Fsa::ConstAutomatonRef fsa) {
	    return Fsa::removeEpsilons(fsa);
	}
    };

    ConstWordLatticeRef removeEpsilons(ConstWordLatticeRef l)
    {
	Remover r;
	return apply(l, r);
    }

    struct FailureRemover
    {
	FailureRemover() {}
	Fsa::ConstAutomatonRef modify(Fsa::ConstAutomatonRef fsa) {
		return Fsa::removeFailure4SpecialSymbols(fsa);
	}
    };

    ConstWordLatticeRef removeFailures(ConstWordLatticeRef l)
    {
	FailureRemover r;
	return apply(l, r);
    }

} //namespace Lattice

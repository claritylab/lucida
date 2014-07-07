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
#include <Core/ProgressIndicator.hh>

#include "tRational.hh"
#include "Rational.hh"
#include "Types.hh"

namespace Fsa {
    ConstAutomatonRef identity(ConstAlphabetRef ab, ConstSemiringRef sr) {
	return Ftl::identity<Automaton>(ab, sr);
    }

    ConstAutomatonRef closure(ConstAutomatonRef f) {
	return Ftl::closure<Automaton>(f);
    }

    ConstAutomatonRef kleeneClosure(ConstAutomatonRef f) {
	return Ftl::kleeneClosure<Automaton>(f);
    }

    ConstAutomatonRef complement(ConstAutomatonRef f) {
	return Ftl::complement<Automaton>(f);
    }

    ConstAutomatonRef concat(const Core::Vector<ConstAutomatonRef> &f) {
	return Ftl::concat<Automaton>(f);
    }

    ConstAutomatonRef concat(ConstAutomatonRef f1, ConstAutomatonRef f2) {
	return Ftl::concat<Automaton>(f1, f2);
    }

    ConstAutomatonRef unite(const Core::Vector<ConstAutomatonRef> &f, const Core::Vector<Weight> &initialWeights) {
	return Ftl::unite<Automaton>(f, initialWeights);
    }

    ConstAutomatonRef unite(ConstAutomatonRef f1, ConstAutomatonRef f2) {
	return Ftl::unite<Automaton>(f1, f2);
    }

    ConstMappingRef mapToSubAutomaton(ConstAutomatonRef f, u32 subAutomaton) {
	return Ftl::mapToSubAutomaton<Automaton>(f, subAutomaton);
    }

    ConstAutomatonRef fuse(const Core::Vector<ConstAutomatonRef> &f) {
	return Ftl::fuse<Automaton>(f);
    }

    ConstAutomatonRef fuse(ConstAutomatonRef f1, ConstAutomatonRef f2) {
	return Ftl::fuse<Automaton>(f1, f2);
    }

    ConstAutomatonRef transpose(ConstAutomatonRef f, bool progress)
    { return Ftl::transpose<Automaton>(f, progress); }


} // namespace Fsa

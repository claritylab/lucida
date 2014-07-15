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
#ifndef _FSA_RATIONAL_HH
#define _FSA_RATIONAL_HH

#include "Automaton.hh"
#include <Core/Vector.hh>
#include "Mapping.hh"

namespace Fsa {

    ConstAutomatonRef identity(ConstAlphabetRef, ConstSemiringRef);
    ConstAutomatonRef closure(ConstAutomatonRef f);
    ConstAutomatonRef kleeneClosure(ConstAutomatonRef f);
    ConstAutomatonRef complement(ConstAutomatonRef f);
    ConstAutomatonRef concat(const Core::Vector<ConstAutomatonRef> &f);
    ConstAutomatonRef concat(ConstAutomatonRef, ConstAutomatonRef);
    ConstAutomatonRef unite(const Core::Vector<ConstAutomatonRef> &f,
			    const Core::Vector<Weight> &initialWeights = Core::Vector<Weight>());
    ConstAutomatonRef unite(ConstAutomatonRef, ConstAutomatonRef);
    ConstMappingRef mapToSubAutomaton(ConstAutomatonRef f, u32 subAutomaton);
    ConstAutomatonRef fuse(const Core::Vector<ConstAutomatonRef> &f);
    ConstAutomatonRef fuse(ConstAutomatonRef, ConstAutomatonRef);
    ConstAutomatonRef transpose(ConstAutomatonRef f, bool progress = false);


    ConstAutomatonRef fuse(const Core::Vector<ConstAutomatonRef> &f);
    ConstAutomatonRef fuse(ConstAutomatonRef, ConstAutomatonRef);
} // namespace Fsa

#endif // _FSA_RATIONAL_HH

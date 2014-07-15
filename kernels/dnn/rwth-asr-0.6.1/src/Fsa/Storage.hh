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
#ifndef _FSA_STORAGE_HH
#define _FSA_STORAGE_HH

#include "tStorage.hh"
#include "Automaton.hh"
#include "Types.hh"

namespace Fsa {
    typedef Ftl::StorageAutomaton<Automaton> StorageAutomaton;

    void copy(StorageAutomaton *f, ConstAutomatonRef f2);
    void copy(StorageAutomaton *f, const std::string &str);

} // namespace Fsa

// According to the ISO C++ standard 14.7.3/2:
// Explicit template specializations have to be declared in
// the same namespace of which the template is a member
namespace Ftl {
    template<>
    StorageAutomaton<Fsa::Automaton>::StorageAutomaton(Fsa::Type type);
} // namespace Ftl

#endif // _FSA_STORAGE_HH

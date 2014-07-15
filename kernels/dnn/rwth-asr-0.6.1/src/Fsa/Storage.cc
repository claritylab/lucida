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
#include "tCopy.hh"
#include "Storage.hh"

namespace Ftl {
    template<>
    StorageAutomaton<Fsa::Automaton>::StorageAutomaton(Fsa::Type type) :
	type_(type),
	semiring_(Fsa::UnknownSemiring),
	initial_(Fsa::InvalidStateId)
    {
	// Inconsistent with generic Ftl::StorageAutomaton::StorageAutomaton !
	// There we set both Fsa::PropertyStorage and Fsa::PropertyCached to true.
	// The only position in Fsa where Fsa::PropertyCached is used, is
	// Ftl::cache(...) and there both bits are checked.
	/*! @todo: check what is correct here */
	Fsa::Automaton::setProperties(Fsa::PropertyStorage | Fsa::PropertyCached, Fsa::PropertyStorage);
    }
}
namespace Fsa
{
    void copy(StorageAutomaton *f, ConstAutomatonRef f2)
    { Ftl::copy<Automaton>(f, f2); }
    void copy(StorageAutomaton *f, const std::string &str)
    { Ftl::copy<Automaton>(f, str); }

} // namespace Fsa

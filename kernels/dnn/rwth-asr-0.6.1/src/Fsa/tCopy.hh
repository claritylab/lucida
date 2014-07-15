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
#ifndef _T_FSA_COPY_HH
#define _T_FSA_COPY_HH

#include <Core/ReferenceCounting.hh>
#include "Hash.hh"
#include "tStorage.hh"
#include "tStatic.hh"

namespace Ftl {

	template<class _Automaton> void copy(StorageAutomaton<_Automaton> *f,
			typename _Automaton::ConstRef f2);
	template<class _Automaton> void copy(StorageAutomaton<_Automaton> *f,
			const std::string &str);
	template<class _Automaton> void copy(StorageAutomaton<_Automaton> *f,
			const Fsa::Hash<std::string, Core::StringHash> &tokens,
			u32 sausage = 0);

	template<class _Automaton> Core::Ref<StaticAutomaton<_Automaton> >
			staticCopy(typename _Automaton::ConstRef);
	template<class _Automaton> Core::Ref<StaticAutomaton<_Automaton> >
			staticCompactCopy(typename _Automaton::ConstRef);
	template<class _Automaton> Core::Ref<StaticAutomaton<_Automaton> >
			staticCopy(const std::string &str,
					typename _Automaton::ConstSemiringRef);
} // namespace Ftl

#include "tCopy.cc"

#endif // _T_FSA_COPY_HH

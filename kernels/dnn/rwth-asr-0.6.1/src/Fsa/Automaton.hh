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
#ifndef _FSA_AUTOMATON_HH
#define _FSA_AUTOMATON_HH

#include "Semiring.hh"
#include "tAutomaton.hh"

namespace Fsa
{
class Automaton : public Ftl::Automaton<Semiring>
{
	typedef Fsa::Automaton Self;
	typedef Ftl::Automaton<Semiring> Precursor;
public:
	typedef Core::Ref<Self> Ref;
	typedef Core::Ref<const Self> ConstRef;
public:
	virtual ConstSemiringRef semiring() const
	{
		return UnknownSemiring;
	}
};

typedef Automaton::ConstRef ConstAutomatonRef;
typedef Automaton::Arc Arc;
typedef Automaton::State State;
typedef Automaton::StateRef StateRef;
typedef Automaton::ConstStateRef ConstStateRef;

typedef Ftl::SlaveAutomaton<Automaton> SlaveAutomaton;
typedef Ftl::ModifyAutomaton<Automaton> ModifyAutomaton;

} // namespace Fsa

#endif // _FSA_AUTOMATON_HH

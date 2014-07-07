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
 * tSssp4SpecialSymbols.hh
 *
 *  Created on: Mar 2, 2011
 *      Author: lehnen
 */

#ifndef _T_FSA_SSSP4SPECIALSYMBOLS_HH
#define _T_FSA_SSSP4SPECIALSYMBOLS_HH

namespace Ftl {
	template<class _Automaton>
	typename _Automaton::ConstRef posterior4SpecialSymbols(typename _Automaton::ConstRef f);

	template<class _Automaton>
	typename _Automaton::ConstRef posterior4SpecialSymbols(typename _Automaton::ConstRef f, typename _Automaton::Weight &totalInv, s32 tol);

	template<class _Automaton, class _Semiring, class _Weight>
	typename _Automaton::ConstRef posterior4SpecialSymbols(typename _Automaton::ConstRef f, typename _Automaton::Weight &totalInv, s32 tol);

	template<class _Automaton>
	typename _Automaton::ConstRef best4SpecialSymbols(typename _Automaton::ConstRef f);

	template<class _Automaton>
	typename _Automaton::ConstRef removeFailure4SpecialSymbols(typename _Automaton::ConstRef f);

	template<class _Automaton>
	typename _Automaton::ConstRef prunePosterior4SpecialSymbols(typename _Automaton::ConstRef f, const typename _Automaton::Weight &threshold, bool relative);

	template<class _Automaton, class _Semiring, class _Weight>
	typename _Automaton::ConstRef prunePosterior4SpecialSymbols(typename _Automaton::ConstRef f, const typename _Automaton::Weight &threshold, bool relative);
} // namespace Ftl

#include "tSssp4SpecialSymbols.cc"

#endif /* _T_FSA_SSSP4SPECIALSYMBOLS_HH */

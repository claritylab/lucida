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
#ifndef _T_FSA_CACHE_HH
#define _T_FSA_CACHE_HH

#include <Core/ReferenceCounting.hh>
#include <Core/Types.hh>

namespace Ftl {

    /**
     * Caches states of an automaton. Helps to substantially reduce
     * computational complexity by using more memory. Caching is only
     * done for none-storage (e.g. static or packed) and non-cache
     * automata. Currently complete states together with their
     * associated outgoing arcs are cached. Uses a simple 8-bit depth
     * ageing algorithm.
     * @param f the automaton we want to cache
     * @param maxAge the number of calls to getState() necessary to
     *   trigger one ageing step
     * @return the cached automaton
     **/
    template<class _Automaton>
    Core::Ref<const _Automaton> cache(Core::Ref<const _Automaton> f, u32 maxAge = 10000);

} // namespace Ftl

#include "tCache.cc"

#endif // _T_FSA_CACHE_HH

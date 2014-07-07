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
#ifndef _T_FSA_PROPERTIES_HH
#define _T_FSA_PROPERTIES_HH

#include "Types.hh"

namespace Ftl {
    /**
     * Get properties.
     * If necessary the requested properties are computed using
     * isAcyclic() or isLinear().
     * @see Fsa/Types.hh
     * @param f automaton to get the properties for
     * @param properties the properties to get
     * @return properties of the automaton
     **/
    template<class _Automaton>
    Fsa::Property getProperties(typename _Automaton::ConstRef f, Fsa::Property properties = Fsa::PropertyAll);

    /**
     * Test properties.
     * If necessary the requested properties are computed.
     * This is a convienance function to getProperties().
     * @see Fsa/Types.hh
     * @param f automaton to test the properties for
     * @param properties the properties to test for
     * @return true if the automaton has any of the properties
     * passed to the funtion.
     **/
    template<class _Automaton>
    bool hasProperties(typename _Automaton::ConstRef f, Fsa::Property properties) {
	return getProperties<_Automaton>(f, properties) & properties;
    }
} // namespace Ftl

#include "tProperties.cc"

#endif // _T_FSA_PROPERTIES_HH

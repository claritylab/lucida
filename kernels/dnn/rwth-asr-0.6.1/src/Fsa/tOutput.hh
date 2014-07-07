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
#ifndef _T_FSA_OUTPUT_HH
#define _T_FSA_OUTPUT_HH

#include <iostream>

#include "tResources.hh"
#include "Types.hh"

namespace Ftl {

    /**
     * storage
     **/
    template<class _Automaton>
    bool write(      const Resources<_Automaton> &resources, typename _Automaton::ConstRef f,
		     const std::string &format, std::ostream &o, Fsa::StoredComponents what = Fsa::storeAll, bool progress = false);
    template<class _Automaton>
    bool write(      const Resources<_Automaton> &resources, typename _Automaton::ConstRef f,
		     const std::string &file, Fsa::StoredComponents what = Fsa::storeAll, bool progress = false);

    template<class _Automaton>
    bool writeAtt(   const Resources<_Automaton> &resources, typename _Automaton::ConstRef f,
		     std::ostream &o, Fsa::StoredComponents what = Fsa::storeAll, bool progress = false);
    template<class _Automaton>
    bool writeAtt(   const Resources<_Automaton> &resources, typename _Automaton::ConstRef f,
		     const std::string &file, Fsa::StoredComponents what = Fsa::storeAll, bool progress = false);

    template<class _Automaton>
    bool writeBinary(const Resources<_Automaton> &resources, typename _Automaton::ConstRef f,
		     std::ostream &o, Fsa::StoredComponents what = Fsa::storeAll, bool progress = false);
    template<class _Automaton>
    bool writeBinary(const Resources<_Automaton> &resources, typename _Automaton::ConstRef f,
		     const std::string &file, Fsa::StoredComponents what = Fsa::storeAll, bool progress = false);

    // default parameters do not work here because of Fsa::getResources()
    template<class _Automaton>
    bool writeLinear(const Resources<_Automaton> &resources, typename _Automaton::ConstRef f,
		     std::ostream &o, Fsa::StoredComponents what, bool progress, bool printAll);
    template<class _Automaton>
    bool writeLinear(const Resources<_Automaton> &resources, typename _Automaton::ConstRef f,
		 std::ostream &o, Fsa::StoredComponents what = Fsa::storeAll, bool progress = false);
    template<class _Automaton>
    bool writeLinear(const Resources<_Automaton> &resources, typename _Automaton::ConstRef f,
		     const std::string &file, Fsa::StoredComponents what = Fsa::storeAll, bool progress = false);

    template<class _Automaton>
    bool writeXml(   const Resources<_Automaton> &resources, typename _Automaton::ConstRef f,
		     std::ostream &o, Fsa::StoredComponents what = Fsa::storeAll, bool progress = false);
    template<class _Automaton>
    bool writeXml(   const Resources<_Automaton> &resources, typename _Automaton::ConstRef f,
		     const std::string &file, Fsa::StoredComponents what = Fsa::storeAll, bool progress = false);

    template<class _Automaton>
    bool writeTrXml( const Resources<_Automaton> &resources, typename _Automaton::ConstRef f,
		     std::ostream &o, Fsa::StoredComponents what = Fsa::storeAll, bool progress = false);
    template<class _Automaton>
    bool writeTrXml( const Resources<_Automaton> &resources, typename _Automaton::ConstRef f,
		     const std::string &file, Fsa::StoredComponents what = Fsa::storeAll, bool progress = false);

} // namespace Ftl

#include "tOutput.cc"

#endif // _T_FSA_OUTPUT_HH

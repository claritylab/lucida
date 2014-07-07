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
#ifndef _T_FSA_INPUT_HH
#define _T_FSA_INPUT_HH

#include "tResources.hh"
#include "tStorage.hh"

namespace Ftl {
    template<class _Automaton>
    typename _Automaton::ConstRef read(const Resources<_Automaton> &resources, const std::string &format, std::istream &i);

    template<class _Automaton>
    bool read(const Resources<_Automaton> &resources, StorageAutomaton<_Automaton> *f, const std::string &format, std::istream &i);

    template<class _Automaton>
    typename _Automaton::ConstRef read(const Resources<_Automaton> &resources, const std::string &file);

    template<class _Automaton>
    bool read(      const Resources<_Automaton> &resources, StorageAutomaton<_Automaton> *f, const std::string &file);

    template<class _Automaton>
    bool readAtt(   const Resources<_Automaton> &resources, StorageAutomaton<_Automaton> *f, std::istream &i);

    template<class _Automaton>
    bool readBinary(const Resources<_Automaton> &resources, StorageAutomaton<_Automaton> *f, std::istream &i);

    template<class _Automaton>
    bool readLinear(const Resources<_Automaton> &resources, StorageAutomaton<_Automaton> *f, std::istream &i);

    template<class _Automaton>
    bool readXml(   const Resources<_Automaton> &resources, StorageAutomaton<_Automaton> *f, std::istream &i);

} // namespace Ftl

#include "tInput.cc"

#endif // _T_FSA_INPUT_HH

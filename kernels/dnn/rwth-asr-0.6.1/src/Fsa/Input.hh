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
#ifndef _FSA_INPUT_HH
#define _FSA_INPUT_HH

#include <iostream>
#include <string>
#include "Storage.hh"

namespace Fsa {
    ConstAutomatonRef read(const std::string &file, ConstSemiringRef semiring = TropicalSemiring);

    bool read(StorageAutomaton *f, const std::string &file);
    bool read(StorageAutomaton *f, const std::string &format, std::istream &i);

    bool readAtt(StorageAutomaton *f, std::istream &i);
    bool readBinary(StorageAutomaton *f, std::istream &i);
    bool readLinear(StorageAutomaton *f, std::istream &i);
    bool readXml(StorageAutomaton *f, std::istream &i);

} // namespace

#endif // _FSA_INPUT_HH

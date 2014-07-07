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
#ifndef _LATTICE_ARITHMETIC_HH
#define _LATTICE_ARITHMETIC_HH

#include "Lattice.hh"
#include <Core/Hash.hh>

namespace Lattice {

    ConstWordLatticeRef multiply(ConstWordLatticeRef, Fsa::Weight);
    ConstWordLatticeRef multiply(ConstWordLatticeRef, const std::vector<Fsa::Weight> &);
    ConstWordLatticeRef extend(ConstWordLatticeRef, Fsa::Weight);
    ConstWordLatticeRef extendFinal(ConstWordLatticeRef, Fsa::Weight);
    ConstWordLatticeRef expm(ConstWordLatticeRef);
    ConstWordLatticeRef linearCombination(ConstWordLatticeRef, const std::vector<Fsa::Weight> &);
    ConstWordLatticeRef linearCombination(ConstWordLatticeRef, const Core::StringHashMap<std::vector<Fsa::Weight> > &);
    ConstWordLatticeRef getParts(ConstWordLatticeRef, const std::vector<std::string> &parts);
    ConstWordLatticeRef getPart(ConstWordLatticeRef, const std::string &part);

    ConstWordLatticeRef isGreaterEqual(ConstWordLatticeRef l, Fsa::Weight threshold);

} // namespace Lattice

#endif // _LATTICE_ARITHMETIC_H

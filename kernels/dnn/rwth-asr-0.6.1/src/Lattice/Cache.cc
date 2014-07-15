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
#include "Cache.hh"
#include <Fsa/Cache.hh>

namespace Lattice {

    ConstWordLatticeRef cache(ConstWordLatticeRef l, u32 maxAge)
    {
	Core::Ref<WordLattice> result(new WordLattice);
	result->setWordBoundaries(l->wordBoundaries());
	for (size_t i = 0; i < l->nParts(); ++ i) {
	    result->setFsa(Fsa::cache(l->part(i), maxAge), l->name(i));
	}
	return result;
    }

} //namespace Lattice

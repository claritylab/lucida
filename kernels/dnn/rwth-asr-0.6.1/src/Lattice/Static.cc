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
#include "Static.hh"
#include <Fsa/Static.hh>

namespace Lattice {

    ConstWordLatticeRef staticCopy(
	ConstWordLatticeRef l,
	const std::vector<std::string> &names)
    {
	require(names.size() == l->nParts());
	Core::Ref<WordLattice> result(new WordLattice);
	result->setWordBoundaries(l->wordBoundaries());
	for (size_t i = 0; i < l->nParts(); ++i) {
	    result->setFsa(Fsa::staticCopy(l->part(i)), names[i]);
	}
	return result;
    }

    ConstWordLatticeRef staticCopy(
	ConstWordLatticeRef l,
	const std::string &name)
    {
	return staticCopy(l, std::vector<std::string>(1, name));
    }

    ConstWordLatticeRef staticCopy(ConstWordLatticeRef l)
    {
	std::vector<std::string> names(l->nParts());
	for (size_t i = 0; i < l->nParts(); ++i) {
	    names[i] = l->name(i);
	}
	return staticCopy(l, names);
    }

} //namespace Lattice

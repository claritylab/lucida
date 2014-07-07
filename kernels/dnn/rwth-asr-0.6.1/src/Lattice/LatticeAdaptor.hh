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
#ifndef _LATTICE_LATTICE_ADAPTOR_HH
#define _LATTICE_LATTICE_ADAPTOR_HH

#include <Lattice/Lattice.hh>
#include <Search/LatticeAdaptor.hh>

namespace Search {
class LatticeHandler;
}

namespace Lattice {

class WordLatticeAdaptor: public Search::LatticeAdaptor
{
public:
    WordLatticeAdaptor() {}
    WordLatticeAdaptor(const WordLattice *l) : l_(l) {}
    WordLatticeAdaptor(ConstWordLatticeRef l) : l_(l) {}
    virtual ~WordLatticeAdaptor() {}

    bool write(const std::string &id, Search::LatticeHandler *handler) const;

    ::Lattice::ConstWordLatticeRef get() const {
	return l_;
    }

    ::Lattice::ConstWordLatticeRef wordLattice(const Search::LatticeHandler *handler) const;

    bool empty() const {
	return !l_;
    }
protected:
    ::Lattice::ConstWordLatticeRef l_;
};

} // namespace Lattice {

#endif // _LATTICE_LATTICE_ADAPTOR_HH

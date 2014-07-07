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
#ifndef _FLF_LATTICE_ADAPTOR_HH
#define _FLF_LATTICE_ADAPTOR_HH

#include <Search/LatticeAdaptor.hh>
#include <Flf/FlfCore/Lattice.hh>

namespace Search {
class LatticeHandler;
}

namespace Flf {

class FlfLatticeAdaptor : public Search::LatticeAdaptor
{
    typedef ::Lattice::ConstWordLatticeRef ConstWordLatticeRef;
public:
    FlfLatticeAdaptor() {}
    FlfLatticeAdaptor(const FlfLatticeAdaptor &o) : l_(o.l_) {}
    explicit FlfLatticeAdaptor(ConstLatticeRef l) : l_(l) {}
    virtual ~FlfLatticeAdaptor() {}

    bool write(const std::string &id, Search::LatticeHandler *handler) const;
    ConstWordLatticeRef wordLattice(const Search::LatticeHandler *handler) const;
    virtual bool empty() const {
	return l_;
    }
    ConstLatticeRef get() const {
	return l_;
    }
private:
    ConstLatticeRef l_;
};


} // namespace Flf



#endif // _FLF_LATTICE_ADAPTOR_HH

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
#ifndef _FLF_LATTICE_HANDLER_HH
#define _FLF_LATTICE_HANDLER_HH

#include <Search/LatticeAdaptor.hh>
#include <Search/LatticeHandler.hh>

namespace Flf {

class LatticeArchiveReader;
class LatticeArchiveWriter;

class LatticeHandler : public Search::LatticeHandler
{
public:
    LatticeHandler(const Core::Configuration &c, Search::LatticeHandler *parent)
	: Search::LatticeHandler(c),
	  parent_(parent), reader_(0), writer_(0) {}

    virtual ~LatticeHandler();

    void setLexicon(Core::Ref<const Bliss::Lexicon> lexicon) {
	parent_->setLexicon(lexicon);
    }
    Core::Ref<const Bliss::Lexicon> lexicon() const {
	return parent_->lexicon();
    }


    bool write(const std::string &id, const WordLatticeAdaptor &l) {
	return parent_->write(id, l);
    }
    bool write(const std::string &id, const FlfLatticeAdaptor &l);
    bool write(const std::string &id, const WfstLatticeAdaptor &l) {
	return parent_->write(id, l);
    }

    Core::Ref<Search::LatticeAdaptor> read(const std::string &id, const std::string &name);

    ConstWordLatticeRef convert(const WordLatticeAdaptor &l) const {
	return parent_->convert(l);
    }
    ConstWordLatticeRef convert(const FlfLatticeAdaptor &l) const;
    ConstWordLatticeRef convert(const WfstLatticeAdaptor &l) const {
	return parent_->convert(l);
    }

private:
    bool createReader();
    bool createWriter();
    Search::LatticeHandler *parent_;
    LatticeArchiveReader *reader_;
    LatticeArchiveWriter *writer_;
};

} // namespace Flf


#endif // _FLF_LATTICE_HANDLER_HH

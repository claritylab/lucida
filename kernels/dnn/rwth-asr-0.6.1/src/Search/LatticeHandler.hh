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
#ifndef _SEARCH_LATTICEHANDLER_HH
#define _SEARCH_LATTICEHANDLER_HH

#include <Core/Component.hh>
#include <Bliss/Lexicon.hh>
#include <Search/LatticeAdaptor.hh>
#include <Lattice/Lattice.hh>

namespace Lattice {
class ArchiveReader;
class ArchiveWriter;
class WordLatticeAdaptor;
}
namespace Bliss {
class Evaluator;
}
namespace Flf {
class FlfLatticeAdaptor;
}

namespace Search {

namespace Wfst {
class WfstLatticeAdaptor;
}

/**
 * Input and output of lattices in various formats.
 * This class handles only Lattice::WordLattice objects. Other formats
 * are expected to be handled by derived classes adding read/write methods
 * by applying the decorator pattern.
 */
class LatticeHandler : public Core::Component
{
    static const Core::Choice choiceLatticeFormat;
    static const Core::ParameterChoice paramLatticeFormat;

protected:
    typedef Lattice::WordLatticeAdaptor WordLatticeAdaptor;
    typedef Flf::FlfLatticeAdaptor FlfLatticeAdaptor;
    typedef Search::Wfst::WfstLatticeAdaptor WfstLatticeAdaptor;
    typedef Lattice::ConstWordLatticeRef ConstWordLatticeRef;
public:
    enum LatticeFormat { formatDefault, formatFlf, formatOpenFst };

    LatticeHandler(const Core::Configuration &c) :
	Core::Component(c),
	format_(static_cast<LatticeFormat>(paramLatticeFormat(config))),
	reader_(0), writer_(0) {}
    virtual ~LatticeHandler();
    virtual void setLexicon(Core::Ref<const Bliss::Lexicon> lexicon) {
	lexicon_ = lexicon;
    }
    virtual Core::Ref<const Bliss::Lexicon> lexicon() const {
	return lexicon_;
    }

    virtual bool write(const std::string &id, const WordLatticeAdaptor &l);
    virtual bool write(const std::string &id, const FlfLatticeAdaptor &l) { return false; }
    virtual bool write(const std::string &id, const WfstLatticeAdaptor &l) { return false; }

    virtual Core::Ref<LatticeAdaptor> read(const std::string &id, const std::string &name);

    virtual ConstWordLatticeRef convert(const WordLatticeAdaptor &l) const;
    virtual ConstWordLatticeRef convert(const FlfLatticeAdaptor &l) const {
	return ConstWordLatticeRef();
    }
    virtual ConstWordLatticeRef convert(const WfstLatticeAdaptor &l) const {
	return ConstWordLatticeRef();
    }

protected:
    LatticeFormat format_;
    Core::Ref<const Bliss::Lexicon> lexicon_;

private:
    bool createReader();
    bool createWriter();
    Lattice::ArchiveReader *reader_;
    Lattice::ArchiveWriter *writer_;
};

} // namespace Search {

#endif // _SEARCH_LATTICEHANDLER_HH

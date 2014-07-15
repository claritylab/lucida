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
#include <Flf/LatticeHandler.hh>
#include <Flf/LatticeAdaptor.hh>
#include <Flf/Archive.hh>
#include <Flf/Copy.hh>
#include <Flf/FlfCore/Basic.hh>
#include <Flf/Map.hh>
#include <Flf/Rescore.hh>
#include <Lattice/LatticeAdaptor.hh>

namespace Flf {

LatticeHandler::~LatticeHandler()
{
    delete reader_;
    delete writer_;
    delete parent_;
}

bool LatticeHandler::createReader()
{
    if (!reader_) {
	reader_ = LatticeArchive::getReader(config);
	if (reader_->hasFatalErrors()) {
	    delete reader_;
	    reader_ = 0;
	}
    }
    return reader_;
}

bool LatticeHandler::createWriter()
{
    if (!writer_) {
	writer_ = LatticeArchive::getWriter(config);
	if (writer_->hasFatalErrors()) {
	    delete writer_;
	    writer_ = 0;
	}
    }
    return writer_;
}


bool LatticeHandler::write(const std::string &id, const FlfLatticeAdaptor &l)
{
    if (format_ != formatFlf) {
	return parent_->write(id, ::Lattice::WordLatticeAdaptor(l.wordLattice(this)));
    } else {
	if (!createWriter())
	    return false;
	writer_->store(id, l.get());
	return true;
    }
}

Core::Ref<Search::LatticeAdaptor> LatticeHandler::read(const std::string &id, const std::string &name)
{
    if (!createReader())
	return Core::Ref<Search::LatticeAdaptor>();
    ConstLatticeRef l = reader_->get(id);
    return Core::ref(new FlfLatticeAdaptor(l));
}

LatticeHandler::ConstWordLatticeRef LatticeHandler::convert(const FlfLatticeAdaptor &la) const
{
    require(lexicon());
    ConstLatticeRef l = la.get();
    ScoreId amId = l->semiring()->id("am");
    require(l->semiring()->hasId(amId));
    ScoreId lmId = l->semiring()->id("lm");
    require(l->semiring()->hasId(lmId));

    l = projectInput(mapInput(l, MapToLemmaPronunciation));
    if (l->semiring()->scale(amId) != Semiring::DefaultScale)
	l = multiply(l, amId, l->semiring()->scale(amId), RescoreModeInPlaceCache);
    if (l->semiring()->scale(lmId) != Semiring::DefaultScale)
	l = multiply(l, lmId, l->semiring()->scale(lmId), RescoreModeInPlaceCache);
    l = extendByPronunciationScore(l, lexicon()->lemmaPronunciationAlphabet(), lmId, Semiring::UndefinedScale, RescoreModeInPlaceCache);
    l = persistent(l);

    ::Lattice::WordLattice *wl = new ::Lattice::WordLattice;
    wl->setFsa(toFsa(l, amId), ::Lattice::WordLattice::acousticFsa);
    wl->setFsa(toFsa(l, lmId), ::Lattice::WordLattice::lmFsa);
    if (l->getBoundaries()->valid()) {
	StaticBoundaries boundaries;
	copyBoundaries(l, &boundaries);
	Core::Ref< ::Lattice::WordBoundaries> wordBoundaries(new ::Lattice::WordBoundaries);
	for (Fsa::StateId s = 0; s < boundaries.size(); ++s) {
	    if (boundaries.valid(s)) {
		const Boundary& bnd = boundaries.get(s);
		wordBoundaries->set(s,
			::Lattice::WordBoundary (bnd.time(),
						 ::Lattice::WordBoundary::Transit(bnd.transit().final, bnd.transit().initial)));
	    }
	}
    }
    return ConstWordLatticeRef(wl);

}

} // namespace Flf

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
#include "HtkWriter.hh"
#include "Basic.hh"
#include <Fsa/Basic.hh>

using namespace Lattice;


HtkWriter::HtkWriter(Core::Ref<const Bliss::Lexicon> lexicon) :
    lexicon_(lexicon)
{
    lpa_ = lexicon_->lemmaPronunciationAlphabet();
}

void HtkWriter::write(
    const std::string &id,
    ConstWordLatticeRef lattice,
    std::ostream &os) const
{
    lattice = normalize(lattice);
    Fsa::ConstAutomatonRef acoustic = lattice->part(WordLattice::acousticFsa);
    Fsa::ConstAutomatonRef lm       = lattice->part(WordLattice::lmFsa);
    require(acoustic->getInputAlphabet() == lpa_);

    os << "VERSION=1.1" << std::endl
       << "UTTERANCE=" << id << std::endl;

    Fsa::AutomatonCounts counts = Fsa::count(acoustic);
    os << "NODES=" << counts.nStates_ << '\t'
       << "LINKS=" << counts.nArcs_ << std::endl;

    for (Fsa::StateId si = 0; si <= counts.maxStateId_; ++si) {
	Fsa::ConstStateRef state = acoustic->getState(si);
	if (!state) continue;
	os << "I=" << state->id() << '\t'
	   << "t=" << lattice->time(state->id())
	   << std::endl;
    }

    u32 linkId = 0;
    for (Fsa::StateId si = 0; si <= counts.maxStateId_; ++si) {
	Fsa::ConstStateRef state = acoustic->getState(si);
	if (!state) continue;
	for (Fsa::State::const_iterator arc = state->begin(); arc != state->end(); ++arc) {
	    // word and pronunciation variant
	    std::string word;
	    u32 variantNumber = 0;
	    const Bliss::LemmaPronunciation *lemmaPronunciation = lpa_->lemmaPronunciation(arc->input());
	    if (lemmaPronunciation) {
		const Bliss::Lemma *lemma = lemmaPronunciation->lemma();
		word = std::string(lemma->name());
		Bliss::Lemma::PronunciationIterator lpi, lpi_end;
		for (Core::tie(lpi, lpi_end) = lemma->pronunciations(); lpi != lpi_end; ++lpi) {
		    if (lpi == lemmaPronunciation) break;
		    ++variantNumber;
		}
	    } else {
		word = "@";
	    }
	    // acoustic likelihood
	    f32 acousticScore = arc->weight();
	    // language model likelihood
	    f32 lmScore = lm->getState(si)->begin()[arc - state->begin()].weight();
	    os << "J=" << linkId++ << '\t'
	       << "S=" << state->id() << '\t'
	       << "E=" << arc->target() << '\t'
	       << "W=" << '"' << word << '"' << '\t'
	       << "v=" << variantNumber << '\t'
	       << "a=" << - acousticScore << '\t'
	       << "l=" << - lmScore
	       << std::endl;
	}

    }
    verify(linkId == counts.nArcs_);
}

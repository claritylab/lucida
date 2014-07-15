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
#include "Search.hh"

using namespace Search;


SearchAlgorithm::SearchAlgorithm(const Core::Configuration &c) : Core::Component(c)
{}

Speech::ModelCombination::Mode SearchAlgorithm::modelCombinationNeeded() const
{
    return Speech::ModelCombination::complete;
}

void SearchAlgorithm::Traceback::write(std::ostream &os, Core::Ref<const Bliss::PhonemeInventory> phi) const {
    for (const_iterator tbi = begin(); tbi != end(); ++tbi) {
	os <<     "t=" << std::setw(5) << tbi->time << "    s=" << std::setw(8) << tbi->score;
	if (tbi->pronunciation) {
	    os << "    "
	       << std::setw(20) << std::setiosflags(std::ios::left)
	       << tbi->pronunciation->lemma()->preferredOrthographicForm()
	       << "    "
	       << "/" << tbi->pronunciation->pronunciation()->format(phi) << "/";
	}
	os << "    "
	   << ((tbi->transit.final   == Bliss::Phoneme::term) ? "#" : phi->phoneme(tbi->transit.final  )->symbol().str())
	   << "|"
	   << ((tbi->transit.initial == Bliss::Phoneme::term) ? "#" : phi->phoneme(tbi->transit.initial)->symbol().str())
	   << std::endl;
    }
}

Fsa::ConstAutomatonRef SearchAlgorithm::Traceback::lemmaAcceptor(
    Core::Ref<const Bliss::Lexicon> lexicon) const
{
    Bliss::LemmaAcceptor *result = new Bliss::LemmaAcceptor(lexicon);
    Fsa::State *s1, *s2;
    s1 = result->newState();
    result->setInitialStateId(s1->id());
    for (u32 i = 0; i < size(); ++i)
	if ((*this)[i].pronunciation) {
	    s2 = result->newState();
	    s1->newArc(s2->id(), result->semiring()->one(), (*this)[i].pronunciation->lemma()->id());
	    s1 = s2;
	}
    result->setStateFinal(s1);
    return Fsa::ConstAutomatonRef(result);
}

Fsa::ConstAutomatonRef SearchAlgorithm::Traceback::lemmaPronunciationAcceptor(
    Core::Ref<const Bliss::Lexicon> lexicon) const
{
    Bliss::LemmaPronunciationAcceptor *result = new Bliss::LemmaPronunciationAcceptor(lexicon);
    const Bliss::LemmaPronunciationAlphabet *abet = result->lemmaPronunciationAlphabet();
    Fsa::State *s1, *s2;
    s1 = result->newState();
    result->setInitialStateId(s1->id());
    for (u32 i = 0; i < size(); ++i) {
	if (!(*this)[i].pronunciation) continue;
	s2 = result->newState();
	s1->newArc(s2->id(), result->semiring()->one(), abet->index((*this)[i].pronunciation));
	s1 = s2;
    }
    result->setStateFinal(s1);
    return Fsa::ConstAutomatonRef(result);
}

Lattice::WordLatticeRef SearchAlgorithm::Traceback::wordLattice(Core::Ref<const Bliss::Lexicon> lexicon) const
{
    // NOTE: This function returns a word lattice with dummy word boundaries
    Lattice::WordLatticeRef result(new Lattice::WordLattice());
    result->setFsa(lemmaPronunciationAcceptor(lexicon), Lattice::WordLattice::acousticFsa);
    result->setWordBoundaries(Core::ref(new Lattice::WordBoundaries));
    return result;
}

void SearchAlgorithm::getCurrentBestSentencePartial(SearchAlgorithm::Traceback& result) const
{
}

void SearchAlgorithm::getPartialSentence(SearchAlgorithm::Traceback&)
{
}

SearchAlgorithm::PruningRef SearchAlgorithm::describePruning()
{
    return SearchAlgorithm::PruningRef();
}

bool SearchAlgorithm::relaxPruning(f32 factor, f32 offset)
{
    return false;
}

void SearchAlgorithm::resetPruning(SearchAlgorithm::PruningRef pruning)
{
}

Core::Ref<const LatticeAdaptor> SearchAlgorithm::getPartialWordLattice()
{
    return Core::Ref<const LatticeAdaptor>();
}

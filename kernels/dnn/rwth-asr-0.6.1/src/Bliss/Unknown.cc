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
#include "Unknown.hh"
#include <Bliss/Fsa.hh>
#include <Bliss/Lexicon.hh>
#include <Fsa/Static.hh>

using namespace Bliss;

/*
 * This is a rather crude model: We allow any sequence of phonemes.
 * Currently the selection of allowed phonemes is very
 * unsophisticated: Only coarticulated phonemes are allowed.  This is
 * reasonable and sufficient for the time being, since context
 * independent phonemes are used for silence and noise
 * pseudo-phonemes.  For the future I envision the Lexicon to support
 * non-standard pronunciations.  Maybe one could write
 * <phon type="random">a b c d e f</phon> or even
 * <phon type="regex">(a|b|c|d|e|f)*</phon>
 */

Fsa::ConstAutomatonRef Bliss::createAnyPhonemeToUnknownTransducer(
    Core::Ref<const Lexicon> lexicon,
    f32 weight)
{
    Core::Ref<Fsa::StaticAutomaton> utap(new Fsa::StaticAutomaton);
    utap->setType(Fsa::TypeTransducer);
    utap->setSemiring(Fsa::TropicalSemiring);
    utap->setInputAlphabet(lexicon->phonemeInventory()->phonemeAlphabet());
    utap->setOutputAlphabet(lexicon->lemmaAlphabet());
    Fsa::State *first  = utap->newState();
    Fsa::State *second = utap->newState();
    utap->setInitialStateId(first->id());
    Fsa::Weight one = Fsa::Weight(utap->semiring()->one());
    Fsa::Weight pen = Fsa::Weight(weight);
    const Lemma *unknown = lexicon->specialLemma("unknown");
    if (unknown) {
	PhonemeInventory::PhonemeIterator pi, pi_end;
	for (Core::tie(pi, pi_end) = lexicon->phonemeInventory()->phonemes(); pi != pi_end; ++pi) {
	    if (!(*pi)->isContextDependent())
		continue;
	    first ->newArc(second->id(), pen, (*pi)->id(), unknown->id());
	    second->newArc(second->id(), pen, (*pi)->id(), Fsa::Epsilon);
	}
    }
    second->newArc(first->id(), one,
		   lexicon->phonemeInventory()->phonemeAlphabet()->disambiguator(0),
		   Fsa::Epsilon);
    utap->setStateFinal(first);
    return utap;
}

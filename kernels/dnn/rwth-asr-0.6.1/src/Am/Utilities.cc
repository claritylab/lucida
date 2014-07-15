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
#include "Utilities.hh"

using namespace Am;


LexiconUtilities::LexiconUtilities(const Core::Configuration& c, Bliss::LexiconRef l) :
    Component(c),
    lexicon_(l)
{}

Bliss::Phoneme::Id LexiconUtilities::determineSilencePhoneme() const
{
    Bliss::Phoneme::Id result = Bliss::Phoneme::term;
    const Bliss::Pronunciation* silencePronunciation =
	determineSilencePronunciation()->pronunciation();
    if (silencePronunciation) {
	if (silencePronunciation->length() < 1)
	    error("Silence pronunciation contains no phoneme.");
	else {
	    result = (*silencePronunciation)[0];
	    if (silencePronunciation->length() > 1)
		error("Silence pronunciation multiple phonemes. Using only /%s/",
		      lexicon_->phonemeInventory()->phoneme(result)->symbol().str());
	}
    }

    if (result == Bliss::Phoneme::term)
	criticalError("Failed to determine silence phoneme.");

    return result;
}

const Bliss::LemmaPronunciation* LexiconUtilities::determineSilencePronunciation() const
{
    const Bliss::Lemma *silenceLemma = lexicon_->specialLemma("silence");
    if (!silenceLemma)
	error("No special lemma \"silence\" defined in lexicon.");
    else if (silenceLemma->nPronunciations() < 1)
	error("Special lemma \"silence\" does not have a pronunciation.");
    else if (silenceLemma->nPronunciations() > 1)
	warning("Special lemma \"silence\" has multiple pronunciations."
		" Using /%s/.", silenceLemma->pronunciations().first->pronunciation()->format(lexicon_->phonemeInventory()).c_str());
    return silenceLemma->pronunciations().first;
}

Fsa::LabelId LexiconUtilities::determineSilenceLemmaPronunciationId() const
{
    const Bliss::LemmaPronunciation *silencePronunciation = determineSilencePronunciation();
    if (silencePronunciation) return silencePronunciation->id();
    else criticalError("Failed to determine silence pronunciation.");
    return Fsa::InvalidLabelId;
}

void LexiconUtilities::getInitialAndFinalPhonemes(
    std::vector<Bliss::Phoneme::Id> &initialPhonemes,
    std::vector<Bliss::Phoneme::Id> &finalPhonemes) const
{
    Bliss::PhonemeMap<u32> isInititalPhoneme(lexicon_->phonemeInventory());
    Bliss::PhonemeMap<u32> isFinalPhoneme(lexicon_->phonemeInventory());
    isInititalPhoneme.fill(0);
    isFinalPhoneme.fill(0);
    Bliss::Lexicon::PronunciationIterator pron, pron_end;
    for (Core::tie(pron, pron_end) = lexicon_->pronunciations(); pron != pron_end; ++pron) {
	const Bliss::Pronunciation &p(**pron);
	if (p.length() < 1) continue;
	++isInititalPhoneme[p[0]];
	++isFinalPhoneme[p[p.length()-1]];
    }

    Bliss::PhonemeInventory::PhonemeIterator phon, phon_end;
    for (Core::tie(phon, phon_end) = lexicon_->phonemeInventory()->phonemes(); phon != phon_end; ++phon) {
	if (isInititalPhoneme[*phon] && (*phon)->isContextDependent())
	    initialPhonemes.push_back((*phon)->id());
	if (isFinalPhoneme   [*phon] && (*phon)->isContextDependent())
	    finalPhonemes.  push_back((*phon)->id());
    }

    log("%zd distinct context dependent initial phonemes, %zd distinct context dependent final phomemes",
	initialPhonemes.size(), finalPhonemes.size());
}

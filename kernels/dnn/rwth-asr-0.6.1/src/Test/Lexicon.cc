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
#include <Test/Lexicon.hh>

using namespace Test;

Lexicon::Lexicon() :
	Bliss::Lexicon(Core::Configuration()),
	phonemes_(new Bliss::PhonemeInventory)
{
    setPhonemeInventory(phonemes_);
}

bool Lexicon::addPhoneme(const std::string &name, bool contextDependent)
{
    if (phonemes_->phoneme(name))
	return false;
    Bliss::Phoneme *p = phonemes_->newPhoneme();
    p->setContextDependent(contextDependent);
    phonemes_->assignSymbol(p, name);
    return true;
}

void Lexicon::addLemma(const std::string &orth, const std::string &pron,
		       const std::string &special)
{
    Bliss::Lemma *lemma = newLemma();
    if (!special.empty())
	defineSpecialLemma(special, lemma);
    Bliss::Pronunciation *p = getPronunciation(pron);
    addPronunciation(lemma, p);
    std::vector<std::string> orths;
    orths.push_back(orth);
    setOrthographicForms(lemma, orths);
    setDefaultLemmaName(lemma);
    setDefaultEvaluationToken(lemma);
    setDefaultSyntacticToken(lemma);
}

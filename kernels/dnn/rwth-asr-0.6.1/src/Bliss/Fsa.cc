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
// $Id: Fsa.cc 8249 2011-05-06 11:57:02Z rybach $

#include "Fsa.hh"

using namespace Bliss;


TokenAlphabet::TokenAlphabet(const TokenInventory &ti) :
    lexicon_(), tokens_(ti), nDisambiguators_(0)
{}

TokenAlphabet::TokenAlphabet(LexiconRef l, const TokenInventory &ti) :
    lexicon_(l), tokens_(ti), nDisambiguators_(0)
{}

TokenAlphabet::~TokenAlphabet()
{}

Fsa::LabelId TokenAlphabet::index(const std::string &sym) const {
    Fsa::LabelId special = specialIndex(sym);
    if (special != Fsa::InvalidLabelId) return special;
    if (sym.size() && sym[0] == '#') {
	int di = atoi(sym.c_str() + 1);
	if (di < 1) return Fsa::InvalidLabelId;
	return disambiguator(di - 1);
    }
    const Token* token = tokens_[sym];
    if (!token) return Fsa::InvalidLabelId;
    return token->id();
}

std::string TokenAlphabet::symbol(Fsa::LabelId id) const {
    std::string special = specialSymbol(id);
    if (!special.empty()) return special;
    if (id < 0)
	return "INVALID_LABEL_ID";
    if (u32(id) < tokens_.size())
	return tokens_[id]->symbol();
    return std::string("#") + Core::itoa(id - tokens_.size());
}

Fsa::Alphabet::const_iterator TokenAlphabet::end() const {
    return const_iterator(Fsa::ConstAlphabetRef(this),
			  tokens_.size() + nDisambiguators_);
}

void TokenAlphabet::writeXml(Core::XmlWriter &os) const {
    os << Core::XmlOpenComment();
    describe(os);
    if (lexicon_)
	os << "\n" << lexicon_->getDependency() << "\n";
    os << Core::XmlCloseComment() << "\n";
    Fsa::Alphabet::writeXml(os);
}

void TokenAlphabet::describe(Core::XmlWriter &os) const {}

u32 TokenAlphabet::nDisambiguators() const {
    return nDisambiguators_;
}

Fsa::LabelId TokenAlphabet::disambiguator(u32 d) const {
    require(d <= Core::Type<Fsa::LabelId>::max - tokens_.size());
    if (d >= nDisambiguators_) nDisambiguators_ = d + 1;
    return tokens_.size() + d;
}

bool TokenAlphabet::isDisambiguator(Fsa::LabelId i) const {
    return (i >= Fsa::LabelId(tokens_.size()));
}

// ===========================================================================
PhonemeAlphabet::PhonemeAlphabet(Core::Ref<const PhonemeInventory> pi) :
    TokenAlphabet(LexiconRef(), pi->phonemes_),
    pi_(pi)
{}

Fsa::LabelId PhonemeAlphabet::index(const std::string &id) const {
    Fsa::LabelId special = specialIndex(id);
    if (special != Fsa::InvalidLabelId) return special;
    if (id[0] == '#') {
	int di = atoi(id.c_str() + 1);
	if (di < 0) return Fsa::InvalidLabelId;
	if (Phoneme::Id(di) > Core::Type<Phoneme::Id>::max - pi_->nPhonemes()) return Fsa::InvalidLabelId;
	return disambiguator(di);
    }
    const Phoneme *phon = pi_->phoneme(id);
    if (phon)
	return phon->id();
    return Fsa::InvalidLabelId;
}

std::string PhonemeAlphabet::symbol(Fsa::LabelId id) const {
    std::string special = specialSymbol(id);
    if (!special.empty()) return special;
    if (id < 1)
	return "INVALID_LABEL_ID";
    if (id <= Fsa::LabelId(pi_->nPhonemes()))
	return pi_->phoneme(id)->symbol();
    return std::string("#") + Core::itoa(id - pi_->nPhonemes() - 1);
}

PhonemeAlphabet::const_iterator PhonemeAlphabet::begin() const {
    return const_iterator(Core::Ref<const Alphabet>(this), 1);
}

PhonemeAlphabet::const_iterator PhonemeAlphabet::end() const {
    return const_iterator(Fsa::ConstAlphabetRef(this), pi_->nPhonemes() + 1 + nDisambiguators());
}

void PhonemeAlphabet::writeXml(Core::XmlWriter &os) const {
    os << Core::XmlOpenComment() << "phoneme inventory";
    if (pi_)
	os << ": " << pi_->nPhonemes() << " phonemes, " << nDisambiguators() << " disambiguation symbols";
    os << Core::XmlCloseComment() << "\n";
    Fsa::Alphabet::writeXml(os);
}

// ===========================================================================
/* Note: We use *three* blanks to separate the lemma name from the
 * pronunciation because the lemma name is white-space normalized */

Fsa::LabelId LemmaPronunciationAlphabet::index(const std::string &id) const {
    Fsa::LabelId special = specialIndex(id);
    if (special != Fsa::InvalidLabelId) return special;

    std::string::size_type i = id.find("   /");
    std::string::size_type j = i + 4;
    if (i == std::string::npos) { // for backward compatibility
	i = id.find("/");
	j = i + 1;
    }

    if (i == std::string::npos) return Fsa::InvalidLabelId;
    std::string lemmaName = id.substr(0, i);

    std::string::size_type k = id.rfind("/");
    if (k == std::string::npos) return Fsa::InvalidLabelId;
    std::string phon = id.substr(j, k-j);

    const Lemma *lemma = lexicon_->lemma(lemmaName);
    if (!lemma) return Fsa::InvalidLabelId;
    Lemma::PronunciationIterator lp, lp_end;
    for (Core::tie(lp, lp_end) = lemma->pronunciations(); lp != lp_end; ++lp) {
	if (phon == lp->pronunciation()->format(lexicon_->phonemeInventory()))
	    return ((const LemmaPronunciation*) lp)->id();
    }
    return Fsa::InvalidLabelId;
}

std::string LemmaPronunciationAlphabet::symbol(Fsa::LabelId id) const {
    std::string special = specialSymbol(id);
    if (!special.empty()) return special;
    if (id >= Fsa::LabelId(lexicon_->nLemmaPronunciations()))
	return "unknown lemma pronunciation";
    const LemmaPronunciation *lp = lemmaPronunciation(id);
    verify(lp);
    return std::string(lp->lemma()->name())
	+ std::string("   /")
	+ lp->pronunciation()->format(lexicon_->phonemeInventory())
	+ std::string("/");
}

void LemmaPronunciationAlphabet::writeXml(Core::XmlWriter &os) const {
    os << Core::XmlOpenComment() << "lemma-pronunciation alphabet";
    if (lexicon_)
	os << "\n" << lexicon_->getDependency() << "\n";
    os << Core::XmlCloseComment() << "\n";
    Fsa::Alphabet::writeXml(os);
}


u32 LemmaPronunciationAlphabet::nDisambiguators() const {
    return nDisambiguators_;
}

Fsa::LabelId LemmaPronunciationAlphabet::disambiguator(u32 d) const {
    require(d <= Core::Type<Fsa::LabelId>::max - lexicon_->nLemmaPronunciations());
    if (d >= nDisambiguators_) nDisambiguators_ = d + 1;
	return lexicon_->nLemmaPronunciations() + d;
}

bool LemmaPronunciationAlphabet::isDisambiguator(Fsa::LabelId i) const {
    return (i >= Fsa::LabelId(lexicon_->nLemmaPronunciations()));
}


// ===========================================================================
LemmaAlphabet::LemmaAlphabet(LexiconRef l) :
    TokenAlphabet(l, l->lemmas_)
{}

void LemmaAlphabet::describe(Core::XmlWriter &os) const {
    os << "lemma alphabet";
}

SyntacticTokenAlphabet::SyntacticTokenAlphabet(LexiconRef l) :
    TokenAlphabet(l, l->syntacticTokens_)
{}

void SyntacticTokenAlphabet::describe(Core::XmlWriter &os) const {
    os << "syntactic token alphabet";
}

EvaluationTokenAlphabet::EvaluationTokenAlphabet(LexiconRef l) :
    TokenAlphabet(l, l->evaluationTokens_)
{}

void EvaluationTokenAlphabet::describe(Core::XmlWriter &os) const {
    os << "evaluation token alphabet";
}

LetterAlphabet::LetterAlphabet(LexiconRef l) :
    TokenAlphabet(l, l->letters_)
{}

void LetterAlphabet::describe(Core::XmlWriter &os) const {
    os << "letter alphabet";
}

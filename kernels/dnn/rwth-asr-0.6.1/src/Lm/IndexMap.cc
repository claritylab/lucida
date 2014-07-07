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
// $Id: IndexMap.cc 9227 2013-11-29 14:47:07Z golik $

#include <cmath>
#include <Core/Utility.hh>

#include "IndexMap.hh"


using namespace Lm;


const IndexMappedLm::InternalClassIndex IndexMappedLm::invalidClass =
   Core::Type<IndexMappedLm::InternalClassIndex>::max;


void IndexMappedLm::initializeMapping(InternalClassIndex nInternalClasses) {
    nInternalClasses_ = nInternalClasses;
    tokenMap_.fill(invalidClass);
    classMap_.resize(nInternalClasses);
    std::fill(classMap_.begin(), classMap_.end(), Token(0));
    classSizes_.resize(nInternalClasses);
    std::fill(classSizes_.begin(), classSizes_.end(), 0);
}

IndexMappedLm::InternalClassIndex IndexMappedLm::newClass() {
    InternalClassIndex result = nInternalClasses_++;
    classMap_.push_back(0);
    classSizes_.push_back(0);
    return result;
}

void IndexMappedLm::mapToken(Token s, InternalClassIndex c) {
    require(s);
    require(0 <= c && c < nInternalClasses_);

    if (tokenMap_[s] != invalidClass) {
	warning("Remapping \"%s\" to \"%s\", was \"%s\".",
		s->symbol().str(), internalClassName(c), internalClassName(tokenMap_[s]));
    }
    if (classMap_[c] != 0) {
	error("Mapping \"%s\" to \"%s\", which is already occupied by \"%s\".",
	      s->symbol().str(), internalClassName(c), classMap_[c]->symbol().str());
    }
    tokenMap_[s] = c;
    classMap_[c] = s;
    classSizes_[c] += 1;
}

void IndexMappedLm::checkForUnmappedTokens() {
    InternalClassIndex fallback = (fallbackToken()) ? tokenMap_[fallbackToken()] : invalidClass;

    for (size_t i = 0; i < nInternalClasses(); ++i)
	if (classMap_[i] == 0) classMap_[i] = fallbackToken();

    u32 nUnknowns = 0;
    Bliss::Lexicon::SyntacticTokenIterator s, s_end;
    for (Core::tie(s, s_end) = lexicon()->syntacticTokens(); s != s_end; ++s) {
	Token t = (*s);
	if (tokenMap_[t] == invalidClass) {
	    if (fallback != invalidClass) {
		++nUnknowns;
		warning("Unknown syntactic token \"%s\" mapped to \"%s\".",
			(*s)->symbol().str(),
			internalClassName(fallback));
		tokenMap_[t] = fallback;
		classSizes_[tokenMap_[t]] += 1;
	    } else {
		error("Unknown syntactic token \"%s\" could not be mapped.",
		      (*s)->symbol().str());
	    }
	}
    }
    if (nUnknowns) warning(
	"%d unknown syntactic tokens mapped to \"%s\".", nUnknowns,
	internalClassName(fallback));
}

void IndexMappedLm::checkForUnusedClasses() {
    for (InternalClassIndex c = 0 ; c < nInternalClasses_ ; ++c) {
	if (classSizes_[c] == 0) {
	    warning("Language model token \"%s\" not used.", internalClassName(c));
	}
    }
}

void IndexMappedLm::initClassEmissionScores() {
    classEmissionScores_.resize(nInternalClasses_);
    for (InternalClassIndex c = 0 ; c < nInternalClasses_ ; ++c) {
	if (classSizes_[c] > 0)
	    classEmissionScores_[c] = ::log(classSizes_[c]);
	else
	    classEmissionScores_[c] = Core::Type<Score>::max;
    }
}

void IndexMappedLm::finalizeMapping() {
    checkForUnmappedTokens();
    checkForUnusedClasses();
    initClassEmissionScores();
    log("number of internal word classes: %d", nInternalClasses());
}

IndexMappedLm::IndexMappedLm(const Core::Configuration &c, Bliss::LexiconRef l) :
    Core::Component(c),
    LanguageModel(c, l),
    tokenMap_(l->syntacticTokenInventory())
{
    nInternalClasses_ = 0;
}

IndexMappedLm::~IndexMappedLm() {}

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
// $Id: LanguageModel.cc 9621 2014-05-13 17:35:55Z golik $

#include "LanguageModel.hh"
#include <Bliss/Fsa.hh>

using namespace Lm;


History::SentinelHistoryManager History::sentinel;

LanguageModelAutomaton::LanguageModelAutomaton(Bliss::LexiconRef lexicon) :
    lexicon_(lexicon),
    alphabet_(lexicon->syntacticTokenAlphabet())
{
    backOffLabel_ = Fsa::Epsilon;
}

LanguageModelAutomaton::LanguageModelAutomaton(Core::Ref<const LanguageModel> lm) :
    lexicon_(lm->lexicon()),
    alphabet_(lm->tokenAlphabet())
{
    backOffLabel_ = Fsa::Epsilon;
}

LanguageModel::LanguageModel(
    const Core::Configuration &c,
    Bliss::LexiconRef l) :
    Component(c),
    historyManager_(0),
    lexicon_(l),
    tokenInventory_(0),
    fallbackToken_(0),
    sentenceBeginToken_(0),
    sentenceEndToken_(0)
{
    tokenInventory_ = &l->syntacticTokenInventory();
    tokenAlphabet_ = l->syntacticTokenAlphabet();
    requiresSentenceBoundaryToken_ = false;
}

LanguageModel::~LanguageModel() {}

void LanguageModel::init() {
    require(tokenInventory_);
    createSpecialTokens();
    load();
}

class TokenAlphabet : public Bliss::TokenAlphabet {
private:
    /*
     * Warning:
     * To store a reference of the langauge model is crucial,
     * as long as no other guarantee is given, that token inventory
     * and symbol set this alphabet is based on are not going to be destroyed
     * during lifetime of this alphabet.
     */
    Core::Ref<const LanguageModel> lm_;
public:
    TokenAlphabet(Core::Ref<const LanguageModel> lm) :
	Bliss::TokenAlphabet(lm->tokenInventory()), lm_(lm) {}
    virtual ~TokenAlphabet() {}
};
void LanguageModel::setTokenInventory(const TokenInventory *_tokenInventory) {
    tokenInventory_ = _tokenInventory;
    tokenAlphabet_ = Fsa::ConstAlphabetRef(new TokenAlphabet(Core::ref(this)));
}
void LanguageModel::setTokenInventoryAndAlphabet(const TokenInventory *_tokenInventory, Fsa::ConstAlphabetRef _tokenAlphabet) {
    tokenInventory_ = _tokenInventory;
    tokenAlphabet_ = _tokenAlphabet;
}

const Bliss::SyntacticToken * LanguageModel::getSpecialSyntacticToken(
    const std::string &name, bool required) const
{
    const Bliss::Lemma *lemma = lexicon()->specialLemma(name);

    if (!lemma) {
	if (required) {
	    error("Cannot determine \"%s\" lm token. "
		  "No special lemma \"%s\" defined in lexicon.",
		  name.c_str(), name.c_str());
	}
	return 0;
    }

    if (!lemma->hasSyntacticTokenSequence()) {
	if (required) {
	    error("Cannot determine \"%s\" lm token. "
		  "Special lemma \"%s\" does not specify a syntactic token.",
		  name.c_str(), name.c_str());
	}
	return 0;
    }

    const Bliss::SyntacticTokenSequence & tokenSequence(
	lemma->syntacticTokenSequence());

    if (tokenSequence.length() < 1) {
	if (required) {
	    error("Empty \"%s\" lm token. "
		  "Special lemma \"%s\" specifies a syntactic token sequence of length zero.",
		  name.c_str(), name.c_str());
	}
	return 0;
    }

    const Bliss::SyntacticToken * token = tokenSequence[0];

    if (tokenSequence.length() > 1) {
	warning("Ambiguous \"%s\" lm token. "
		"Special lemma \"%s\" specifies a syntactic token sequence longer than one. "
		"Using only \"%s\".",
		name.c_str(), name.c_str(), token->symbol().str());
    }

    return token;
}

Token LanguageModel::getSpecialToken(
    const std::string &name, bool required) const
{
    const Bliss::SyntacticToken *syntTok = getSpecialSyntacticToken(name, required);
    if (!syntTok)
	return InvalidToken;
    Token tok = getToken(syntTok->symbol());
    if (!tok && required)
	criticalError("Cannot find required special token \"%s\"",
		      name.c_str());
    return tok;
}

void LanguageModel::createSpecialTokens() {
    fallbackToken_ = getSpecialToken("unknown", false);
    if (lexicon()->specialLemma("sentence-boundary")) {
	sentenceBeginToken_ = sentenceEndToken_ = getSpecialToken("sentence-boundary", requiresSentenceBoundaryToken_);
    } else {
	sentenceBeginToken_ = getSpecialToken("sentence-begin", requiresSentenceBoundaryToken_);
	sentenceEndToken_   = getSpecialToken("sentence-end", requiresSentenceBoundaryToken_);
    }
    log("Sentence boundary: %s ... %s",
	(sentenceBeginToken_) ? sentenceBeginToken_->symbol().str() : "",
	(sentenceEndToken_)   ? sentenceEndToken_  ->symbol().str() : "");
}

Fsa::ConstAutomatonRef LanguageModel::getFsa() const {
    criticalError("getAcceptor() not implemented for this language model");
    return Fsa::ConstAutomatonRef();
}

History LanguageModel::reducedHistory(const History &h, u32) const {
    return h;
}

Lm::Score LanguageModel::sentenceEndScore(const History &h) const {
    require(sentenceEndToken());
    return score(h, sentenceEndToken());
}

// ===========================================================================
CompiledBatchRequest *LanguageModel::compileBatchRequest(const BatchRequest &request, Score scale) const {
    NonCompiledBatchRequest *result = new NonCompiledBatchRequest(scale);
    result->request = request;
    return result;
}

void LanguageModel::getBatch(
    const History &history,
    const CompiledBatchRequest *cbr,
    std::vector<f32> &result) const
{
    const NonCompiledBatchRequest *ncbr = required_cast(const NonCompiledBatchRequest*, cbr);
    const BatchRequest &request(ncbr->request);

    for (BatchRequest::const_iterator r = request.begin(); r != request.end(); ++r) {
	Score sco = 0.0;
	if (r->tokens.length() >= 1) {
	    sco += score(history, r->tokens[0]);
	    if (r->tokens.length() > 1) {
		History h = extendedHistory(history, r->tokens[0]);
		for (u32 ti = 1; ; ++ti) {
		    Token st = r->tokens[ti];
		    sco += score(h, st);
		    if (ti+1 >= r->tokens.length()) break;
		    h = extendedHistory(h, st);
		}
	    }
	}
	sco *= ncbr->scale();
	sco += r->offset;

	if (result[r->target] > sco)
	    result[r->target] = sco;
    }
}

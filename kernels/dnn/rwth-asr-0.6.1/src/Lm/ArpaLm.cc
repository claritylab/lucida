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
// $Id: ArpaLm.cc 9621 2014-05-13 17:35:55Z golik $

#include "ArpaLm.hh"
#include "ReverseArpaLm.hh"
#include <Core/CompressedStream.hh>
#include <Core/MD5.hh>
#include <Core/ProgressIndicator.hh>
#include <Core/StringUtilities.hh>
#include <Core/TextStream.hh>
#include <Fsa/Arithmetic.hh>
#include <Fsa/Compose.hh>
#include <cstdio>

using namespace Lm;



const Core::ParameterString ArpaLm::paramEncoding(
    "encoding",
    "encoding of ARPA language model file to load",
    "utf-8");
const Core::ParameterString ArpaLm::paramFilename(
    "file",
    "name of ARPA language model file to load");
const Core::ParameterBool ArpaLm::paramSkipInfScore(
    "skip-inf-score",
    "ignore events with probability 0, which are encoded with a dummy score in "
    "ARPA LM files generated with SRILM", false);
const Core::ParameterBool ArpaLm::paramReverseLm(
    "reverse-lm",
    "reverse the LM in-place (temporarily needs a lot of memory)"
    "", false);

/**
 * Since log(0) (minus infinity) has no portable representation,
 * such values are mapped to a large negative number. However, the designated
 * dummy value (-99 in SRILM) _should_ be interpreted as log(0) when read back
 * from file into memory.
 * [man ngram-format]
 */
const f64 ArpaLm::InfScore = -99;

ArpaLm::ArpaLm(const Core::Configuration &c, Bliss::LexiconRef l) :
    Core::Component(c),
    BackingOffLm(c, l)
{}

ArpaLm::~ArpaLm() {}

class ArpaLm::InitData {
public:
    Core::Obstack<Token> histories;
    std::vector<InitItem> items;
    InitItem wsii, boii;

    InitData() {
	wsii.history = boii.history = histories.add(0);
	boii.token = 0;
    }

    void reserve(size_t nScores, size_t nBackOffScores) {
	items.reserve(nScores + nBackOffScores);
    }

    void setHistory(InitItem &ii, const Token *newest, const Token *oldest) {
	const Token *h, *t;
	for (h = ii.history, t = newest; t != oldest && (*h == *t); ++h, ++t);
	if (*h == 0 && t == oldest) return;
	ii.history = histories.add0(newest, oldest);
    }

    void addScore(const Token *newest, const Token *oldest, Token predicted, Score score) {
	setHistory(wsii, newest, oldest);
	wsii.token = predicted;
	wsii.score = score;
	items.push_back(wsii);
    }

    void addBackOffScore(const Token *newest, const Token *oldest, Score score) {
	setHistory(boii, newest, oldest);
	boii.score = score;
	items.push_back(boii);
    }
};

void ArpaLm::read() {
    static const f64 ln10 = 2.30258509299404568402;
    static const u32 maxNGramLength = 32;

    std::string filename(paramFilename(config));
    log("reading ARPA language model from file \"%s\" ...", filename.c_str());

    std::string tempfile;

    if (paramReverseLm(config))
    {
	char* tmp = tempnam(0, "lm");
	tempfile = tmp;
	::free(tmp);
	log() << "reversing ARPA language model into temporary file '" << tempfile << "'";
	Lm::reverseArpaLm(filename, tempfile);
	log() << "successfully reversed";
	filename = tempfile;
    }

    Core::CompressedInputStream *cis = new Core::CompressedInputStream(filename.c_str());
    Core::TextInputStream is(cis);
    is.setEncoding(paramEncoding(config));

    if (!is) {
	error("Failed to open language model file \"%s\".", filename.c_str());
	return;
    }
    const bool includeZeroProb = !(paramSkipInfScore(config));

    InitData *data = new InitData;

    Core::MD5 md5;
    std::string line;
    u32 lineNumber = 0, totalNGrams = 0, expectedTotalNGrams = 0, maxTotalNGrams = 0;
    enum { preamble, sizes, ngrams, postamble, unknown } state = preamble;
    u32 nGram, n;
    Token tokens[maxNGramLength];
    Core::ProgressIndicator pi("reading ARPA lm", "n-grams");
    pi.start();
    while (!std::getline(is, line).eof()) {
	++lineNumber;
	if (line.size() == 0) continue;
	else if (line[0] == '\\') switch (state) { // handle section header
	case preamble: {
	    if (line == "\\data\\") state = sizes;
	} break;
	case sizes:
	case ngrams:
	case unknown:
	    if (line == "\\end\\")
		state = postamble;
	    else if (sscanf(line.c_str(), "\\%u-gram:", &nGram) == 1) {
		state = ngrams;
	    } else {
		warning("Unrecognized section starting in line %d", lineNumber);
		state = unknown;
	    }
	case postamble: break;
	default: defect();

	} else switch (state) { // handle other lines
	case preamble:  break;
	case postamble: break;
	case unknown:   break;
	case sizes: {
	    u32 nNGrams;
	    // u32 fraction = 3;
	    if (sscanf(line.c_str(), "ngram %u=%u", &nGram, &nNGrams) == 2) {
		maxTotalNGrams += nNGrams;
		/**
		 * Reserve memory only for a fraction of the LM,
		 * assume that some part of the LM is discarded because of a resricted vocabulary.
		 **/
		// expectedTotalNGrams += nNGrams / fraction;
		// data->reserve(expectedTotalNGrams, expectedTotalNGrams - nNGrams / fraction);
		// fraction *= 3;
		expectedTotalNGrams += nNGrams;
		data->reserve(expectedTotalNGrams, expectedTotalNGrams - nNGrams);
	    }
	    pi.setTotal(expectedTotalNGrams);
	} break;
	case ngrams: {
	    md5.update(line);
	    std::istringstream lis(line);
	    f64 score;
	    std::string word;
	    if (!(lis >> score))
		error("Expected float value in line %d", lineNumber);
	    hope(nGram <= maxNGramLength);
	    for (n = 0; n < nGram; ++n) {
		if (lis >> word) {
		    Core::normalizeWhitespace(word);
		    Core::suppressTrailingBlank(word);
		    Token t = getToken(word);
		    if (t) {
			tokens[n] = t;
		    } else {
			warning("unknown syntactic token '%s' in line %d", word.c_str(), lineNumber);
			break;
		    }
		}
	    }
	    if (n == nGram) {
		std::reverse(tokens, tokens + n);
		if (includeZeroProb || !Core::isAlmostEqual(score, InfScore, 0.1))
		    data->addScore(&tokens[1], &tokens[n], tokens[0], - ln10 * score);
		if (lis >> score)
		    data->addBackOffScore(&tokens[0], &tokens[n], - ln10 * score);
		++totalNGrams;
	    }
	    pi.notify();
	} break;
	default: defect();
	}
    }
    pi.finish();
    if (state != postamble)
	error("Premature end of language model file.");
    dependency_.setValue(md5);
    /*
      log("%d/%d/%d loaded/expected/all n-grams", totalNGrams, expectedTotalNGrams, maxTotalNGrams);
    */
    initialize(&*data->items.begin(), &(*(data->items.end()-1))+1);
    delete data;

    if (tempfile.size())
	std::remove(tempfile.c_str());
}


ArpaClassLm::ArpaClassLm(const Core::Configuration &c, Bliss::LexiconRef l) :
    Core::Component(c),
    ArpaLm(c, l), ClassLm(c)
{
    ClassMapping *mapping = new ClassMapping(select("classes"), lexicon());
    mapping->load();
    mapping_ = ConstClassMappingRef(mapping);
    setTokenInventoryAndAlphabet(&mapping_->tokenInventory(), mapping_->tokenAlphabet());
}

ArpaClassLm::~ArpaClassLm() {}

Token ArpaClassLm::getSpecialToken(
    const std::string &name, bool required) const
{
    const Bliss::SyntacticToken *syntTok = getSpecialSyntacticToken(name, required);
    if (!syntTok)
	return InvalidToken;
    Token tok = (*mapping_)[syntTok].first;
    if (!tok && required)
	error("Cannot find special token \"%s\"",
	      name.c_str());
    return tok;
}

History ArpaClassLm::extendedHistory(const History &h, Token t) const {
    return ArpaLm::extendedHistory(h, (*mapping_)[t].first);
}

Score ArpaClassLm::score(const History &h, Token t) const {
    const std::pair<ClassToken*, Score> &cs = (*mapping_)[t];
    return ArpaLm::score(h, cs.first) + classEmissionScale() * cs.second;
}

Fsa::ConstAutomatonRef ArpaClassLm::getFsa() const {
    return Fsa::composeMatching(
	Fsa::multiply(mapping_->createSyntacticTokenToClassTokenTransducer(), Fsa::Weight(classEmissionScale())),
	ArpaLm::getFsa());
}

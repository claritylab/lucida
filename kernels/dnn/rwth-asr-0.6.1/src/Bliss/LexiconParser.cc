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
// $Id: LexiconParser.cc 6036 2006-08-30 09:15:40Z hoffmeister $

#include "LexiconParser.hh"
#include <Core/CompressedStream.hh>
#include <Core/Parameter.hh>
#include <Core/StringUtilities.hh>
#include <Core/TextStream.hh>


using namespace Bliss;
using namespace Core;

// Note: Features marked "FFE" are placeholders "for furture extensions".

// ===========================================================================
PhonemeInventoryElement::PhonemeInventoryElement(
    XmlContext *_context, Handler _handler) :
    Precursor("phoneme-inventory", _context, _handler)
{
    phoneme_ = 0;

    { // build schema
	XmlElement *symbol =
	    collect(new XmlStringBuilderElement(
			"symbol", this,
			XmlStringBuilderElement::handler(&Self::phonemedefSymbol)));

	XmlRegularElement *phoneme = new XmlRegularElementRelay(
	    "phoneme", this,
	    XmlMixedElementRelay::startHandler(&Self::startPhonemedef),
	    XmlMixedElementRelay::endHandler(&Self::endPhonemedef));
	collect(phoneme);
	phoneme->addTransition(initial, 1, symbol);
	phoneme->addTransition(1, 1, symbol);
	phoneme->addTransition(
	    1, 2, collect(new XmlStringBuilderElement(
			      "variation", this,
			      XmlStringBuilderElement::handler(&Self::phonemedefVariation))));
	phoneme->addTransition(2, 3, collect(new XmlIgnoreElement("features")));  // FFE
	phoneme->addFinalState(1);
	phoneme->addFinalState(2);
	phoneme->addFinalState(3);
	phoneme->ignoreUnknownElements();

	addTransition(initial, initial, phoneme);
	addFinalState(initial);
    }
}

void PhonemeInventoryElement::characters(const char *ch, int len) {

}

void PhonemeInventoryElement::startPhonemedef(const XmlAttributes atts) {
    phoneme_ = product_->newPhoneme() ;
}

void PhonemeInventoryElement::phonemedefSymbol(const std::string &s) {
    std::string symbol(s);
    stripWhitespace(symbol);

    const Phoneme *test = product_->phoneme(symbol);
    if (test == phoneme_) {
	parser()->warning(
	    "Redundant definition of phonetic symbol \"%s\"",
	    symbol.c_str());
    } else if (test != 0) {
	parser()->error(
	    "Phonetic symbol \"%s\" already assigned to a different phoneme",
	    symbol.c_str());
    } else {
	product_->assignSymbol(phoneme_, symbol);
    }
}

void PhonemeInventoryElement::phonemedefVariation(const std::string &s) {
    std::string spec(s);
    stripWhitespace(spec);
    if (spec == std::string("none")) {
	phoneme_->setContextDependent(false);
    } else if (spec == std::string("context")) {
	phoneme_->setContextDependent(true);
    } else {
	parser()->error(
	    "variation must be one of \"none\" or \"context\", \"%s\" given",
	    spec.c_str());
    }
}

void PhonemeInventoryElement::endPhonemedef() {
    phoneme_ = 0 ;
}

// ===========================================================================
struct Bliss::WeightedPhonemeString {
    std::string phon;
    f32 weight;
    WeightedPhonemeString() : weight(1.0) {}
};

class Bliss::PronunciationElement :
    public Core::XmlBuilderElement<WeightedPhonemeString, XmlEmptyElement, CreateStatic>
{
    typedef Core::XmlBuilderElement<WeightedPhonemeString, XmlEmptyElement, CreateStatic> Precursor;
public:
    PronunciationElement(Core::XmlContext*, Handler handler = 0);
    virtual void start(const XmlAttributes atts);
    virtual void characters(const char *ch, int len);
};

void PronunciationElement::start(const Core::XmlAttributes atts) {
    Precursor::start(atts);
    const char *weightStr = atts["weight"];
    const char *scoreStr = atts["score"];
    if (weightStr && scoreStr) {
	parser()->criticalError("not possible to set both, pronunciation weight and score");
	return;
    }
    f32 weight = (weightStr) ? atof(weightStr) : 1.0;
    product_.weight = (scoreStr) ? exp(-atof(scoreStr)) : weight;
    if (product_.weight < 0.0) {
	parser()->error("pronunciation weight must be non-negative");
	product_.weight = 0.0;
    }

}

PronunciationElement::PronunciationElement(XmlContext *context, Handler handler) :
    Precursor("phon", context, handler)
{}

void PronunciationElement::characters(const char *ch, int len) {
    product_.phon.append(ch, len);
}

// ===========================================================================
const Core::ParameterBool LexiconElement::paramNormalizePronunciation(
    "normalize-pronunciation",
    "normalize pronunciation weights/scores",
    true);

LexiconElement::LexiconElement(
    XmlContext *_context, CreationHandler _newLexicon, const Core::Configuration &c) :
    Precursor("lexicon", _context, _newLexicon)
{
    lexicon_ = 0;
    lemma_ = 0;
    isNormalizePronunciation_ = paramNormalizePronunciation(c);

    { // build schema
	addTransition(
	    initial, 1, collect(
		new PhonemeInventoryElement(
		    this, PhonemeInventoryElement::handler(&Self::addPhonemeInventory))));

	XmlElement *orth =
	    collect(new XmlStringBuilderElement(
			"orth", this, XmlStringBuilderElement::handler(&Self::addOrth)));
	XmlElement *phon =
	    collect(new PronunciationElement(
			this, PronunciationElement::handler(&Self::addPhon)));

	XmlRegularElement *synt = new XmlRegularElementRelay(
	    "synt", this,
	    startHandler(&Self::startSynt),
	    endHandler(&Self::endSynt));
	collect(synt);
	synt->addTransition(
	    0, 0, collect(new XmlStringBuilderElement(
			      "tok", this, XmlStringBuilderElement::handler(&Self::tok))));
	synt->addFinalState(0);

	XmlRegularElement *eval = new XmlRegularElementRelay(
	    "eval", this,
	    startHandler(&Self::startEval),
	    endHandler(&Self::endEval));
	collect(eval);
	eval->addTransition(
	    0, 0, collect(new XmlStringBuilderElement(
			      "tok", this, XmlStringBuilderElement::handler(&Self::tok))));
	eval->addFinalState(0);

	XmlRegularElement *lemma = new XmlRegularElementRelay(
	    "lemma", this,
	    startHandler(&Self::startLemma),
	    endHandler(&Self::endLemma));
	collect(lemma);
	lemma->addTransition(initial, 1, orth);
	lemma->addTransition(1, 1, orth);
	lemma->addTransition(1, 2, phon);
	lemma->addTransition(2, 2, phon);
	lemma->addTransition(1, 3, synt);
	lemma->addTransition(2, 3, synt);
	lemma->addTransition(1, 4, eval);
	lemma->addTransition(2, 4, eval);
	lemma->addTransition(3, 4, eval);
	lemma->addTransition(4, 4, eval);
	lemma->addFinalState(1);
	lemma->addFinalState(2);
	lemma->addFinalState(3);
	lemma->addFinalState(4);

	addTransition(0, 1, lemma);
	addTransition(1, 1, lemma);
	addFinalState(0);
	addFinalState(1);
    }
}

void LexiconElement::addPhonemeInventory(std::auto_ptr<PhonemeInventory> &pi) {
    if (product_->phonemeInventory_) {
	parser()->error("Phoneme inventory already defined") ;
	return ;
    }
    product_->setPhonemeInventory(Core::ref(pi.release()));
}

void LexiconElement::startLemma(const XmlAttributes atts) {
    verify(!lemma_);
    const char *c;
    c = atts["name"];
    if (c)
	lemmaName_ = std::string(c);
    c = atts["special"];
    if (c)
	specialLemmaName_ = std::string(c);
}

void LexiconElement::addOrth(const std::string &_orth) {
    std::string orth(_orth);
    normalizeWhitespace(orth);
    suppressTrailingBlank(orth);
    if (specialLemmaName_.empty() && !whitelist_.empty() && (whitelist_.find(orth) == whitelist_.end()))
	return;
    orths_.push_back(orth);
    if (!lemma_) {
	if (!lemmaName_.empty()) {
	    std::string name(lemmaName_);
	    normalizeWhitespace(lemmaName_);
	    suppressTrailingBlank(lemmaName_);
	    if (product_->lemma(lemmaName_)) {
		parser()->error("Lemma name \"%s\" already taken", lemmaName_.c_str());
		lemma_ = product_->newLemma();
	    } else {
		lemma_ = product_->newLemma(lemmaName_);
	    }
	} else {
	    lemma_ = product_->newLemma() ;
	}
	verify(lemma_);
	if (!specialLemmaName_.empty()) {
	    if (product_->specialLemma(specialLemmaName_))
		parser()->error("Special lemma \"%s\" already defined" , specialLemmaName_.c_str()) ;
	    else
		product_->defineSpecialLemma(specialLemmaName_, lemma_) ;
	}
    }
}

void LexiconElement::addPhon(const WeightedPhonemeString &phon) {
    if(!lemma_)
	return;
    if (!product_->phonemeInventory()) {
	parser()->warning(
	    "No phoneme inventory defined. Ingnoring pronunciation");
	return;
    }

    Pronunciation *pron = product_->getPronunciation(phon.phon);
    if (lemma_->hasPronunciation(pron)) {
	parser()->error("duplicate pronunciation");
	return;
    }

    product_->addPronunciation(lemma_, pron, phon.weight);
}

void LexiconElement::startTokSeq(const XmlAttributes atts) {
    if(!lemma_)
	return;
    verify(tokSeq_.size() == 0);
}

void LexiconElement::tok(const std::string &_tok) {
    if(!lemma_)
	return;
    std::string tok(_tok);
    normalizeWhitespace(tok);
    suppressTrailingBlank(tok);
    tokSeq_.push_back(tok);
}

void LexiconElement::endTokSeq() {
    if(!lemma_)
	return;
    tokSeq_.clear();
}

void LexiconElement::startSynt(const XmlAttributes atts) {
    if(!lemma_)
	return;
    startTokSeq(atts);
    verify(!lemma_->hasSyntacticTokenSequence());
}

void LexiconElement::endSynt() {
    if(!lemma_)
	return;
    product_->setSyntacticTokenSequence(lemma_, tokSeq_);
    endTokSeq();
}

void LexiconElement::startEval(const XmlAttributes atts) {
    if(!lemma_)
	return;
    startTokSeq(atts);
}

void LexiconElement::endEval() {
    if(!lemma_)
	return;
    product_->addEvaluationTokenSequence(lemma_, tokSeq_);
    endTokSeq();
}

void LexiconElement::endLemma() {
    if(!lemma_)
	return;

    product_->setOrthographicForms(lemma_, orths_);

    if (lemma_->nOrthographicForms() == 0) {
	parser()->warning("Lemma without orthographic form");
    } else {
	if (!lemma_->hasName())
	    product_->setDefaultLemmaName(lemma_);
	if (!lemma_->hasSyntacticTokenSequence())
	    product_->setDefaultSyntacticToken(lemma_);
	if (!lemma_->hasEvaluationTokenSequence())
	    product_->setDefaultEvaluationToken(lemma_);
    }

    if(isNormalizePronunciation_)
	product_->normalizePronunciationWeights(lemma_);

    lemmaName_.clear();
    specialLemmaName_.clear();
    orths_.clear();
    lemma_ = 0;
}

// ===========================================================================
namespace {
    const Core::ParameterString paramFile(
	"file",
	"file name",
	"");
    const Core::ParameterString paramEncoding(
	"encoding",
	"encoding",
	"utf-8");
} // namespace

void LexiconParser::loadWhitelist(const Core::Configuration &config, Core::StringHashSet &whitelist)
{
    std::string filename = paramFile(config);
    if (!filename.empty()) {
	Core::CompressedInputStream *cis = new Core::CompressedInputStream(filename.c_str());
	Core::TextInputStream is(cis);
	is.setEncoding(paramEncoding(config));
	if (!is)
	    criticalError("Failed to open vocab file \"%s\".", filename.c_str());
	std::string s;
	while (Core::getline(is, s) != EOF) {
	    if ((s.size() == 0) || (s.at(0) == '#'))
		continue;
	    Core::normalizeWhitespace(s);
	    Core::suppressTrailingBlank(s);
	    whitelist.insert(s);
	}
	if (!whitelist.empty())
	    log("Use a vocab list with %d entries.", u32(whitelist.size()));
    }
}

LexiconParser::LexiconParser(const Core::Configuration &c, Lexicon *_lexicon) :
    Precursor(c)
{
    lexicon_ = _lexicon;

    // build schema
    LexiconElement *lexElement = new LexiconElement(this, LexiconElement::creationHandler(&Self::pseudoCreateLexicon), c);
    loadWhitelist(select("vocab"), lexElement->whitelist_);
    setRoot(collect(lexElement));
}

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
// $Id: LexiconParser.hh 6036 2006-08-30 09:15:40Z hoffmeister $

#ifndef _BLISS_LEXICONPARSER_HH
#define _BLISS_LEXICONPARSER_HH

#include <Core/Hash.hh>
#include <Core/XmlBuilder.hh>
#include "Lexicon.hh"

namespace Bliss {

    class PhonemeInventoryElement :
	public Core::XmlBuilderElement<
	    PhonemeInventory,
	    Core::XmlRegularElement,
	    Core::CreateUsingNew>
    {
	typedef Core::XmlBuilderElement<
	    PhonemeInventory,
	    Core::XmlRegularElement,
	    Core::CreateUsingNew> Precursor;
	typedef PhonemeInventoryElement Self;
    private:
	Phoneme *phoneme_;

	void startPhonemedef(const Core::XmlAttributes atts) ;
	void endPhonemedef() ;
	void phonemedefSymbol(const std::string&);
	void phonemedefVariation(const std::string&);
    public:
	PhonemeInventoryElement(
	    Core::XmlContext *_context, Handler _handler = 0);
	virtual void characters(const char*, int);
    };

    struct WeightedPhonemeString;
    class PronunciationElement;
    class LexiconElement;
    class LexiconParser;

    class LexiconElement :
	public Core::XmlBuilderElement<
	    Lexicon,
	    Core::XmlRegularElement,
	    Core::CreateByContext>
    {
	friend class LexiconParser;
	typedef Core::XmlBuilderElement<
	    Lexicon,
	    Core::XmlRegularElement,
	    Core::CreateByContext> Precursor;
	typedef LexiconElement Self;
    private:
	Lexicon *lexicon_;
	Core::StringHashSet whitelist_;
	Lemma   *lemma_;
	std::string lemmaName_;
	std::string specialLemmaName_;
	std::vector<std::string> orths_;
	std::vector<std::string> tokSeq_;
	void addPhonemeInventory(std::auto_ptr<PhonemeInventory>&);
	void startLemma(const Core::XmlAttributes atts);
	void addOrth(const std::string&);
	void addPhon(const WeightedPhonemeString&);
	void startTokSeq(const Core::XmlAttributes atts);
	void tok(const std::string&);
	void endTokSeq();
	void startSynt(const Core::XmlAttributes atts);
	void syntTok(const std::string&);
	void endSynt();
	void startEval(const Core::XmlAttributes atts);
	void evalTok(const std::string&);
	void endEval();
	void endLemma();
	static const Core::ParameterBool paramNormalizePronunciation;
	bool isNormalizePronunciation_;
    public:
	LexiconElement(Core::XmlContext*, CreationHandler, const Core::Configuration &c);
	virtual void characters(const char*, int) {};
    };


    /**
     * Parser for Bliss lexicon files.
     * This class implements parsing of the lexicon XML format
     * described in <a href="../../doc/Lexicon.pdf">Lexicon File
     * Format Reference</a>.  It is normally not used directly but
     * through Lexicon.
     */

    class LexiconParser :
	public Core::XmlSchemaParser
    {
	typedef Core::XmlSchemaParser Precursor;
	typedef LexiconParser Self;
    private:
	Lexicon *lexicon_;
	Lexicon *pseudoCreateLexicon(Core::XmlAttributes) { return lexicon_; }
	void loadWhitelist(const Core::Configuration &, Core::StringHashSet &);
    public:
	LexiconParser(const Core::Configuration &c, Lexicon*);
	Lexicon *lexicon() const { return lexicon_; }
    } ;

} // namescape Bliss

#endif // _BLISS_LEXICONPARSER_HH

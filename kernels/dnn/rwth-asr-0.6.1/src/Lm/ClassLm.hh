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
#ifndef _LM_CLASSLM_HH
#define _LM_CLASSLM_HH

#include <Core/Component.hh>
#include <Core/Parameter.hh>
#include <Core/ReferenceCounting.hh>
#include <Bliss/Lexicon.hh>
#include <Bliss/Symbol.hh>

#include "LanguageModel.hh"

namespace Lm {

    /*
      class lm:
      p(lemma | history) = p(lemma| synt) * p(synt| class) * p(class| history)
    */
    class ClassToken : public Bliss::Token
    {
	friend class ClassMapping;
    private:
	typedef std::vector<const Bliss::SyntacticToken*> SyntacticTokenList;
    public:
	typedef SyntacticTokenList::const_iterator Iterator;
    private:
	SyntacticTokenList syntacticTokens_;
    public:
	ClassToken(Bliss::Symbol _symbol) : Bliss::Token(_symbol) {}
	Iterator begin() { return syntacticTokens_.begin(); }
	Iterator end() { return syntacticTokens_.end(); }
    };

    class ClassMappingAutomaton;

    class ClassMapping :
	public Core::Component, public Core::ReferenceCounted
    {
	friend class ClassMappingAutomaton;
    public:
	static const Core::ParameterString paramClassFileName;
	static const Core::ParameterString paramClassFileEncoding;
    private:
	typedef std::vector<std::pair<ClassToken*, Score> > ClassMap;

    private:
	Bliss::LexiconRef lexicon_;
	Bliss::SymbolSet symbolSet_;
	Bliss::TokenInventory tokenInventory_;
	Fsa::ConstAlphabetRef tokenAlphabet_;
	ClassMap classMap_;

    protected:
	ClassToken * addToken(const std::string &word);

    public:
	ClassMapping(const Core::Configuration &config, Bliss::LexiconRef lexicon);

	/*
	  class file format:
	  # comment
	  <syntactic token> <class> [q(<syntactic token>| <class>)]

	  q(<syntactic token>| <class>) needs not be normalized; normalization is done after
	  if no class emission probabilities are given, a uniform distribution is assumed
	*/
	void load();

	Bliss::LexiconRef lexicon() const
	    { return lexicon_; }
	const Bliss::TokenInventory & tokenInventory() const
	    { return tokenInventory_; }
	Fsa::ConstAlphabetRef tokenAlphabet() const
	    { return tokenAlphabet_; }
	Fsa::ConstAutomatonRef createSyntacticTokenToClassTokenTransducer() const;

	ClassToken* classToken(Token t) const
	    { return classMap_[t->id()].first; }
	Score classEmissionScore(Token t) const
	    { return classMap_[t->id()].second; }

	std::pair<ClassToken*, Score> operator[] (Token t) const
	    { return classMap_[t->id()]; }

	void writeClasses(Core::XmlWriter &xml) const;
    };
    typedef Core::Ref<const ClassMapping> ConstClassMappingRef;


    class ClassLm : public virtual Core::Component {
    public:
	static const Core::ParameterFloat paramClassEmissionScale;
    private:
	Score classEmissionScale_;
    public:
	ClassLm(const Core::Configuration &config);
	virtual ~ClassLm();

	Score classEmissionScale() const
	    { return classEmissionScale_; }
	void setClassEmissionScale(Score _classEmissionScale)
	    { classEmissionScale_ = _classEmissionScale; }

	virtual ConstClassMappingRef classMapping() const = 0;
    };

} // namespace Lm

#endif // _LM_CLASSLM_HH

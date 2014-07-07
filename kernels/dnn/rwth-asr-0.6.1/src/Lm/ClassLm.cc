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
#include <Bliss/Fsa.hh>
#include <Core/CompressedStream.hh>
#include <Core/TextStream.hh>
#include <Fsa/Automaton.hh>

#include "ClassLm.hh"

#include <iomanip>
#include <sstream>

using namespace Lm;

const Core::ParameterString ClassMapping::paramClassFileName(
    "file",
    "class mapping file",
    "");
const Core::ParameterString ClassMapping::paramClassFileEncoding(
    "encoding",
    "encoding of class mapping file",
    "utf-8");

class ClassTokenAlphabet : public Bliss::TokenAlphabet {
private:
    ConstClassMappingRef mapping_;
public:
    ClassTokenAlphabet(ConstClassMappingRef mapping) :
	Bliss::TokenAlphabet(mapping->tokenInventory()), mapping_(mapping) {}
    virtual ~ClassTokenAlphabet() {}
};

ClassMapping::ClassMapping(const Core::Configuration &config, Bliss::LexiconRef lexicon) :
    Core::Component(config), lexicon_(lexicon) {
    tokenAlphabet_ = Fsa::ConstAlphabetRef(new ClassTokenAlphabet(Core::ref(this)));
}

ClassToken * ClassMapping::addToken(const std::string &word) {
    Bliss::Symbol symbol = symbolSet_[word];
    ClassToken *token = new ClassToken(symbol);
    tokenInventory_.add(token);
    return token;
}

void ClassMapping::load() {
    std::string filename = paramClassFileName(config);
    Core::CompressedInputStream *cis =
	new Core::CompressedInputStream(filename.c_str());
    Core::TextInputStream tis(cis);
    tis.setEncoding(paramClassFileEncoding(config));
    u32 nSyntTok = lexicon_->syntacticTokenInventory().size();
    classMap_.resize(nSyntTok, std::make_pair(static_cast<ClassToken*>(0), 0.0));
    std::string syntWord, classWord;
    f32 classScore;
    while (tis) {
	std::string line;
	Core::getline(tis, line);
	std::istringstream iss(line);
	if (!(iss >> std::ws).good())
	    continue;
	iss >> syntWord >> std::ws;
	if ((syntWord.at(0) == '#') || (syntWord.at(0) == ';'))
	    continue;
	require(iss.good());
	if ((iss >> classWord >> std::ws).good()) iss >> classScore;
	else classScore = 1.0;
	const Bliss::SyntacticToken *syntTok = lexicon_->syntacticToken(syntWord);
	if (!syntTok) {
	    warning("In class file: Unknown syntactic token \"%s\" found.",
		    syntWord.c_str());
	    continue;
	}
	if (classMap_[syntTok->id()].first) {
	    error("In class file: Duplicated entry for syntactic token \"%s\"; discard entry.",
		    syntWord.c_str());
	    continue;
	}
	ClassToken *classTok = static_cast<ClassToken*>(tokenInventory_[classWord]);
	if (!classTok)
	    classTok = addToken(classWord);
	classTok->syntacticTokens_.push_back(syntTok);
	ensure(size_t(syntTok->id()) < classMap_.size());
	classMap_[syntTok->id()] = std::make_pair(classTok, classScore);
	--nSyntTok;
    }
    if (nSyntTok) {
	log("No mapping specified for %d syntactic tokens; "
	    "add identity mappings.",
	    nSyntTok);
	Bliss::Lexicon::SyntacticTokenIterator synt_tok_it, synt_tok_end;
	for (Core::tie(synt_tok_it, synt_tok_end) = lexicon_->syntacticTokens();
	     synt_tok_it != synt_tok_end; ++synt_tok_it) {
	    const Bliss::SyntacticToken *syntTok = *synt_tok_it;
	    if (!classMap_[syntTok->id()].first) {
		std::string classWord = syntTok->symbol().str();
		ClassToken *classTok = dynamic_cast<ClassToken*>(tokenInventory_[classWord]);
		if (!classTok)
		    classTok = addToken(classWord);
		classTok->syntacticTokens_.push_back(syntTok);
		ensure(size_t(syntTok->id()) < classMap_.size());
		classMap_[syntTok->id()] = std::make_pair(classTok, 1.0);
	    }
	}
    }
    for (Bliss::TokenInventory::Iterator itClassTok = tokenInventory_.begin();
	 itClassTok != tokenInventory_.end(); ++itClassTok) {
	ClassToken *classTok = static_cast<ClassToken*>(*itClassTok);
	Score sum = 0.0;
	for (ClassToken::SyntacticTokenList::iterator itSyntTok = classTok->syntacticTokens_.begin();
	     itSyntTok != classTok->syntacticTokens_.end(); ++itSyntTok)
	    sum += classMap_[(*itSyntTok)->id()].second;
	for (ClassToken::SyntacticTokenList::iterator itSyntTok = classTok->syntacticTokens_.begin();
	     itSyntTok != classTok->syntacticTokens_.end(); ++itSyntTok) {
	    std::pair<ClassToken*, Score> &tokenScore = classMap_[(*itSyntTok)->id()];
	    tokenScore.second = -::log(tokenScore.second / sum);
	}
    }
}


class Lm::ClassMappingAutomaton : public Fsa::Automaton {
private:
    ConstClassMappingRef mapping_;
public:
    ClassMappingAutomaton(ConstClassMappingRef mapping) : mapping_(mapping) {
	addProperties(Fsa::PropertySortedByInput);
	setProperties(Fsa::PropertyLinear | Fsa::PropertyAcyclic, Fsa::PropertyNone);
    }
    virtual ~ClassMappingAutomaton() {}

    virtual ConstSemiringRef semiring() const {
	return Fsa::TropicalSemiring;
    }

    virtual Fsa::StateId initialStateId() const {
	return 0;
    }

    virtual Fsa::ConstAlphabetRef getInputAlphabet() const {
	return mapping_->lexicon()->syntacticTokenAlphabet();
    }

    virtual Fsa::ConstAlphabetRef getOutputAlphabet() const {
	return mapping_->tokenAlphabet();
    }

    virtual Fsa::ConstStateRef getState(Fsa::StateId sid) const {
	Fsa::State *sp = new Fsa::State(sid);
	sp->setFinal(semiring()->one());
	Fsa::LabelId syntTokId = 0;
	for (ClassMapping::ClassMap::const_iterator itClassScore = mapping_->classMap_.begin();
	     itClassScore != mapping_->classMap_.end(); ++itClassScore, ++syntTokId) {
	    sp->newArc(0, Fsa::Weight(itClassScore->second), syntTokId, itClassScore->first->id());
	}
	return Fsa::ConstStateRef(sp);
    }

    virtual std::string describe() const {
	return "class-mapping";
    }
};

Fsa::ConstAutomatonRef ClassMapping::createSyntacticTokenToClassTokenTransducer() const {
    return Fsa::ConstAutomatonRef(new ClassMappingAutomaton(Core::ref(this)));
}

void ClassMapping::writeClasses(Core::XmlWriter &xml) const {
    u32 nClass = 1;
    for (Bliss::TokenInventory::Iterator itClassTok = tokenInventory_.begin();
	 itClassTok != tokenInventory_.end(); ++itClassTok, ++nClass) {
	ClassToken *classTok = static_cast<ClassToken*>(*itClassTok);
	xml << std::setw(4) << std::right << nClass << ". " << std::string(classTok->symbol()) << "={";
	ClassToken::Iterator itSyntTok = classTok->begin();
	xml << std::string((*itSyntTok)->symbol())
	    << "/" << classEmissionScore(*itSyntTok);
	for (++itSyntTok; itSyntTok != classTok->end(); ++itSyntTok)
	    xml << ", " << std::string((*itSyntTok)->symbol())
		<< "/" << classEmissionScore(*itSyntTok);
	xml << "}\n";
    }
}


const Core::ParameterFloat ClassLm::paramClassEmissionScale(
    "scale",
    "class emission scale",
    1.0);

ClassLm::ClassLm(const Core::Configuration &config) :
    Core::Component(config) {
    classEmissionScale_ = paramClassEmissionScale(select("classes"));
}

ClassLm::~ClassLm() {}

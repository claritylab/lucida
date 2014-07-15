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
#ifndef _FLF_LEXICON_HH
#define _FLF_LEXICON_HH

#include <Bliss/Fsa.hh>
#include <Bliss/Lexicon.hh>
#include <Core/Channel.hh>
#include <Core/ReferenceCounting.hh>
#include <Core/Vector.hh>
#include <Fsa/Automaton.hh>

#include "FlfCore/Lattice.hh"
#include "Network.hh"

namespace Flf {

    class Lexicon;
    typedef Core::Ref<Lexicon> LexiconRef;

    class Lexicon : public Bliss::Lexicon {
	typedef Bliss::Lexicon Precursor;

    private:
	static LexiconRef us_;
    public:
	static LexiconRef us()
	    { verify_(us_); return us_; }

    public:
	static const Core::ParameterString paramFile;
	static const Core::ParameterBool paramNormalizePronunciationWeights;
	static const Core::ParameterBool paramReadOnly;

	typedef enum {
	    InvalidAlphabetId,
	    UnknownAlphabetId,
	    LemmaAlphabetId,
	    LemmaPronunciationAlphabetId,
	    SyntacticTokenAlphabetId,
	    EvaluationTokenAlphabetId,
	    PhonemeAlphabetId
	} AlphabetId;
	static const Core::Choice AlphabetNameMap;

	typedef Core::Ref<const Bliss::PhonemeAlphabet>            PhonemeAlphabetRef;
	typedef Core::Ref<const Bliss::LemmaAlphabet>              LemmaAlphabetRef;
	typedef Core::Ref<const Bliss::LemmaPronunciationAlphabet> LemmaPronunciationAlphabetRef;
	typedef Core::Ref<const Bliss::SyntacticTokenAlphabet>     SyntacticTokenAlphabetRef;
	typedef Core::Ref<const Bliss::EvaluationTokenAlphabet>    EvaluationTokenAlphabetRef;

	typedef std::vector<const Bliss::Lemma*>                   ConstLemmaPtrList;
	typedef std::vector<const Bliss::LemmaPronunciation*>      ConstLemmaPronunciationPtrList;

	/**
	 * Map symbol to index; insert symbols if necessary
	 **/
	class SymbolMap {
	public:
	    typedef Fsa::LabelId (Lexicon::*IndexFcn)(const std::string &);
	private:
	    Lexicon *lexicon_;
	    IndexFcn index_;
	    Fsa::ConstAlphabetRef alphabet_;
	public:
	    SymbolMap() : lexicon_(0), index_(0) {}
	    SymbolMap(Fsa::ConstAlphabetRef alphabet, IndexFcn indexFcn);
	    Fsa::ConstAlphabetRef alphabet() const { return alphabet_; }
	    std::string symbol(Fsa::LabelId label) const { return alphabet_->symbol(label); }
	    Fsa::LabelId index(const std::string &symbol) const { return (lexicon_->*index_)(symbol); }
	    void indices(const std::string &str, std::vector<Fsa::LabelId> &) const;
	};

	/**
	 * Map an Fsa alphabet to one of the lexicon's alphabets; insert symbols if necessary
	 **/
	class AlphabetMap : public Core::Vector<Fsa::LabelId>, public Core::ReferenceCounted {
	    typedef Core::Vector<Fsa::LabelId> Precursor;
	public:
	    static const Fsa::LabelId Unmapped;
	protected:
	    Lexicon *lexicon_;
	    Fsa::LabelId unkLabel_;
	    Fsa::ConstAlphabetRef from_;
	    Fsa::ConstAlphabetRef to_;
	protected:
	    virtual Fsa::LabelId index(Fsa::LabelId label) = 0;
	public:
	    AlphabetMap(Fsa::ConstAlphabetRef from, Fsa::ConstAlphabetRef to);
	    virtual ~AlphabetMap() {}
	    Fsa::ConstAlphabetRef from() const { return from_; }
	    Fsa::ConstAlphabetRef to() const { return to_; }
	    Fsa::LabelId operator[] (Fsa::LabelId label);
	};
	typedef Core::Ref<AlphabetMap> AlphabetMapRef;


	typedef Core::Ref<const Bliss::PhonemeToLemmaPronunciationTransducer> PhonemeToLemmaPronunciationTransducerRef;
	typedef Core::Ref<const Bliss::LemmaPronunciationToLemmaTransducer>   LemmaPronunciationToLemmaTransducerRef;
	typedef Core::Ref<const Bliss::LemmaToSyntacticTokenTransducer>       LemmaToSyntacticTokenTransducerRef;
	typedef Core::Ref<const Bliss::LemmaToEvaluationTokenTransducer>      LemmaToEvaluationTokenTransducerRef;

    private:
	Fsa::ConstAlphabetRef unknownAlphabet_;
	Bliss::PhonemeInventory *phonemeInventory_;
	typedef Core::hash_map<std::string, Bliss::Lemma*, Core::StringHash, Core::StringEquality> LemmaMap;
	LemmaMap orthographyMap_;
	bool normalizePronunciationWeights_;
	bool isReadOnly_;
	Core::XmlChannel *insertionChannel_;
	u32 nLemmaUpdates_;
	Bliss::Pronunciation *emptyPron_;

	PhonemeToLemmaPronunciationTransducerRef phonemeToLemmaPronunciationTransducer_;
	LemmaPronunciationToLemmaTransducerRef lemmaPronunciationToLemmaTransducer_;
	LemmaToSyntacticTokenTransducerRef lemmaToSyntacticTokenTransducer_;
	LemmaToEvaluationTokenTransducerRef lemmaToEvaluationTokenTransducer_;
	LemmaToEvaluationTokenTransducerRef lemmaToPreferredEvaluationTokenSequenceTransducer_;

	Fsa::LabelId siLemmaId_;
	Fsa::LabelId siLemmaPronunciationId_;
	Fsa::LabelId unkLemmaId_;
	Fsa::LabelId unkLemmaPronunciationId_;
	Fsa::LabelId sentenceEndLemmaId_;
	Fsa::LabelId sentenceEndLemmaPronunciationId_;

    private:
	void initSpecialLemmas();
	std::pair<Bliss::Lemma*, bool> getLemma(const std::string &orth);
	void updateLemma(Bliss::Lemma*);
	Bliss::Pronunciation * getPronunciation(const std::string &phons);

    public:
	Lexicon(const Core::Configuration &config);
	virtual ~Lexicon();

	/*
	  If the lexicon is not run in read-only mode:
	  how often was a lemma updated, because a
	  pronunciation, an orthography, a syntactic or
	  an evaluation token was inserted?
	*/
	bool isReadOnly() const { return isReadOnly_; }
	u32 nLemmaUpdates() const { return nLemmaUpdates_; }

	/*
	  Lexicon entries
	*/
	Fsa::LabelId siLemmaId() { return siLemmaId_; }
	Fsa::LabelId siLemmaPronunciationId() { return siLemmaPronunciationId_; }
	Fsa::LabelId unkLemmaId() { return unkLemmaId_; }
	Fsa::LabelId unkLemmaPronunciationId() { return unkLemmaPronunciationId_; }
	Fsa::LabelId sentenceEndLemmaId() { return sentenceEndLemmaId_; }
	Fsa::LabelId sentenceEndLemmaPronunciationId() { return sentenceEndLemmaPronunciationId_; }

	Fsa::LabelId unknownId(const std::string &);
	Fsa::LabelId phonemeId(const std::string &);
	Fsa::LabelId lemmaId(const std::string &);
	Fsa::LabelId lemmaPronunciationId(const std::string &);
	Fsa::LabelId lemmaPronunciationId(const std::string &, s32);
	Fsa::LabelId syntacticTokenId(const std::string &);
	Fsa::LabelId evaluationTokenId(const std::string &);

	const Bliss::LemmaPronunciation* lemmaPronunciation(const Bliss::Lemma *, s32 variant);
	std::pair<const Bliss::Lemma*, s32> lemmaPronunciationVariant(const Bliss::LemmaPronunciation *);

	// list of all lemmas having no or an empty evaluation token sequence
	LabelIdList nonWordLemmaIds();
	ConstLemmaPtrList nonWordLemmas();
	ConstLemmaPronunciationPtrList nonWordLemmaPronunciations();

	/*
	  Alphabets
	*/
	Fsa::ConstAlphabetRef unknownAlphabet() const { return unknownAlphabet_; }

	AlphabetId alphabetId(const std::string &name, bool dieOnFailure = false);
	AlphabetId alphabetId(Fsa::ConstAlphabetRef alphabet, bool dieOnFailure = false);

	Fsa::ConstAlphabetRef alphabet(AlphabetId);
	const std::string & alphabetName(AlphabetId);
	SymbolMap symbolMap(AlphabetId);
	AlphabetMapRef alphabetMap(Fsa::ConstAlphabetRef from, AlphabetId);


	/*
	  Internal alphabet mappings
	*/
	PhonemeToLemmaPronunciationTransducerRef phonemeToLemmaPronunciationTransducer();
	LemmaPronunciationToLemmaTransducerRef lemmaPronunciationToLemmaTransducer();
	LemmaToSyntacticTokenTransducerRef lemmaToSyntacticTokenTransducer();
	LemmaToEvaluationTokenTransducerRef lemmaToEvaluationTokenTransducer();
	LemmaToEvaluationTokenTransducerRef lemmaToPreferredEvaluationTokenSequenceTransducer();
    };


    /**
     * Extract vocab out of all seen lattices
     **/
    NodeRef createWordListExtractorNode(const std::string &name, const Core::Configuration &config);


    /**
     * Edit distance between two symbols in the alphabet
     *
     * distance(<special symbol>, <special symbol>) = 0
     * distance(<special symbol>, w) = distance(w, <special symbol>) = len(w)
     *
     **/
    class EditDistance;
    typedef Core::Ref<EditDistance> ConstEditDistanceRef;
    class EditDistance : public Core::ReferenceCounted {
    public:
	typedef u16 Cost;
    private:
	Fsa::ConstAlphabetRef alphabet_;
	const Bliss::TokenAlphabet *toks_;
	mutable Cost *D_, lengthD_;
    public:
	EditDistance(Fsa::ConstAlphabetRef alphabet, const Bliss::TokenAlphabet *toks);
	~EditDistance();
	Fsa::ConstAlphabetRef alphabet() const { return alphabet_; }
	/**
	 * Edit distance
	 **/
	Cost operator() (Fsa::LabelId label1, Fsa::LabelId label2) const;
	/**
	 * Edit distance
	 **/
	Score cost(Fsa::LabelId label1, Fsa::LabelId label2) const;
	/**
	 * Normalized edit distance
	 **/
	Score normCost(Fsa::LabelId label1, Fsa::LabelId label2) const;
    public:
	static ConstEditDistanceRef create(Lexicon::AlphabetId alphabetId);
    };

} // namespace Flf

#endif // _FLF_LEXICON_HH

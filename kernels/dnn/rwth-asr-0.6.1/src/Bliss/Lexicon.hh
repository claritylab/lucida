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
// $Id: Lexicon.hh 9621 2014-05-13 17:35:55Z golik $

#ifndef _BLISS_LEXICON_HH
#define _BLISS_LEXICON_HH

#include <vector>
#include <set>
#include <utility>
#include <Core/Component.hh>
#include <Core/Dependency.hh>
#include <Core/Extensions.hh>
#include <Core/Hash.hh>
#include <Core/Obstack.hh>
#include <Core/Parameter.hh>
#include <Core/ReferenceCounting.hh>
#include <Core/StringUtilities.hh>
#include "Phoneme.hh"
#include "Symbol.hh"

namespace Fsa { class Automaton; }

namespace Bliss {

    class Lexicon;
    typedef Core::Ref<const Lexicon> LexiconRef;
    class Lemma;
    class Pronunciation;
    class OrthographicToken;
    class SyntacticToken;
    class EvaluationToken;

    typedef Symbol OrthographicForm;
    typedef SymbolSequence<OrthographicForm> OrthographicFormList;
    typedef SymbolSequence<const SyntacticToken*> SyntacticTokenSequence;
    typedef SymbolSequence<const EvaluationToken*> EvaluationTokenSequence;
    typedef std::vector<EvaluationTokenSequence> EvaluationTokenSequenceList;

    /**
     * @todo pronunciation probabilities are quite slow due to repeated calculation of logarithm
     */
    class LemmaPronunciation {
    public:
	typedef s32 Id;
	/** Id used for the invalid pronunciation (equivalent to a LemmaPronunciation* with value 0) */
	static const Id invalidId = -1;

    private:
	friend class Lexicon;
	Id id_;
	const Lemma *lemma_;
	const Pronunciation *pronunciation_;
	f32 score_; /** -log[ p(pronunciation | lemma) ] */
	LemmaPronunciation *nextForThisLemma_, *nextForThisPronunciation_;

	LemmaPronunciation(Id _id) :
	    id_(_id),
	    lemma_(NULL),
	    pronunciation_(NULL),
	    score_(0),
	    nextForThisLemma_(NULL),
	    nextForThisPronunciation_(NULL)
	{}

    public:
	Id id() const { return id_; }
	const Lemma *lemma() const { return lemma_; }
	const Pronunciation *pronunciation() const { return pronunciation_; }

	/** Probability of pronunciation given lemma. */
	f32 pronunciationProbability() const { return exp(-score_); }
	f32 pronunciationScore() const { return score_; }
	void setPronunciationProbability(const f32 probability) {
	    require(probability >= 0 and probability <= 1);
	    score_ = -std::log(probability);
	}

	const LemmaPronunciation *nextForThisLemma() const { return nextForThisLemma_; }
	const LemmaPronunciation *nextForThisPronunciation() const { return nextForThisPronunciation_; }
    };

    /**
     * An entry in the Bliss lexion.
     * @see <a href="../../doc/Bliss.pdf">Bliss documentation</a>
     */
    class Lemma : public Token {
    private:
	OrthographicFormList orth_;
	LemmaPronunciation *pronunciations_;
	SyntacticTokenSequence syntacticTokens_;
	EvaluationTokenSequenceList evaluationTokensList_;

    protected:
	friend class Lexicon;

	Lemma();
	~Lemma();

	void setName(Symbol);

	void setOrthographicForms(const OrthographicFormList &orth) {
	    require(orth.valid());
	    orth_ = orth;
	}
	void setSyntacticTokenSequence(const SyntacticTokenSequence &synt) {
	    require(synt.valid());
	    syntacticTokens_ = synt;
	}
	void addEvaluationTokenSequence(const EvaluationTokenSequence &eval) {
	    require(eval.valid());
	    evaluationTokensList_.push_back(eval);
	}

    public:
	/**
	 * Test for presence of a name.
	 * Under all normal circumstances the answer is "yes".
	 * You don't have to test for this.
	 */
	bool hasName() const {
	    return symbol();
	}

	/**
	 * A unique string identifier for the lemma.
	 * The sole purpose of the name is to be used as a reference
	 * to the lemma lemma from an external data file
	 * (e.g. a word lattice).
	 */
	Symbol name() const { return symbol(); }

	/** Number of orthographic forms of this lemma. */
	OrthographicFormList::Size nOrthographicForms() const {
	    return orth_.size();
	}

	/** Set of orthographic forms of this lemma. */
	const OrthographicFormList & orthographicForms() const {
	    return orth_;
	}

	/** Preferred (standard) orthographic form of this lemma. */
	OrthographicForm preferredOrthographicForm() const {
	    require(nOrthographicForms());
	    return orth_.front();
	}

	class PronunciationIterator {
	private:
	    const LemmaPronunciation *current_;
	    friend class Lemma;
	    explicit PronunciationIterator(const LemmaPronunciation *c) :
		current_(c) {}
	public:
	    PronunciationIterator() {}
	    operator const LemmaPronunciation* () const {
		return current_;
	    }
	    const LemmaPronunciation* operator->() const {
		return current_;
	    }
	    PronunciationIterator& operator++() {
		current_ = current_->nextForThisLemma();
		return *this;
	    }
	    bool operator==(const PronunciationIterator &rhs) const {
		return current_ == rhs.current_;
	    }

	    bool operator!=(const PronunciationIterator &rhs) const {
		return !(this->operator==(rhs));
	    }

	};

	/** Number of pronunciations assigned to this lemma */
	u32 nPronunciations() const;

	typedef std::pair<PronunciationIterator, PronunciationIterator> LemmaPronunciationRange;
	/** Iterator-range giving access to the set of pronunciations. */
	LemmaPronunciationRange pronunciations() const {
	    return std::make_pair(PronunciationIterator(pronunciations_),
				  PronunciationIterator(0));
	}

	/**
	 * Test if a pronunciation is one of the possible
	 * pronunciations for htis lemms.
	 * This is used for preventing duplicate pronunciations.
	 */
	bool hasPronunciation(const Pronunciation*) const;

	/** Syntactic token sequence
	 * Under all normal circumstances the answer is "yes".
	 * You don't have to test for this. */
	bool hasSyntacticTokenSequence() const {
	    return syntacticTokens_.valid();
	}

	const SyntacticTokenSequence &syntacticTokenSequence() const {
	    require(hasSyntacticTokenSequence());
	    return syntacticTokens_;
	}


	/** Evaluation token sequence.
	 * Under all normal circumstances the answer is "yes".
	 * You don't have to test for this. */
	bool hasEvaluationTokenSequence() const {
	    return !evaluationTokensList_.empty();
	}

	/** Number of evaluation token sequences assigned to this lemma;
	 *  > 0, see above. */
	u32 nEvaluationTokenSequences() const {
	    return evaluationTokensList_.size();
	}

	typedef EvaluationTokenSequenceList::const_iterator EvaluationTokenSequenceIterator;
	/** Iterator-range giving access to the set of pronunciations. */
	std::pair<EvaluationTokenSequenceIterator, EvaluationTokenSequenceIterator>
	evaluationTokenSequences() const {
	    return std::make_pair(evaluationTokensList_.begin(),
				  evaluationTokensList_.end());
	}
    };

    class Letter : public Token {
    protected:
	friend class Lexicon;
	Letter(Bliss::Symbol _symbol) : Token(_symbol) {}
    };


    /**
     * A pronunciation in the Bliss lexicon.
     * @see <a href="../../doc/Bliss.pdf">Bliss documentation</a>
     */

    class Pronunciation {
    private:
	LemmaPronunciation *lemmas_;
	const Phoneme::Id *phonemes_;

    protected:
	friend class Lexicon;

	Pronunciation(const Phoneme::Id *phonemes);
	~Pronunciation();

    public:
	struct Hash {
	    u32 operator()(const Phoneme::Id*) const;
	    u32 operator()(const Pronunciation*) const;
	};
	struct Equality {
	    bool operator()(const Phoneme::Id*, const Phoneme::Id*) const;
	    bool operator()(const Pronunciation*, const Pronunciation*) const;
	};

    public:
	class LemmaIterator {
	private:
	    const LemmaPronunciation *current_;
	    friend class Pronunciation;
	    explicit LemmaIterator(const LemmaPronunciation *c) :
		current_(c) {}
	public:
	    LemmaIterator() : current_(NULL) {}

	    operator const LemmaPronunciation* () const {
		return current_;
	    }
	    const LemmaPronunciation* operator->() const {
		return current_;
	    }
	    LemmaIterator& operator++() {
		current_ = current_->nextForThisPronunciation();
		return *this;
	    }
	    bool operator==(const LemmaIterator &rhs) const {
		return current_ == rhs.current_;
	    }

	    bool operator!=(const LemmaIterator &rhs) const {
		return current_ != rhs.current_;
	    }
	};

	/** The number of lemmata this pronunciation occurs in. */
	u32 nLemmas() const;

	/**
	 * Iterator-range giving access to the set of lemmata this
	 * pronunciation occurs in.
	 */
	std::pair<LemmaIterator, LemmaIterator>
	lemmas() const {
	    return std::make_pair(LemmaIterator(lemmas_),
				  LemmaIterator(0));
	}

	/** Phoneme sequence terminated by Phoneme::term. */
	const Phoneme::Id *phonemes() const {
	    return phonemes_;
	}

	/** Length of pronunciation in number of phonemes. */
	u32 length() const;

	Phoneme::Id operator[](unsigned i) const {
	    require_((0 <= i) && (i < unsigned(length())));
	    return phonemes_[i];
	}

	/** Create string representation */
	std::string format(Core::Ref<const PhonemeInventory>) const;
    };

    /**
     * Wrapper for coarticulated objects, e.g. Pronunciation with coarticulation
     * at the beginning and end.
     */

    template<class Object>
    class Coarticulated
    {
    private:
	const Object &object_;
	Phoneme::Id leftContext_;
	Phoneme::Id rightContext_;
    public:
	Coarticulated(const Object &object,
		      Phoneme::Id leftContext = Phoneme::term,
		      Phoneme::Id rightContext = Phoneme::term) :
	    object_(object),
	    leftContext_(leftContext),
	    rightContext_(rightContext) {}
	~Coarticulated() {}

	const Object& object() const { return object_; }
	Phoneme::Id leftContext() const { return leftContext_; }
	Phoneme::Id rightContext() const { return rightContext_; }
	std::string format(Core::Ref<const PhonemeInventory> phonemeInventory) const;
    };

    template<class Object>
    std::string Coarticulated<Object>::format(Core::Ref<const PhonemeInventory> phonemeInventory) const {
	const std::string l(leftContext() == Phoneme::term ? "#" :
			    phonemeInventory->phoneme(leftContext())->symbol().str());
	const std::string r(rightContext() == Phoneme::term ? "#" :
			    phonemeInventory->phoneme(rightContext())->symbol().str());
	return Core::form("%s|%s|%s", l.c_str(), object().format(phonemeInventory).c_str(), r.c_str());
    }

    /**
     * A syntactic unit aka language model token in the Bliss lexicon.
     * @see <a href="../../doc/Bliss.pdf">Bliss documentation</a>
     * @todo class emission probabilities are quite slow due to repeated calculation of logarithm
     */

    class SyntacticToken : public Token {
    private:
	typedef std::vector<const Lemma*> LemmaList;
	LemmaList lemmas_;
    protected:
	friend class Lexicon;
	SyntacticToken(Bliss::Symbol _symbol) : Token(_symbol) {}

	void addLemma(const Lemma *lemma) {
	    lemmas_.push_back(lemma);
	}

    public:
	/** The number of lemmata this token occurs in. */
	u32 nLemmas() const {
	    return lemmas_.size();
	}

	/**
	 * Probability of a lemma with this syntactic token.
	 * Several lemmata can be assigned to the same syntactic
	 * token.  Conceptually we can write this as p(lemma |
	 * history) = p(lemma | synt) p(synt | history).  This
	 * function returns only - log(p(lemma | synt)) under the
	 * assumption of a unifor distribution.
	 * @warning The interaction with language model phrases is a
	 * theoretically unsolved problem.
	 */
	f32 classEmissionScore() const {
	    return std::log(float(nLemmas()));
	}

	typedef LemmaList::const_iterator LemmaIterator;

	/**
	 * Iterator-range giving access to the set of lemmata this
	 * token occurs in.
	 */
	std::pair<LemmaIterator, LemmaIterator> lemmas() const {
	    return std::make_pair(lemmas_.begin(), lemmas_.end());
	}
    };

    /**
     * A evaluation token in the Bliss lexicon.
     * @see <a href="../../doc/Bliss.pdf">Bliss documentation</a>
     */

    class EvaluationToken : public Token {
    protected:
	friend class Lexicon;
	EvaluationToken(Bliss::Symbol _symbol) : Token(_symbol) {}
    };

    class LetterAlphabet;
    class LemmaAlphabet;
    class LemmaPronunciationAlphabet;
    class SyntacticTokenAlphabet;
    class EvaluationTokenAlphabet;
    class PhonemeToLemmaPronunciationTransducer;
    class LemmaPronunciationToLemmaTransducer;
    class LemmaToSyntacticTokenTransducer;
    class LemmaToEvaluationTokenTransducer;

    /**
     * The Bliss lexicon.
     *
     * See the <a href="../../doc/Bliss.pdf">Bliss documentation</a> and
     * <a href="../../doc/Lexicon.pdf">Lexicon Format Reference</a>
     * for comporehensive explanation.
     *
     * An entry of the lexicon are called "lemma".  (The term "word"
     * is avoided because of its ambiguity.)  A lemma has one or more
     * orthographic forms, zero or more pronunciations (aka "acoustic
     * words") and zero or more syntactic tokens (aka "language model
     * words").
     *
     * A lemma may be assigned a symbolic name, which the system can
     * use to identify lemmas which have a special meaning to it.
     * E.g. the silence word is is identified by the symbolic name
     * "silence".  Such lemmas a called "special lemmas".
     */

    class Lexicon :
	public Core::ReferenceCounted,
	public Core::Component
    {
	static Core::ParameterString paramFilename;
    protected:
	friend class LexiconElement;

	Core::Dependency dependency_;

	/** central string storage
	 * contains: orthographic forms, letters, lemma names, and
	 * syntactic and evaluation symbols */
	SymbolSet symbols_;

	/** orthographic form lists */
	SymbolSequenceSet<Symbol> symbolSequences_;

	// letters
	friend class Bliss::LetterAlphabet;
	TokenInventory letters_;

	// phonemes
	Core::Ref<const PhonemeInventory> phonemeInventory_;

	// lemmas
	friend class Bliss::LemmaAlphabet;
	TokenInventory lemmas_;
	typedef Core::StringHashMap<Lemma*> LemmaMap;
	LemmaMap specialLemmas_;

	friend class Bliss::LemmaPronunciationAlphabet;
	typedef std::vector<const LemmaPronunciation*> LemmaPronunciationList;
	LemmaPronunciationList lemmaPronunciationsByIndex_;
	Core::Obstack<LemmaPronunciation> lemmaPronunciations_;

	// pronunciations
	Core::Obstack<Phoneme::Id> phon_; /**< pronunciations */
	typedef std::vector<const Pronunciation*> PronunciationList;
	PronunciationList pronunciations_;
	typedef Core::hash_set<Pronunciation*, Pronunciation::Hash,
			       Pronunciation::Equality> PronunciationMap;
	PronunciationMap pronunciationMap_;
	Pronunciation *getOrCreatePronunciation(const std::vector<Phoneme::Id> &phonemes);

	// syntactic tokens
	friend class Bliss::SyntacticTokenAlphabet;
	Core::Obstack<const SyntacticToken *> synts_; /**< syntactic token sequences */
	TokenInventory syntacticTokens_;
	SyntacticToken *getOrCreateSyntacticToken(Symbol);

	// evaluation tokens
	friend class Bliss::EvaluationTokenAlphabet;
	Core::Obstack<const EvaluationToken *> evals_; /**< evaluation token sequences */
	TokenInventory evaluationTokens_;
	EvaluationToken *getOrCreateEvaluationToken(Symbol);

	/** Convert phonemic string to sequence of phoneme ids */
	void parsePronunciation(const std::string&, std::vector<Phoneme::Id>&) const;

	struct Internal;
	Internal *internal_;

    public:
	Lexicon(const Core::Configuration&);

	const Core::Dependency& getDependency() const { return dependency_; }

	/** Create a new lemma. */
	Lemma *newLemma();

	/** Create a new lemma with forced name. */
	Lemma *newLemma(const std::string&);

	/**
	 * Set the list of orthographic forms for a lemma.
	 */
	void setOrthographicForms(Lemma *lemma, const std::vector<std::string> &orths);

	/**
	 * Set the unique name of a lemma.
	 */
	void setDefaultLemmaName(Lemma *lemma);

	/**
	 * Get a pronunciation for a string representation.
	 * @param phon a string containing a white-space separate list
	 * of phoneme symbols.
	 */
	Pronunciation *getPronunciation(const std::string &phon);

	/**
	 * Add a pronunciation to a lemma.
	 * @param pron a pronunciation, typically obtained from
	 * getPronunciation()
	 * @param weight a non negative (e.g. the observation count)
	 */
	void addPronunciation(
	    Lemma *lemma,
	    Pronunciation *pron,
	    f32 weight = 1.0);
	void normalizePronunciationWeights(Lemma *lemma);

	/**
	 * Set the a syntactic token sequence for a lemma.
	 */
	void setSyntacticTokenSequence(Lemma *lemma, const std::vector<std::string> &synt);
	void setDefaultSyntacticToken(Lemma *lemma);

	/**
	 * Set the a evaluation token sequence for a lemma.
	 */
	void addEvaluationTokenSequence(Lemma *lemma, const std::vector<std::string> &eval);
	void setDefaultEvaluationToken(Lemma *lemma);

	/**
	 * Assign a symbolic name to a lemma.
	 * @param name is a symbolic name to which @c lemma is to be
	 * assigned.  @c name must not have been assigned before.
	 * @param lemma to be assigned to @c name
	 */
	void defineSpecialLemma(const std::string &name, Lemma *lemma);

	/**
	 * Load lexicon from XML file.
	 */
	void load(const std::string &filename);

	void writeXml(Core::XmlWriter&) const;

	void logStatistics() const;

	/**
	 * Create a new lexicon from a Configuration.
	 * If a file name is specified, that file is loaded into the
	 * new lexicon.
	 * @return the newly created lexicon or 0 in case of failure
	 * (e.g. file not found).
	 */
	static LexiconRef create(const Core::Configuration&);

	~Lexicon();

	// -------------------------------------------------------------------
	// Lemmas

	u32 nLemmas() const {
	    return lemmas_.size();
	}

	typedef const Lemma *const *LemmaIterator;

	/** Iterator-range giving access to the set of lemmas. */
	std::pair<LemmaIterator, LemmaIterator> lemmas() const {
	    return std::make_pair(
		LemmaIterator(lemmas_.begin()),
		LemmaIterator(lemmas_.end()));
	}

#ifdef OBSOLETE
	/**
	 * Find a lemma via ID number.
	 * Remember that lemma IDs must not be associated with a
	 * particular meaning.  The only purpose you want to use them
	 * for, is referencing a lemma from an external data structure such as
	 * a Fsa::Automaton.
	 * @param id an integer lemma ID
	 * @return the lemma with the requested ID or 0 if there is no
	 * such lemma.  A return value of 0, usually indicates an
	 * error (e.g. wrong lexicon), but it is the callers duty to
	 * handle this.
	 */
	const Lemma *lemma(Lemma::Id id) const {
	    const Lemma *result = 0;
	    if (id < lemmas_.size()) {
		result = lemmas_[id];
		ensure(result->id() == id);
	    }
	    return result;
	}
#endif // OBSOLETE

	/**
	 * Find a lemma via ID string. This name can either be given
	 * upon creation of a lemma or by setting a list of
	 * orthographic forms.  In the later case we simple take the
	 * preferred orthographic form, possibly disambiguating it.
	 * The only purpose you want to use the lemma name for, is
	 * referencing a lemma from an external file (e.g. a word
	 * lattice).
	 * @warning You must not assciate a particular meaning with
	 * the name of a lemma.  Never hard-code lemma names!  Use
	 * specialLemma() instead!
	 * @param str a string ID
	 * @return the lemma with the requested ID or 0 if there is no
	 * such lemma.  A return value of 0, usually indicates an
	 * error (e.g. wrong lexicon), but it is the callers duty to
	 * handle this.
	 */
	const Lemma *lemma(const std::string &str) const {
	    return static_cast<const Lemma*>(lemmas_[str]);
	}

	/**
	 * Find a lemma for a given symbolic name.
	 * This function should be used to associate a particular
	 * meaning with a lemma.
	 * @return the lemma which is assigned to @c name or 0 iff @c
	 * name has not been assigned. A return value of 0 indicates
	 * that the used has not added the requested special lemma.
	 * It is the callers duty to handle this case appropriately.
	 */
	const Lemma *specialLemma(const std::string &name) const;

	Core::Ref<const LemmaAlphabet> lemmaAlphabet() const;

	// -------------------------------------------------------------------
	// Pronunciations

	void setPhonemeInventory(Core::Ref<const PhonemeInventory>);

	/** The phoneme inventory of the lexicon. */
	Core::Ref<const PhonemeInventory> phonemeInventory() const { return phonemeInventory_; }

	/** Number of distinct pronunciations defined in the lexicon. */
	u32 nPronunciations() const {
	    return pronunciations_.size();
	}

	typedef PronunciationList::const_iterator PronunciationIterator;

	/** Iterator-range giving access to the set of pronunciations. */
	std::pair<PronunciationIterator, PronunciationIterator> pronunciations() const {
	    return std::make_pair(pronunciations_.begin(), pronunciations_.end());
	}

	// -------------------------------------------------------------------
	// Lemma Pronunciations

	/** Number of distinct lemma pronunciations defined in the lexicon. */
	u32 nLemmaPronunciations() const {
	    return lemmaPronunciationsByIndex_.size();
	}

	typedef LemmaPronunciationList::const_iterator LemmaPronunciationIterator;

	/** Iterator-range giving access to the set of lemma-pronunciations. */
	std::pair<LemmaPronunciationIterator, LemmaPronunciationIterator> lemmaPronunciations() const {
	    return std::make_pair(lemmaPronunciationsByIndex_.begin(),
				  lemmaPronunciationsByIndex_.end());
	}

	Core::Ref<const LemmaPronunciationAlphabet> lemmaPronunciationAlphabet() const;

	const LemmaPronunciation* lemmaPronunciation(s32 id) const {
	    return lemmaPronunciationsByIndex_[id];
	}

	// -------------------------------------------------------------------
	// Syntactic Tokens

	u32 nSyntacticTokens() const {
	    return syntacticTokens_.size();
	}

	typedef const SyntacticToken *const *SyntacticTokenIterator;

	/** Iterator-range giving access to the set of syntactic tokens. */
	std::pair<SyntacticTokenIterator, SyntacticTokenIterator> syntacticTokens() const {
	    return std::make_pair(
		SyntacticTokenIterator(syntacticTokens_.begin()),
		SyntacticTokenIterator(syntacticTokens_.end()));
	}

	/**
	 * Find a syntactic token given its symbolic name.
	 * This function is intended to be used for language model
	 * initialization.  The symbolic name of a syntactic token
	 * must not be associated with a particular meaning.
	 * @return the syntactic token with the given name, or 0 if
	 * there is no such token.
	 */
	const SyntacticToken *syntacticToken(const std::string &synt) const {
	    return static_cast<const SyntacticToken*>(syntacticTokens_[synt]);
	}

	const TokenInventory &syntacticTokenInventory() const {
	    return syntacticTokens_;
	}
	Core::Ref<const SyntacticTokenAlphabet> syntacticTokenAlphabet() const;

	// -------------------------------------------------------------------
	// Evaluation Tokens

	u32 nEvaluationTokens() const {
	    return evaluationTokens_.size();
	}

	typedef const EvaluationToken *const *EvaluationTokenIterator;

	/** Iterator-range giving access to the set of syntactic tokens. */
	std::pair<EvaluationTokenIterator, EvaluationTokenIterator> evaluationTokens() const {
	    return std::make_pair(
		EvaluationTokenIterator(evaluationTokens_.begin()),
		EvaluationTokenIterator(evaluationTokens_.end()));
	}

	const TokenInventory &evaluationTokenInventory() const {
	    return evaluationTokens_;
	}
	Core::Ref<const EvaluationTokenAlphabet> evaluationTokenAlphabet() const;

	// -------------------------------------------------------------------

	const Letter *letter(const std::string&) const;

	const TokenInventory &letterInventory() const {
	    return letters_;
	}
	Core::Ref<const LetterAlphabet> letterAlphabet() const;

	/**
	 * Create conversion transducer from lemma to orthographic forms.
	 * Note: For conversion from text string to lemmata you may
	 * prefer to use OrthographicParser or LemmaAcceptorBuilder.
	 * lemma-to-orthography transducer properties:
	 * - The lemma label is the first input symbol.
	 * - Output labels are (Unicode) letters.  (Yes, individual
	 * letters! There are nor orthographic tokens!)
	 * @param shallIncludeVariants if true, all orthographic forms
	 * are included.  Otherwise only the preferred (first)
	 * orthographic form of each lemma is included.
	 * @param onlyWithEvalToken if true, only orthographic forms
	 * of lemmas which have at least one eval token are included.
	 */
	Core::Ref<Fsa::Automaton> createLemmaToOrthographyTransducer(bool shallIncludeVariants = true, bool onlyWithEvalToken = false) const;

	/**
	 * Create mapping transducer from phoneme sequences to lemma
	 * pronunciation sequences.
	 * Properties of the result transducer:
	 * - Phonemes are input labels.
	 * - The LemmaPronunciation is the first output label.
	 * - All phoneme strings are terminated with a disambiguator
	 * which serves as an end-of-word maker.
	 * - If shallDisambiguate is true, homophones are terminated
	 * by *different* disambiguators, so that the resulting
	 * transducer is determinizable.  Otherwise all pronunciations
	 * end with the same disambiguator.
	 * - No disambiguators occur on the output side.
	 * - All weights are one.
	 * - The transducer is cyclic an accepts zero or more word.
	 * Partial words are not accepted.
	 * @param disambiguate if true, disambiguation symbols are
	 * added on the input side.
	 */
	Core::Ref<PhonemeToLemmaPronunciationTransducer> createPhonemeToLemmaPronunciationTransducer(bool disambiguate = true) const;
	Core::Ref<PhonemeToLemmaPronunciationTransducer> createPhonemeToLemmaPronunciationTransducer(
			size_t nDisambiguators, bool disambiguate, bool markInitialPhonesAndSilence = false) const;
	Core::Ref<PhonemeToLemmaPronunciationTransducer> createPhonemeToLemmaPronunciationAndStickPunctuationTransducer() const;

	/**
	 * Create mapping transducer from lemma pronunciation to lemma
	 * sequences.
	 * Pronunciation prior probability is included in the weights:
	 * Each weight correspond to
	 * LemmaPronunciation::pronunciationScore().  This means the
	 * result transducer models p(pron | lemma).
	 */
	Core::Ref<LemmaPronunciationToLemmaTransducer> createLemmaPronunciationToLemmaTransducer(u32 nDisambiguators = 0) const;

	/**
	 * Create mapping transducer from lemma sequences to syntactic
	 * token sequences.  Necessary for composition with language
	 * model transducers/acceptors.
	 * - The lemma is the first input label.
	 * - Syntactic token emission scores are included.  Weights
	 * correspond to SyntacticToken::classEmissionScore(),
	 * i.e. this automaton models p(lemma | synt).
	 * @param if useEmptySyntacticTokenSequences is false, those
	 * lemmas with an empty syntactic token sequence are
	 * suppressed.
	 */
	Core::Ref<LemmaToSyntacticTokenTransducer> createLemmaToSyntacticTokenTransducer
	(bool useEmptySyntacticTokenSequences = true, size_t nDisambiguators = 0) const;

	/**
	 * Create mapping transducer from lemma sequences to
	 * evaluation token sequences.  Necessary, e.g., for
	 * computation of word (graph) error rate.
	 */
	Core::Ref<LemmaToEvaluationTokenTransducer> createLemmaToEvaluationTokenTransducer(bool allowAlternatives = true) const;

	/**
	 * Create mapping transducer from lemma sequences to
	 * evaluation token sequences. In the presence of alternative
	 * evaluation token sequences, the first is used.
	 */
	Core::Ref<LemmaToEvaluationTokenTransducer> createLemmaToPreferredEvaluationTokenSequenceTransducer() const;
    };

} // namespace Bliss

#endif // _BLISS_LEXICON_HH

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
// $Id: LanguageModel.hh 9621 2014-05-13 17:35:55Z golik $

#ifndef _LM_LANGUAGE_MODEL_HH
#define _LM_LANGUAGE_MODEL_HH

#include <Core/Component.hh>
#include <Core/Dependency.hh>
#include <Core/Types.hh>
#include <Bliss/Lexicon.hh>
#include <Bliss/Symbol.hh>
#include <Fsa/Automaton.hh>
#include <Core/Hash.hh>
#include <vector>
#include "HistoryManager.hh"

namespace Lm {

    /**
     * Language model probabilities are generally represented by the
     * negative of their natural (!) logarithm:
     * probability = exp( - score )
     */
    typedef f32 Score;

    typedef const Bliss::Token * Token;
    static const Token InvalidToken = Token(0);

    typedef Bliss::TokenInventory TokenInventory;


    class History {
    private:
	HistoryManager *mang_;
	HistoryHandle desc_;
	class SentinelHistoryManager : public SingletonHistoryManager {
	    virtual HistoryHash hashKey(HistoryHandle) const { defect(); return 0; }
	    virtual std::string format(HistoryHandle) const { return "UNDEFINED"; }
	};
	static SentinelHistoryManager sentinel;
    private:
	friend class LanguageModel;
	History(HistoryManager *hm, HistoryHandle hd) : mang_(hm), desc_(hm->acquire(hd)) {}

    public:
	History() : mang_(&sentinel), desc_(0) {}
	History(const History &hh) : mang_(hh.mang_), desc_(mang_->acquire(hh.desc_)) {}
	~History() { mang_->release(desc_); }

	HistoryHandle handle() const {
	    return desc_;
	}

	const History &operator= (const History &rhs) {
	    HistoryHandle oldDesc = desc_;
	    desc_ = rhs.mang_->acquire(rhs.desc_);
	    mang_->release(oldDesc);
	    mang_ = rhs.mang_;
	    return *this;
	}

	bool operator<(const History& rhs) const {
	    return desc_ < rhs.desc_;
	}

	/** Test for non-voidness. */
	bool isValid() const {
	    return mang_ != &sentinel;
	}

	bool operator==(const History &rhs) const {
#ifdef LM_ALLOW_CROSS_LM_COMPARISON
	    if (mang_ != rhs.mang_) return false;
#else
	    require_(mang_ == rhs.mang_);
#endif
	    if (desc_ == rhs.desc_) return true;
	    return mang_->isEquivalent(desc_, rhs.desc_);
	}

	HistoryHash hashKey() const {
	    return mang_->hashKey(desc_);
	}

	struct Hash {
	    HistoryHash operator()(const Lm::History &h) const {
		return h.hashKey();
	    }
	};

	/**
	 * Create string representation (for debugging puposes).
	 * The preferred way to access this function is through
	 * calling LanguageModel::formatHistory(), since some language
	 * models specialize this formatHistory() to give more
	 * readable information.
	 * If your language model is m-gram like, describe the history
	 * chronologically, i.e. most recent word last.
	 */
	std::string format() const {
	    return mang_->format(desc_);
	}
    };

    struct Request {
	Bliss::SyntacticTokenSequence tokens;
	u32 target;
	Score offset;
	Request(const Bliss::SyntacticTokenSequence &s, u32 t, Score o = 0.0) : tokens(s), target(t), offset(o) {}
    };
    typedef std::vector<Request> BatchRequest;

    class CompiledBatchRequest {
    protected:
	Score scale_;
	CompiledBatchRequest(Score scale) : scale_(scale) {}
    public:
	virtual ~CompiledBatchRequest() {}
	Score scale() const { return scale_; }
	void setScale(Score scale) { scale_ = scale; }
    };

    /**
     */
    class LanguageModel;

    class LanguageModelAutomaton : public Fsa::Automaton {
    protected:
	Bliss::LexiconRef lexicon_;
	Fsa::ConstAlphabetRef alphabet_;
	Fsa::LabelId backOffLabel_;
    public:
	LanguageModelAutomaton(Bliss::LexiconRef lexicon);
	LanguageModelAutomaton(Core::Ref<const LanguageModel> lm);
	virtual Fsa::Type type() const { return Fsa::TypeAcceptor; }
	virtual Fsa::ConstSemiringRef semiring() const { return Fsa::TropicalSemiring; }
	virtual Fsa::ConstAlphabetRef getInputAlphabet() const { return alphabet_; }
	virtual Fsa::ConstAlphabetRef getOutputAlphabet() const { return alphabet_; }
	virtual std::string describe() const { return "lm"; }
    };

    /**
     * Abstract language model.
     *
     * This class provides the abtract state machine interface used by
     * all language models in Sprint.
     *
     * Design: An abstract state object (of class History) is used to
     * describe the history (preceeding words).  History object can be
     * used to inquire the probability of the next word (score()),
     * and they can be extended by a word (extendHistory()).  The
     * (concrete) language model itself specifies what information is
     * stored in the history descriptor.
     *
     * Building your own language model: Mimimally, you have to
     * implement the three abstract (prue virtual) methods
     * startHistory(), extendHistory() and score().  Some other
     * methods only have inefficient default implmentations, you might
     * have to overload them.
     *
     * On sentence boundaries:
     * In order to fit sentence boundary modelling smoothly into the
     * n-gram method, one commoly introduces special sentence boundary
     * tokens.  Finite state grammars will normally not do this: Here
     * the initial state and final scores do the job.  For n-gram
     * models however, there are two common conventions:
     * A. The nesting-oriented (two symbol) approach uses one token
     * for the beginning of the sentence and one for the end of the
     * sentence.  Borrowing from XML/SGML syntax these are commonly
     * denoted as "<s>" and "</s>" respectively.  It is silently
     * assumed that p(<s> | </s>) = 1.
     * B. The stream-oriented (one symbol) approach uses only a single
     * sentence boundary symbol.  In XML-ish syntax it should be
     * called "<s/>", but common symbols are "@", "<s>" (which would
     * be aprropriate in SGML), and "</s>" (which would be wrong in
     * both XML and SGML).
     */

    class LanguageModel :
	public Core::ReferenceCounted,
	public virtual Core::Component
    {
    protected:
	/**
	 * Concrete LanguageModels should set this member in their
	 * constructor or load() method.  It is entirely legitimate to
	 * inherit HistoryManager and set historyManager_ = this.
	 */
	HistoryManager *historyManager_;

	History history(const void *hd) const {
	    require_(historyManager_);
	    return History(historyManager_, hd);
	}

	template <class LM>
	const typename LM::HistoryDescriptor *descriptor(const History &hh) const {
	    require_(hh.mang_ == historyManager_);
	    return static_cast<const typename LM::HistoryDescriptor*>(hh.desc_);
	}

    private:
	Bliss::LexiconRef lexicon_;
	const TokenInventory *tokenInventory_;
	Fsa::ConstAlphabetRef tokenAlphabet_;
	Token fallbackToken_;
	Token sentenceBeginToken_, sentenceEndToken_;
	bool requiresSentenceBoundaryToken_;

    protected:
	Core::Dependency dependency_;

    protected:
	void setTokenInventory(const TokenInventory *_tokenInventory);
	void setTokenInventoryAndAlphabet(const TokenInventory *_tokenInventory, Fsa::ConstAlphabetRef _tokenAlphabet);

	Token getToken(
	    const std::string &word) const
	    { require_(tokenInventory_); return (*tokenInventory_)[word]; }

	virtual Token getSpecialToken(
	    const std::string &name, bool required = false) const;

	/*
	  Candidate for being moved to Am::LexiconUtilities,
	  which in turn should be moved to Bliss::LexiconUtilities
	*/
	const Bliss::SyntacticToken * getSpecialSyntacticToken(
	    const std::string &name, bool required) const;

	/**
	 * Find token to use for unknown words.
	 *
	 * This function should be used by language model
	 * initialization routines to replace any syntactic token that
	 * is unknown to the language model.  (In this case a warning
	 * must be issued.)
	 */
	Token fallbackToken() const {
	    return fallbackToken_;
	}
    public:
	Token sentenceBeginToken() const {
	    return sentenceBeginToken_;
	}
	virtual Lm::Score sentenceBeginScore() const {
	    return 0;
	}
	Token sentenceEndToken() const {
	    return sentenceEndToken_;
	}
	Token sentenceBoundaryToken() const {
	    return (sentenceBeginToken_ == sentenceEndToken_) ? sentenceEndToken_ : 0;
	}
    protected:
	/*
	  create set of special tokens;
	  does normally require synchronisation with lexicon
	*/
	virtual void createSpecialTokens();
	/*
	  load language model
	*/
	virtual void load() {}

	/**
	 * Call this function before init(), iff your language
	 * model requires a sentence boundary token to be defined in
	 * the lexicon.
	 */
	void requireSentenceBoundaryToken()
	    { requiresSentenceBoundaryToken_ = true; }

	LanguageModel(const Core::Configuration &c,
		      Bliss::LexiconRef l);
    public:
	virtual ~LanguageModel();

	/**
	 * precondition:
	 * the token inventory is set and fixed
	 *
	 * Initalize language model.  This method is called immeditelly
	 * after construction.  Implement this function in order to
	 * perform all non-trivial initialization, like reading the
	 * language model from a file.
	 *
	 */
	virtual void init();

	virtual void getDependencies(Core::DependencySet &dependencies) const {
	    return dependencies.add("language model", dependency_);
	}

	Bliss::LexiconRef lexicon() const
	    { return lexicon_; }
	const TokenInventory & tokenInventory() const
	    { require(tokenInventory_); return *tokenInventory_; }
	Fsa::ConstAlphabetRef tokenAlphabet() const
	    { require(tokenAlphabet_); return tokenAlphabet_; }

	/**
	 * Convert language model into an EQUIVALENT acceptor.
	 * Keep in mind that the default method may produce
	 * transducers that are very large.  Note: This does just
	 * convert your language model to a different format, it does
	 * NOT implement the language model.
	 *
	 * Labels of the resulting acceptor are syntactic token ids.
	 **/
	virtual Fsa::ConstAutomatonRef getFsa() const;

	/**
	 * Language model history for beginning of a sentence.
	 * Typically this is the bigram history which contains just
	 * the sentence start (or boundary) token.  In the terminology
	 * of finite-state automata this is the initial state.
	 */
	virtual History startHistory() const = 0;

	/**
	 * Language model history for the next sentence position.
	 * This is the given history with given word appended, i.e. w
	 * becomes the most recent word.  Typically the returned
	 * history will reflect the finite memory of the language
	 * model: for an m-gram it will only contain the m-1 most
	 * recent words.  In WFST terminology this is the target state
	 * of the outgoin arc labeled w.
	 */
	virtual History extendedHistory(const History&, Token w) const = 0;

	/**
	 * Reduce amount on conditioning information.
	 * In theory extendHistory() adds more and more information to
	 * the distory descriptor.  Most reasonable language models
	 * will drop irrelevant information.  E.g. a trigram LM will
	 * actually store only two words in its history objects; so
	 * the number of distinct histories is size of vocabulary
	 * squared.  In some situations one wants to limit the number
	 * of histories considered, by reducing the amount of
	 * conditioning information in the history.  E.g. when using a
	 * bigram lookahead in a trigram search.  This is the purpose
	 * of extendHistory().
	 * @param limit Maximum amount of conditioning information in
	 * the returned history.  The meaning this parameter is
	 * dependent on the language model.  In general higher values
	 * mean more con longer histories.
	 * The default implementation returns the given history unchanged.
	 */
	virtual History reducedHistory(const History&, u32 limit) const;

	/**
	 * Create string representation of history (mainly for debugging purposes).
	 * By default this function just delegates to
	 * History::format(), but language models may specialize
	 * formatHistory() to give more readable information, which
	 * would be inaccessible from the HistoryDescriptor alone.
	 * If your language model is m-gram like, describe the history
	 * chronologically, i.e. most recent word last.
	 */
	virtual std::string formatHistory(const History &h) const {
	    return h.format();
	}

	/**
	 * Probability of a token to occur with a given history.
	 *
	 * @return: negative natural(!) logarithm of probability of
	 * the token w to occur with history h , i.e. -log(p(w|h))
	 *
	 * Note on syntactic classes: The Bliss lexicon can map
	 * several lexicon entries (lemmas) to the same syntactic
	 * token.  Conceptually we can write this case as p(lemma |
	 * history) = p(lemma | synt) p(synt | history).  However this
	 * function returns only p(synt | history).  Clients have to
	 * add the lemmaScore -log(p(lemma | synt)) when needed.
	 *
	 * Note on pronunciation variants: Pronuciation probabilities
	 * are not into account.
	 */
	virtual Score score(const History&, Token w) const = 0;

	/**
	 * Probability for sentence to end in current position.
	 * In WFST terminology this is the final weight.
	 * If the lexicon defines a "sentence-end" special token this
	 * must be equivalent to getScore(h, sentenceBoundaryToken()).
	 * The default implementation does just this.
	 * @return negative natural(!) logarithm of probability of
	 * sentence end, i.e. -log(p(EOS|h))
	 */
	virtual Score sentenceEndScore(const History&) const;

	/**
	 * Preprocess a batch request.
	 * @return a pre-compiled batch request which can be used with getBatch()
	 */
	virtual CompiledBatchRequest* compileBatchRequest(const BatchRequest&, Score scale = 1.0) const;

	/**
	 * Get probabilities for a (large) set of syntactic tokens given a history.
	 * This is mainly useful for LanguageModelLookahead.
	 * @warning Default implementation is inefficient.
	 * @param h history
	 * @param r pre-compiled batch request created using compileBatchRequest()
	 * @param result a vector in which results are stored (output parameter)
	 */
	virtual void getBatch(
	    const History &h,
	    const CompiledBatchRequest *r,
	    std::vector<f32> &result) const;

    protected:
	class NonCompiledBatchRequest : public CompiledBatchRequest {
	public:
	    BatchRequest request;
	    NonCompiledBatchRequest(Score scale) : CompiledBatchRequest(scale) {}
	};
    };

    /**
     * Language model score convenience function for lemma pronunciations.
     */
    inline void extendHistoryByLemmaPronunciation(
	Core::Ref<const LanguageModel> lm,
	const Bliss::LemmaPronunciation *pronunciation,
	History &history)
    {
	require(pronunciation);
	const Bliss::Lemma *lemma = pronunciation->lemma();
	require(lemma);
	const Bliss::SyntacticTokenSequence tokenSequence(lemma->syntacticTokenSequence());
	for (u32 ti = 0; ti < tokenSequence.length(); ++ti) {
	    const Bliss::SyntacticToken *st = tokenSequence[ti];
	    history = lm->extendedHistory(history, st);
	}
    }

} // namespace Lm

#endif // _LM_LANGUAGE_MODEL_HH

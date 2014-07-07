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
// $Id: BackingOff.hh 9621 2014-05-13 17:35:55Z golik $

#ifndef _LM_BACKINGOFF_HH
#define _LM_BACKINGOFF_HH

#include <Core/Parameter.hh>
#include "LanguageModel.hh"
#include <unistd.h> // for lseek, write, etc.

namespace Lm {

    /**
     * General backing-off language model.
     *
     * Decendant classes only have to implement the read() method,
     * which should build a range of InitItem and call initialize().
     */

    class BackingOffLm :
	public LanguageModel
    {
	typedef BackingOffLm Self;
    public:
	class Internal;
	class Node;
    private:
	static const Core::ParameterString paramImage;
	friend class Internal;
	Core::Ref<Internal> internal_;
	class Automaton;
	void logInitialization() const;
    protected:
	BackingOffLm(const Core::Configuration&, Bliss::LexiconRef);

	/**
	 * Back-off language model item.
	 *
	 * For items with non-zero token, score gives the negative
	 * natural logarithm of p(token | history):
	 * score = - ln ( p(token | history) )
	 *
	 * Items with token equals zero provide back-off weights:
	 * score = - ln ( p(w | history) / p(w | shortened_history) )
	 * Obviously this term has to be constant over all words w
	 * for which the back-off case occurs.  (Actually, this could
	 * be used as the the definition of "back-off case".)
	 */
	struct InitItem {
	    Token *history; /**< zero-terminated string, recent-most first */
	    Token token;    /**< predicted word, or zero iff back-off */
	    Score score;    /**< negative natural logarithm of p(token | history). */
	};

	/**
	 * Initialize back-off language model from list of InitItems.
	 * Should be called from read().  The input data may be
	 * modified during the call, and is no longer needed
	 * afterwards.
	 */
	void initialize(InitItem *begin, InitItem *end);

	/**
	 * Alternative initialization method.
	 * Should be called from read().
	 * Build everything yourself, and pass over the finished
	 * internal data structure.
	 */
	void initialize(Core::Ref<Internal>);

	/**
	 * Implement this this function to read the language model
	 * from a file.
	 */
	virtual void read() = 0;

    public:
	typedef Node HistoryDescriptor;
	virtual void load();
	virtual ~BackingOffLm();
	virtual History startHistory() const;
	virtual Lm::Score sentenceBeginScore() const;
	virtual History extendedHistory(const History&, Token w) const;
	virtual History reducedHistory(const History&, u32 limit) const;
	virtual std::string formatHistory(const History&) const;
	virtual Lm::Score score(const History&, Token w) const;
	virtual void getBatch(const History&, const CompiledBatchRequest*,
			      std::vector<f32> &result) const;
	virtual Fsa::ConstAutomatonRef getFsa() const;
	/**
	 * Writes all tokens stored in the given history into the given vector
	 */
	void historyTokens(const History& history, const Bliss::Token** target, u32& size, u32 arraySize) const;

	u32 historyLenght(const History& history) const;

	struct WordScore {
	    Bliss::Token::Id token_;
	    Lm::Score score_;
	public:
	    Bliss::Token::Id token() const { return token_; }
	    Lm::Score score() const { return score_; }
	    struct Ordering {
		bool operator() (const WordScore &a, const WordScore &b) const {
		    return (a.token() < b.token());
		}
	    };
	};

	struct BackOffScores {
	    BackOffScores() : start(0), end(0), backOffScore(0) {
	    }
	    const WordScore* start;
	    const WordScore* end;

	    //Back-off score offset that is applied to the lower-order back-off scores (not to these ones)
	    Score backOffScore;
	};

	/**
	 * Directly returns the scores stored in the LM for the given context history.
	 */
	BackOffScores getBackOffScores(const History &history, int depth = 0) const;
    /**
     * Returns the accumulated backing-off scores up the the given limit (not the actual contained word-scores)
     * Examples:
     * With limit 0, returns the sum of the back-off offsets up to the _zerogram_ level.
     * With limit 1, returns the sum of the back-off offsets up to the _unigram_ level.
     * */
    Score getAccumulatedBackOffScore(const History &history, int limit) const;
    };

} // namespace Lm

#endif // _LM_BACKINGOFF_HH

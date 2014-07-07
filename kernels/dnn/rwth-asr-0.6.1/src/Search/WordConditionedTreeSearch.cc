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
// $Id: WordConditionedTreeSearch.cc 9227 2013-11-29 14:47:07Z golik $

#include "WordConditionedTreeSearch.hh"

#include <Modules.hh>
#include <vector>
#include <iomanip>
#include <Core/Hash.hh>
#include <Core/Statistics.hh>
#include <Core/Utility.hh>
#include <Fsa/Automaton.hh>
#include <Fsa/Alphabet.hh>
#include <Fsa/Hash.hh>
#include <Fsa/Output.hh>
#include <Lattice/Lattice.hh>
#include <Lattice/LatticeAdaptor.hh>
#include "Histogram.hh"
#include "StateTree.hh"
#include "LanguageModelLookahead.hh"

#ifdef MODULE_LM_FSA
#include <Lm/FsaLm.hh>
#endif

using namespace Search;
using Core::Ref;
using Core::tie;


/*
 * Delayed tree deletion:
 *
 * Normally trees without active hypotheses would be deleted
 * immediatelly.  With tree deletion latency, empty trees will stay
 * around for @c treeDeletionLatency_ time-frames.  This is meant to
 * reduce to over-head of immediate reactivation.
 *
 *
 * Language model look-ahead laziness
 *
 * Normally when a new tree is activated, the corresponding LM
 * look-ahead table is calculated immediately.  This may cause many
 * wasteful calculations, because the tree can be pruned soon after
 * that.  With look-ahead laziness the strategy is as follows: As long
 * as the tree contains less than @c lmLookaheadLaziness_ states,
 * the LM look-ahead is only used if the table is already in the
 * cache.  When the number of states in the tree grows beyond that
 * limit, calculation of the table is enforced.
 */

// ===========================================================================

/**
 * Bookkeeping
 */
class WordConditionedTreeSearch::Trace :
	public Core::ReferenceCounted,
	public TracebackItem
{
public:
	Ref<Trace> predecessor, sibling;

	Trace(const Ref<Trace> &pre, const Bliss::LemmaPronunciation *p,
		  TimeframeIndex t, ScoreVector s, const Transit &transit):
		TracebackItem(p, t, s, transit), predecessor(pre) {}
	Trace(TimeframeIndex t, ScoreVector s, const Transit &transit):
		TracebackItem(0, t, s, transit) {}

	void write(std::ostream &os, Core::Ref<const Bliss::PhonemeInventory> phi) const {
		if (predecessor) predecessor->write(os, phi);
		os <<	 "t=" << std::setw(5) << time
		   << "	s=" << std::setw(8) << score;
		if (pronunciation) {
			os << "	"
			   << std::setw(20) << std::setiosflags(std::ios::left)
			   << pronunciation->lemma()->preferredOrthographicForm()
			   << "	"
			   << "/" << pronunciation->pronunciation()->format(phi) << "/";
		}
		os << std::endl;
	}
};

/**
 * create a traceback from the last (best) trace @c end
 */
void WordConditionedTreeSearch::traceback(Ref<Trace> end, Traceback &result) const {
	result.clear();
	for (; end; end = end->predecessor) {
		result.push_back(*end);
	}
	std::reverse(result.begin(), result.end());
}

// ===========================================================================


/**
 * search space for word conditioned tree search
 */
class WordConditionedTreeSearch::SearchSpace {
private:
	const StateTree *tree_;
	const LanguageModelLookahead *lmLookahead_;
	f64   globalScoreOffset_;

	typedef s32 StateHypothesisIndex;
	typedef s32 TraceId;
	static const TraceId invalidTraceId = -1;
	struct StateHypothesis {
		StateTree::StateId state;
		TraceId trace;
		Score score;
		Score prospect;
		StateHypothesis(StateTree::StateId _state, TraceId _trace, Score _score) :
			state(_state), trace(_trace), score(_score), prospect(_score) {}
		StateHypothesis(StateTree::StateId _state, TraceId _trace, Score _score, Score _prospect) :
			state(_state), trace(_trace), score(_score), prospect(_prospect) {}

	};
public:
	typedef std::vector<StateHypothesis> StateHypothesesList;
private:
	StateHypothesesList stateHypotheses;
	StateHypothesesList newStateHypotheses;
	mutable StateHypothesesList::const_iterator bestStateHypothesis_;

	/**
	 * tree instance (a.k.a. tree copy) representing a set of state hypotheses
	 * with a common language model history
	 */
	struct TreeInstance {
		u32 inactive; /**< number of time-frames this tree has been inactive */
		Lm::History history;
		LanguageModelLookahead::ContextLookaheadReference lookahead;

		struct LmCacheItem {
			Lm::Score score;
			Lm::History extendedHistory;
			LmCacheItem(Lm::Score s, const Lm::History &h) : score(s), extendedHistory(h) {}
		};
		typedef Core::hash_map<const Bliss::LemmaPronunciation*, LmCacheItem, Core::conversion<const Bliss::LemmaPronunciation*, size_t> > LmCache;
		mutable LmCache lmCache;

		std::vector<Ref<Trace> > traces;
		Ref<Trace> trace(const StateHypothesis &sh) const {
			verify_(0 <= sh.trace && sh.trace < TraceId(traces.size()));
			return traces[sh.trace];
		}

		std::vector<StateHypothesis> entryStateHypotheses;
		struct {
			StateHypothesisIndex begin, end;
		} stateHypotheses;

		s32 nStateHypotheses() const {
			return stateHypotheses.end - stateHypotheses.begin
				+  entryStateHypotheses.size();
		}

		TreeInstance(Lm::History _history) :
			inactive(0),
			history(_history)
			{
				stateHypotheses.begin = stateHypotheses.end = 0;
			}

		void enter(Ref<Trace> trace,
				   StateTree::StateId entryState,
				   Score score)
			{
				entryStateHypotheses.push_back(
					StateHypothesis(entryState, traces.size(), score));
				traces.push_back(trace);
			}
	};
	typedef std::vector<TreeInstance*> TreeInstanceList;
	TreeInstanceList activeTrees;
	Core::hash_map<Lm::History, TreeInstance*,Lm::History::Hash> activeTreeMap;
	u32 treeDeletionLatency_;

	std::vector<StateHypothesisIndex> stateHypothesisForTreeState;
	StateHypothesisIndex currentTreeFirstNewStateHypothesis;

	struct  WordEndHypothesis;
	typedef std::vector<WordEndHypothesis> WordEndHypothesisList;

	struct WordEndHypothesis {
		Lm::History history;
		StateTree::StateId transitEntry;
		const Bliss::LemmaPronunciation *pronunciation;
		ScoreVector score;
		Ref<Trace> trace;

		WordEndHypothesis(const Lm::History &h, StateTree::StateId e,
						  const Bliss::LemmaPronunciation *p, ScoreVector s,
						  const Ref<Trace> &t) :
			history(h), transitEntry(e), pronunciation(p), score(s), trace(t) {}

		inline void addLmScore(Core::Ref<const Lm::ScaledLanguageModel> lm, Score wpScale);
		inline void addLmScore(Core::Ref<const Lm::ScaledLanguageModel> lm, Score wpScale,
							   TreeInstance::LmCache &lmCache);

		u32 hashKey() const {
			u32 hash = history.hashKey();
			hash = (hash << 5 | hash >> 27) ^ transitEntry;
			return hash;
		}

		struct Hash {
			size_t operator()(const WordEndHypothesis *weh) const {
				return weh->hashKey();
			}

			size_t operator()(const WordEndHypothesisList::iterator weh) const {
				return weh->hashKey();
			}

		};
		struct Equality {
			bool operator() (const WordEndHypothesis *l, const WordEndHypothesis *r) const {
				return (l->history	  == r->history)
					&& (l->transitEntry == r->transitEntry);
			}

			bool operator() (const  WordEndHypothesisList::iterator l,
							 const  WordEndHypothesisList::iterator r) const {
				return (l->history == r->history)
					&& (l->transitEntry == r->transitEntry);

			}
		};
	};

	WordEndHypothesisList wordEndHypotheses;
	typedef Core::hash_set<WordEndHypothesisList::iterator, WordEndHypothesis::Hash, WordEndHypothesis::Equality> WordEndHypothesisRecombinationMap;
	WordEndHypothesisRecombinationMap wordEndHypothesisMap;

	TreeInstance *activateTree(Lm::History);
	void activateOrUpdateTree(
		const Ref<Trace>&, Lm::History, StateTree::StateId, Score);
	void deactivateTree(TreeInstance*);
	/** Check whether tree should be deactivated, and if so do it.
	 * @return true iff tree has been deactivared */
	inline bool mayDeactivateTree(TreeInstance*);
	inline void activateOrUpdateStateHypothesis(StateTree::StateId, TraceId, Score);
	inline void expandState(const StateHypothesis&);

	void expandTree(TreeInstance&);

	mutable Histogram stateHistogram_;
	mutable Histogram wordEndHistogram_;

	LanguageModelLookahead::ContextLookaheadReference currentLookahead_;
	Score currentStateTreshold_, currentBestScore_;
	Score pruningThreshold_;
	bool anticipatedLookaheadPruning_;

public:
	SearchSpace(
		const StateTree*,
		const LanguageModelLookahead*,
		u32 nStateHistogramBins, u32 nWordEndHistogramBins);
	~SearchSpace();
	void setPruningThreshold(Score threshold) { pruningThreshold_ = threshold; }
	void setAnticipatedLookaheadPruning(bool prune) { anticipatedLookaheadPruning_ = prune; }
	void setTreeDeletionLatency(u32 l) { treeDeletionLatency_ = l; }
	void clear();
	void expand();
	void addAcousticScores(const Mm::FeatureScorer::Scorer&);
	void activateLmLookahead(s32 laziness);
	void determinePruningCriterion();
	StateHypothesesList::const_iterator bestStateHypothesis() const;
	Score minimumStateScore() const {
		return (stateHypotheses.size()) ? bestStateHypothesis()->prospect : Core::Type<Score>::max;
	}
	Score quantileStateScore(Score min, Score max, u32 nHyp) const;
	void pruneStates(Score threshold);
	void rescale(Score);
	void findWordEnds(Core::Ref<const Lm::ScaledLanguageModel>, Score wpScale);
	Score minimumWordEndScore() const;
	Score quantileWordEndScore(Score min, Score max, u32 nHyp) const;
	void pruneWordEnds(Score lmPruningThreshold);
	void recombineWordEnds(bool shallCreateLattice = false);
	void createTraces(TimeframeIndex time);
	void pruneSilenceSiblingTraces(Core::Ref<Trace>, const Bliss::Lemma *silence);
	void optimizeSilenceInWordLattice(const Bliss::Lemma *silence);
	void hypothesizeEpsilonPronunciations(
		Core::Ref<const Lm::ScaledLanguageModel>, Score wpScale, Score threshold);
	void addStartupWordEndHypothesis(const Lm::History&, TimeframeIndex);
	void dumpWordEnds(std::ostream&, Core::Ref<const Bliss::PhonemeInventory>) const;
	void startNewTrees();
	Ref<Trace> getSentenceEnd(
		Core::Ref<const Lm::ScaledLanguageModel>, TimeframeIndex, bool shallCreateLattice);
	Ref<Trace> getSentenceEndFallBack(
		Core::Ref<const Lm::ScaledLanguageModel>, TimeframeIndex, bool shallCreateLattice);
	Ref<Trace> getLastUnambiguousTrace() const;
	void killTrace(Ref<Trace>);

	u32 nStateHypotheses() const { return stateHypotheses.size(); }
	u32 nWordEndHypotheses() const { return wordEndHypotheses.size(); }
	u32 nActiveTrees() const { return activeTrees.size(); }


};

WordConditionedTreeSearch::SearchSpace::SearchSpace(
	const StateTree *tree,
	const LanguageModelLookahead *lmLookahead,
	u32 nStateHistogramBins, u32 nWordEndHistogramBins) :
	tree_(tree),
	lmLookahead_(lmLookahead),
	globalScoreOffset_(0.0),
	treeDeletionLatency_(0),
	stateHistogram_(nStateHistogramBins),
	wordEndHistogram_(nWordEndHistogramBins)
{
	stateHypothesisForTreeState.resize(tree_->nStates());
}

WordConditionedTreeSearch::SearchSpace::~SearchSpace() {
	for (u32 t = 0; t < activeTrees.size(); ++t)
		delete activeTrees[t];
}

/**
 * reset the search space
 */
void WordConditionedTreeSearch::SearchSpace::clear() {
	globalScoreOffset_ = 0.0;
	stateHypotheses.clear();
	newStateHypotheses.clear();
	for (u32 t = 0; t < activeTrees.size(); ++t)
		delete activeTrees[t];
	activeTrees.clear();
	activeTreeMap.clear();
	wordEndHypotheses.clear();
	wordEndHypothesisMap.clear();
	stateHistogram_.clear();
	wordEndHistogram_.clear();
}

/**
 * create a new tree
 */
WordConditionedTreeSearch::SearchSpace::TreeInstance *WordConditionedTreeSearch::SearchSpace::activateTree(Lm::History history) {
	require(!activeTreeMap[history]);

	TreeInstance *t = new TreeInstance(history);
	activeTrees.push_back(t);
	activeTreeMap[history] = t;

	return t;
}

/**
 * create a new tree or update an existing tree with the same history.
 * a entry state hypothesis is created with the given trace, state, and score
 */
void WordConditionedTreeSearch::SearchSpace::activateOrUpdateTree(
	const Ref<Trace> &trace,
	Lm::History history,
	StateTree::StateId entry,
	Score score)
{
	TreeInstance *at = activeTreeMap[history];
	if (!at) {
		at = activateTree(history);
	} else {
		verify(at->history == history);
	}
	at->enter(trace, entry, score);
}

/**
 * delete a currently active tree
 */
void WordConditionedTreeSearch::SearchSpace::deactivateTree(TreeInstance *t) {
	require(t->nStateHypotheses() == 0);

	verify(activeTreeMap[t->history] == t);
	activeTreeMap.erase(t->history);
	delete t;
}

/**
 * check if tree instance @c at can be disabled, delete it if possible.
 * a state hypothesis can be disabled if it has no active state hypotheses
 * and it is inactive for more then treeDeletetionLatency time steps
 */
inline bool WordConditionedTreeSearch::SearchSpace::mayDeactivateTree(TreeInstance *at) {
	if (at->nStateHypotheses()) {
		at->inactive = 0;
		return false;
	} else {
		if (at->inactive < treeDeletionLatency_) {
			++(at->inactive);
			at->traces.clear();
			return false;
		} else {
			deactivateTree(at);
			return true;
		}
	}
}

/**
 * create or update score and trace of a state hypothesis.
 * a state hypothesis is updated if a state hypothesis for the same exists
 * with a higher score
 */
inline void WordConditionedTreeSearch::SearchSpace::activateOrUpdateStateHypothesis(
	StateTree::StateId state,
	TraceId trace,
	Score score)
{
	Score prospect = score;
	if (anticipatedLookaheadPruning_) {
	    if (currentLookahead_)
		prospect += currentLookahead_->score(state);
	    if (prospect > currentStateTreshold_)
		return;
	}
	StateHypothesisIndex shi = stateHypothesisForTreeState[state];
	if (shi >= StateHypothesisIndex(newStateHypotheses.size()) ||
		shi < currentTreeFirstNewStateHypothesis ||
		newStateHypotheses[shi].state != state)
		{
			stateHypothesisForTreeState[state] = newStateHypotheses.size();
			newStateHypotheses.push_back(StateHypothesis(state, trace, score, prospect));
		} else {
		StateHypothesis &sh(newStateHypotheses[shi]);
		verify_(sh.state == state);
		if (sh.score >= score) {
			sh.score = score;
			sh.prospect = prospect;
			sh.trace = trace;
		}
	}
	if (anticipatedLookaheadPruning_ && prospect < currentBestScore_) {
	    currentBestScore_ = prospect;
	    currentStateTreshold_ = prospect + pruningThreshold_;
	}
}

/**
 * expand state @c sh using forward, loop, and skip transitions.
 * new state hypotheses are stored in newStateHypotheses
 */
inline void WordConditionedTreeSearch::SearchSpace::expandState(const StateHypothesis &sh) {
	const Am::StateTransitionModel &tdp(*tree_->transitionModel(sh.state));
	StateTree::SuccessorIterator state, state_end;
	Score score;

	// self-loop
	score = sh.score + tdp[Am::StateTransitionModel::loop];
	if (score < Core::Type<Score>::max) {
		state = sh.state;
		activateOrUpdateStateHypothesis(state, sh.trace, score);
	}

	// forward
	score = sh.score + tdp[Am::StateTransitionModel::forward];
	if (score < Core::Type<Score>::max)
		for (tie(state, state_end) = tree_->successors(sh.state); state < state_end; ++state)
			activateOrUpdateStateHypothesis(*state, sh.trace, score);

	// skip
	score = sh.score + tdp[Am::StateTransitionModel::skip];
	if (score < Core::Type<Score>::max)
		for (tie(state, state_end) = tree_->successors2(sh.state); state < state_end; ++state)
			activateOrUpdateStateHypothesis(*state, sh.trace, score);
}

/**
 * expand all state hypotheses in tree @c at
 */
void WordConditionedTreeSearch::SearchSpace::expandTree(TreeInstance &at) {
	currentTreeFirstNewStateHypothesis = newStateHypotheses.size();
	currentLookahead_ = at.lookahead;
	for (std::vector<StateHypothesis>::const_iterator sh = at.entryStateHypotheses.begin(); sh != at.entryStateHypotheses.end(); ++sh)
		expandState(*sh);
	at.entryStateHypotheses.clear();

	for (StateHypothesesList::const_iterator sh = stateHypotheses.begin() + at.stateHypotheses.begin; sh != stateHypotheses.begin() +  at.stateHypotheses.end; ++sh)
		expandState(*sh);
	currentLookahead_.reset();
	at.stateHypotheses.begin = currentTreeFirstNewStateHypothesis;
	at.stateHypotheses.end   = newStateHypotheses.size();
}

/**
 * expand all state hypotheses in all active trees.
 * bestStateHypothesis_ is invalidated
 */
void WordConditionedTreeSearch::SearchSpace::expand() {
	newStateHypotheses.clear();
	wordEndHypotheses.clear();
	currentStateTreshold_ = currentBestScore_ = Core::Type<Score>::max;
	for (u32 t = 0; t < activeTrees.size(); ++t)
		expandTree(*activeTrees[t]);
	stateHypotheses.swap(newStateHypotheses);
	bestStateHypothesis_ = stateHypotheses.end();
}

/**
 * add acoustic scores to all active state hypotheses.
 * the best state hypothesis (w.r.t s.prospect) is written in bestStateHypothesis_.
 * the aoustic score is added to both s.score and s.prospect
 */
void WordConditionedTreeSearch::SearchSpace::addAcousticScores(
	const Mm::FeatureScorer::Scorer &featureScorer)
{
	bestStateHypothesis_ = stateHypotheses.begin();
	Score bestScore = Core::Type<Score>::max;
	for (StateHypothesesList::iterator sh = stateHypotheses.begin(); sh != stateHypotheses.end(); ++sh) {
		Mm::MixtureIndex mix = tree_->stateDesc(sh->state).acousticModel;
		verify_(mix != StateTree::invalidAcousticModel);
		Score s = featureScorer->score(mix);
		sh->score	+= s;
		sh->prospect += s;
		if (bestScore > sh->prospect)
			bestScore = (bestStateHypothesis_ = sh)->prospect;
	}
}

void  WordConditionedTreeSearch::SearchSpace::activateLmLookahead(s32 lmLookaheadLaziness) {
	require(lmLookahead_);

	for (TreeInstanceList::iterator t = activeTrees.begin(); t != activeTrees.end(); ++t) {
		TreeInstance &at(**t);
		if (!at.lookahead) {
			if (at.nStateHypotheses() > lmLookaheadLaziness)
				at.lookahead = lmLookahead_->getLookahead(at.history);
			else
				at.lookahead = lmLookahead_->tryToGetLookahead(at.history);
		}
	}
}

/**
 * calculate s.prospect (using LM lookahead) and find best state hypothesis in all active trees.
 * result: bestStateHypothesis_
 */
void WordConditionedTreeSearch::SearchSpace::determinePruningCriterion() {
	if (anticipatedLookaheadPruning_) {
	    // lookahead scores are already computed in expand()
	    // bestStateHypothesis will be computed using bestStateHypothesis()
	    bestStateHypothesis_ = stateHypotheses.end();
	    return;
	}
	bestStateHypothesis_ = stateHypotheses.begin();
	Score bestScore = Core::Type<Score>::max;
	for (TreeInstanceList::const_iterator t = activeTrees.begin(); t != activeTrees.end(); ++t) {
		const TreeInstance &at(**t);
		StateHypothesesList::iterator sh_begin = stateHypotheses.begin() + at.stateHypotheses.begin;
		StateHypothesesList::iterator sh_end   = stateHypotheses.begin() + at.stateHypotheses.end;

		for (StateHypothesesList::iterator sh = sh_begin; sh != sh_end; ++sh) {
			sh->prospect = sh->score;
		}

		if (at.lookahead) {
			const LanguageModelLookahead::ContextLookahead &la(*at.lookahead);
			for (StateHypothesesList::iterator sh = sh_begin; sh != sh_end; ++sh)
				sh->prospect += la.score(sh->state);
		}

		for (StateHypothesesList::iterator sh = sh_begin; sh != sh_end; ++sh)
			if (bestScore > sh->prospect)
				bestScore = (bestStateHypothesis_ = sh)->prospect;
	}

}

/**
 * best state hypothesis w.r.t. state prospect (score + lm lookahed).
 * if bestStateHypothesis_ is set, this will be used, otherwise the
 * searched by iterating over all state hypotheses
 */
WordConditionedTreeSearch::SearchSpace::StateHypothesesList::const_iterator
WordConditionedTreeSearch::SearchSpace::bestStateHypothesis() const {
	if (bestStateHypothesis_ == stateHypotheses.end()) {
		bestStateHypothesis_ = stateHypotheses.begin();
		Score bestScore = Core::Type<Score>::max;
		for (StateHypothesesList::const_iterator sh = stateHypotheses.begin(); sh != stateHypotheses.end(); ++sh) {
			if (bestScore > sh->prospect)
				bestScore = (bestStateHypothesis_ = sh)->prospect;
		}
	}
	return bestStateHypothesis_;
}

/**
 * score s where the nHyps active state hypotheses have a score s' <= s
 */
Score WordConditionedTreeSearch::SearchSpace::quantileStateScore(Score minScore, Score maxScore, u32 nHyps) const {
	stateHistogram_.clear();
	stateHistogram_.setLimits(minScore, maxScore);

	for (StateHypothesesList::const_iterator sh = stateHypotheses.begin(); sh != stateHypotheses.end(); ++sh)
		stateHistogram_ += sh->prospect;

	return stateHistogram_.quantile(nHyps);
}

/**
 * remove all state hypotheses s with s.prospect > threshold.
 * empty trees are removed.
 * bestStateHypothesis_ is invalidated
 */
void WordConditionedTreeSearch::SearchSpace::pruneStates(Score threshold) {
	u32 treeIn, treeOut;
	StateHypothesesList::iterator hypIn, hypOut, hypBegin, treeHypEnd;
	hypIn = hypOut = hypBegin = stateHypotheses.begin();
	for (treeIn = treeOut = 0; treeIn < activeTrees.size(); ++treeIn) {
		TreeInstance *at(activeTrees[treeIn]);
		verify_(hypIn == hypBegin + at->stateHypotheses.begin);
		at->stateHypotheses.begin = hypOut - hypBegin;
		for (treeHypEnd = hypBegin + at->stateHypotheses.end; hypIn < treeHypEnd; ++hypIn) {
			verify_(hypIn < stateHypotheses.end());
			if (hypIn->prospect <= threshold)
				*(hypOut++) = *hypIn;
		}
		at->stateHypotheses.end = hypOut - hypBegin;
		if (!mayDeactivateTree(at))
			activeTrees[treeOut++] = at;
	}
	stateHypotheses.erase(hypOut, stateHypotheses.end());
	activeTrees.resize(treeOut);
	bestStateHypothesis_ = stateHypotheses.end();
}

/**
 * adjust scores such that the smallest score is 0.
 * the offset is added to globalScoreOffset_
 */
void WordConditionedTreeSearch::SearchSpace::rescale(Score offset) {
	require(wordEndHypotheses.size() == 0);
	for (StateHypothesesList::iterator sh = stateHypotheses.begin(); sh != stateHypotheses.end(); ++sh)
		sh->score -= offset;
	globalScoreOffset_ += offset;
	bestStateHypothesis_ = stateHypotheses.end();
}

/**
 * add lm score and update history
 */
inline void WordConditionedTreeSearch::SearchSpace::WordEndHypothesis::addLmScore(
	Core::Ref<const Lm::ScaledLanguageModel> lm, Score wpScale)
{
	Lm::addLemmaPronunciationScore(lm, pronunciation, wpScale, lm->scale(), history, score.lm);
}

/**
 * add lm score and update history using the cached entries
 */
inline void WordConditionedTreeSearch::SearchSpace::WordEndHypothesis::addLmScore(
	Core::Ref<const Lm::ScaledLanguageModel> lm, Score wpScale, TreeInstance::LmCache &lmCache)
{
	require(pronunciation);

	TreeInstance::LmCache::iterator lci;
	bool needToCalculate;
	Core::tie(lci, needToCalculate) = lmCache.insert(
		std::make_pair(pronunciation, TreeInstance::LmCacheItem(score.lm, history)));
	TreeInstance::LmCacheItem &lc(lci->second);
	if (needToCalculate) {
		addLmScore(lm, wpScale);
		lc.score = score.lm - lc.score;
		lc.extendedHistory = history;
	} else {
		score.lm += lc.score;
		history = lc.extendedHistory;
	}
}

/**
 * find word ends in stateHypotheses and create word end hypotheses
 */
void WordConditionedTreeSearch::SearchSpace::findWordEnds(
	Core::Ref<const Lm::ScaledLanguageModel> lm, Score wpScale)
{
	require(wordEndHypotheses.size() == 0);
	for (TreeInstanceList::const_iterator t = activeTrees.begin(); t != activeTrees.end(); ++t) {
		const TreeInstance &at(**t);
		for (StateHypothesesList::iterator sh = stateHypotheses.begin() + at.stateHypotheses.begin; sh != stateHypotheses.begin() +  at.stateHypotheses.end; ++sh) {
			StateTree::const_ExitIterator we, we_end;
			for (tie(we, we_end) = tree_->wordEnds(sh->state); we != we_end; ++we) {
				Ref<Trace> trace(at.trace(*sh));
				ScoreVector score(sh->score - trace->score.lm, trace->score.lm);
				WordEndHypothesis weh(at.history, we->transitEntry, we->pronunciation, score, trace);
				if (weh.pronunciation) {
					weh.score.acoustic += (*tree_->transitionModel(sh->state))[Am::StateTransitionModel::exit];
					weh.addLmScore(lm, wpScale, at.lmCache);
				}
				wordEndHypotheses.push_back(weh);
			}
		}
	}
}

/**
 * score of the best word end hypothesis in wordEndHypotheses
 */
Score WordConditionedTreeSearch::SearchSpace::minimumWordEndScore() const {
	Score minScore = Core::Type<Score>::max;
	for (WordEndHypothesisList::const_iterator weh = wordEndHypotheses.begin(); weh != wordEndHypotheses.end(); ++weh) {
		if (minScore > weh->score)
			minScore = weh->score;
	}
	return minScore;
}

/**
 * score s where the nHyps active word end hypotheses have a score s' <= s
 */
Score WordConditionedTreeSearch::SearchSpace::quantileWordEndScore( Score minScore, Score maxScore, u32 nHyps) const {
	wordEndHistogram_.clear();
	wordEndHistogram_.setLimits(minScore, maxScore);
	for (WordEndHypothesisList::const_iterator weh = wordEndHypotheses.begin(); weh != wordEndHypotheses.end(); ++weh)
		wordEndHistogram_ += weh->score;

	return wordEndHistogram_.quantile(nHyps);
}

/**
 * remove word end hypotheses with a score > threshold
 */
void WordConditionedTreeSearch::SearchSpace::pruneWordEnds(Score threshold) {
	WordEndHypothesisList::iterator in, out;
	for (in = out = wordEndHypotheses.begin(); in != wordEndHypotheses.end(); ++in)
		if (in->score <= threshold)
			*(out++) = *in;
	wordEndHypotheses.erase(out, wordEndHypotheses.end());
}

/**
 * update the bookkeeping entrys for all word end hypotheses
 */
void WordConditionedTreeSearch::SearchSpace::createTraces(TimeframeIndex time) {
	for (WordEndHypothesisList::iterator weh = wordEndHypotheses.begin(); weh != wordEndHypotheses.end(); ++weh) {
		weh->trace = Core::ref(new Trace(weh->trace, weh->pronunciation, time, weh->score,
										 tree_->describeRootState(weh->transitEntry)));
		weh->trace->score.acoustic += globalScoreOffset_;
	}
}

/**
 * create word end hypotheses for words ending at the coarticulated
 * root state of the state tree (with an empty phoneme sequence).
 * this will be 0 new word end hypotheses with nearly all usual lexicons
 */
void WordConditionedTreeSearch::SearchSpace::hypothesizeEpsilonPronunciations(
	Core::Ref<const Lm::ScaledLanguageModel> lm, Score wpScale, Score threshold)
{
	for (u32 w = 0; w < wordEndHypotheses.size(); ++w) {
		StateTree::StateId state = wordEndHypotheses[w].transitEntry;
		StateTree::const_ExitIterator we, we_end;
		for (tie(we, we_end) = tree_->wordEnds(state); we != we_end; ++we) {
			if (!we->pronunciation) continue;
			WordEndHypothesis weh(wordEndHypotheses[w]);
			weh.pronunciation = we->pronunciation;
			weh.addLmScore(lm, wpScale);
			weh.score.acoustic += (*tree_->transitionModel(state))[Am::StateTransitionModel::exit];
			if (weh.score <= threshold) {
				weh.trace = Core::ref(new Trace(weh.trace, weh.pronunciation, weh.trace->time, weh.score,
												tree_->describeRootState(weh.transitEntry)));
				weh.trace->score.acoustic += globalScoreOffset_;
				wordEndHypotheses.push_back(weh);
			}
		}
	}
}

/**
 * find equal word end hypotheses (same history and same transit) and keep only
 * the best one
 */
void WordConditionedTreeSearch::SearchSpace::recombineWordEnds(
	bool shallCreateLattice)
{
	wordEndHypothesisMap.clear();

	WordEndHypothesisList::iterator in, out;
	for (in = out = wordEndHypotheses.begin(); in != wordEndHypotheses.end(); ++in) {
		WordEndHypothesisRecombinationMap::iterator i = wordEndHypothesisMap.find(in);
		if (i != wordEndHypothesisMap.end()) {
			WordEndHypothesis &a(*in);
			WordEndHypothesis &b(**i);
			verify_(b.history == a.history);
			verify_(b.transitEntry == a.transitEntry);
			if (b.score > a.score) {
				b.pronunciation = a.pronunciation;
				b.score = a.score;
				if (shallCreateLattice) {
					verify(!a.trace->sibling);
					a.trace->sibling = b.trace;
				}
				b.trace = a.trace;
			} else {
				if (shallCreateLattice) {
					verify(!a.trace->sibling);
					a.trace->sibling = b.trace->sibling;
					b.trace->sibling = a.trace;
				}
			}
		} else {
			*out = *in;
			wordEndHypothesisMap.insert(out);
			++out;
		}
	}
	wordEndHypotheses.erase(out, wordEndHypotheses.end());
}

/**
 * Remove sibling traces that are silence.
 */
void WordConditionedTreeSearch::SearchSpace::pruneSilenceSiblingTraces(
	Core::Ref<Trace> trace, const Bliss::Lemma *silence)
{
	for (Core::Ref<Trace> tr = trace; tr->sibling;) {
		if (tr->sibling->pronunciation->lemma() == silence)
			tr->sibling = tr->sibling->sibling;
		else
			tr = tr->sibling;
	}
}

/**
 * This is the simple lattice optimization selected by optimize-lattice=simple.
 * The effect is this: All partial sentence hypotheses ending with
 * silence are suppressed from the lattice - except that the best
 * scoring hypothesis is always preserved, even if it ends with
 * silence.
 */
void WordConditionedTreeSearch::SearchSpace::optimizeSilenceInWordLattice(
	const Bliss::Lemma *silence)
{
	for (WordEndHypothesisList::iterator weh = wordEndHypotheses.begin(); weh != wordEndHypotheses.end(); ++weh)
		pruneSilenceSiblingTraces(weh->trace, silence);
}

/**
 * create initial (empty) word end hypothesis.
 * used only at the begin of a segment (t=0, history = <eps>)
 */
void WordConditionedTreeSearch::SearchSpace::addStartupWordEndHypothesis(
	const Lm::History &h, TimeframeIndex time)
{
	ScoreVector score(0.0, 0.0);
	Ref<Trace> t(new Trace(time, score, tree_->describeRootState(tree_->root())));
	t->score.acoustic += globalScoreOffset_;
	wordEndHypotheses.push_back(WordEndHypothesis(h, tree_->root(), 0, score, t));
}

void WordConditionedTreeSearch::SearchSpace::dumpWordEnds(
	std::ostream &os, Core::Ref<const Bliss::PhonemeInventory> phi) const
{
	for (WordEndHypothesisList::const_iterator weh = wordEndHypotheses.begin(); weh != wordEndHypotheses.end(); ++weh) {
		os << "trace:" << std::endl;
		weh->trace->write(os, phi);
		os << "history:	   " << weh->history.format() << std::endl
		   << "transit entry: " << weh->transitEntry << std::endl
		   << std::endl;
	}
}

/**
 * create or update tree instances for word ends in wordEndHypotheses
 */
void WordConditionedTreeSearch::SearchSpace::startNewTrees() {
	for (WordEndHypothesisList::const_iterator weh = wordEndHypotheses.begin(); weh != wordEndHypotheses.end(); ++weh) {
		activateOrUpdateTree(weh->trace, weh->history, weh->transitEntry, weh->score);
	}
}

/**
 * Find the best sentence end hypothesis.
 * Returns a back-trace reference which can be used to determine the
 * word sequence.  The returned reference immediately points a
 * trace-back item which includes the language model sentence end
 * score.  Its predecessor indicates the final word of the sentence.
 * In case no word-end hypothesis is active, a null reference is returned.
 * If creation of the word lattice is requested, the returned trace
 * has siblings corresponding to sub-optimal sentence ends.
 */

Ref<WordConditionedTreeSearch::Trace>
WordConditionedTreeSearch::SearchSpace::getSentenceEnd(
	Core::Ref<const Lm::ScaledLanguageModel> lm, TimeframeIndex time, bool shallCreateLattice)
{
	Ref<Trace> best;
	for (WordEndHypothesisList::const_iterator weh = wordEndHypotheses.begin(); weh != wordEndHypotheses.end(); ++weh) {
		if (weh->transitEntry != tree_->root() && weh->transitEntry != tree_->ciRoot()) continue; // do not allow coarticulated sentence end
		Ref<Trace> t(new Trace(weh->trace, 0, time, weh->score,
							   tree_->describeRootState(weh->transitEntry)));
		t->score.acoustic += globalScoreOffset_;
		t->score.lm += lm->sentenceEndScore(weh->history);
		if (!best || best->score > t->score) {
			if (shallCreateLattice)
				t->sibling = best;
			best = t;
		} else {
			if (shallCreateLattice) {
				t->sibling = best->sibling;
				best->sibling = t;
			}
		}
	}
	return best;
}

/**
 * Fall back strategy for finding the best sentence hypothesis when
 * there is no active word end hypothesis.  Typically used when
 * getBestSentenceEnd() fails.  This problem can occur when the
 * recording is truncated in the middle of a word and pruning is
 * tight.  The strategy is to select the best state from each active
 * tree and consider it as a "word end" without a pronunciation.
 */

Ref<WordConditionedTreeSearch::Trace>
WordConditionedTreeSearch::SearchSpace::getSentenceEndFallBack(
	Core::Ref<const Lm::ScaledLanguageModel> lm, TimeframeIndex time, bool shallCreateLattice)
{
	Ref<Trace> best;
	for (TreeInstanceList::const_iterator ti = activeTrees.begin(); ti != activeTrees.end(); ++ti) {
		const TreeInstance *at(*ti);
		if (!at->nStateHypotheses()) continue;
		StateHypothesisIndex s = at->stateHypotheses.begin;
		StateHypothesis bestStateHyp(stateHypotheses[s]);
		while (++s < at->stateHypotheses.end) {
			const StateHypothesis &sh(stateHypotheses[s]);
			if (bestStateHyp.score > sh.score)
				bestStateHyp = sh;
		}
		Ref<Trace> pre(at->trace(bestStateHyp));
		Ref<Trace> t(new Trace(pre, 0, time, pre->score,
							   tree_->describeRootState(bestStateHyp.state)));
		t->score.acoustic = globalScoreOffset_ + bestStateHyp.score - pre->score.lm;
		t->score.lm += lm->sentenceEndScore(at->history);
		if (!best || best->score > t->score) {
			if (shallCreateLattice)
				t->sibling = best;
			best = t;
		} else {
			if (shallCreateLattice) {
				t->sibling = best->sibling;
				best->sibling = t;
			}
		}
	}
	return best;
}

Ref<WordConditionedTreeSearch::Trace>
WordConditionedTreeSearch::SearchSpace::getLastUnambiguousTrace() const {
	std::vector<Trace*> traces;
	for (WordEndHypothesisList::const_iterator weh = wordEndHypotheses.begin(); weh != wordEndHypotheses.end(); ++weh)
		traces.push_back(weh->trace.get());
	for (TreeInstanceList::const_iterator at = activeTrees.begin(); at != activeTrees.end(); ++at)
		for (std::vector<Ref<Trace> >::const_iterator t = (*at)->traces.begin(); t != (*at)->traces.end(); ++t)
			traces.push_back(t->get());

	Core::hash_map<const Trace*, u32, Core::conversion<const Trace*, size_t> > m;
	std::vector<Trace*> chain;
	for (Trace *t = traces.back(); t; t = t->predecessor.get()) {
		m[t] = 2 + chain.size();
		chain.push_back(t);
	}
	traces.pop_back();

	u32 mLimit = 2;
	while (traces.size()) {
		for (Trace *t = traces.back(); t; t = t->predecessor.get()) {
			u32 mt = m[t];
			if (mt == 1) break;
			m[t] = 1;
			if (mLimit < mt) { mLimit = mt; break; }
		}
		traces.pop_back();
	}

	verify(mLimit >= 2);
	mLimit -= 2;
	verify(mLimit < chain.size());
	return Ref<Trace>(chain[mLimit]);
}

void WordConditionedTreeSearch::SearchSpace::killTrace(Ref<Trace> t) {
	t->predecessor.reset();
	t->sibling.reset();
	t->pronunciation = 0;
}

// ===========================================================================
struct WordConditionedTreeSearch::SearchSpaceStatistics {
	Core::Statistics<u32>
	treesBeforePruning,  treesAfterPrePruning,  treesAfterPruning,
		statesBeforePruning, statesAfterPrePruning, statesAfterPruning,
		wordEndsBeforePruning, wordEndsAfterPruning, epsilonWordEndsAdded,
		wordEndsAfterRecombination, wordEndsAfterSecondPruning;
	Core::Statistics<Score>
	acousticHistogramPruningThreshold,
		lmHistogramPruningThreshold;
	SearchSpaceStatistics();
	void clear();
	void write(Core::XmlWriter&) const;
};

void WordConditionedTreeSearch::SearchSpaceStatistics::write(Core::XmlWriter &os) const {
	os << Core::XmlOpen("search-space-statistics")
	   << treesBeforePruning
	   << treesAfterPrePruning
	   << treesAfterPruning
	   << statesBeforePruning
	   << statesAfterPrePruning
	   << statesAfterPruning
	   << wordEndsBeforePruning
	   << wordEndsAfterPruning
	   << epsilonWordEndsAdded
	   << wordEndsAfterRecombination
	   << wordEndsAfterSecondPruning
	   << acousticHistogramPruningThreshold
	   << lmHistogramPruningThreshold
	   << Core::XmlClose("search-space-statistics");
}

// ===========================================================================
// class WordConditionedTreeSearch

const Core::ParameterFloat WordConditionedTreeSearch::paramAcousticPrePruningThreshold(
	"acoustic-prepruning",
	"threshold for pruning of state hypotheses before emission scores",
	Core::Type<Score>::max, 0.0);
const Core::ParameterFloat WordConditionedTreeSearch::paramAcousticPruningThreshold(
	"acoustic-pruning",
	"threshold for pruning of state hypotheses",
	1000.0, 0.0);
const  Core::ParameterInt WordConditionedTreeSearch::paramAcousticPruningLimit(
	"acoustic-pruning-limit",
	"maximum number of active states, enforced by histogram pruning",
	Core::Type<s32>::max, 1);
const  Core::ParameterInt WordConditionedTreeSearch::paramAcousticPruningBins(
	"acoustic-pruning-bins", "number of bins for histogram pruning of states",
	100, 2);
const Core::ParameterFloat WordConditionedTreeSearch::paramLmPruningThreshold(
	"lm-pruning",
	"threshold for pruning of word end hypotheses",
	1000.0, 0.0);
const  Core::ParameterInt WordConditionedTreeSearch::paramLmPruningLimit(
	"lm-pruning-limit",
	"maximum number of word ends, enforced by histogram pruning",
	Core::Type<s32>::max, 1);
const  Core::ParameterInt WordConditionedTreeSearch::paramLmPruningBins(
	"lm-pruning-bins",
	"number of bins for histogram pruning of word ends",
	100, 2);
const  Core::ParameterInt WordConditionedTreeSearch::paramTreeDeletionLatency(
	"tree-deletion-latency",
	"number of time-frames a tree without state hypotheses stays around",
	0, 0);
const Core::ParameterBool  WordConditionedTreeSearch::paramEnableLmLookahead(
	"lm-lookahead",
	"enable language model look-ahead",
	false);
const Core::ParameterInt WordConditionedTreeSearch::paramLmLookaheadLaziness(
	"lm-lookahead-laziness",
	"number of states allowed in a tree before calculation of language model look-ahead is enforced",
	0, 0);
const Core::ParameterBool  WordConditionedTreeSearch::paramCreateLattice(
	"create-lattice",
	"enable generation of word lattice",
	false);
const Core::ParameterBool WordConditionedTreeSearch::paramSentenceEndFallBack(
	"sentence-end-fall-back",
	"allow for fallback solution if no active sentence end hypothesis exists",
	true);
const Core::ParameterFloat WordConditionedTreeSearch::paramLatticePruningThreshold(
	"lattice-pruning",
	"threshold for pruning of hypotheses stored in word lattice",
	1000.0, 0.0);
const  Core::ParameterInt WordConditionedTreeSearch::paramLatticePruningLimit(
	"lattice-pruning-limit",
	"maximum number of arcs stored in word lattice per time frame",
	Core::Type<s32>::max, 1);
const  Core::ParameterBool WordConditionedTreeSearch::paramAnticipatedLmLookahedPruning(
	"anticipated-lm-lookahead-pruning",
	"prune hypotheses using the lm lookahead score before acoustic score calculation",
	false);
const Core::Choice WordConditionedTreeSearch::latticeOptimizationMethodChoice(
	"no",		 noLatticeOptimization,			// for backward compatibility - remove asap
	"yes",		simpleSilenceLatticeOptimization, // for backward compatibility - remove asap
	"none",	   noLatticeOptimization,
	"simple",	 simpleSilenceLatticeOptimization,
	Core::Choice::endMark());
const Core::ParameterChoice WordConditionedTreeSearch::paramOptimizeLattice(
	"optimize-lattice",
	&latticeOptimizationMethodChoice,
	"optimization method for word lattice generation (default is 'simple silence approximation')",
	simpleSilenceLatticeOptimization);

WordConditionedTreeSearch::WordConditionedTreeSearch(
	const Core::Configuration &c) :
	Core::Component(c),
	SearchAlgorithm(c),
	statisticsChannel_(config, "statistics")
{
	lmLookahead_ = 0;
	acousticPrePruningThreshold_ = paramAcousticPrePruningThreshold(config);
	acousticPruningThreshold_ = paramAcousticPruningThreshold(config);
	acousticPruningBins_	  = paramAcousticPruningBins(config);
	acousticPruningLimit_	 = paramAcousticPruningLimit(config);
	anticipatedLookaheadPruning_ = paramAnticipatedLmLookahedPruning(config);
	lmPruningThreshold_	   = paramLmPruningThreshold(config);
	lmPruningBins_			= paramLmPruningBins(config);
	lmPruningLimit_		   = paramLmPruningLimit(config);
	treeDeletionLatency_	  = paramTreeDeletionLatency(config);
	shallCreateLattice_	   = paramCreateLattice(config);
	allowSentenceEndFallBack_ = paramSentenceEndFallBack(config);
	latticePruningThreshold_  = paramLatticePruningThreshold(config, lmPruningThreshold_);
	latticePruningLimit_	  = paramLatticePruningLimit(config, lmPruningLimit_);

	if (latticePruningThreshold_ < lmPruningThreshold_ ||
		latticePruningLimit_	 < lmPruningLimit_)
		warning("Lattice pruning is tighter than LM pruning");
	if (shallCreateLattice_) {
		shallOptimizeLattice_ = LatticeOptimizationMethod(paramOptimizeLattice(config));
	} else {
		if (latticePruningThreshold_ != lmPruningThreshold_) {
			log("Ignoring lattice pruning threshold");
			latticePruningThreshold_ = lmPruningThreshold_;
		}
		shallOptimizeLattice_ = noLatticeOptimization;
	}
	if (anticipatedLookaheadPruning_) {
	    log("using anticipated LM lookahead pruning");
	}
	ss_ = 0;
	searchSpaceStatistics_ = new SearchSpaceStatistics;
}

bool WordConditionedTreeSearch::setModelCombination(const Speech::ModelCombination &modelCombination)
{
	lexicon_ = modelCombination.lexicon();
	silence_ = lexicon_->specialLemma("silence");
	acousticModel_ = modelCombination.acousticModel();
	lm_ = modelCombination.languageModel();
	wpScale_ = modelCombination.pronunciationScale();

	tree_ = new StateTree(select("state-tree"), lexicon_, acousticModel_);

	if (paramEnableLmLookahead(config)) {
		lmLookahead_ = new LanguageModelLookahead(select("lm-lookahead"), wpScale_, lm_, tree_);
		lmLookaheadLaziness_ = paramLmLookaheadLaziness(config);
		if (anticipatedLookaheadPruning_ && lmLookaheadLaziness_)
		    error("anticipated lm lookahead pruning cannot be used with lookahead laziness > 0");
	} else {
	    if (anticipatedLookaheadPruning_)
		error("anticipated lm lookahead requires enabled lm lookahead");
	}
	return true;
}

void WordConditionedTreeSearch::setGrammar(Fsa::ConstAutomatonRef g) {
	log("Set grammar");
#ifdef MODULE_LM_FSA
	require(lm_);
	const Lm::FsaLm *constFsaLm = dynamic_cast<const Lm::FsaLm*>(lm_->unscaled().get());
	require(constFsaLm);
	Lm::FsaLm *fsaLm = const_cast<Lm::FsaLm*>(constFsaLm);
	fsaLm->setFsa(g);
#endif

	delete ss_; ss_ = 0;
	if (paramEnableLmLookahead(config)) {
		delete lmLookahead_;
		lmLookahead_ = new LanguageModelLookahead(
			select("lm-lookahead"), wpScale_, lm_, tree_);
	}
}

void WordConditionedTreeSearch::restart() {
	if (!ss_) {
		ss_ = new SearchSpace(tree_, lmLookahead_, acousticPruningBins_, lmPruningBins_);
		ss_->setTreeDeletionLatency(treeDeletionLatency_);
		ss_->setPruningThreshold(acousticPruningThreshold_);
		ss_->setAnticipatedLookaheadPruning(anticipatedLookaheadPruning_);
	} else {
		ss_->clear();
	}

	time_ = 0;
	ss_->addStartupWordEndHypothesis(lm_->startHistory(), time_);
	ss_->hypothesizeEpsilonPronunciations(lm_, wpScale_, lmPruningThreshold_);
	sentenceEnd_.reset();
}


void WordConditionedTreeSearch::feed(
	const Mm::FeatureScorer::Scorer &emissionScores)
{
	require(emissionScores->nEmissions() >= acousticModel_->nEmissions());

	sentenceEnd_.reset();
	// start new tree instances for word ends at t-1
	ss_->startNewTrees();

	if (anticipatedLookaheadPruning_)
		ss_->activateLmLookahead(0);

	// expand state hypotheses
	ss_->expand();
	++time_;

	searchSpaceStatistics_->treesBeforePruning  += ss_->nActiveTrees();
	searchSpaceStatistics_->statesBeforePruning += ss_->nStateHypotheses();

	if (!anticipatedLookaheadPruning_ && lmLookahead_)
		ss_->activateLmLookahead(lmLookaheadLaziness_);

	// calculate lm lookahead scores
	ss_->determinePruningCriterion();

	// acoustic pre-pruning
	if (ss_->nStateHypotheses() && acousticPrePruningThreshold_ < Core::Type<Score>::max) {
		SearchSpace::StateHypothesesList::const_iterator sh = ss_->bestStateHypothesis();
		Mm::MixtureIndex mix = tree_->stateDesc(sh->state).acousticModel;
		verify(mix != StateTree::invalidAcousticModel);
		Score acuThreshold = sh->prospect
			+ emissionScores->score(mix)
			+ acousticPrePruningThreshold_;
		ss_->pruneStates(acuThreshold);

		searchSpaceStatistics_->treesAfterPrePruning  += ss_->nActiveTrees();
		searchSpaceStatistics_->statesAfterPrePruning += ss_->nStateHypotheses();
	}

	// calculate acoustic scores
	ss_->addAcousticScores(emissionScores);

	// acoustic pruning
	Score minStateScore = ss_->minimumStateScore();
	Score acuThreshold = minStateScore + acousticPruningThreshold_;
	ss_->pruneStates(acuThreshold);
	if ((ss_->nStateHypotheses() > acousticPruningLimit_) && (minStateScore < acuThreshold)) {
		acuThreshold = ss_->quantileStateScore(minStateScore, acuThreshold, acousticPruningLimit_);
		searchSpaceStatistics_->acousticHistogramPruningThreshold += acuThreshold - minStateScore;
		ss_->pruneStates(acuThreshold);
	}

	searchSpaceStatistics_->treesAfterPruning  += ss_->nActiveTrees();
	searchSpaceStatistics_->statesAfterPruning += ss_->nStateHypotheses();

	// adjust scores
	if (time_ % 10 == 0)
		ss_->rescale(minStateScore);

	// find word ends and add lm score
	ss_->findWordEnds(lm_, wpScale_);

	searchSpaceStatistics_->wordEndsBeforePruning += ss_->nWordEndHypotheses();

	// LM pruning: reducce number of word ends
	Score minWordEndScore = ss_->minimumWordEndScore();
	Score lmThreshold = minWordEndScore + latticePruningThreshold_;
	ss_->pruneWordEnds(lmThreshold);
	if ((ss_->nWordEndHypotheses() > latticePruningLimit_) && (minWordEndScore < lmThreshold)) {
		lmThreshold = ss_->quantileWordEndScore(minWordEndScore, lmThreshold, latticePruningLimit_);
		searchSpaceStatistics_->lmHistogramPruningThreshold += lmThreshold - minWordEndScore;
		ss_->pruneWordEnds(lmThreshold);
	}

	searchSpaceStatistics_->wordEndsAfterPruning += ss_->nWordEndHypotheses();

	// bookkeeping
	ss_->createTraces(time_);

	u32 nWordEnds = ss_->nWordEndHypotheses();
	ss_->hypothesizeEpsilonPronunciations(lm_, wpScale_, lmThreshold);
	searchSpaceStatistics_->epsilonWordEndsAdded += ss_->nWordEndHypotheses() - nWordEnds;

	// recombination of word ends
	ss_->recombineWordEnds(shallCreateLattice_);
	switch (shallOptimizeLattice_) {
	case noLatticeOptimization:
		break;
	case simpleSilenceLatticeOptimization:
		ss_->optimizeSilenceInWordLattice(silence_);
		break;
	default: defect();
	}

	searchSpaceStatistics_->wordEndsAfterRecombination += ss_->nWordEndHypotheses();

	// second LM pruning
	if (lmPruningThreshold_ < latticePruningThreshold_) {
		lmThreshold = minWordEndScore + lmPruningThreshold_;
		ss_->pruneWordEnds(lmThreshold);
	}
	if ((ss_->nWordEndHypotheses() > lmPruningLimit_) && (minWordEndScore < lmThreshold)) {
		lmThreshold = ss_->quantileWordEndScore(minWordEndScore, lmThreshold, lmPruningLimit_);
		searchSpaceStatistics_->lmHistogramPruningThreshold += lmThreshold - minWordEndScore;
		ss_->pruneWordEnds(lmThreshold);
	}

	searchSpaceStatistics_->wordEndsAfterSecondPruning += ss_->nWordEndHypotheses();

	if (lmLookahead_)
		lmLookahead_->collectStatistics();
}

void WordConditionedTreeSearch::getPartialSentence(Traceback &result) {
	Ref<Trace> t = ss_->getLastUnambiguousTrace();
	traceback(t, result);
	if (shallCreateLattice_)
		error("partial traceback not compatible with lattice creation");
	else
		ss_->killTrace(t);
}

Ref<WordConditionedTreeSearch::Trace> WordConditionedTreeSearch::sentenceEnd() const {
	if (!sentenceEnd_) {
		sentenceEnd_ = ss_->getSentenceEnd(lm_, time_ + 1, shallCreateLattice_);
		if (!sentenceEnd_) {
			warning("No active word end hypothesis at sentence end.");
			if (allowSentenceEndFallBack_) {
				sentenceEnd_ = ss_->getSentenceEndFallBack(lm_, time_ + 1, shallCreateLattice_);
			}
		}
		if (sentenceEnd_) {
			switch (shallOptimizeLattice_) {
			case noLatticeOptimization:
			case simpleSilenceLatticeOptimization:
				break;
			default: defect();
			}
		}
	}
	return sentenceEnd_;
}

void WordConditionedTreeSearch::getCurrentBestSentence(Traceback &result) const {
	Ref<Trace> t = sentenceEnd();
	if (!t) {
		error("Cannot determine sentence hypothesis: No active word end hypothesis.");
		result.clear();
		return;
	}
	traceback(t, result);
}

Core::Ref<const LatticeAdaptor> WordConditionedTreeSearch::getCurrentWordLattice() const {
	Ref<Trace> tt = sentenceEnd();
	if (!tt) {
		error("Cannot create word lattice: No active word end hypothesis.");
		return Core::ref(new ::Lattice::WordLatticeAdaptor);
	}

	Core::Ref<Lattice::StandardWordLattice>	result(new Lattice::StandardWordLattice(lexicon_));
	Core::Ref<Lattice::WordBoundaries> wordBoundaries(new Lattice::WordBoundaries);
	Ref<Trace> initialTrace;

	Core::hash_map<const Trace*, Fsa::State*, Core::conversion<const Trace*, size_t> > state;
	state[tt.get()] = result->finalState();
	std::stack< Ref<Trace> > stack;
	stack.push(tt);

	Fsa::State *sn, *tn;
	while (!stack.empty()) {
		tt = stack.top(); stack.pop();
		tn = state[tt.get()];
		wordBoundaries->set(
			tn->id(),
			Lattice::WordBoundary(tt->time, tt->transit));

		for (Ref<Trace> st = tt; st; st = st->sibling) {
			Ref<Trace> pt = st->predecessor;
			if (pt->predecessor) {
				if (state.find(pt.get()) == state.end()) {
					sn = state[pt.get()] = result->newState();
					stack.push(pt);
				} else {
					sn = state[pt.get()];
				}
			} else {
				sn = result->initialState();
				initialTrace = pt;
			}

			result->newArc(sn, tn, st->pronunciation,
						   (st->score.acoustic - pt->score.acoustic),
						   (st->score.lm	   - pt->score.lm	  ));
		}
	}
	verify(initialTrace);
	wordBoundaries->set(
		result->initialState()->id(),
		Lattice::WordBoundary(initialTrace->time, initialTrace->transit));
	result->setWordBoundaries(wordBoundaries);
	result->addAcyclicProperty();

	return Core::ref(new ::Lattice::WordLatticeAdaptor(result));
}

void WordConditionedTreeSearch::resetStatistics() {
	searchSpaceStatistics_->clear();
}

WordConditionedTreeSearch::SearchSpaceStatistics::SearchSpaceStatistics() :
	treesBeforePruning		("trees before pruning"),
	treesAfterPrePruning	  ("trees after pre-pruning"),
	treesAfterPruning		 ("trees after  pruning"),
	statesBeforePruning	   ("states before pruning"),
	statesAfterPrePruning	 ("states after pre-pruning"),
	statesAfterPruning		("states after pruning"),
	wordEndsBeforePruning	 ("ending words before pruning"),
	wordEndsAfterPruning	  ("ending words after pruning"),
	epsilonWordEndsAdded	  ("epsilon word ends added"),
	wordEndsAfterRecombination("ending words after recombi"),
	wordEndsAfterSecondPruning("ending words after 2nd pruning"),
	acousticHistogramPruningThreshold("acoustic histogram pruning threshold"),
	lmHistogramPruningThreshold("lm histogram pruning threshold")
{}

void WordConditionedTreeSearch::SearchSpaceStatistics::clear() {
	treesBeforePruning.clear();
	treesAfterPrePruning.clear();
	treesAfterPruning.clear();
	statesBeforePruning.clear();
	statesAfterPrePruning.clear();
	statesAfterPruning.clear();
	wordEndsBeforePruning.clear();
	wordEndsAfterPruning.clear();
	epsilonWordEndsAdded.clear();
	wordEndsAfterRecombination.clear();
	wordEndsAfterSecondPruning.clear();
	acousticHistogramPruningThreshold.clear();
	lmHistogramPruningThreshold.clear();
}

void WordConditionedTreeSearch::logStatistics() const {
	if (statisticsChannel_.isOpen())
		searchSpaceStatistics_->write(statisticsChannel_);
	if (lmLookahead_)
		lmLookahead_->logStatistics();
}

WordConditionedTreeSearch::~WordConditionedTreeSearch() {
	delete searchSpaceStatistics_;
	delete ss_;
	delete tree_;
	delete lmLookahead_;
}

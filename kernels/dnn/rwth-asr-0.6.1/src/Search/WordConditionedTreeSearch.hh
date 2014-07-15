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
// $Id: WordConditionedTreeSearch.hh 8748 2012-08-09 09:47:30Z rybach $

#ifndef _SEARCH_WORD_CONDITIONED_TREE_SEARCH_HH
#define _SEARCH_WORD_CONDITIONED_TREE_SEARCH_HH

#include <Core/Component.hh>
#include <Core/ReferenceCounting.hh>
#include <Speech/ModelCombination.hh>
#include "Search.hh"

namespace Speech {
    class StateTying;
}

namespace Search {

    class LanguageModelLookahead;
    class StateTree;

    class WordConditionedTreeSearch : public SearchAlgorithm {
	class ReverseWordLattice;
	friend class ReverseWordLattice;
    public:
	static const Core::ParameterFloat paramAcousticPrePruningThreshold;
	static const Core::ParameterFloat paramAcousticPruningThreshold;
	static const Core::ParameterInt   paramAcousticPruningLimit;
	static const Core::ParameterInt   paramAcousticPruningBins;
	static const Core::ParameterFloat paramLmPruningThreshold;
	static const Core::ParameterInt   paramLmPruningLimit;
	static const Core::ParameterInt   paramLmPruningBins;
	static const Core::ParameterInt   paramTreeDeletionLatency;
	static const Core::ParameterBool  paramEnableLmLookahead;
	static const Core::ParameterInt   paramLmLookaheadLaziness;
	static const Core::ParameterBool  paramAnticipatedLmLookahedPruning;
	static const Core::ParameterBool  paramCreateLattice;
	static const Core::ParameterBool  paramSentenceEndFallBack;
	static const Core::ParameterFloat paramLatticePruningThreshold;
	static const Core::ParameterInt   paramLatticePruningLimit;
	static const Core::Choice latticeOptimizationMethodChoice;
	static const Core::ParameterChoice paramOptimizeLattice;
	static const Core::ParameterBool  paramShallComputeAcousticScores;
	static const Core::ParameterBool  paramShallComputeLmScores;

    private:
	Bliss::LexiconRef lexicon_;
	const Bliss::Lemma *silence_;
	Core::Ref<const Am::AcousticModel> acousticModel_;
	Core::Ref<const Lm::ScaledLanguageModel> lm_;
	const LanguageModelLookahead *lmLookahead_;

	Score acousticPrePruningThreshold_;
	Score acousticPruningThreshold_;
	u32   acousticPruningBins_;
	u32   acousticPruningLimit_;
	Score wpScale_;
	Score lmPruningThreshold_;
	u32   lmPruningBins_;
	u32   lmPruningLimit_;
	u32   treeDeletionLatency_;
	u32   lmLookaheadLaziness_;
	bool  shallCreateLattice_;
	bool allowSentenceEndFallBack_;
	bool anticipatedLookaheadPruning_;
	enum LatticeOptimizationMethod {
	    noLatticeOptimization,
	    simpleSilenceLatticeOptimization
	};
	LatticeOptimizationMethod shallOptimizeLattice_;
	Score latticePruningThreshold_;
	u32   latticePruningLimit_;
	bool  shallComputeAcousticScores_;
	bool  shallComputeLmScores_;
	TimeframeIndex time_;

	class SearchSpace;
	class Trace;

	StateTree *tree_;
	SearchSpace *ss_;

	struct SearchSpaceStatistics;
	SearchSpaceStatistics *searchSpaceStatistics_;
	mutable Core::XmlChannel statisticsChannel_;

    private:
	mutable Core::Ref<Trace> sentenceEnd_;
	Core::Ref<Trace> sentenceEnd() const;
	void traceback(Core::Ref<Trace>, Traceback &result) const;
    public:
	WordConditionedTreeSearch(const Core::Configuration &);
	virtual ~WordConditionedTreeSearch();
	virtual bool setModelCombination(const Speech::ModelCombination &modelCombination);

	virtual void setGrammar(Fsa::ConstAutomatonRef);
	virtual void restart();
	virtual void feed(const Mm::FeatureScorer::Scorer&);
	virtual void getPartialSentence(Traceback &result);
	virtual void getCurrentBestSentence(Traceback &result) const;
	virtual Core::Ref<const LatticeAdaptor> getCurrentWordLattice() const;
	virtual void resetStatistics();
	virtual void logStatistics() const;
    };

}

#endif // _SEARCH_WORD_CONDITIONED_TREE_SEARCH_HH

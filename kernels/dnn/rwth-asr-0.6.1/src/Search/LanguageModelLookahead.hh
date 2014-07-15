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
// $Id: LanguageModelLookahead.hh 6178 2006-11-03 08:12:57Z rybach $

#ifndef _SEARCH_LANGUAGEMODELLOOKAHEAD_HH
#define _SEARCH_LANGUAGEMODELLOOKAHEAD_HH

#include <Core/Hash.hh>
#include <iostream>
#include <list>
#include <Core/Component.hh>
#include <Core/Parameter.hh>
#include <Core/ReferenceCounting.hh>
#include <Lm/ScaledLanguageModel.hh>
#include "StateTree.hh"

namespace Search {

    /** Language model look-ahead */

    class LanguageModelLookahead :
	public Core::Component
    {
    public:
	typedef u32 LookaheadId;
	typedef f32 Score;
    private:
	static const LookaheadId invalidId;
	u32 historyLimit_;
	s32 cutoffDepth_;
	u32 minimumRepresentation_;
	Lm::Score wpScale_;
	Core::Ref<const Lm::ScaledLanguageModel> lm_;

	class ConstructionNode;
	class ConstructionTree;
	friend class LanguageModelLookahead::ConstructionTree;

	struct Node;
	typedef std::vector<const Bliss::LemmaPronunciation*> Ends;
	Ends ends_;
	typedef std::vector<LookaheadId> Successors;
	Successors successors_;
	std::vector<Node> nodes_;

	LookaheadId nEntries_;

	std::vector<LookaheadId> nodeId_; // StateTree::StateId -> nodes_ indes

	bool shouldPruneConstructionNode(const ConstructionNode &sn) const;
	void buildCompressesLookaheadStructure(const StateTree*, const ConstructionTree&);
	void buildBatchRequest();
	void buildLookaheadStructure(const StateTree*);

	const Lm::CompiledBatchRequest *batchRequest_;
	void computeScores(const Lm::History&, std::vector<Score>&) const;

    public:
	class ContextLookahead;
    private:
	friend class ContextLookahead;
	u32 cacheSizeHighMark_, cacheSizeLowMark_;
	typedef std::list<ContextLookahead*> List;
	mutable List tables_, freeTables_;
	mutable u32 nTables_, nFreeTables_;
	typedef Core::hash_map<Lm::History, ContextLookahead*, Lm::History::Hash> Map;
	mutable Map map_;

	ContextLookahead *acquireTable(const Lm::History&) const;
	ContextLookahead *getCachedTable(const Lm::History&) const;
	void releaseTable(const ContextLookahead*) const;

	struct CacheStatistics;
	CacheStatistics *cacheStatistics_;
	mutable Core::XmlChannel statisticsChannel_;

    public:
	static const Core::ParameterInt paramHistoryLimit;
	static const Core::ParameterInt paramTreeCutoff;
	static const Core::ParameterInt paramMinimumRepresentation;
	static const Core::ParameterInt paramCacheSizeLow, paramCacheSizeHigh;

	LanguageModelLookahead(const Core::Configuration&,
			       Lm::Score wpScale,
			       Core::Ref<const Lm::ScaledLanguageModel>,
			       const StateTree*);
	~LanguageModelLookahead();

	void draw(std::ostream&) const;

	LookaheadId lookaheadId(StateTree::StateId s) const {
	    require_(0 <= s && s < StateTree::StateId(nodeId_.size()));
	    LookaheadId result = nodeId_[s];
	    ensure_(result < nEntries_);
	    return result;
	};

	//    public:

	class ContextLookahead :
	    public Core::ReferenceCounted
	{
	private:
	    const LanguageModelLookahead *la_;
	    Lm::History history_;
	    List::iterator pos_, freePos_;
	    std::vector<Score> scores_;

	    friend class Core::Ref<const ContextLookahead>;
	    void free() const { la_->releaseTable(this); }
	protected:
	    friend class LanguageModelLookahead;
	    ContextLookahead(const LanguageModelLookahead*,
			     const Lm::History&,
			     u32 nEntries);
	    bool isActive() const { return freePos_ == la_->freeTables_.end(); }
	public:
	    Score score(StateTree::StateId s) const {
		return scores_[la_->lookaheadId(s)];
	    }

	    //DEBUG_AREA
	    bool checkScores() {
		int nScores = scores_.size();
		int nAbnorm = 0;
		for (std::vector<Score>::iterator i = scores_.begin(); i != scores_.end(); i++) {
		    if ( (*i > +1.0e+20F) || (*i < -1.0e+20F) )
		    nAbnorm++;
		}

		std::cout << "checkScores: abnormal scores:" << nAbnorm << "/"
			  << nScores << std::endl;

		return (nAbnorm == 0);
	    }
	    //END_DEBUG
	};

    public:
	typedef Core::Ref<const ContextLookahead> ContextLookaheadReference;

    private:

	u32 nTables() const {
	    verify_(nTables_ == tables_.size());
	    return nTables_;
	}
	u32 nActiveTables() const {
	    verify_(nTables_ == tables_.size());
	    verify_(nFreeTables_ == freeTables_.size());
	    return nTables_ - nFreeTables_;
	}

    public:
	ContextLookaheadReference getLookahead(const Lm::History&) const;
	ContextLookaheadReference tryToGetLookahead(const Lm::History&) const;

	void collectStatistics() const;
	void logStatistics() const;
    };

} // namespace Search

#endif //_SEARCH_LANGUAGEMODELLOOKAHEAD_HH

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
// $Id: LanguageModelLookahead.cc 9621 2014-05-13 17:35:55Z golik $

#include <Core/Statistics.hh>
#include <Core/Utility.hh>
#include "LanguageModelLookahead.hh"

using namespace Search;
using Core::tie;

/*
 * To enhance opportunities for future reuse the look-ahead tree
 * structure is independent from the HMM state-tree structure.
 *
 * Memory consumption of LM look-ahead is determined by two
 * parameters: The size of the look-ahead tables, and the number of
 * tables kept im memory.
 *
 * The look-ahead table size is determined by the level of detail
 * reflected in the look-ahead structure.  This can be controled the
 * parameters tree-cutoff and minimum-representation.  By default the
 * look-ahead tables are not pruned, so memory consumption is
 * (unnecessarily) large.
 *
 * The number of tables kept in memory is affected by history-limit,
 * cache-size-low and cache-size-high.
 *
 * By default tables are deleted as soon as they are not immediately
 * needed (i.e. all relevant trees are pruned).  We say, the table
 * becomes inactive.  This saves memory but wastes computation time.
 * The caching strategy works as follows: As long as there are less
 * than cache-size-high tables in memory, no table is deleted.  Above
 * that limit the table that has least recently become inactive is
 * freed whenwever a table is release.  Note that cache-size-high is
 * not a strict upper bound on the number of tables.  It will be
 * exceeded when the search process requires it.
 *
 * When a new table is requested, usually one of the inactive tables
 * is re-used.  But when there are less than cache-size-low tables in
 * memory, a new table is created instead.  cache-size-low is thus a
 * (soft) lower bound on the number of tables in memory.  (Of course
 * there may be less tables, if the search never requires that many
 * different contexts.)
 *
 * Tree cutoff: In the full look-ahead structure non-branching state
 * sequences are represented by a single node.  The MINIMUM depth of
 * these states is used as pruning criterion: If it is larger than
 * "tree-cutoff", the look-ahead node is merged with its parent.
 * (This minimum criterion is believed to best reproduce the behavior
 * of the old standard system.  However, other criteria might be
 * better.)
 *
 * TODO:
 * Optimizations to be considered:
 * - Different LA table cut-off criteria (may-be)
 * - Lazy-evaluation of LA tables (may-be; partially done by "lazy lookahead")
 * - Tune LM scale for look-ahead (questionable)
 */

struct LanguageModelLookahead::CacheStatistics {
    enum CacheEvent {shareInCacheHit, freeCacheHit, cacheMiss};
    static const Core::Choice cacheEventChoice;
    Core::ChoiceStatistics cacheEvents;
    Core::Statistics<u32> nTables, nActiveTables;
    void clear();
    void write(Core::XmlWriter&) const;
    CacheStatistics();
};

const Core::ParameterInt LanguageModelLookahead::paramHistoryLimit(
    "history-limit",
    "length of history considered for look-ahead (effective m-grammity of the look-ahead model - 1)",
    1, 0);
const Core::ParameterInt LanguageModelLookahead::paramTreeCutoff(
    "tree-cutoff",
    "maximum depth of state tree covered by look-ahead (number of HMM state covered)",
    Core::Type<s32>::max, 0);
const Core::ParameterInt LanguageModelLookahead::paramMinimumRepresentation(
    "minimum-representation",
    "minimum number of HMM states represented by one look-ahead node",
    1, 1);
const Core::ParameterInt LanguageModelLookahead::paramCacheSizeLow(
    "cache-size-low",
    "number of look-ahead tables retained before starting to re-use inactive tables",
    0, 0);
const Core::ParameterInt LanguageModelLookahead::paramCacheSizeHigh(
    "cache-size-high",
    "number of look-ahead tables allowed before starting to delete inactive tables",
    0, 0);

const LanguageModelLookahead::LookaheadId LanguageModelLookahead::invalidId
= Core::Type<LanguageModelLookahead::LookaheadId>::max;

struct LanguageModelLookahead::Node {
    u32 firstEnd, firstSuccessor;
};

LanguageModelLookahead::LanguageModelLookahead(
    const Core::Configuration &c,
    Lm::Score wpScale,
    Core::Ref<const Lm::ScaledLanguageModel> lm,
    const StateTree *st) :
    Core::Component(c),
    wpScale_(wpScale),
    lm_(lm),
    batchRequest_(0),
    nTables_(0), nFreeTables_(0),
    statisticsChannel_(config, "statistics")
{
    historyLimit_ = paramHistoryLimit(config);
    cutoffDepth_  = paramTreeCutoff(config);
    minimumRepresentation_ = paramMinimumRepresentation(config);
    cacheSizeHighMark_ = paramCacheSizeHigh(config);
    cacheSizeLowMark_  = paramCacheSizeLow(config);

    log("look-ahead history limit is %d (usually means %d-gram look-ahead)",
	historyLimit_, historyLimit_ + 1);
    buildLookaheadStructure(st);

    cacheStatistics_ = new CacheStatistics;
}

LanguageModelLookahead::~LanguageModelLookahead() {
    require(!nActiveTables());
    delete cacheStatistics_;
    for (List::iterator t = tables_.begin(); t != tables_.end(); ++t)
	delete *t;
    delete batchRequest_;
}

// ===========================================================================
// static structure

class LanguageModelLookahead::ConstructionNode {
public:
    LookaheadId id;
    struct { StateTree::Depth min, max; } depth;
    typedef std::vector<ConstructionNode*> Successors;
    typedef std::vector<StateTree::StateId> Represents;
    Represents represents;

private:
    Ends ends_;
    Successors successors_;

    enum Consolidation { dirty, unique, domineesValid, hashValid };
    mutable Consolidation consolidation_;
    mutable Ends dominees_;
    mutable u32 hash_;

public:
    bool isUnique() const {
	return consolidation_ >= unique;
    }

    void makeUnique() {
	std::sort(ends_.begin(), ends_.end());
	ends_.erase(std::unique(ends_.begin(), ends_.end()), ends_.end());
	std::sort(successors_.begin(), successors_.end());
	successors_.erase(std::unique(successors_.begin(), successors_.end()), successors_.end());
	consolidation_ = unique;
    }

private:
    void updateDominiees() const {
	require(consolidation_ >= unique);
	dominees_ = ends_;
	Ends curr;
	for (Successors::const_iterator s = successors_.begin(); s != successors_.end(); ++s) {
	    dominees_.swap(curr);
	    dominees_.clear();
	    const Ends &succ((*s)->dominees());
	    std::set_union(curr.begin(), curr.end(),
			   succ.begin(), succ.end(),
			   std::back_inserter(dominees_));
	}
	consolidation_ = domineesValid;
    }

    void updateHash() const {
	require(consolidation_ >= domineesValid);
	hash_ = 0;
	for (Ends::const_iterator e = dominees_.begin(); e != dominees_.end(); ++e)
	    hash_ = ((hash_ << 3) | (hash_ >> 29)) ^ u32(long(*e /* this is a pointer */));
	consolidation_ = hashValid;
    }

    struct DominationEquality;
    friend struct ConstructionNode::DominationEquality;
    friend class  LanguageModelLookahead::ConstructionTree;

public:
    ConstructionNode() :
	id(invalidId),
	consolidation_(dirty) {
	depth.min = Core::Type<StateTree::Depth>::max;
	depth.max = Core::Type<StateTree::Depth>::min;
    }

    const Ends &dominees() const {
	switch (consolidation_) {
	case dirty: require(consolidation_ > dirty);
	case unique: updateDominiees();
	case domineesValid: ;
	case hashValid: ;
	}
	verify_(consolidation_ >= domineesValid);
	return dominees_;
    }

    u32 hash() const {
	switch (consolidation_) {
	case dirty: require(consolidation_ > dirty);
	case unique: updateDominiees();
	case domineesValid: updateHash();
	case hashValid: ;
	}
	verify_(consolidation_ >= hashValid);
	return hash_;
    }

    const Ends &ends() const {
	return ends_;
    }
    Ends &ends() {
	consolidation_ = dirty;
	return ends_;
    }

    const Successors &successors() const {
	return successors_;
    }
    Successors &successors() {
	consolidation_ = dirty;
	return successors_;
    }

protected:

    struct DominationHash {
	u32 operator() (const ConstructionNode *n) const {
	    return n->hash();
	}
    };

private:
    struct DominationEquality {
	bool operator() (const ConstructionNode *l, const ConstructionNode *r) const {
	    if (l->consolidation_ >= hashValid && r->consolidation_ >= hashValid)
		if (l->hash_ != r->hash_) return false;
	    const Ends &ld(l->dominees());
	    const Ends &rd(r->dominees());
	    if (ld.size() != rd.size()) return false;
	    return std::equal(ld.begin(), ld.end(), rd.begin());
	}
    };

};

class LanguageModelLookahead::ConstructionTree {
private:
    typedef std::vector<ConstructionNode*> NodeList;
    NodeList nodeList_;
    struct LevelStatistics;
public:
    ConstructionTree();
    ~ConstructionTree();

    bool isWellOrdered() const;
    void writeStatistics(std::ostream&) const;
    void build(const StateTree *st);
    void prune(const LanguageModelLookahead *master);
    void purge();
    LookaheadId nNodes() const {
	return nodeList_.size();
    }
    const ConstructionNode &node(LookaheadId i) const {
	ensure(nodeList_[i]->isUnique());
	return *nodeList_[i];
    }
};

LanguageModelLookahead::ConstructionTree::ConstructionTree() {
}

LanguageModelLookahead::ConstructionTree::~ConstructionTree() {
    for (NodeList::const_iterator i = nodeList_.begin(); i != nodeList_.end(); ++i)
	delete *i;
}


/** Check wether each node has a lower index than its parent. */

bool LanguageModelLookahead::ConstructionTree::isWellOrdered() const {
    bool result = true;
    for (u32 ci = 0; ci < nodeList_.size(); ++ci) {
	const ConstructionNode &cn(*nodeList_[ci]);
	if (cn.id == invalidId) continue;
	verify(cn.id == ci);
	for (ConstructionNode::Successors::const_iterator si = cn.successors().begin(); si != cn.successors().end(); ++si) {
	    result = result && ((*si)->id != invalidId);
	    result = result && ((*si)->id < ci);
	}
    }
    return result;
}

struct LanguageModelLookahead::ConstructionTree::LevelStatistics {
    u32 nNodes, nSuccessors, nEnds;
};

void LanguageModelLookahead::ConstructionTree::writeStatistics(std::ostream &os) const {
    typedef std::map<StateTree::Depth, LevelStatistics> LevelStatisticsMap;
    LevelStatisticsMap levels;
    for (u32 ci = 0; ci < nodeList_.size(); ++ci) {
	const ConstructionNode &cn(*nodeList_[ci]);
	if (cn.id == invalidId) continue;
	LevelStatistics &ls(levels[cn.depth.min]);
	ls.nNodes      += 1;
	ls.nSuccessors += cn.successors().size();
	ls.nEnds       += cn.ends().size();
    }
    for (std::map<StateTree::Depth, LevelStatistics>::const_iterator  l = levels.begin(); l != levels.end(); ++l) {
	StateTree::Depth depth = l->first;
	const LevelStatistics &thisLevel(l->second);
	os << Core::form("level %3d: %6d nodes, branching factor %3.2f, %4d ends\n",
		depth, thisLevel.nNodes, float(thisLevel.nSuccessors)  / float(thisLevel.nNodes), thisLevel.nEnds);
    }
}

void LanguageModelLookahead::ConstructionTree::build(const StateTree *st) {
    std::vector<LookaheadId> nodeId(st->nStates(), invalidId);
    typedef Core::hash_set<ConstructionNode*,
			  ConstructionNode::DominationHash,
			  ConstructionNode::DominationEquality> NodeSet;
    NodeSet nodeSet;

    for (StateTree::ReverseTopologicalStateIterator rtsi(st); rtsi; ++rtsi) {
	const StateTree::StateId si(*rtsi);
	verify(nodeId[si] == invalidId);
	StateTree::const_ExitIterator e, e_end;
	tie(e, e_end) = st->wordEnds(si);
	bool hasEnds = (e != e_end);

	StateTree::SuccessorIterator s, s_end;
	tie(s, s_end) = st->successors(si);
	u32 nSuccessors = s_end - s;

	ConstructionNode *cn = 0;
	if (nSuccessors == 1 && !hasEnds) {
	    verify(nodeId[s] != invalidId);
	    cn = nodeList_[nodeId[s]];
	} else {
	    cn = new ConstructionNode;
	    for (; e != e_end; ++e) {
		if (e->pronunciation) {
		    cn->ends().push_back(e->pronunciation);
		} else {
		    StateTree::StateId te = e->transitEntry;
		    verify(nodeId[te] != invalidId);
		    cn->successors().push_back(nodeList_[nodeId[te]]);
		}
	    }
	    for (; s != s_end; ++s) {
		verify(nodeId[s] != invalidId);
		cn->successors().push_back(nodeList_[nodeId[s]]);
	    }
	    cn->makeUnique();

	    NodeSet::iterator cni = nodeSet.find(cn);
	    if (cni == nodeSet.end()) {
		cn->id = nodeList_.size();
		nodeList_.push_back(cn);
		nodeSet.insert(cn);
	    } else {
		delete cn;
		cn = *cni;
	    }
	}
	nodeId[si] = cn->id;
	cn->depth.min = std::min(cn->depth.min, st->stateDepth(si));
	cn->depth.max = std::max(cn->depth.max, st->stateDepth(si));
	cn->represents.push_back(si);
    }
    ensure(isWellOrdered());
}

/** Decide whether a look-ahead node should be merged with its father. */

bool LanguageModelLookahead::shouldPruneConstructionNode(const ConstructionNode &sn) const {
    bool isTooDeep = (sn.depth.min > cutoffDepth_);
    bool isTooSmall = (sn.represents.size() < minimumRepresentation_);
    return isTooDeep || isTooSmall;
}

/**
 * Merge nodes of construction tree which should be pruned with their
 * parents.  Proceed in in bottom-up direction.  A pruned nodes is
 * indicated by its @c id field being invalid.  Call purge() to remove
 * pruned nodes.
 */

void LanguageModelLookahead::ConstructionTree::prune(
    const LanguageModelLookahead *master)
{
    for (NodeList::iterator ci = nodeList_.begin(); ci != nodeList_.end(); ++ci) {
	ConstructionNode &cn(**ci);
	verify(cn.successors().size() || cn.ends().size());
	ConstructionNode::Successors newSuccessors;
	for (ConstructionNode::Successors::iterator si = cn.successors().begin(); si != cn.successors().end(); ++si) {
	    ConstructionNode &sn(**si);
	    if (sn.id == invalidId || master->shouldPruneConstructionNode(sn)) {
		std::copy(sn.ends().begin(), sn.ends().end(), std::back_insert_iterator<Ends>(cn.ends()));
		std::copy(sn.successors().begin(), sn.successors().end(),
			  std::back_insert_iterator<ConstructionNode::Successors>(newSuccessors));
		std::copy(sn.represents.begin(), sn.represents.end(),
			  std::back_insert_iterator<ConstructionNode::Represents>(cn.represents));
		cn.depth.min = std::min(cn.depth.min, sn.depth.min);
		cn.depth.max = std::max(cn.depth.max, sn.depth.max);
		sn.id = invalidId;
	    } else {
		newSuccessors.push_back(*si);
	    }
	}
	cn.successors().swap(newSuccessors);
	verify(cn.successors().size() || cn.ends().size());
	cn.makeUnique();
    }
    ensure(isWellOrdered());
}

/** Remove pruned nodes from construction tree. */

void LanguageModelLookahead::ConstructionTree::purge() {
    for (NodeList::iterator ci = nodeList_.begin(); ci != nodeList_.end(); ++ci) {
	if ((*ci)->id == invalidId) {
	    delete *ci; *ci = 0;
	}
    }
    // reassign ids
    nodeList_.erase(std::remove(nodeList_.begin(), nodeList_.end(), (ConstructionNode*) 0), nodeList_.end());
    for (LookaheadId id = 0; id < nodeList_.size(); ++id) {
	nodeList_[id]->id = id;
    }
    ensure(isWellOrdered());
}

/** Initialize internal compact look-ahead structure from construction tree. */

void LanguageModelLookahead::buildCompressesLookaheadStructure(
    const StateTree *st,
    const ConstructionTree &ct)
{
    require(ct.isWellOrdered());
    require(ct.nNodes() > 0);
    nodeId_.resize(st->nStates(), invalidId);

    for (LookaheadId ci = 0; ci < ct.nNodes(); ++ci) {
	const ConstructionNode &cn(ct.node(ci));
	verify(ci == nodes_.size());
	nodes_.push_back(Node());
	Node &n(nodes_.back());
	n.firstEnd = ends_.size();
	n.firstSuccessor = successors_.size();
	std::copy(cn.ends().begin(), cn.ends().end(), std::back_insert_iterator<Ends>(ends_));
	for (ConstructionNode::Successors::const_iterator si = cn.successors().begin(); si != cn.successors().end(); ++si)
	    successors_.push_back((*si)->id);
	for (std::vector<StateTree::StateId>::const_iterator ri = cn.represents.begin(); ri != cn.represents.end(); ++ri)
	    nodeId_[*ri] = ci;
    }

    for (StateTree::StateId si = 0; si < st->nStates(); ++si)
	verify(nodeId_[si] != invalidId);

    // add sentinel
    nodes_.push_back(Node());
    Node &n(nodes_.back());
    n.firstEnd = ends_.size();
    n.firstSuccessor = successors_.size();

    nEntries_ = nodes_.size() - 1;
}

void LanguageModelLookahead::buildBatchRequest() {
    require(!batchRequest_);

    Lm::BatchRequest batch;
    for (u32 n = 0; n < nEntries_; ++n) {
	for (Ends::const_iterator
	     e     = ends_.begin() + nodes_[n  ].firstEnd,
	     e_end = ends_.begin() + nodes_[n+1].firstEnd;
	     e != e_end; ++e)
	{
	    Lm::Request request((*e)->lemma()->syntacticTokenSequence(), n);
	    for (u32 ti = 0; ti < request.tokens.length(); ++ti)
		request.offset += lm_->scale() * request.tokens[ti]->classEmissionScore();
	    request.offset += wpScale_ * (*e)->pronunciationScore();
	    batch.push_back(request);
	}
    }
    batchRequest_ = lm_->compileBatchRequest(batch);
}

void LanguageModelLookahead::buildLookaheadStructure(const StateTree *st) {
    log("building look-ahead structure...");

    ConstructionTree ct;
    ct.build(st);

    log("full look-ahead tree: %d nodes", ct.nNodes());
//  ct.writeStatistics(log("full look-ahead tree statistics:\n"));

    ct.prune(this);
    ct.purge();

    log("reduced look-ahead tree: %d nodes", ct.nNodes());
    ct.writeStatistics(log("reduced look-ahead tree statistics:\n"));

    buildCompressesLookaheadStructure(st, ct);
    log("table size (%d entries): %zd bytes", nEntries_,
	sizeof(ContextLookahead) + nEntries_ * sizeof(Score));
    buildBatchRequest();

    Core::Channel dc(config, "dot");
    if (dc.isOpen()) draw(dc);
}

void LanguageModelLookahead::draw(std::ostream &os) const {
    os << "digraph \"" << fullName() << "\" {" << std::endl
       << "ranksep = 1.5" << std::endl
       << "rankdir = LR" << std::endl
       << "node [fontname=\"Helvetica\"]" << std::endl
       << "edge [fontname=\"Helvetica\"]" << std::endl;

    for (u32 ni = 0; ni < nEntries_; ++ni) {
	const Node &n(nodes_[ni]);
	os << Core::form("n%d [label=\"%d\\n", ni, ni);
	for (StateTree::StateId si = 0; si < StateTree::StateId(nodeId_.size()); ++si)
	    if (nodeId_[si] == ni) os << Core::form("%d ", si);
	for (u32 e = n.firstEnd; e < nodes_[ni+1].firstEnd; ++e)
	    os << Core::form("\\n%s", ends_[e]->lemma()->preferredOrthographicForm().str());
	os << Core::form("\"]\n");
	for (u32 s = n.firstSuccessor; s < nodes_[ni+1].firstSuccessor; ++s) {
	    os << Core::form("n%d -> n%d\n", ni, successors_[s]);
	}
    }

    os << "}" << std::endl;
}

// ===========================================================================
// dynamic data and caching

void LanguageModelLookahead::computeScores(const Lm::History &history, std::vector<Score> &scores) const {
    require(scores.size() == nEntries_);

//  log("computing look-ahead table for history ") << lm_->formatHistory(history);

    std::fill(scores.begin(), scores.end(), Core::Type<Score>::max);

    lm_->getBatch(history, batchRequest_, scores);

    std::vector<Score>::iterator score = scores.begin();
    for (std::vector<Node>::const_iterator n = nodes_.begin(); n != nodes_.end()-1; ++n) {
	Score minScore = *score;
	for (Successors::const_iterator
	     s     = successors_.begin() +  n   ->firstSuccessor,
	     s_end = successors_.begin() + (n+1)->firstSuccessor;
	     s != s_end; ++s)
	{
	    verify_(*s < LookaheadId(score - scores.begin()));
	    if (minScore > scores[*s])
		minScore = scores[*s];
	}
	*score++ = minScore;
    }
    verify(score == scores.end());
}

LanguageModelLookahead::ContextLookahead::ContextLookahead(
    const LanguageModelLookahead *la,
    const Lm::History &_history,
    u32 nEntries) :
    la_(la),
    history_(_history),
    freePos_(la->freeTables_.end()),
    scores_(nEntries)
{}

LanguageModelLookahead::ContextLookahead *LanguageModelLookahead::acquireTable(
    const Lm::History &h) const
{
    ContextLookahead *t = 0;
    if ((nTables() < cacheSizeLowMark_) || !nFreeTables_) {
	t = new ContextLookahead(this, h, nEntries_);
	tables_.push_front(t); t->pos_ = tables_.begin(); ++nTables_;
    } else {
	t = freeTables_.back(); freeTables_.pop_back(); --nFreeTables_;
	t->freePos_ = freeTables_.end();
	map_.erase(t->history_);
	t->history_ = h;
    }
    ensure(t->history_ == h);
    ensure(t->isActive());
    return t;
}

void LanguageModelLookahead::releaseTable(const ContextLookahead *ct) const {
    ContextLookahead *t = const_cast<ContextLookahead*>(ct);
    require(t->isActive());
    if (nTables() > cacheSizeHighMark_) {
	if (nFreeTables_) {
	    freeTables_.push_front(t); t->freePos_ = freeTables_.begin();
	    t = freeTables_.back();  freeTables_.pop_back();
	}
	verify(*t->pos_ == t);
	tables_.erase(t->pos_); --nTables_;
	map_.erase(t->history_);
	delete t;
    } else {
	freeTables_.push_front(t); t->freePos_ = freeTables_.begin(); ++nFreeTables_;
    }
}

LanguageModelLookahead::ContextLookahead *LanguageModelLookahead::getCachedTable(const Lm::History &h) const {
    ContextLookahead *t = 0;

    Map::const_iterator i = map_.find(h);
    if (i != map_.end()) {
	t = i->second;
	if (t->freePos_ != freeTables_.end()) {
	    cacheStatistics_->cacheEvents += CacheStatistics::freeCacheHit;
	    verify(*t->freePos_ == t);
	    freeTables_.erase(t->freePos_); --nFreeTables_;
	    t->freePos_ = freeTables_.end();
	} else {
	    cacheStatistics_->cacheEvents += CacheStatistics::shareInCacheHit;
	}
    }

    return t;
}


LanguageModelLookahead::ContextLookaheadReference
LanguageModelLookahead::getLookahead(const Lm::History &fh) const {
    Lm::History h(lm_->reducedHistory(fh, historyLimit_));

    ContextLookahead *t = getCachedTable(h);
    if (!t) {
	cacheStatistics_->cacheEvents += CacheStatistics::cacheMiss;
	map_[h] = t = acquireTable(h);
	computeScores(h, t->scores_);
    }

    ensure(t->history_ == h);
    ensure(t->isActive());

    return ContextLookaheadReference(t);
}

LanguageModelLookahead::ContextLookaheadReference
LanguageModelLookahead::tryToGetLookahead(const Lm::History &fh) const {
    Lm::History h(lm_->reducedHistory(fh, historyLimit_));

    ContextLookahead *t = getCachedTable(h);

    ensure(!t || t->history_ == h);
    ensure(!t || t->isActive());
    if (t)
	return ContextLookaheadReference(t);
    else
	return ContextLookaheadReference();
}

const Core::Choice LanguageModelLookahead::CacheStatistics::cacheEventChoice(
    "cache hits on active tables  ", shareInCacheHit,
    "cache hits on inactive tables", freeCacheHit,
    "number of table calculations ", cacheMiss,
    Core::Choice::endMark());

void LanguageModelLookahead::CacheStatistics::clear() {
    cacheEvents.clear();
    nTables.clear();
    nActiveTables.clear();
}

void LanguageModelLookahead::CacheStatistics::write(Core::XmlWriter &os) const {
    os << Core::XmlOpen("language-model-lookahead-cache-statistics")
       << cacheEvents
       << nActiveTables
       << nTables
       << Core::XmlClose("language-model-lookahead-cache-statistics");
}

LanguageModelLookahead::CacheStatistics::CacheStatistics() :
    cacheEvents("look-ahead requests", cacheEventChoice),
    nTables("number of tables in memory"),
    nActiveTables("number of active tables")
{
    clear();
}

void LanguageModelLookahead::collectStatistics() const {
    cacheStatistics_->nTables       += nTables();
    cacheStatistics_->nActiveTables += nActiveTables();
}

void LanguageModelLookahead::logStatistics() const {
    if (statisticsChannel_.isOpen())
	cacheStatistics_->write(statisticsChannel_);
}

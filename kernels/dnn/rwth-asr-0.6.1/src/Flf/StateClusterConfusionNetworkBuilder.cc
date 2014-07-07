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
#include <Core/Application.hh>
#include <Core/Choice.hh>
#include <Core/Component.hh>
#include <Core/Parameter.hh>
#include <Core/ProgressIndicator.hh>
#include <Core/StringUtilities.hh>
#include <Core/Utility.hh>
#include <Core/Vector.hh>

#include "FlfCore/Basic.hh"
#include "FlfCore/Traverse.hh"
#include "FlfCore/Utility.hh"
#include "ConfusionNetwork.hh"
#include "EpsilonRemoval.hh"
#include "FwdBwd.hh"
#include "StateClusterConfusionNetworkBuilder.hh"


namespace Flf {

    // -------------------------------------------------------------------------
    class ClusteredLattice;
    typedef Core::Ref<ClusteredLattice> ClusteredLatticeRef;

    class ClusterBuilder;
    typedef Core::Ref<ClusterBuilder> ClusterBuilderRef;

    class ClusterBuilder : public Core::Component, public Core::ReferenceCounted {
    public:
	static const Core::ParameterBool paramBwdMatch;

    public:
	struct Statistics {
	    typedef enum {
		None,
		NullArc,
		NoMatch,
		FwdMatch,
		PseudoBwdMatch,
		BwdMatchWithoutSplit,
		BwdMatchWithSplit
	    } Operation;
	    u32 nArc;
	    u32 nNullArc;
	    u32 nNoMatch;
	    u32 nFwdMatch;
	    u32 nPseudoBwdMatch;
	    u32 nBwdMatchWithoutSplit;
	    u32 nBwdMatchWithSplit;
	    u32 nSlot;
	    u32 minSlotSize;
	    u32 maxSlotSize;
	    Statistics() { clear(); }
	    void inc(Operation op) {
		++nArc;
		switch (op) {
		case None : defect(); break;
		case NullArc : ++nNullArc; break;
		case NoMatch : ++nNoMatch; break;
		case FwdMatch : ++nFwdMatch; break;
		case PseudoBwdMatch : ++nPseudoBwdMatch; break;
		case BwdMatchWithoutSplit : ++nBwdMatchWithoutSplit; break;
		case BwdMatchWithSplit : ++nBwdMatchWithSplit; break;
		}
	    }
	    void incSlot(u32 slotSize) {
		++nSlot;
		minSlotSize = std::min(minSlotSize, slotSize);
		maxSlotSize = std::max(maxSlotSize, slotSize);
	    }
	    void clear() {
		nArc = 0;
		nNullArc = 0;
		nNoMatch = 0;
		nFwdMatch = 0;
		nPseudoBwdMatch = 0;
		nBwdMatchWithoutSplit = 0;
		nBwdMatchWithSplit = 0;
		nSlot = 0;
		minSlotSize = Core::Type<u32>::max;
		maxSlotSize = Core::Type<u32>::min;
	    }
	    void dump(Core::XmlChannel &xml) {
		xml << Core::XmlFull("count", nArc)
		    + Core::XmlAttribute("name", "arc");
		xml << Core::XmlFull("count", nSlot)
		    + Core::XmlAttribute("name", "slot");

		if (nSlot > 0) {
		    xml << Core::XmlOpen("slot-statistic");
		    xml << Core::XmlFull("min", minSlotSize)
			+ Core::XmlAttribute("name", "arcs per slot");
		    xml << Core::XmlFull("avg", f32(nArc) / f32(nSlot))
			+ Core::XmlAttribute("name", "arcs per slot");
		    xml << Core::XmlFull("max", maxSlotSize)
			+ Core::XmlAttribute("name", "arcs per slot");
		    xml << Core::XmlClose("slot-statistic");
		}

		xml << Core::XmlOpen("operation-statistic");
		xml << Core::XmlFull("count", nNullArc)
		    + Core::XmlAttribute("name", "null arc match");
		xml << Core::XmlFull("count", nNoMatch)
		    + Core::XmlAttribute("name", "no match");
		xml << Core::XmlFull("count", nFwdMatch)
		    + Core::XmlAttribute("name", "fwd. match");
		xml << Core::XmlFull("count", nPseudoBwdMatch)
		    + Core::XmlAttribute("name", "pseudo bwd. match");
		xml << Core::XmlFull("count", nBwdMatchWithoutSplit)
		    + Core::XmlAttribute("name", "bwd. match w/o split");
		xml << Core::XmlFull("count", nBwdMatchWithSplit)
		    + Core::XmlAttribute("name", "bwd. match w/ split");
		xml << Core::XmlClose("operation-statistic");
	    }
	};


    protected:
	Core::XmlChannel statisticsChannel_;
	bool allowBwdMatch_;
	Statistics statistics_;

    public:
	ClusterBuilder(const Core::Configuration &config) :
	    Core::Component(config), statisticsChannel_(config, "statistics") {
	    allowBwdMatch_ = paramBwdMatch(config);
	}
	virtual ~ClusterBuilder() {}

	void reset() {
	    statistics_.clear();
	}

	void dump(std::ostream &os) {
	    os << "Cluster builder parameters:" << std::endl;
	    if (allowBwdMatch_)
		os << "Allow fwd. and bwd. matches." << std::endl;
	    else
		os << "Allow only fwd. matches." << std::endl;
	}

	void dumpStatistics() {
	    if (statisticsChannel_.isOpen()) {
		statisticsChannel_ << Core::XmlOpen("statistics")
		    + Core::XmlAttribute("component", name());
		statistics_.dump(statisticsChannel_);
		statisticsChannel_ << Core::XmlClose("statistics");
	    }
	}

	bool allowBwdMatch() const {
	    return allowBwdMatch_;
	}

	ClusteredLatticeRef cluster(ConstLatticeRef l, ConstFwdBwdRef fb);

	static ClusterBuilderRef create(const Core::Configuration &config) {
	    return ClusterBuilderRef(new ClusterBuilder(config));
	}
    };
    const Core::ParameterBool ClusterBuilder::paramBwdMatch(
	"allow-bwd-match",
	"allow backward matches",
	false);


    typedef Core::Vector<Fsa::StateId> StateIdList;
    class ClusteredLattice : public Core::ReferenceCounted {
	friend class ClusterBuilder;
    public:
	typedef Fsa::StateId ArcId;
	static const ArcId InvalidArcId;
	typedef Core::Vector<ArcId> ArcIdList;

	typedef Fsa::StateId StateId;
	static const StateId InvalidStateId;
	typedef Core::Vector<StateId> StateIdList;

	typedef Fsa::StateId ClusterId;

	struct State;
	typedef Core::Vector<State> StateList;
	class StateCluster;
	typedef Core::Vector<StateCluster*> StateClusterPtrList;


	struct Arc;
	typedef Core::Vector<Arc> ArcList;
	class ArcCluster;
	typedef Core::Vector<ArcCluster*> ArcClusterPtrList;


	struct State {
	    StateId id;
	    Time time;
	    ArcId begin, end;
	    ArcIdList bwd;
	    Score posteriorScore;
	    StateCluster *cluster;
	    State() : cluster(0) {}
	};

	class StateCluster : public StateIdList {
	public:
	    ArcCluster *prev, *next;
	    ClusterId id;
	    StateCluster() : prev(0), next(0) {}
	    StateCluster(ArcCluster *prev) : prev(prev), next(0) {}
	    StateCluster(ArcCluster *prev, ArcCluster *next) : prev(prev), next(next) {}
	    void add(StateId sid, State &state) {
		push_back(sid);
		state.cluster = this;
	    }
	};


	struct Arc {
	    StateId from, to;
	    u32 aid;
	    Score begin, end;
	    Fsa::LabelId label;
	    ScoresRef scores;
	    Score posteriorScore;
	    ArcCluster *cluster;
	    Arc() : cluster(0) {}
	};

	class ArcCluster : public ArcIdList {
	public:
	    StateCluster *prev, *next;
	    ClusterId id;
	    ArcCluster() : prev(0), next(0) {}
	    ArcCluster(StateCluster *prev) : prev(prev), next(0) {}
	    ArcCluster(StateCluster *prev, StateCluster *next) : prev(prev), next(next) {}
	    void add(ArcId aid, Arc &arc) {
		push_back(aid);
		arc.cluster = this;
	    }
	};

	struct LabelWeakOrder {
	    const ArcList &arcs;
	    bool operator() (ArcId aid1, ArcId aid2) const {
		return arcs[aid1].label < arcs[aid2].label;
	    }
	    LabelWeakOrder(const ArcList &arcs) : arcs(arcs) {}
	};

	struct BeginWeakOrder {
	    const ArcList &arcs;
	    bool operator() (ArcId aid1, ArcId aid2) const {
		return arcs[aid1].begin < arcs[aid2].begin;
	    }
	    BeginWeakOrder(const ArcList &arcs) : arcs(arcs) {}
	};

    protected:
	ConstLatticeRef l_;
	StateList states_;
	ArcList arcs_;
	StateClusterPtrList stateClusters_;
	ArcClusterPtrList arcClusters_;
	ArcCluster *nullArcCluster_;

	mutable ArcIdList sortedAids_;

    protected:
	const ArcIdList & sortArcsByLabel() const {
	    if (sortedAids_.empty()) {
		sortedAids_.resize(arcs_.size());
		ArcIdList::iterator itArc = sortedAids_.begin();
		for (ArcId aid = 0, endAid = arcs_.size(); aid < endAid; ++aid, ++itArc) *itArc = aid;
		const LabelWeakOrder arcLabelLessThan(arcs_);
		std::sort(sortedAids_.begin(), sortedAids_.end(), arcLabelLessThan);
	    }
	    return sortedAids_;
	}

	template<class T>
	inline T l1norm(const T &val1, const T &val2) {
	    return (val1 < val2) ? (val2 - val1) : (val1 - val2);
	}

	/*
	  Similarity between two arcs and
	  Similarity between arc and a hypothetic new arc cluster [beginTime, inf)

	  based on overlap
	*/
	inline Score _similarity(const Arc &hyp, const Arc &ref) const {
	    Score score = std::min(hyp.end, ref.end) - std::max(hyp.begin, ref.begin);
	    if (hyp.label == ref.label)
		score *= 2.5;
	    /*
	    dbg("sim(" << l_->getInputAlphabet()->symbol(hyp.label) << ":" << hyp.begin << "-" << hyp.end << ", "
		<< l_->getInputAlphabet()->symbol(ref.label) << ":" << ref.begin << "-" << ref.end << ") = " << score);
	    */
	    return score;
	}
	inline Score _similarity(const Arc &hyp, Score beginTime) const {
	    Score score = 2.0 * (hyp.end - beginTime);
	    /*
	    dbg("sim(" << l_->getInputAlphabet()->symbol(hyp.label) << ":" << hyp.begin << "-" << hyp.end << ", "
		<< "<new arc cluster>" << ":" << beginTime << "-" << hyp.end << ") = " << score);
	    */
	    return score;
	}

	/*
	  Similarity between two arcs and
	  Similarity between arc and a hypothetic new arc cluster [beginTime, inf)

	  based on overlap and non-overlap
	*/
	inline Score similarity(const Arc &hyp, const Arc &ref) const {
	    Score
		overlapScore = std::min(hyp.end, ref.end) - std::max(hyp.begin, ref.begin),
		diffScore = (std::max(hyp.begin, ref.begin) - std::min(hyp.begin, ref.begin)) + (std::max(hyp.end, ref.end) - std::min(hyp.end, ref.end));
	    Score score = 2.0 * overlapScore - diffScore;
	    if (hyp.label == ref.label)
		score += 2.0 * std::abs(overlapScore);
	    /*
	    dbg("sim(" << l_->getInputAlphabet()->symbol(hyp.label) << ":" << hyp.begin << "-" << hyp.end << ", "
		<< l_->getInputAlphabet()->symbol(ref.label) << ":" << ref.begin << "-" << ref.end << ") = " << score);
	    */
	    return score;
	}
	inline Score similarity(const Arc &hyp, Score beginTime) const {
	    Score
		overlapScore = hyp.end - beginTime,
		diffScore = beginTime - hyp.begin;
	    Score score = 3.0 * overlapScore - diffScore;
	    /*
	    dbg("sim(" << l_->getInputAlphabet()->symbol(hyp.label) << ":" << hyp.begin << "-" << hyp.end << ", "
		<< "<new arc cluster>" << ":" << beginTime << "-" << hyp.end << ") = " << score);
	    */
	    return score;
	}


	struct Match {
	    Score score;
	    ArcId id;
	    Match () : score(Core::Type<Score>::min), id(InvalidArcId) {}
	};
	inline void updateFwdMatch(const ArcId hypId, const Arc &hyp, const ArcId refId, const Arc &ref, Match &bestFwdMatch) const {
	    Score score = similarity(hyp, ref);
	    if (score > bestFwdMatch.score) {
		bestFwdMatch.score = score;
		bestFwdMatch.id = refId;
	    }
	}
	inline void updateBwdMatch(const ArcId hypId, const Arc &hyp, const ArcId refId, const Arc &ref, Match &bestBwdMatch) const {
	    // bwd matches are expensive: try to avoid them
	    if (hyp.begin < ref.end) {
		Score
		    hypLen = hyp.end - hyp.begin,
		    refLen = ref.end - ref.begin,
		    overlap = ref.end - hyp.begin;
		if (((hyp.label == ref.label) &&
		     (overlap > 0.25 * hypLen) &&
		     (overlap > 0.25 * refLen))
		    ||
		    ((overlap > 0.5 * hypLen) &&
		     (overlap > 0.5 * refLen))) {
		    Score score = similarity(hyp, ref);
		    if (score > bestBwdMatch.score) {
			bestBwdMatch.score = score;
			bestBwdMatch.id = refId;
		    }
		}
	    }
	}

	/*
	  Check several post-conditions for so-far produced state and arc clusters;
	  can produce verbose output for debugging.
	*/
	bool checkPostCondition(const StateCluster *initial, bool verbose = false) const;
	/*
	  re-merge arc clusters -> no empty arc clusters or state clusters
	*/
	ArcCluster * remerge(ArcCluster *newBwdArcCluster);

    private:
	ClusteredLattice() {
	    nullArcCluster_ = 0;
	}
	void cluster(ConstLatticeRef l, ConstFwdBwdRef fb, bool allowBwdMatch, ClusterBuilder::Statistics &statistics);

    public:
	virtual ~ClusteredLattice() {}


	const State & state(StateId sid) const {
	    verify_(sid < states_.size());
	    return states_[sid];
	}

	const Arc & arc(ArcId aid) const {
	    verify_(aid < arcs_.size());
	    return arcs_[aid];
	}

	Probability calcConfidence(ProbabilityList::const_iterator itP, ProbabilityList::const_iterator endP) const {
	    switch (u32(endP - itP)) {
	    case 0:
		return 1.0;
	    case 1:
		return *itP;
	    default:
		Probability maxP = 0.0;
		for (; itP != endP; ++itP)
		    if (*itP > maxP) maxP = *itP;
		return maxP;
	    }
	}

	Score calcScore(ProbabilityList::const_iterator itP, ProbabilityList::const_iterator endP, Score alpha = 0.05) const {
	    switch (u32(endP - itP)) {
	    case 0:
		return 0.0;
	    case 1:
		return 1.0 - *itP;
	    default:
		Probability sum = 0.0, length = endP - itP;
		for (; itP != endP; ++itP)
		    sum += *itP;
		verify((0.0 <= sum) && (sum <= length));
		return (length - sum) / (1.0 + alpha * (length - 1.0));
	    }
	}

	ConfusionNetwork::ConstMapPropertiesRef getMapping(bool reduce) const;

	ConstLatticeRef getLattice() const;
	std::pair<ConstConfusionNetworkRef, ConstLatticeRef> getNormalizedCn(ScoreId confidenceId, bool mapping = false) const;
	ConstConfusionNetworkRef getCn(ScoreId posteriorId = Semiring::InvalidId, bool mapping = false) const;
	ConstLatticeRef getClusteredLattice() const;
    };
    const ClusteredLattice::ArcId ClusteredLattice::InvalidArcId = Core::Type<ArcId>::max;
    const ClusteredLattice::StateId ClusteredLattice::InvalidStateId = Core::Type<StateId>::max;

    ClusteredLatticeRef ClusterBuilder::cluster(ConstLatticeRef l, ConstFwdBwdRef fb) {
	ClusteredLatticeRef clusteredL = ClusteredLatticeRef(new ClusteredLattice);
	clusteredL->cluster(l, fb, allowBwdMatch_, statistics_);
	dumpStatistics();
	return clusteredL;
    }


    ConstLatticeRef ClusteredLattice::getLattice() const {
	return l_;
    }


    bool ClusteredLattice::checkPostCondition(const StateCluster *initial, bool verbose) const {
	Core::XmlWriter *log = verbose ? &Core::Application::us()->clog() : 0;
	if (log)
	    *log << "Check post-condition:\n";
	bool good = true;
	u32 clusterId = 0;
	for (const StateCluster *cluster = initial;; cluster = cluster->next->next, ++clusterId) {
	    if (log)
		*log << "check state cluster " << cluster->id << "\n";
	    if (cluster->empty() && cluster->next) {

		if (log)
		    *log << "cluster " << cluster->id << " is empty\n";


		Core::Application::us()->warning("State cluster %d empty.", cluster->id);
		good = false;
	    }
	    if (cluster->id != clusterId) {
		Core::Application::us()->warning("State cluster id mismatch: expected %d, got %d.", clusterId, cluster->id);
		good = false;
	    }
	    for (StateCluster::const_iterator s = cluster->begin(), end_s = cluster->end(); s != end_s; ++s) {
		const State &state = states_[*s];
		if (log)
		    *log << "\tcheck state " << state.id << "\n";
		if (state.cluster->id != cluster->id) {
		    Core::Application::us()->warning("State cluster mismatch: expected %d, got %d.", cluster->id, state.cluster->id);
		    good = false;
		}
	    }
	    if (!cluster->next)
		break;
	    if (log)
		*log << "check arc cluster " << cluster->next->id << "\n";
	    if (cluster->next->id != clusterId + 1) {
		Core::Application::us()->warning("Arc cluster id mismatch: expected %d, got %d.", (clusterId + 1), cluster->next->id);
		good = false;
	    }
	    if (cluster->next->empty()) {
		Core::Application::us()->warning("Arc cluster %d is empty.", cluster->next->id);
		good = false;
	    }
	    ClusterId prevToClusterId = 0;
	    ClusterId maxFromClusterId = 0, minToClusterId = Core::Type<ClusterId>::max;
	    for (ArcCluster::const_iterator a = cluster->next->begin(), a_end = cluster->next->end(); a != a_end; ++a) {
		const Arc &arc = arcs_[*a];
		if (log)
		    *log << "\tcheck arc " << arc.from << "-" << arc.to << ", "
			 << "[" << states_[arc.from].cluster->id << ":" << arc.cluster->id << ":" << states_[arc.to].cluster->id << "], "
			 << l_->getInputAlphabet()->symbol(arc.label) << ":" << arc.begin << "-" << arc.end << "\n";
		if (arc.cluster->id != cluster->next->id) {
		    Core::Application::us()->warning("Arc cluster mismatch: expected %d, got %d.", cluster->next->id, arc.cluster->id);
		    good = false;
		}
		ClusterId fromClusterId = states_[arc.from].cluster->id;
		ClusterId toClusterId = states_[arc.to].cluster->id;
		if (fromClusterId >= toClusterId) {
		    Core::Application::us()->warning("State cluster order violation: expected %d < %d.", fromClusterId, toClusterId);
		    good = false;
		}
		if ((arc.cluster->id <= fromClusterId) || (toClusterId < arc.cluster->id)) {
		    Core::Application::us()->warning("Arc cluster out of bounds: %d not in (%d,%d].", arc.cluster->id, fromClusterId, toClusterId);
		    good = false;
		}
		/*
		  Allow any order

		  if (toClusterId < prevToClusterId)
		  Core::Application::us()->criticalError(
		  "Inner arc cluster order violation: expected %d < %d.", prevToClusterId, toClusterId);
		*/
		prevToClusterId = toClusterId;
		maxFromClusterId = std::max(maxFromClusterId, fromClusterId);
		minToClusterId = std::min(minToClusterId, toClusterId);
	    }
	    if (maxFromClusterId >= minToClusterId) {
		Core::Application::us()->warning("Arc cluster consistency violation: expected %d < %d.", maxFromClusterId, minToClusterId);
		good = false;
	    }
	}
	return good;
    }


    ClusteredLattice::ArcCluster * ClusteredLattice::remerge(ArcCluster *newBwdArcCluster) {
	if (!newBwdArcCluster)
	    return newBwdArcCluster;
	ClusterId leftBwdClusterId = newBwdArcCluster->prev->id, rightBwdClusterId = newBwdArcCluster->next->id;
	bool leftMerge = true, rightMerge = true;
	if (newBwdArcCluster->prev->prev) {
	    for (ArcCluster::const_iterator itBwdArc = newBwdArcCluster->begin(), endBwdArc = newBwdArcCluster->end();
		 itBwdArc != endBwdArc; ++itBwdArc)
		if (states_[arcs_[*itBwdArc].from].cluster->id == leftBwdClusterId)
		    { leftMerge = false; break; }
	    if (leftMerge)
		for (ArcCluster::const_iterator itLeftBwdArc = newBwdArcCluster->prev->prev->begin(),
			 endLeftBwdArc = newBwdArcCluster->prev->prev->end(); itLeftBwdArc != endLeftBwdArc; ++itLeftBwdArc)
		    if (states_[arcs_[*itLeftBwdArc].to].cluster->id == leftBwdClusterId)
			{ leftMerge = false; break; }
	} else
	    leftMerge = false;
	if (newBwdArcCluster->next->next) {
	    for (ArcCluster::const_iterator itBwdArc = newBwdArcCluster->begin(), endBwdArc = newBwdArcCluster->end();
		 itBwdArc != endBwdArc; ++itBwdArc) {
		if (states_[arcs_[*itBwdArc].to].cluster->id == rightBwdClusterId)
		    { rightMerge = false; break; }
	    }
	    if (rightMerge)
		for (ArcCluster::const_iterator itRightBwdArc = newBwdArcCluster->next->next->begin(),
			 endRightBwdArc = newBwdArcCluster->next->next->end(); itRightBwdArc != endRightBwdArc; ++itRightBwdArc) {
		    if (states_[arcs_[*itRightBwdArc].from].cluster->id == rightBwdClusterId)
			{ rightMerge = false; break; }
		}
	} else
	    rightMerge = false;
	if (leftMerge && rightMerge) {
	    Score leftScore = Core::Type<Score>::min, rightScore = Core::Type<Score>::min;
	    ArcCluster::const_iterator
		beginLeftBwdArc = newBwdArcCluster->prev->prev->begin(),
		endLeftBwdArc = newBwdArcCluster->prev->prev->end();
	    for (ArcCluster::const_iterator itBwdArc = newBwdArcCluster->begin(), endBwdArc = newBwdArcCluster->end();
		 itBwdArc != endBwdArc; ++itBwdArc) {
		const Arc &bwdArc = arcs_[*itBwdArc];
		for (ArcCluster::const_iterator itLeftBwdArc = beginLeftBwdArc; itLeftBwdArc != endLeftBwdArc; ++itLeftBwdArc) {
		    Score score = similarity(bwdArc, arcs_[*itLeftBwdArc]);
		    if (score > leftScore) leftScore = score;
		}
	    }
	    verify(leftScore != Core::Type<Score>::min);
	    ArcCluster::const_iterator
		beginRightBwdArc = newBwdArcCluster->next->next->begin(),
		endRightBwdArc = newBwdArcCluster->next->next->end();
	    for (ArcCluster::const_iterator itBwdArc = newBwdArcCluster->begin(), endBwdArc = newBwdArcCluster->end();
		 itBwdArc != endBwdArc; ++itBwdArc) {
		const Arc &bwdArc = arcs_[*itBwdArc];
		for (ArcCluster::const_iterator itRightBwdArc = beginRightBwdArc; itRightBwdArc != endRightBwdArc; ++itRightBwdArc) {
		    Score score = similarity(bwdArc, arcs_[*itRightBwdArc]);
		    if (score > rightScore) rightScore = score;
		}
	    }
	    verify(rightScore != Core::Type<Score>::min);
	    if (leftScore >= rightScore)
		rightMerge = false;
	    else
		leftMerge = false;
	}
	if (rightMerge || leftMerge) {
	    verify(!(rightMerge && leftMerge));
	    if (leftMerge) {

		//dbg("l0");

		verify(newBwdArcCluster->prev && newBwdArcCluster->prev->prev);
		ArcCluster *leftBwdArcCluster = newBwdArcCluster->prev->prev;
		for (ArcCluster::iterator itBwdArc = newBwdArcCluster->begin(), endBwdArc = newBwdArcCluster->end();
		     itBwdArc != endBwdArc; ++itBwdArc) leftBwdArcCluster->add(*itBwdArc, arcs_[*itBwdArc]);

		//dbg("l1");

		StateCluster *leftBwdCluster = newBwdArcCluster->prev;
		for (StateCluster::iterator itBwd = newBwdArcCluster->next->begin(), endBwd = newBwdArcCluster->next->end();
		     itBwd != endBwd; ++itBwd) leftBwdCluster->add(*itBwd, states_[*itBwd]);

		//dbg("l2");

		leftBwdCluster->next = leftBwdCluster->next->next->next;
		if (leftBwdCluster->next)
		    leftBwdCluster->next->prev = leftBwdCluster;

		//dbg("delete=" << newBwdArcCluster->next);

		delete newBwdArcCluster->next;

		//dbg("delete=" << newBwdArcCluster);

		delete newBwdArcCluster;
		newBwdArcCluster = leftBwdArcCluster;
	    } else if(rightMerge) {

		//dbg("r0");

		ensure(newBwdArcCluster->next && newBwdArcCluster->next->next);
		ArcCluster *rightBwdArcCluster = newBwdArcCluster->next->next;
		for (ArcCluster::iterator itRightBwdArc = rightBwdArcCluster->begin(), endRightBwdArc = rightBwdArcCluster->end();
		     itRightBwdArc != endRightBwdArc; ++itRightBwdArc) newBwdArcCluster->add(*itRightBwdArc, arcs_[*itRightBwdArc]);

		//dbg("r1");

		StateCluster *newBwdCluster = newBwdArcCluster->next;
		for (StateCluster::iterator itRightBwd = rightBwdArcCluster->next->begin(), endRightBwd = rightBwdArcCluster->next->end();
		     itRightBwd != endRightBwd; ++itRightBwd) newBwdCluster->add(*itRightBwd, states_[*itRightBwd]);
		newBwdCluster->next = newBwdCluster->next->next->next;

		//dbg("r2");

		if (newBwdCluster->next)
		    newBwdCluster->next->prev = newBwdCluster;

		//dbg("delete=" << rightBwdArcCluster->next);

		delete rightBwdArcCluster->next;

		//dbg("delete=" << rightBwdArcCluster);

		delete rightBwdArcCluster;
	    }
	    for (ArcCluster *itCluster = newBwdArcCluster; itCluster; itCluster = itCluster->next->next)
		itCluster->id = itCluster->next->id = itCluster->prev->id + 1;
	}
	return newBwdArcCluster;
    }

    void ClusteredLattice::cluster(ConstLatticeRef l, ConstFwdBwdRef fb, bool allowBwdMatch, ClusterBuilder::Statistics &statistics) {
	l_ = l;
	const Boundaries &boundaries = *l->getBoundaries();
	ConstStateMapRef chronologicalSort = sortChronologically(l);
	ensure(l->initialStateId() == chronologicalSort->front());
	states_.grow(chronologicalSort->front());

	const BeginWeakOrder beginWeakOrder(arcs_);
	// const ReverseBeginWeakOrder reverseBeginWeakOrder(arcs_);

	// initialize CN
	StateCluster *initial = new StateCluster;
	initial->id = 0;
	StateCluster *cluster = initial;

	// special arc cluster for arcs of length null
	nullArcCluster_ = new ArcCluster;
	nullArcCluster_->id = 0;

	/*
	  iterate over all states in chrono-topological order
	*/
	Core::ProgressIndicator pi(
	    Core::form("CN(%d states/ %.2f sec)",
		       u32(chronologicalSort->end() - chronologicalSort->begin()),
		       f32(boundaries.time(chronologicalSort->back()) - boundaries.time(chronologicalSort->front())) / 100.0),
	    "states");
	pi.start(u32(chronologicalSort->end() - chronologicalSort->begin()));
	for (StateMap::const_iterator itSid = chronologicalSort->begin(),
		 endSid = chronologicalSort->end(); itSid != endSid; ++itSid, pi.notify()) {
	    // state
	    Fsa::StateId sid = *itSid;
	    verify_(sid < states_.size());
	    ConstStateRef sr = l->getState(sid);
	    states_[sid].id = sid;
	    Score t = states_[sid].time = boundaries.time(sid);
	    states_[sid].posteriorScore = fb->state(sid).score();

	    // fwd arcs
	    ArcId arcId = states_[sid].begin = arcs_.size();
	    arcs_.grow(arcs_.size() + sr->nArcs() - 1);
	    states_[sid].end = arcs_.size();
	    u32 aid = 0;
	    for (Flf::State::const_iterator a = sr->begin(), end_a = sr->end();
		 a != end_a; ++a, ++aid, ++arcId) {
		verify(arcId < arcs_.size());
		Arc &arc = arcs_[arcId];
		arc.from = sid;
		arc.to = a->target();
		arc.aid = aid;
		arc.begin = t;
		arc.label = a->input();
		arc.scores = a->weight();
		arc.posteriorScore = fb->arc(sr, a).score();
		arc.cluster = 0;
		states_.grow(arc.to);
		if (states_[arc.to].cluster)
		    Core::Application::us()->criticalError(
			"CN builder: Arc violates topological order; probably a back pointing arc (%d-%d).",
			s32(t), s32(states_[arc.to].time));
		states_[arc.to].bwd.push_back(arcId);
	    } // iterate over arcs starting from current state

	    // bwd arcs (what is the best order? is there any?)
	    std::sort(states_[sid].bwd.begin(), states_[sid].bwd.end(), beginWeakOrder);
	    // std::sort(states_[sid].bwd.begin(), states_[sid].bwd.end(), reverseBeginWeakOrder);
	    for (ArcIdList::const_iterator beginAid = states_[sid].bwd.begin(), itAid = states_[sid].bwd.begin(), endAid = states_[sid].bwd.end();
		 itAid != endAid; ++itAid) {
		ArcId arcId = *itAid;
		Arc &arc = arcs_[arcId];
		verify(arc.to == sid);
		arc.end = t;

		// dbg("new arc " << arcId << " (" << l_->getInputAlphabet()->symbol(arc.label) << ":" << arc.begin << "-" << arc.end << ")");

		/*
		  Deal with null-length arcs
		*/

		/*
		if (arc.begin == arc.end) {
		    Core::Application::us()->error(
			"CN builder: Zero-length arc detected(%s,%d-%d); zero-length arcs might cause malfunctions in the alignment algorithm.",
			l->getInputAlphabet()->symbol(arc.label).c_str(), s32(arc.begin), s32(arc.end));
		    nullArcCluster_->add(arcId, arc);
		    statistics.inc( ClusterBuilder::Statistics::NullArc);
		    continue;
		}
		*/

		/*
		  Find best matching arc or start new arc cluster:
		  distinguish between non-order-violating/order-violating matches
		*/
		ClusterBuilder::Statistics::Operation op =  ClusterBuilder::Statistics::None;
		Match bestFwdMatch, bestBwdMatch;
		StateCluster *fromCluster = states_[arc.from].cluster;
		if (cluster == fromCluster) {
		    verify(!cluster->empty());
		    bestFwdMatch.score = similarity(arc, states_[cluster->back()].time);
		}
		for (ArcIdList::const_iterator itAid2 = beginAid; itAid2 != itAid; ++itAid2) {
		    ArcId arcId2 = *itAid2;
		    const Arc &arc2 = arcs_[arcId2];
		    if (fromCluster->id < arc2.cluster->id)
			updateFwdMatch(arcId, arc, arcId2, arc2, bestFwdMatch);
		    else if (allowBwdMatch)
			updateBwdMatch(arcId, arc, arcId2, arc2, bestBwdMatch);
		}
		for (StateCluster *itCluster = cluster; allowBwdMatch || (itCluster != fromCluster); itCluster = itCluster->prev->prev) {
		    for (StateIdList::const_iterator itSid2 = itCluster->begin(), endSid2 = itCluster->end();
			 itSid2 != endSid2; ++itSid2) if (*itSid2 != arc.from) {
			    const State &state2 = states_[*itSid2];
			    for (ArcIdList::const_iterator itAid2 = state2.bwd.begin(), endAid2 = state2.bwd.end();
				 itAid2 != endAid2; ++itAid2) {
				ArcId arcId2 = *itAid2;
				const Arc &arc2 = arcs_[arcId2];
				if (fromCluster->id < arc2.cluster->id)
				    updateFwdMatch(arcId, arc, arcId2, arc2, bestFwdMatch);
				else if (allowBwdMatch /* && (*itSid2 != arc.from) */)
				    updateBwdMatch(arcId, arc, arcId2, arc2, bestBwdMatch);
			    }
			}
		    if (itCluster == fromCluster)
			break;
		}
		/*
		  Assign arc to best matching arc cluster;
		  move arcs for which the new arc is the (better) nearest neighbour,
		  if necessary, split "from" state cluster and introduce new arc cluster
		*/
		ArcCluster *newBwdArcCluster = 0;
		if (bestBwdMatch.score > bestFwdMatch.score) {
		    Arc &bestBwdArc = arcs_[bestBwdMatch.id];
		    ArcCluster *bwdArcCluster = bestBwdArc.cluster;
		    if ((bestFwdMatch.id != InvalidArcId) && (states_[arcs_[bestBwdMatch.id].to].cluster->id >= arcs_[bestFwdMatch.id].cluster->id)) {
			/*
			  bwd. match -> add to existing cluster;
			  correct previous match of best bwd. arc and its nearest neighbours
			*/
			/*
			  Find representative for arcs remaining in bwd. cluster (stay) and arcs going with best bwd. arc (move);
			  if stay representative is closer to best bwd. arc than new arc => cancel bwd. match.
			*/
			ClusterId fwdArcClusterId = arcs_[bestFwdMatch.id].cluster->id;
			ArcId stayRepresentativeId = InvalidArcId, moveRepresentativeId = arcId /* bestBwdMatch.id ? */ ;
			Score moveScore = similarity(bestBwdArc, arc), bestStayScore = Core::Type<Score>::min;
			for (ArcCluster::iterator itStayArc = bwdArcCluster->begin(), endStayCluster = bwdArcCluster->end(); itStayArc != endStayCluster; itStayArc++) {
			    const Arc &stayArc = arcs_[*itStayArc];
			    ClusterId toClusterId = states_[stayArc.to].cluster->id;
			    if (toClusterId < fwdArcClusterId) {
				Score score = similarity(bestBwdArc, stayArc);
				if (score > bestStayScore) {
				    bestStayScore = score;
				    stayRepresentativeId = *itStayArc;
				    if (bestStayScore >= moveScore)
					break;
				}
			    }
			}
			if (bestStayScore == Core::Type<Score>::min) {
			    op = ClusterBuilder::Statistics::PseudoBwdMatch;
			} else if (bestStayScore >= moveScore) {
			    bestBwdMatch.score = Core::Type<Score>::min;
			    bestBwdMatch.id = InvalidArcId;
			    op = ClusterBuilder::Statistics::None;
			} else {

			    //dbg("perform bwd. match w/o split");
			    //dbg("cluster=" << cluster);

			    /*
			      Decide what arcs will stay and what will move;
			      use the result to deterimine whether a bwd. split is necesary.
			    */
			    op = ClusterBuilder::Statistics::BwdMatchWithoutSplit;
			    u32 nMove = 0;
			    const Arc &stayRepresentative = arcs_[stayRepresentativeId], &moveRepresentative = arcs_[moveRepresentativeId];
			    for (ArcCluster::iterator itBwdArc = bwdArcCluster->begin(), endBwdArc = bwdArcCluster->end(); itBwdArc != endBwdArc; ++itBwdArc) {
				if (*itBwdArc == bestBwdMatch.id) {
				    bestBwdArc.cluster = 0;
				} else {
				    Arc &bwdArc = arcs_[*itBwdArc];
				    ClusterId toClusterId = states_[bwdArc.to].cluster->id;
				    if (toClusterId >= fwdArcClusterId) {
					Score stayScore = similarity(bwdArc, stayRepresentative), moveScore = similarity(bwdArc, moveRepresentative);
					if (moveScore > stayScore) {
					    bwdArc.cluster = 0;
					    ++nMove;
					}
				    }
				}
			    }
			    verify(bestBwdArc.cluster == 0);
			    ArcCluster *fwdArcCluster = arcs_[bestFwdMatch.id].cluster;
			    fwdArcCluster->reserve(fwdArcCluster->size() + nMove);
			    newBwdArcCluster = bwdArcCluster->prev->next = bwdArcCluster->next->prev = new ArcCluster(bwdArcCluster->prev, bwdArcCluster->next);
			    newBwdArcCluster->id = bwdArcCluster->id;
			    newBwdArcCluster->reserve(bwdArcCluster->size() - nMove);
			    for (ArcCluster::const_iterator itBwdArc = bwdArcCluster->begin(), endBwdArc = bwdArcCluster->end(); itBwdArc != endBwdArc; ++itBwdArc) {
				Arc &bwdArc = arcs_[*itBwdArc];
				if (bwdArc.cluster != 0)
				    newBwdArcCluster->add(*itBwdArc, bwdArc);
				else
				    fwdArcCluster->add(*itBwdArc, bwdArc);
			    }
			    verify(!newBwdArcCluster->empty() && !fwdArcCluster->empty());

			    //dbg("delete=" << bwdArcCluster);

			    delete bwdArcCluster;
			    fwdArcCluster->add(arcId, arc);
			    states_[sid].cluster = cluster;
			    newBwdArcCluster = remerge(newBwdArcCluster);

			    ensure(initial->next->next);
			    //dbg("cluster=" << cluster);
			    for (cluster = initial; cluster->next; cluster = cluster->next->next);
			    //dbg("cluster=" << cluster);
			    states_[sid].cluster = cluster;
			}
		    } else {
			/*
			  bwd. split -> split "from" state cluster and insert new arc cluster;
			  correct previous match of best bwd. arc and its nearest neighbours
			*/
			/*
			  Find representative for arcs remaining in bwd. cluster (stay) and arcs going with best bwd. arc (move);
			  if stay representative is closer to best bwd. arc than new arc => cancel bwd. match.
			*/
			ArcId stayRepresentativeId = InvalidArcId, moveRepresentativeId = arcId /* bestBwdMatch.id ? */ ;
			Arc &bestBwdArc = arcs_[bestBwdMatch.id];
			ArcCluster *bwdArcCluster = bestBwdArc.cluster;
			Score moveScore = similarity(bestBwdArc, arc), bestStayScore = Core::Type<Score>::min;
			verify(states_[arcs_[bestBwdMatch.id].to].cluster->id >= fromCluster->id);
			for (ArcCluster::iterator itStayArc = bwdArcCluster->begin(), endStayCluster = bwdArcCluster->end(); itStayArc != endStayCluster; itStayArc++) {
			    const Arc &stayArc = arcs_[*itStayArc];
			    ClusterId toClusterId = states_[stayArc.to].cluster->id;
			    if ((toClusterId < fromCluster->id) || (stayArc.to == arc.from)) {
				Score score = similarity(bestBwdArc, stayArc);
				if (score > bestStayScore) {
				    bestStayScore = score;
				    stayRepresentativeId = *itStayArc;
				    if (bestStayScore >= moveScore)
					break;
				}
			    }
			}
			if (bestStayScore == Core::Type<Score>::min) {
			    op = ClusterBuilder::Statistics::PseudoBwdMatch;
			} else if (bestStayScore >= moveScore) {
			    bestBwdMatch.score = Core::Type<Score>::min;
			    bestBwdMatch.id = InvalidArcId;
			    op = ClusterBuilder::Statistics::None;
			} else {

			    //dbg("perform bwd. match w/ split");
			    //dbg("cluster=" << cluster);

			    /*
			      Decide what arcs will stay and what will move;
			      use the result to deterimine whether a bwd. split is necesary.
			    */
			    op =  ClusterBuilder::Statistics::BwdMatchWithSplit;
			    const Arc &stayRepresentative = arcs_[stayRepresentativeId], &moveRepresentative = arcs_[moveRepresentativeId];
			    ClusterId minToClusterId = states_[bestBwdArc.to].cluster->id;
			    Time minToTime = states_[bestBwdArc.to].time;
			    for (ArcCluster::iterator itBwdArc = bwdArcCluster->begin(), endBwdArc = bwdArcCluster->end(); itBwdArc != endBwdArc; ++itBwdArc) {
				if (*itBwdArc == bestBwdMatch.id) {
				    bestBwdArc.cluster = 0;
				} else {
				    Arc &bwdArc = arcs_[*itBwdArc];
				    ClusterId toClusterId = states_[bwdArc.to].cluster->id;
				    if ((toClusterId >= fromCluster->id) && (bwdArc.to != arc.from)) {
					Score stayScore = similarity(bwdArc, stayRepresentative), moveScore = similarity(bwdArc, moveRepresentative);
					if (moveScore > stayScore) {
					    bwdArc.cluster = 0;
					    minToClusterId = std::min(minToClusterId, toClusterId);
					    minToTime = std::min(minToTime, states_[bwdArc.to].time);
					}
				    }
				}
			    }
			    verify(bestBwdArc.cluster == 0);
			    verify(minToClusterId >= states_[arc.from].cluster->id);
			    /*
			      Best matching cluster violates time constraints.
			      Create new arc cluster by splitting most left state cluster:
			      (0<-prev)---(1<-from)---(2<-next) -> (0)--(1a)--(1b)--(2)

			      Split cluster (1) into (1a) and (1b),
			      where the current arc starts at (1a) and the best matching arc ends at (1b)
			    */
			    newBwdArcCluster = bwdArcCluster->prev->next = bwdArcCluster->next->prev = new ArcCluster(bwdArcCluster->prev, bwdArcCluster->next);
			    newBwdArcCluster->id = bwdArcCluster->id;
			    StateCluster
				*leftStateCluster = new StateCluster(fromCluster->prev, 0),
				*rightStateCluster = new StateCluster(0, fromCluster->next);
			    fromCluster->prev->next = leftStateCluster;
			    if (fromCluster->next) fromCluster->next->prev = rightStateCluster;
			    if (cluster == fromCluster) cluster = rightStateCluster;
			    ArcCluster
				*newArcCluster = leftStateCluster->next = rightStateCluster->prev = new ArcCluster(leftStateCluster, rightStateCluster);
			    leftStateCluster->id = fromCluster->id;
			    for (ArcCluster *itCluster = newArcCluster; itCluster; itCluster = itCluster->next->next)
				itCluster->id = itCluster->next->id = itCluster->prev->id + 1;
			    Time bwdSplitOverlapBegin = states_[arc.from].time, bwdSplitOverlapEnd = states_[bestBwdArc.to].time;
			    verify(bwdSplitOverlapBegin < bwdSplitOverlapEnd);
			    for (StateIdList::const_iterator itFrom = fromCluster->begin(), endFrom = fromCluster->end(); itFrom != endFrom; ++itFrom) {
				StateId fromSid = *itFrom;
				State &fromState = states_[fromSid];
				if ((fromSid != arc.from) &&
				    ((fromState.time >= minToTime) ||
				     (l1norm(fromState.time, bwdSplitOverlapEnd) <
				      l1norm(fromState.time, bwdSplitOverlapBegin)
					 ))) {
				    verify(fromSid != arc.from);
				    rightStateCluster->add(fromSid, fromState);
				} else {
				    verify(fromSid != bestBwdArc.to);
				    leftStateCluster->add(fromSid, fromState);
				}
			    }

			    //dbg("delete=" << fromCluster);

			    delete fromCluster;
			    fromCluster = leftStateCluster;
			    for (ArcCluster::iterator itBwdArc = bwdArcCluster->begin(), endBwdArc = bwdArcCluster->end(); itBwdArc != endBwdArc; ++itBwdArc) {
				Arc &bwdArc = arcs_[*itBwdArc];
				if (bwdArc.cluster != 0)
				    newBwdArcCluster->add(*itBwdArc, bwdArc);
				else
				    { verify((bwdArc.to == sid) || (states_[bwdArc.to].cluster->id >= newArcCluster->id)); newArcCluster->add(*itBwdArc, bwdArc); }
			    }
			    verify(!newBwdArcCluster->empty() && !newArcCluster->empty());

			    //dbg("delete=" << bwdArcCluster);

			    delete bwdArcCluster;
			    newArcCluster->add(arcId, arc);
			    states_[sid].cluster = cluster;
			    {
				ArcCluster *remergeArcCluster = fromCluster->next;
				while (remergeArcCluster) {
				    remergeArcCluster = remerge(remergeArcCluster);
				    remergeArcCluster = remergeArcCluster->next->next;
				}
			    }
			    //newBwdArcCluster = remerge(newBwdArcCluster);
			    //newArcCluster = remerge(newArcCluster);

			    ensure(initial->next->next);
			    //dbg("cluster=" << cluster);
			    for (cluster = initial; cluster->next; cluster = cluster->next->next);
			    //dbg("cluster=" << cluster);
			    states_[sid].cluster = cluster;
			}
		    }
		    if (op == ClusterBuilder::Statistics::PseudoBwdMatch) {

			// dbg
			//if (!checkPostCondition(initial, false))
			//  defect();

			verify(bestBwdMatch.score > bestFwdMatch.score);
			StateId fromSid = arc.from;
			State &fromState = states_[fromSid];
			StateCluster *newFromCluster = arcs_[bestBwdMatch.id].cluster->prev;
			ClusterId newFromClusterId = newFromCluster->id;
			bool isConsistent = true;
			for (ArcIdList::const_iterator itAid2 = fromState.bwd.begin(), endAid2 = fromState.bwd.end();
			     itAid2 != endAid2; ++itAid2) if (arcs_[*itAid2].cluster->id > newFromClusterId) {
				isConsistent = false;
				break;
			    }
			if (!isConsistent) {
			    /*
			      checkPostCondition(initial, true);
			      log() << "new arc " << arcId << " (" << l_->getInputAlphabet()->symbol(arc.label) << ":" << arc.begin << "-" << arc.end << ")" << "\n"
			      << "from " << fromCluster->id << " to " << arcs_[bestBwdMatch.id].cluster->prev->id << "\n";
			    */
			    Core::Application::us()->warning(
				"Give up on bwd. match, because CN has a really odd structure; fall back to no/fwd. match.");
			    bestBwdMatch.score = Core::Type<Score>::min;
			    bestBwdMatch.id = InvalidArcId;
			    op = ClusterBuilder::Statistics::None;
			} else {

			    //dbg("perform pseudo bwd. match");
			    //dbg("cluster=" << cluster);

			    StateCluster::iterator itFromCluster = fromCluster->begin();
			    for (; *itFromCluster != fromSid; ++itFromCluster) verify(itFromCluster != fromCluster->end());
			    fromCluster->erase(itFromCluster);
			    newFromCluster->add(fromSid, fromState);
			    newFromCluster->next->add(arcId, arc);
			    fromCluster = newFromCluster;
			    states_[sid].cluster = cluster;
			    remerge(fromCluster->next);
			    ensure(initial->next->next);
			    //dbg("cluster=" << cluster);
			    for (cluster = initial; cluster->next; cluster = cluster->next->next);
			    //dbg("cluster=" << cluster);
			    states_[sid].cluster = cluster;
			}

			// dbg
			/*
			if (!checkPostCondition(initial, false))
			    defect();
			*/

		    }
		}
		if (bestFwdMatch.score >= bestBwdMatch.score) {
		    verify(op == ClusterBuilder::Statistics::None);
		    if (bestFwdMatch.id == InvalidArcId) {

			// dbg("no match");

			/*
			  no match -> new cluster
			*/
			op = ClusterBuilder::Statistics::NoMatch;
			verify(cluster == fromCluster);
			cluster->next = new ArcCluster(cluster);
			cluster->next->id = cluster->id + 1;
			cluster->next->next = new StateCluster(cluster->next);
			cluster->next->next->id = cluster->id + 1;
			cluster->next->add(arcId, arc);
			cluster = cluster->next->next;
			states_[sid].cluster = cluster;
		    } else {

			// dbg("perform fwd. match");

			/*
			  fwd. match -> add to existing cluster
			*/
			op = ClusterBuilder::Statistics::FwdMatch;
			verify(cluster != fromCluster);
			arcs_[bestFwdMatch.id].cluster->add(arcId, arc);
			states_[sid].cluster = cluster;
		    }
		}
		statistics.inc(op);
		/*
		  Check partial CN for consistency
		*/
		/*
		Core::Application::us()->log() << "new arc " << arcId << " (" << l_->getInputAlphabet()->symbol(arc.label) << ":" << arc.begin << "-" << arc.end << ")\n"
					       << "operation " << op << "\n";
		checkPostCondition(initial, true);
		*/
		/*
		Core::Application::us()->log() << "added arc " << arcId << " " << states_[arc.from].id << "-" << states_[arc.to].id << " (" << l_->getInputAlphabet()->symbol(arc.label) << ":" << arc.begin << "-" << arc.end << ", "
					       << "[" << states_[arc.from].cluster->id << ":" << arc.cluster->id << ":" << states_[arc.to].cluster->id << "])\n"
					       << "operation " << op << "\n";
		if (!checkPostCondition(initial, true))
		    defect();
		*/

	    } // iterate over arcs ending in current state
	    cluster->add(sid, states_[sid]);
	    if (cluster->prev)
		verify(!cluster->prev->empty());
	} // iterate over states
	pi.finish(false);
	/*
	  Check CN for consistency
	*/
	if (checkPostCondition(initial, false)) {
	    /*
	      Convert linked list of clusters to arrays
	    */
	    stateClusters_.reserve(cluster->id + 1);
	    arcClusters_.reserve(cluster->id);
	    for (cluster = initial; cluster->next; cluster = cluster->next->next) {
		stateClusters_.push_back(cluster);
		arcClusters_.push_back(cluster->next);
		statistics.incSlot(cluster->next->size());
	    }
	    stateClusters_.push_back(cluster);
	    verify(stateClusters_.size() == (cluster->id + 1));
	    verify(arcClusters_.size() == cluster->id);
	} else
	    defect();
    }


    std::pair<ConstConfusionNetworkRef, ConstLatticeRef> ClusteredLattice::getNormalizedCn(ScoreId confidenceId, bool mapping) const {
	if (stateClusters_.empty())
	    return std::make_pair(ConstConfusionNetworkRef(), ConstLatticeRef());
	ConstSemiringRef semiring = l_->semiring();
	const LabelWeakOrder arcLabelLessThan(arcs_);

	// Posterior CN
	ConfusionNetwork *cn = new ConfusionNetwork(arcClusters_.size());
	cn->alphabet = l_->getInputAlphabet();
	cn->semiring = semiring;
	cn->normalizedProperties = ConfusionNetwork::ConstNormalizedPropertiesRef(
	    new ConfusionNetwork::NormalizedProperties(confidenceId));
	// best lattice
	StaticBoundaries *b = new StaticBoundaries;
	StaticLattice *s = new StaticLattice;
	s->setDescription("decode-cn(" + l_->describe() + ",state-cluster)");
	s->setType(Fsa::TypeAcceptor);
	s->setProperties(Fsa::PropertyAcyclic | Fsa::PropertyLinear, Fsa::PropertyAll);
	s->setInputAlphabet(l_->getInputAlphabet());
	s->setSemiring(semiring);
	s->setBoundaries(ConstBoundariesRef(b));
	s->setInitialStateId(0);
	Flf::State *sp = new Flf::State(0); s->setState(sp);
	Time minEndTime = 0, preferredEndTime = 0; // Attention: preferredEndTime >= minEndTime
	ProbabilityList pendingEpsConfidences;
	Collector
	    *col = createCollector(Fsa::SemiringTypeLog),
	    *sumCol = createCollector(Fsa::SemiringTypeLog),
	    *colBegin = createCollector(Fsa::SemiringTypeLog),
	    *colEnd = createCollector(Fsa::SemiringTypeLog);
	for (u32 i = 0; i < arcClusters_.size(); ++i) {
	    // distribute probabilities over words
	    ConfusionNetwork::Slot &slot = (*cn)[i];
	    ArcCluster &arcCluster = *arcClusters_[i];
	    verify(!arcCluster.empty());
	    std::sort(arcCluster.begin(), arcCluster.end(), arcLabelLessThan);
	    ArcId bestCcnArcId = InvalidArcId;
	    ScoresRef bestCcnArcScores = ScoresRef();
	    f64 bestPosteriorScore = Core::Type<f64>::max;
	    ArcId bestCurrentLabelCcnArcId = arcCluster.front();
	    Probability epsConfidence = 0.0;
	    for (ArcCluster::const_iterator itArc = arcCluster.begin(), endArc = arcCluster.end(); itArc != endArc; ++itArc) {
		const ArcId ccnArcId = *itArc;
		const Arc &ccnArc = arcs_[ccnArcId];
		const Arc &bestCurrentLabelCcnArc = arcs_[bestCurrentLabelCcnArcId];
		if (ccnArc.label != bestCurrentLabelCcnArc.label) {
		    f64 posteriorScore = col->get();
		    col->reset();
		    Probability confidence = ::exp(-posteriorScore);
		    if (bestCurrentLabelCcnArc.label == Fsa::Epsilon)
			epsConfidence = confidence;
		    Score start = ::exp(posteriorScore - colBegin->get()), end = ::exp(posteriorScore - colEnd->get());
		    colBegin->reset(); colEnd->reset();
		    const Flf::State &state = *l_->getState(bestCurrentLabelCcnArc.from);
		    const Flf::Arc &arc = *(state.begin() + bestCurrentLabelCcnArc.aid);
		    ScoresRef scores = arc.weight();
		    if (confidenceId != Semiring::InvalidId) {
			scores = semiring->clone(scores);
			scores->set(confidenceId, confidence);
		    }
		    slot.push_back(ConfusionNetwork::Arc(
				       bestCurrentLabelCcnArc.label,
				       scores,
				       Time(Core::round(start)), std::max(Time(1), Time(Core::round(end - start)))));
		    if (posteriorScore < bestPosteriorScore) {
			bestPosteriorScore = posteriorScore;
			bestCcnArcId = bestCurrentLabelCcnArcId;
			bestCcnArcScores = scores;
		    }
		    bestCurrentLabelCcnArcId = ccnArcId;
		    sumCol->feed(posteriorScore);
		} else if (ccnArc.posteriorScore < bestCurrentLabelCcnArc.posteriorScore) {
		    bestCurrentLabelCcnArcId = ccnArcId;
		}
		col->feed(ccnArc.posteriorScore);
		colBegin->feed(ccnArc.posteriorScore - ::log(ccnArc.begin));
		colEnd->feed(ccnArc.posteriorScore - ::log(ccnArc.end));
	    }
	    f64 posteriorScore = col->get();
	    col->reset();
	    {
		const Arc &bestCurrentLabelCcnArc = arcs_[bestCurrentLabelCcnArcId];
		Probability confidence = ::exp(-posteriorScore);
		if (bestCurrentLabelCcnArc.label == Fsa::Epsilon)
		    epsConfidence = confidence;
		Score start = ::exp(posteriorScore - colBegin->get()), end = ::exp(posteriorScore - colEnd->get());
		colBegin->reset(); colEnd->reset();
		const Flf::State &state = *l_->getState(bestCurrentLabelCcnArc.from);
		const Flf::Arc &arc = *(state.begin() + bestCurrentLabelCcnArc.aid);
		ScoresRef scores = arc.weight();
		if (confidenceId != Semiring::InvalidId) {
		    scores = semiring->clone(scores);
		    scores->set(confidenceId, confidence);
		}
		slot.push_back(ConfusionNetwork::Arc(
				   bestCurrentLabelCcnArc.label,
				   scores,
				   Time(Core::round(start)), std::max(Time(1), Time(Core::round(end - start)))));
		if (posteriorScore < bestPosteriorScore) {
		    bestPosteriorScore = posteriorScore;
		    bestCcnArcId = bestCurrentLabelCcnArcId;
		    bestCcnArcScores = scores;
		}
	    }
	    sumCol->feed(posteriorScore);
	    verify(bestCcnArcId != InvalidArcId);
	    // add remaining probability mass to eps arc
	    Probability bestConfidence = ::exp(-bestPosteriorScore);
	    Probability sum = ::exp(-sumCol->get());
	    sumCol->reset();
	    if (sum != 1.0) {
		bool normalize = false;
		Probability dEpsConfidence = 1.0 - sum;
		if (dEpsConfidence < 0.0) {
		    // if (dEpsConfidence < -0.001)
		    if (dEpsConfidence < -0.01)
			Core::Application::us()->warning(
			    "PivotCnBuilder: Expected 1.0, got %f", (1.0 - dEpsConfidence));
		    normalize = true;
		} else {
		    if (slot.front().label == Fsa::Epsilon) {
			epsConfidence += dEpsConfidence;
			if (confidenceId != Semiring::InvalidId)
			    slot.front().scores->set(confidenceId, epsConfidence);
		    } else if (dEpsConfidence >= 0.001) {
			epsConfidence += dEpsConfidence;
			ScoresRef scores = semiring->one();
			if (confidenceId != Semiring::InvalidId) {
			    scores = semiring->clone(semiring->one());
			    scores->set(confidenceId, epsConfidence);
			}
			slot.insert(slot.begin(), ConfusionNetwork::Arc(
					Fsa::Epsilon,
					scores,
					slot.front().begin + slot.front().duration / 2, 0));
		    } else
			normalize = true;
		    if (epsConfidence > bestConfidence) {
			bestConfidence = epsConfidence;
			bestCcnArcId = InvalidArcId;
		    }
		}
		if (normalize) {
		    Score norm = 1.0 / sum;
		    if (confidenceId != Semiring::InvalidId)
			for (ConfusionNetwork::Slot::iterator itSlot = slot.begin(), endSlot = slot.end(); itSlot != endSlot; ++itSlot)
			    itSlot->scores->multiply(confidenceId, norm);
		    bestConfidence *= norm;
		}
	    }
	    // add best arc
	    if ((bestCcnArcId != InvalidArcId) && (arcs_[bestCcnArcId].label != Fsa::Epsilon)) {
		const Arc &bestCcnArc = arcs_[bestCcnArcId];
		Time beginTime = Time(Core::round(bestCcnArc.begin)), endTime = Time(Core::round(bestCcnArc.end));
		if (preferredEndTime < beginTime) {
		    b->set(sp->id(), Boundary(preferredEndTime));
		    ScoresRef scores = semiring->one();
		    if (confidenceId != Semiring::InvalidId) {
			Score epsConfidence = calcConfidence(pendingEpsConfidences.begin(), pendingEpsConfidences.end());
			scores = semiring->clone(semiring->one());
			scores->set(confidenceId, epsConfidence);
		    }
		    pendingEpsConfidences.clear();
		    sp->newArc(sp->id() + 1, scores, Fsa::Epsilon, Fsa::Epsilon);
		    sp = new Flf::State(sp->id() + 1); s->setState(sp);
		    preferredEndTime = beginTime;
		} else
		    preferredEndTime = std::max(minEndTime , (preferredEndTime + beginTime) / 2);
		b->set(sp->id(), Boundary(preferredEndTime));
		if (confidenceId != Semiring::InvalidId) {
		    bestCcnArcScores = semiring->clone(bestCcnArcScores);
		    bestCcnArcScores->set(confidenceId, bestConfidence);
		}
		sp->newArc(sp->id() + 1, bestCcnArcScores, bestCcnArc.label, bestCcnArc.label);
		sp = new Flf::State(sp->id() + 1); s->setState(sp);
		minEndTime = preferredEndTime + 1;
		preferredEndTime = std::max(minEndTime, endTime);
	    } else
		pendingEpsConfidences.push_back(bestConfidence);
	}
	b->set(sp->id(), Boundary(preferredEndTime));
	ScoresRef scores = semiring->one();
	if ((confidenceId != Semiring::InvalidId) && !pendingEpsConfidences.empty()) {
	    scores = semiring->clone(scores);
	    scores->set(confidenceId, calcConfidence(pendingEpsConfidences.begin(), pendingEpsConfidences.end()));
	    pendingEpsConfidences.clear();
	}
	sp->setFinal(scores);
	delete col; delete sumCol; delete colBegin; delete colEnd;
	if (mapping)
	    cn->mapProperties = getMapping(true);
	return std::make_pair(ConstConfusionNetworkRef(cn), ConstLatticeRef(s));
    }


    namespace {
	class StateIndexBuilder : public TraverseState {
	private:
	    Core::Vector<Fsa::StateId> &stateIndex;
	protected:
	    void exploreState(ConstStateRef sr) {
		Fsa::StateId targetSid = sr->id() + 1;
		stateIndex.grow(targetSid, 0);
		stateIndex[targetSid] = sr->nArcs();
	    }
	public:
	    StateIndexBuilder(ConstLatticeRef l, Core::Vector<Fsa::StateId> &stateIndex) :
		TraverseState(l), stateIndex(stateIndex) {}

	    void build() {
		if (l->getTopologicalSort())
		    stateIndex.grow(l->getTopologicalSort()->maxSid, 0);
		traverse();
		verify(!stateIndex.empty());
		stateIndex.front() = 0;
		for (Fsa::StateId sid = 0; sid < stateIndex.size() - 1; ++sid)
		    stateIndex[sid + 1] += stateIndex[sid];
	    }
	};
    } // namespace
    ConfusionNetwork::ConstMapPropertiesRef ClusteredLattice::getMapping(bool reduce) const {
	ConfusionNetwork::MapProperties *mapProps = new ConfusionNetwork::MapProperties;
	Core::Vector<Fsa::StateId> &stateIndex = mapProps->stateIndex;
	ConfusionNetwork::MapProperties::Map &lat2cn = mapProps->lat2cn;
	{
	    StateIndexBuilder stateIndexBuilder(l_, stateIndex);
	    stateIndexBuilder.build();
	    lat2cn.resize(stateIndex.back(), ConfusionNetwork::MapProperties::Mapping(Fsa::InvalidStateId, 0));
	}
	if (reduce) {
	    for (Fsa::StateId sid = 0; sid < arcClusters_.size(); ++sid) {
		const ArcCluster &arcCluster = *arcClusters_[sid];
		for (ArcCluster::const_iterator itArc = arcCluster.begin(), endArc = arcCluster.end(); itArc != endArc; ++itArc) {
		    const Arc &ccnArc = arcs_[*itArc];
		    verify(stateIndex[ccnArc.from] + ccnArc.aid < lat2cn.size());
		    lat2cn[stateIndex[ccnArc.from] + ccnArc.aid] = ConfusionNetwork::MapProperties::Mapping(sid, Fsa::InvalidStateId);
		}
	    }
	} else {
	    Core::Vector<Fsa::StateId> &slotIndex = mapProps->slotIndex;
	    ConfusionNetwork::MapProperties::Map &cn2lat = mapProps->cn2lat;
	    {
		slotIndex.resize(arcClusters_.size() + 1);
		u32 nCnArcs = 0;
		for (Fsa::StateId sid = 0; sid < arcClusters_.size(); ++sid) {
		    const ArcCluster &arcCluster = *arcClusters_[sid];
		    slotIndex[sid] = nCnArcs;
		    nCnArcs += arcCluster.size();
		}
		slotIndex.back() = nCnArcs;
		cn2lat.resize(slotIndex.back(), ConfusionNetwork::MapProperties::Mapping(Fsa::InvalidStateId, 0));
	    }
	    for (Fsa::StateId sid = 0; sid < arcClusters_.size(); ++sid) {
		const ArcCluster &arcCluster = *arcClusters_[sid];
		Fsa::StateId aid = 0;
		for (ArcCluster::const_iterator itArc = arcCluster.begin(), endArc = arcCluster.end(); itArc != endArc; ++itArc, ++aid) {
		    const Arc &ccnArc = arcs_[*itArc];
		    verify(slotIndex[sid] + aid < cn2lat.size());
		    cn2lat[slotIndex[sid] + aid] = ConfusionNetwork::MapProperties::Mapping(ccnArc.from, ccnArc.aid);
		    verify(stateIndex[ccnArc.from] + ccnArc.aid < lat2cn.size());
		    lat2cn[stateIndex[ccnArc.from] + ccnArc.aid] = ConfusionNetwork::MapProperties::Mapping(sid, aid);
		}
	    }
	}
	return ConfusionNetwork::ConstMapPropertiesRef(mapProps);
    }


    ConstConfusionNetworkRef ClusteredLattice::getCn(ScoreId posteriorId, bool mapping) const {
	if (stateClusters_.empty())
	    return ConstConfusionNetworkRef();
	    ConstSemiringRef semiring = l_->semiring();

	ConfusionNetwork *cn = new ConfusionNetwork(arcClusters_.size());
	cn->alphabet = l_->getInputAlphabet();
	cn->semiring = semiring;
	for (u32 i = 0; i < arcClusters_.size(); ++i) {
	    ConfusionNetwork::Slot &slot = (*cn)[i];
	    const ArcCluster &arcCluster = *arcClusters_[i];
	    for (ArcCluster::const_iterator itArc = arcCluster.begin(), endArc = arcCluster.end(); itArc != endArc; ++itArc) {
		const Arc &arc = arcs_[*itArc];
		ScoresRef scores = arc.scores;
		if (posteriorId != Semiring::InvalidId) {
		    scores = semiring->clone(scores);
		    scores->set(posteriorId, ::exp(-arc.posteriorScore));
		}
		slot.push_back(ConfusionNetwork::Arc(
				   arc.label, scores,
				   Time(arc.begin), Time(arc.end - arc.begin),
				   states_[arc.from].cluster->id, states_[arc.to].cluster->id));
	    }
	}
	if (mapping)
	    cn->mapProperties = getMapping(false);
	return ConstConfusionNetworkRef(cn);
    }


    ConstLatticeRef ClusteredLattice::getClusteredLattice() const {
	if (stateClusters_.empty())
	    return ConstLatticeRef();
	ConstSemiringRef semiring = l_->semiring();
	StaticBoundaries *b = new StaticBoundaries;
	StaticLattice *s = new StaticLattice;
	s->setDescription(Core::form("overlap-cn(%s)", l_->describe().c_str()));
	s->setType(Fsa::TypeAcceptor);
	s->setProperties(Fsa::PropertyAcyclic, Fsa::PropertyAll);
	s->setInputAlphabet(l_->getInputAlphabet());
	s->setSemiring(semiring);
	s->setBoundaries(ConstBoundariesRef(b));
	s->setInitialStateId(0);
	Flf::State *sp = 0;
	s32 lastT = -1;
	for (Fsa::StateId sid = 0; sid < stateClusters_.size(); ++sid) {
	    const StateCluster &cluster = *stateClusters_[sid];
	    verify(cluster.id == sid);
	    sp = s->newState(sid); s->setState(sp);
	    Time tSum = 0;
	    for (StateCluster::const_iterator itState = cluster.begin(), endState = cluster.end(); itState != endState; ++itState)
		tSum += states_[*itState].time;

	    verify(!cluster.empty());

	    s32 t = tSum / cluster.size();
	    if (t <= lastT) {
		/*
		Core::Application::us()->warning(
		    "Clustered lattice of \"%s\" violates time constraints: %d > %d violated",
		    l_->describe().c_str(), t, lastT);
		*/
		t = lastT + 1;
	    }
	    b->set(sid, Boundary(Time(t)));
	    lastT = t;
	}
	sp->setFinal(semiring->one());
	const ArcIdList &sortedAids = sortArcsByLabel();
	for (ArcIdList::const_iterator itAid = sortedAids.begin(), endAid = sortedAids.end(); itAid != endAid; ++itAid) {
	    const Arc &arc = arcs_[*itAid];
	    u32 startCluster = states_[arc.from].cluster->id, endCluster = states_[arc.to].cluster->id;
	    Flf::State *sp = s->fastState(startCluster);
	    // if arc with same source, target, and label exists -> collect weights,
	    // else add new arc
	    bool isDuplicate = false;
	    Flf::State::reverse_iterator a = sp->rbegin();
	    for (; (a != sp->rend()) && (a->input() == arc.label); ++a)
		if (a->target() == endCluster) { isDuplicate = true; break; }
	    if (isDuplicate) {
		    a->setWeight(semiring->collect(a->weight(), arc.scores));
	    } else {
		sp->newArc(endCluster, arc.scores, arc.label, arc.label);
	    }
	}
	return ConstLatticeRef(s);
    }
    // -------------------------------------------------------------------------


    // -------------------------------------------------------------------------
    class StateClusterCnBuilderNode : public Node {
    public:
	static const Core::ParameterString paramConfKey;
	static const Core::ParameterBool paramMap;
	static const Core::ParameterBool paramRemoveNullArcs;

    private:
	u32 n_;
	FwdBwdBuilderRef fbBuilder_;
	ClusterBuilderRef clusterBuilder_;
	Key confidenceKey_;
	bool map_;
	bool removeNullArcs_;

	bool isClustered_;
	ClusteredLatticeRef clusteredLat_;
	ConstLatticeRef best_;
	ConstConfusionNetworkRef normalizedCn_;
	ConstConfusionNetworkRef cn_;
	ConstLatticeRef clustered_;

    private:
	ClusteredLatticeRef clusteredLattice() {
	    if (!isClustered_) {
		ConstLatticeRefList lats(n_);
		for (u32 i = 0; i < n_; ++i) {
		    lats[i] = requestLattice(i);
		    if (removeNullArcs_)
			lats[i] = fastRemoveNullArcs(lats[i]);
		}
		std::pair<ConstLatticeRef, ConstFwdBwdRef> fbResult = (n_ == 1) ?
		    fbBuilder_->build(lats.front()) : fbBuilder_->build(lats);
		clusteredLat_ = clusterBuilder_->cluster(fbResult.first, fbResult.second);
		isClustered_ = true;
	    }
	    return clusteredLat_;
	}

	ScoreId getConfidenceId() {
	    ScoreId confidenceId = Semiring::InvalidId;
	    if (!confidenceKey_.empty()) {
		ConstSemiringRef semiring = clusteredLattice()->getLattice()->semiring();
		confidenceId = semiring->id(confidenceKey_);
		if (confidenceId == Semiring::InvalidId)
		    warning("Semiring \"%s\" has no dimension labeled \"%s\".",
			    semiring->name().c_str(), confidenceKey_.c_str());
	    }
	    return confidenceId;
	}

	ConstConfusionNetworkRef getCn() {
	    if (!cn_)
		cn_ = clusteredLattice()->getCn(getConfidenceId(), map_);
	    return cn_;
	}

	ConstConfusionNetworkRef getNormalizedCn() {
	    if (!normalizedCn_) {
		std::pair<ConstConfusionNetworkRef, ConstLatticeRef> result = clusteredLattice()->getNormalizedCn(getConfidenceId(), map_);
		normalizedCn_ = result.first; best_ = result.second;
	    }
	    return normalizedCn_;
	}

	ConstLatticeRef getBestLattice() {
	    if (!best_) {
		std::pair<ConstConfusionNetworkRef, ConstLatticeRef> result = clusteredLattice()->getNormalizedCn(getConfidenceId(), map_);
		normalizedCn_ = result.first; best_ = result.second;
	    }
	    return best_;
	}

	ConstLatticeRef getClusteredLattice() {
	    if (!clustered_) {
		clustered_ = clusteredLattice()->getClusteredLattice();
	    }
	    return clustered_;
	}

    public:
	StateClusterCnBuilderNode(const std::string &name, const Core::Configuration &config) :
	    Node(name, config), n_(0) {}
	virtual ~StateClusterCnBuilderNode() {}

	virtual void init(const std::vector<std::string> &arguments) {
	    for (n_ = 0; connected(n_); ++n_);
	    if (n_ == 0)
		criticalError("At least one incoming lattice at port 0 required.");
	    Core::Component::Message msg = log();
	    if (n_ > 1)
		msg << "Combine " << n_ << " lattices.\n\n";
	    fbBuilder_ = FwdBwdBuilder::create(select("fb"));
	    clusterBuilder_ = ClusterBuilder::create(config);
	    clusterBuilder_->dump(msg); msg << "\n";
	    confidenceKey_ = paramConfKey(config);
	    if (!confidenceKey_.empty())
		msg << "Confidence key is \"" << confidenceKey_ << "\"\n";
	    map_ = paramMap(config);
	    if (map_)
		msg << "Build lattice-to-CN resp. CN-to-lattice mapping\n";
	    removeNullArcs_ = paramRemoveNullArcs(config);
	    if (removeNullArcs_)
		msg << "Remove null-length before building the lattice union\n";
	    isClustered_ = false;
	}

	virtual void finalize() {}

	virtual ConstLatticeRef sendLattice(Port to) {
	    switch (to) {
	    case 0:
		return getBestLattice();
	    case 2:
		return cn2lattice(getNormalizedCn());
	    case 4:
		return cn2lattice(getCn());
	    case 6:
		return getClusteredLattice();
	    default:
		defect();
		return ConstLatticeRef();
	    }
	}

	virtual ConstConfusionNetworkRef sendCn(Port to) {
	    switch (to) {
	    case 1:
		return getNormalizedCn();
	    case 3:
		return getCn();
	    default:
		defect();
		return ConstConfusionNetworkRef();
	    }
	}

	virtual void sync() {
	    clusteredLat_.reset();
	    best_.reset();
	    normalizedCn_.reset();
	    cn_.reset();
	    clustered_.reset();
	    clusterBuilder_->reset();
	    isClustered_ = false;
	}
    };
    const Core::ParameterString StateClusterCnBuilderNode::paramConfKey(
	"confidence-key",
	"store confidence score",
	"");
    const Core::ParameterBool StateClusterCnBuilderNode::paramMap(
	"map",
	"map lattice to CN and vice versa",
	false);
    const Core::ParameterBool StateClusterCnBuilderNode::paramRemoveNullArcs(
	"remove-null-arcs",
	"remove null arcs",
	false);
    NodeRef createStateClusterCnBuilderNode(const std::string &name, const Core::Configuration &config) {
	return NodeRef(new StateClusterCnBuilderNode(name, config));
    }
    // -------------------------------------------------------------------------

} // namespace Flf

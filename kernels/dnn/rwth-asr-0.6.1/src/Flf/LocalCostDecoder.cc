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
#include <Core/Choice.hh>
#include <Core/Component.hh>
#include <Core/Hash.hh>
#include <Core/Parameter.hh>
#include <Core/ProgressIndicator.hh>

#include "FlfCore/Basic.hh"
#include "FlfCore/LatticeInternal.hh"
#include "Copy.hh"
#include "FwdBwd.hh"
#include "LocalCostDecoder.hh"
#include "RescoreInternal.hh"
#include "Union.hh"


namespace Flf {

    // -------------------------------------------------------------------------
    class LocalCostDecoder;
    typedef Core::Ref<const LocalCostDecoder> ConstLocalCostDecoderRef;

    class LocalCostDecoder : public Core::ReferenceCounted {
    public:
	class RiskBuilder;
	typedef Core::Ref<RiskBuilder> RiskBuilderRef;

	class LocalOverlapRiskBuilder;
	class ArcSymetricFrameErrorRiskBuilder;
	class LocalAlignmentRiskBuilder;

    private:
	const RiskBuilderRef riskBuilder_;
	const Score wordPenalty_;
	ConstLatticeRef l_;
	Core::Vector<u32> offsets_;
	ScoreList risks_, confidences_;

    public:
	LocalCostDecoder(RiskBuilderRef riskBuilder, Score wordPenalty);

	void build(ConstLatticeRef hypSpaceL, ConstLatticeRef sumSpaceL, ConstFwdBwdRef sumSpaceFb);

	std::pair<ConstLatticeRef, Score> best(ScoreId confidenceId = Semiring::InvalidId) const;

	std::string name() const;

	Score wordPenalty() const {
	    return wordPenalty_;
	}

	ScoreList::const_iterator beginRisk(Fsa::StateId sid) const {
	    verify_(l_);
	    return risks_.begin() + offsets_[sid];
	}

	Score risk(Fsa::StateId sid, u32 aid) const {
	    verify_(l_);
	    return risks_[offsets_[sid] + aid];
	}

	ScoreList::const_iterator beginConfidence(Fsa::StateId sid) const {
	    verify_(l_);
	    return confidences_.begin() + offsets_[sid];
	}

	Score confidence(Fsa::StateId sid, u32 aid) const {
	    verify_(l_);
	    return confidences_[offsets_[sid] + aid];
	}

	// static create function for reference counted objects
	static ConstLocalCostDecoderRef create(
	    ConstLatticeRef hypSpaceL, ConstLatticeRef sumSpaceL, ConstFwdBwdRef sumSpaceFb,
	    RiskBuilderRef riskBuilder, Score wordPenalty);
    };
    // -------------------------------------------------------------------------


    // -------------------------------------------------------------------------
    class LocalCostDecoder::RiskBuilder : public Core::ReferenceCounted {
    protected:
	Core::Vector<u32> *offsets;
	ScoreList *risks;
	ScoreList *confidences;

	struct SummationSpace {
	    struct State {
		Time time;
		u32 beginArc;
		u32 nArcs;
		f64 fwdScore, bwdScore;
	    };
	    typedef Core::Vector<State> StateList;
	    StateList states;
	    struct Arc {
		Fsa::StateId from, to;
		Time beginTime, endTime;
		f64 duration;
		Fsa::LabelId label;
		f64 score, normScore;
		Probability posterior;
	    };
	    typedef Core::Vector<Arc> ArcList;
	    ArcList arcs;
	} sumSpace;

	struct HypothesisSpace {
	    struct State {
		u32 beginArc;
		u32 nArcs;
	    };
	    typedef Core::Vector<State> StateList;
	    StateList states;
	    struct Arc {
		// Fsa::StateId from, to;
		u32 order;
		Time beginTime, endTime;
		f64 duration;
		Fsa::LabelId label;
		Score risk, confidence;
	    };
	    typedef Core::Vector<Arc> ArcList;
	    ArcList arcs;

	    struct SortArcByOrder {
		bool operator()(const Arc &arc1, const Arc &arc2) const
		    { return arc1.order < arc2.order; }
	    };
	    struct SortArcByLabelAndEndTime {
		bool operator()(const Arc &arc1, const Arc &arc2) const
		    { return (arc1.label < arc2.label) || ((arc1.label == arc2.label) && (arc1.endTime < arc2.endTime)); }
	    };
	} hypSpace;

    protected:
	void initializeHypothesisSpace(ConstLatticeRef l, ConstStateMapRef sortedStates);
	void finalizeHypothesisSpace(ConstLatticeRef l, ConstStateMapRef sortedStates);
	void initializeSummationSpace(ConstLatticeRef l, ConstStateMapRef sortedStates, ConstFwdBwdRef fb);

	inline Time overlap(const HypothesisSpace::Arc &arc1, const SummationSpace::Arc &arc2) const {
	    return ((arc1.endTime < arc2.endTime) ? arc1.endTime : arc2.endTime)
		- ((arc1.beginTime > arc2.beginTime) ? arc1.beginTime : arc2.beginTime);
	}

	inline bool equalRisk(const HypothesisSpace::Arc &arc1, const HypothesisSpace::Arc &arc2) const {
	    return (arc1.beginTime == arc2.beginTime)
		&& (arc1.endTime == arc2.endTime)
		&& (arc1.label == arc2.label);
	}

    public:
	RiskBuilder() : offsets(0), risks(0), confidences(0) {}
	virtual ~RiskBuilder() {}

	void set(Core::Vector<u32> *offsets, ScoreList *risks, ScoreList *confidences) {
	    this->offsets = offsets;
	    this->risks =risks;
	    this->confidences = confidences;
	}

	virtual void build(ConstLatticeRef hypSpaceL, ConstLatticeRef sumSpaceL, ConstFwdBwdRef sumSpaceFb) = 0;

	virtual std::string name() const = 0;
    };

    void LocalCostDecoder::RiskBuilder::initializeHypothesisSpace(ConstLatticeRef l, ConstStateMapRef sortedStates) {
	require(offsets && risks && confidences);
	offsets->clear(); risks->clear(); confidences->clear();
	HypothesisSpace::StateList &states = hypSpace.states;
	HypothesisSpace::ArcList & arcs = hypSpace.arcs;
	const Boundaries &b = *l->getBoundaries();
	u32 nArcs = 0;
	// collect some lattice statistics, construct list of states, initialize data structure
	states.clear(); states.grow(sortedStates->maxSid);
	{
	    for (StateMap::const_iterator itSid = sortedStates->begin(), endSid = sortedStates->end(); itSid != endSid; ++itSid) {
		HypothesisSpace::State &state = states[*itSid];
		{
		    const Flf::State &s = *l->getState(*itSid);
		    state.beginArc = nArcs;
		    nArcs += s.nArcs();
		    state.nArcs = s.nArcs();
		}
	    }
	}
	arcs.clear(); arcs.grow(nArcs - 1);
	// construct list of arcs sorted by 1) start time and 2) end time
	{
	    HypothesisSpace::ArcList::iterator itArc = arcs.begin();
	    for (StateMap::const_iterator itSid = sortedStates->begin(), endSid = sortedStates->end(); itSid != endSid; ++itSid) {
		Time beginTime = b.get(*itSid).time();
		const Flf::State &s = *l->getState(*itSid);
		HypothesisSpace::ArcList::iterator beginArc = itArc;
		u32 arcOrder = 0;
		for (Flf::State::const_iterator itA = s.begin(), endA = s.end(); itA != endA; ++itA, ++itArc, ++arcOrder) {
		    const Flf::Arc &a = *itA;
		    HypothesisSpace::Arc &arc = *itArc;
		    arc.order = arcOrder;
		    arc.beginTime = beginTime;
		    arc.endTime = b.get(a.target()).time();
		    arc.duration = f64(arc.endTime - arc.beginTime);
		    arc.label = a.input();
		}
		std::sort(beginArc, itArc, HypothesisSpace::SortArcByLabelAndEndTime());
	    }
	}
    }

    void LocalCostDecoder::RiskBuilder::finalizeHypothesisSpace(ConstLatticeRef l, ConstStateMapRef sortedStates) {
	HypothesisSpace::StateList &states = hypSpace.states;
	HypothesisSpace::ArcList & arcs = hypSpace.arcs;
	// copy risk and confidence to final data structure, restore original arc order
	offsets->grow(states.size() - 1, Core::Type<u32>::max);
	risks->grow(arcs.size() - 1);
	confidences->grow(arcs.size() - 1);
	ScoreList::iterator itRisk = risks->begin();
	ScoreList::iterator itConfidence = confidences->begin();
	HypothesisSpace::ArcList::iterator itArc = arcs.begin();
	for (StateMap::const_iterator itSid = sortedStates->begin(), endSid = sortedStates->end(); itSid != endSid; ++itSid) {
	    const HypothesisSpace::State &state = states[*itSid];
	    (*offsets)[*itSid] = state.beginArc;
	    HypothesisSpace::ArcList::iterator endArc = itArc + state.nArcs;
	    std::sort(itArc, endArc, HypothesisSpace::SortArcByOrder());
	    for (; itArc != endArc; ++itArc, ++itRisk, ++itConfidence) {
		const HypothesisSpace::Arc &arc = *itArc;
		*itRisk = arc.risk;
		*itConfidence = arc.confidence;
	    }
	}
    }

    void LocalCostDecoder::RiskBuilder::initializeSummationSpace(ConstLatticeRef l, ConstStateMapRef sortedStates, ConstFwdBwdRef fb) {
	SummationSpace::StateList &states = sumSpace.states;
	SummationSpace::ArcList & arcs = sumSpace.arcs;
	const Boundaries &b = *l->getBoundaries();
	u32 nArcs = 0;
	// collect some lattice statistics, construct list of states, initialize data structure
	states.clear(); states.grow(sortedStates->maxSid);
	{
	    for (StateMap::const_iterator itSid = sortedStates->begin(), endSid = sortedStates->end(); itSid != endSid; ++itSid) {
		SummationSpace::State &state = states[*itSid];
		state.time = b.get(*itSid).time();
		{
		    const Flf::State &s = *l->getState(*itSid);
		    state.beginArc = nArcs;
		    nArcs += s.nArcs();
		    state.nArcs = s.nArcs();
		}
		{
		    const FwdBwd::State &s = fb->state(*itSid);
		    state.fwdScore = s.fwdScore;
		    state.bwdScore = s.bwdScore;
		}
	    }
	}
	arcs.clear(); arcs.grow(nArcs - 1);
	// construct list of arcs sorted by start time
	{
	    SummationSpace::ArcList::iterator itArc = arcs.begin();
	    for (StateMap::const_iterator itSid = sortedStates->begin(), endSid = sortedStates->end(); itSid != endSid; ++itSid) {
		Time beginTime = b.get(*itSid).time();
		const Flf::State &s = *l->getState(*itSid);
		FwdBwd::State::const_iterator itFbArc = fb->state(*itSid).begin();
		for (Flf::State::const_iterator itA = s.begin(), endA = s.end(); itA != endA; ++itA, ++itArc, ++itFbArc) {
		    const Flf::Arc &a = *itA;
		    SummationSpace::Arc &arc = *itArc;
		    arc.from = *itSid;
		    arc.to = a.target();
		    arc.beginTime = beginTime;
		    arc.endTime = b.get(a.target()).time();
		    arc.duration = f64(arc.endTime - arc.beginTime);
		    arc.label = a.input();
		    arc.score = itFbArc->arcScore;
		    arc.normScore = itFbArc->normScore;
		    arc.posterior = itFbArc->probability();
		}
	    }
	}
    }
    // -------------------------------------------------------------------------


    // -------------------------------------------------------------------------
    class LocalCostDecoder::LocalOverlapRiskBuilder : public LocalCostDecoder::RiskBuilder {
	typedef RiskBuilder Precursor;
    private:
	const f64 hypWeight_, refWeight_;
    public:
	LocalOverlapRiskBuilder(f64 alpha = 0.5) :
	    Precursor(), hypWeight_(alpha), refWeight_(1.0 - alpha) {}
	void build(ConstLatticeRef hypSpaceL, ConstLatticeRef sumSpaceL, ConstFwdBwdRef sumSpaceFb);
	std::string name() const { return Core::form("local-time-overlap(alpha=%.2f)", hypWeight_); }
    };

    void LocalCostDecoder::LocalOverlapRiskBuilder::build(ConstLatticeRef hypSpaceL, ConstLatticeRef sumSpaceL, ConstFwdBwdRef sumSpaceFb) {
	Core::ProgressIndicator pi("initialize");
	pi.start();
	// initialize
	ConstStateMapRef sortedHypSpaceStates = sortChronologically(hypSpaceL);
	initializeHypothesisSpace(hypSpaceL, sortedHypSpaceStates);
	ConstStateMapRef sortedSumSpaceStates = sortChronologically(sumSpaceL);
	initializeSummationSpace(sumSpaceL, sortedSumSpaceStates, sumSpaceFb);
	// calculate costs and confidences
	SummationSpace::ArcList::const_iterator beginRefArc = sumSpace.arcs.begin(), endRefArc = sumSpace.arcs.end();
	const HypothesisSpace::Arc *lastHypArcPtr = 0;
	pi.setTask(Core::form("#arcs=%zu", hypSpace.arcs.size()));
	pi.setTotal(hypSpace.arcs.size());
	for (HypothesisSpace::ArcList::iterator itHypArc = hypSpace.arcs.begin(), endHypArc = hypSpace.arcs.end(); itHypArc != endHypArc; ++itHypArc) {
	    HypothesisSpace::Arc &hypArc = *itHypArc;
	    if (hypArc.beginTime == hypArc.endTime) {
		// null-length score
		hypArc.risk = 0.0;
		hypArc.confidence = 1.0;
	    } else if (lastHypArcPtr && equalRisk(*lastHypArcPtr, hypArc)) {
		// cached score
		hypArc.risk = lastHypArcPtr->risk;
		hypArc.confidence = lastHypArcPtr->confidence;
	    } else {
		// compute score
		f64 hypRisk = 0.0, refRisk = 0.0;
		for (; beginRefArc->endTime <= hypArc.beginTime; ++beginRefArc) verify(beginRefArc != endRefArc);
		for (SummationSpace::ArcList::const_iterator itRefArc = beginRefArc; (itRefArc != endRefArc) && (itRefArc->beginTime < hypArc.endTime); ++itRefArc) {
		    const SummationSpace::Arc &refArc = *itRefArc;
		    if ((hypArc.beginTime < refArc.endTime) && (refArc.beginTime < refArc.endTime)) {
			verify(refArc.beginTime < hypArc.endTime);
			if (hypArc.label != refArc.label) {
			    const f64 overlapDuration = f64(overlap(hypArc, refArc));
			    hypRisk += refArc.posterior * (overlapDuration / hypArc.duration);
			    refRisk += refArc.posterior * (overlapDuration / refArc.duration);
			}
		    }
		}
		hypArc.risk = hypWeight_ * hypRisk + refWeight_ * refRisk;
		hypArc.confidence = std::max(0.0, 1.0 - hypRisk);
	    }
	    pi.notify(u32(itHypArc - hypSpace.arcs.begin()));
	    lastHypArcPtr = &hypArc;
	}
	// finalize
	finalizeHypothesisSpace(hypSpaceL, sortedHypSpaceStates);
	pi.finish(false);
    }
    // -------------------------------------------------------------------------


    // -------------------------------------------------------------------------
    class LocalCostDecoder::ArcSymetricFrameErrorRiskBuilder : public LocalCostDecoder::RiskBuilder {
	typedef RiskBuilder Precursor;
    public:
	ArcSymetricFrameErrorRiskBuilder() : Precursor() {}
	void build(ConstLatticeRef hypSpaceL, ConstLatticeRef sumSpaceL, ConstFwdBwdRef sumSpaceFb);
	std::string name() const { return Core::form("arc-symetric-frame-error"); }
    };

    void LocalCostDecoder::ArcSymetricFrameErrorRiskBuilder::build(ConstLatticeRef hypSpaceL, ConstLatticeRef sumSpaceL, ConstFwdBwdRef sumSpaceFb) {
	Core::ProgressIndicator pi("initialize");
	pi.start();
	// initialize
	ConstStateMapRef sortedHypSpaceStates = sortChronologically(hypSpaceL);
	initializeHypothesisSpace(hypSpaceL, sortedHypSpaceStates);
	ConstStateMapRef sortedSumSpaceStates = sortChronologically(sumSpaceL);
	initializeSummationSpace(sumSpaceL, sortedSumSpaceStates, sumSpaceFb);
	// calculate costs and confidences
	SummationSpace::ArcList::const_iterator beginRefArc = sumSpace.arcs.begin(), endRefArc = sumSpace.arcs.end();
	const HypothesisSpace::Arc *lastHypArcPtr = 0;
	pi.setTask(Core::form("#arcs=%zu", hypSpace.arcs.size()));
	pi.setTotal(hypSpace.arcs.size());
	for (HypothesisSpace::ArcList::iterator itHypArc = hypSpace.arcs.begin(), endHypArc = hypSpace.arcs.end(); itHypArc != endHypArc; ++itHypArc) {
	    HypothesisSpace::Arc &hypArc = *itHypArc;
	    if (hypArc.beginTime == hypArc.endTime) {
		// null-length score
		hypArc.risk = 0.0;
		hypArc.confidence = 1.0;
	    } else if (lastHypArcPtr && equalRisk(*lastHypArcPtr, hypArc)) {
		// cached score
		hypArc.risk = lastHypArcPtr->risk;
		hypArc.confidence = lastHypArcPtr->confidence;
	    } else {
		// compute score
		hypArc.risk = 0.0;
		for (; beginRefArc->endTime <= hypArc.beginTime; ++beginRefArc) verify(beginRefArc != endRefArc);
		for (SummationSpace::ArcList::const_iterator itRefArc = beginRefArc; (itRefArc != endRefArc) && (itRefArc->beginTime < hypArc.endTime); ++itRefArc) {
		    const SummationSpace::Arc &refArc = *itRefArc;
		    if ((hypArc.beginTime < refArc.endTime) && (refArc.beginTime < refArc.endTime)) {
			verify(refArc.beginTime < hypArc.endTime);
			Time ferr = std::max(refArc.endTime, hypArc.endTime) - std::min(refArc.beginTime, hypArc.beginTime);
			if (hypArc.label == refArc.label)
			    ferr -= overlap(hypArc, refArc);
			hypArc.risk += refArc.posterior * (f64(ferr) / std::min(refArc.duration, hypArc.duration));
		    }
		}
		hypArc.confidence = std::max(0.0, 1.0 - hypArc.risk);
	    }
	    pi.notify(u32(itHypArc - hypSpace.arcs.begin()));
	    lastHypArcPtr = &hypArc;
	}
	// finalize
	finalizeHypothesisSpace(hypSpaceL, sortedHypSpaceStates);
	pi.finish(false);
    }
    // -------------------------------------------------------------------------


    // -------------------------------------------------------------------------
    class LocalCostDecoder::LocalAlignmentRiskBuilder : public LocalCostDecoder::RiskBuilder {
    public:
	struct Closure {
	    struct Path {
		u32 target;
		f64 fbScore;
		// cost = nToken - nCorrect
		f64 nToken;    // number of reference words
		f64 nCorrect;  // number of correct words - number of insertions
		// accuracy aka confidence
		f64 accuracy;
		Path() :
		    target(Core::Type<u32>::max), fbScore(Core::Type<f64>::max),
		    nToken(0.0), nCorrect(0.0),
		    accuracy(0.0) {}
	    };

	    typedef std::vector<Path> PathList;
	};

	class OverlapScorer : public Core::ReferenceCounted {
	public:
	    virtual ~OverlapScorer() {}
	    virtual void initialize(Closure::Path &path, const HypothesisSpace::Arc &hypArc, const f64 hypProp, const SummationSpace::Arc &refArc, const f64 refProp) const = 0;
	    virtual void expand(Closure::Path &path, const HypothesisSpace::Arc &hypArc, const f64 hypProp, const SummationSpace::Arc &refArc, const f64 refProp) const = 0;
	    virtual void addRiskAndConfidence(f64 &risk, f64 &confidence, const HypothesisSpace::Arc &hypArc, const Closure::Path &path, const Probability pathPosterior) const = 0;
	    virtual std::string name() const = 0;
	};
	typedef Core::Ref<const OverlapScorer> ConstOverlapScorerRef;

	class LocalAccuracyScorer;
	class CostScorer;
	class ContinousCostScorer1;
	class ContinousCostScorer2;
	class DiscreteCostScorer;

    private:
	ConstOverlapScorerRef scorer_;
    public:
	LocalAlignmentRiskBuilder(ConstOverlapScorerRef scorer) : scorer_(scorer)
	    { verify(scorer_); }
	void build(ConstLatticeRef hypSpaceL, ConstLatticeRef sumSpaceL, ConstFwdBwdRef sumSpaceFb);
	std::string name() const { return "local-time-alignment(scorer=" + scorer_->name() + ")"; }
    };

    class LocalCostDecoder::LocalAlignmentRiskBuilder::LocalAccuracyScorer :
	public LocalCostDecoder::LocalAlignmentRiskBuilder::OverlapScorer {
    public:
	void initialize(Closure::Path &path, const HypothesisSpace::Arc &hypArc, const f64 hypProp, const SummationSpace::Arc &refArc, const f64 refProp) const {
	    path.accuracy = (hypArc.label == refArc.label) ? -1.0 + 2.0 * refProp : -1.0 + refProp;
	}
	void expand(Closure::Path &path, const HypothesisSpace::Arc &hypArc, const f64 hypProp, const SummationSpace::Arc &refArc, const f64 refProp) const {
	    const f64 accuracy = (hypArc.label == refArc.label) ? -1.0 + 2.0 * refProp : -1.0 + refProp;
	    if (accuracy > path.accuracy) path.accuracy = accuracy;
	}
	void addRiskAndConfidence(f64 &risk, f64 &confidence, const HypothesisSpace::Arc &hypArc, const Closure::Path &path, const Probability pathPosterior) const {
	    risk -= pathPosterior * path.accuracy;
	    confidence += pathPosterior * path.accuracy;
	}
	std::string name() const { return "local-acc(a la Povey)"; }
    };

    class LocalCostDecoder::LocalAlignmentRiskBuilder::CostScorer :
	public LocalCostDecoder::LocalAlignmentRiskBuilder::OverlapScorer {
    public:
	void addRiskAndConfidence(f64 &risk, f64 &confidence, const HypothesisSpace::Arc &hypArc, const Closure::Path &path, const Probability pathPosterior) const {
	    risk += pathPosterior * (path.nToken - path.nCorrect);
	    if (hypArc.label != Fsa::Epsilon)
		confidence += pathPosterior * path.accuracy;
	    else
		if (path.nToken < 1.0)
		    confidence += pathPosterior * (1.0 - path.nToken);
	}
    };

    class LocalCostDecoder::LocalAlignmentRiskBuilder::ContinousCostScorer1 :
	public LocalCostDecoder::LocalAlignmentRiskBuilder::CostScorer {
	typedef CostScorer Precursor;
    private:
	const f64 alpha_;
    public:
	ContinousCostScorer1(f64 alpha = 1.0) : Precursor(), alpha_(alpha) {}

	void initialize(Closure::Path &path, const HypothesisSpace::Arc &hypArc, const f64 hypProp, const SummationSpace::Arc &refArc, const f64 refProp) const {
	    // insertion
	    if (hypArc.label != Fsa::Epsilon)
		path.nCorrect = -1.0;
	    expand(path, hypArc, hypProp, refArc, refProp);
	}

	void expand(Closure::Path &path, const HypothesisSpace::Arc &hypArc, const f64 hypProp, const SummationSpace::Arc &refArc, const f64 refProp) const {
	    if (refArc.label != Fsa::Epsilon) {
		// substitution + deletion
		path.nToken += refProp;
		if (hypArc.label != Fsa::Epsilon) {
		    // correct substitution + insertion (hyp-insertion=(hypProp-1.0) + alpha * ref-insertion=(refProp-1.0))
		    const f64 nCorrect = (refArc.label == hypArc.label) ?
			refProp + (hypProp - 1.0) + alpha_ * (refProp - 1.0):
			(hypProp - 1.0) + alpha_ * (refProp - 1.0);
		    if (nCorrect > path.nCorrect) {
			path.nCorrect = nCorrect;
			path.accuracy = (refArc.label == hypArc.label) ? hypProp : 0.0;
		    }
		}
	    }
	}

	std::string name() const { return Core::form("continous-cost1(alpha=%.2f)", alpha_); }
    };

    class LocalCostDecoder::LocalAlignmentRiskBuilder::ContinousCostScorer2 :
	public LocalCostDecoder::LocalAlignmentRiskBuilder::CostScorer {
	typedef CostScorer Precursor;
    private:
	const f64 alpha_;
    public:
	ContinousCostScorer2(f64 alpha = 0.5) : Precursor(), alpha_(alpha) {}

	void initialize(Closure::Path &path, const HypothesisSpace::Arc &hypArc, const f64 hypProp, const SummationSpace::Arc &refArc, const f64 refProp) const {
	    // insertion
	    if (hypArc.label != Fsa::Epsilon)
		path.nCorrect = -1.0;
	    if (refArc.label != Fsa::Epsilon) {
		// substitution + deletion
		path.nToken = refProp;
		if (hypArc.label != Fsa::Epsilon) {
		    // correct substitution + insertion (hyp-insertion=(hypProp-1.0))
		    if (refProp > alpha_) {
			const f64 nCorrect = (refArc.label == hypArc.label) ?
			    refProp + (hypProp - 1.0) :
			    (hypProp - 1.0);
			if (nCorrect > path.nCorrect) {
			    path.nCorrect = nCorrect;
			    path.accuracy = (refArc.label == hypArc.label) ? hypProp : 0.0;
			}
		    }
		}
	    }
	}

	void expand(Closure::Path &path, const HypothesisSpace::Arc &hypArc, const f64 hypProp, const SummationSpace::Arc &refArc, const f64 refProp) const {
	    if (refArc.label != Fsa::Epsilon) {
		// substitution + deletion
		path.nToken += refProp;
		if (hypArc.label != Fsa::Epsilon) {
		    // correct substitution + insertion (hyp-insertion=(hypProp-1.0))
		    if (refProp >= alpha_) {
			const f64 nCorrect = (refArc.label == hypArc.label) ?
			    refProp + (hypProp - 1.0) :
			    (hypProp - 1.0);
			if (nCorrect > path.nCorrect) {
			    path.nCorrect = nCorrect;
			    path.accuracy = (refArc.label == hypArc.label) ? hypProp : 0.0;
			}
		    }
		}
	    }
	}

	std::string name() const { return Core::form("continous-cost2(alpha=%.2f)", alpha_); }
    };

    class LocalCostDecoder::LocalAlignmentRiskBuilder::DiscreteCostScorer :
	public LocalCostDecoder::LocalAlignmentRiskBuilder::CostScorer {
	typedef CostScorer Precursor;
    private:
	const f64 alpha_;
    public:
	DiscreteCostScorer(f64 alpha = 0.5) : Precursor(), alpha_(alpha) {}

	void initialize(Closure::Path &path, const HypothesisSpace::Arc &hypArc, const f64 hypProp, const SummationSpace::Arc &refArc, const f64 refProp) const {
	    // insertion
	    if (hypArc.label != Fsa::Epsilon)
		path.nCorrect = -1.0;
	    if (refArc.label != Fsa::Epsilon) {
		if (refProp > alpha_) {
		    // substitution + deletion
		    path.nToken = 1.0;
		    // match or substitution
		    if (refArc.label == hypArc.label) {
			path.nCorrect = 1.0;
			path.accuracy = 1.0;
		    } else
			path.nCorrect = 0.0;
		}
	    }
	}

	void expand(Closure::Path &path, const HypothesisSpace::Arc &hypArc, const f64 hypProp, const SummationSpace::Arc &refArc, const f64 refProp) const {
	    if (refArc.label != Fsa::Epsilon) {
		if (refProp >= alpha_) {
		    // substitution + deletion
		    path.nToken += 1.0;
		    // correct substitution
		    if (refArc.label == hypArc.label) {
			path.nCorrect = 1.0;
			path.accuracy = 1.0;
		    } else
			path.nCorrect = 0.0;
		}
	    }
	}

	std::string name() const { return Core::form("discrete-cost(alpha=%.2f)", alpha_); }
    };

    void LocalCostDecoder::LocalAlignmentRiskBuilder::build(ConstLatticeRef hypSpaceL, ConstLatticeRef sumSpaceL, ConstFwdBwdRef sumSpaceFb) {
	const OverlapScorer &scorer = *scorer_;
	Core::ProgressIndicator pi("initialize");
	pi.start();
	// initialize
	ConstStateMapRef sortedHypSpaceStates = sortChronologically(hypSpaceL);
	initializeHypothesisSpace(hypSpaceL, sortedHypSpaceStates);
	ConstStateMapRef sortedSumSpaceStates = sortChronologically(sumSpaceL);
	initializeSummationSpace(sumSpaceL, sortedSumSpaceStates, sumSpaceFb);
	// calculate costs and confidences
	SummationSpace::ArcList::const_iterator beginRefArc = sumSpace.arcs.begin(), endRefArc = sumSpace.arcs.end();
	Closure::PathList S;
	const HypothesisSpace::Arc *lastHypArcPtr = 0;
	// traverse hypothesis arcs
	pi.setTask(Core::form("#arcs=%zu", hypSpace.arcs.size()));
	pi.setTotal(hypSpace.arcs.size());
	for (HypothesisSpace::ArcList::iterator itHypArc = hypSpace.arcs.begin(), endHypArc = hypSpace.arcs.end(); itHypArc != endHypArc; ++itHypArc) {
	    HypothesisSpace::Arc &hypArc = *itHypArc;
	    if (hypArc.beginTime == hypArc.endTime) {
		// null-length score
		hypArc.risk = 0.0;
		hypArc.confidence = 1.0;
	    } else if (lastHypArcPtr && equalRisk(*lastHypArcPtr, hypArc)) {
		// cached score
		hypArc.risk = lastHypArcPtr->risk;
		hypArc.confidence = lastHypArcPtr->confidence;
	    } else {
		// compute score
		f64 risk = 0.0, confidence = 0.0;
		// traverse overlapping reference arcs
		for (; beginRefArc->endTime <= hypArc.beginTime; ++beginRefArc) verify(beginRefArc != endRefArc);
		for (SummationSpace::ArcList::const_iterator itRefArc = beginRefArc; (itRefArc != endRefArc) && (itRefArc->beginTime <= hypArc.beginTime); ++itRefArc) {
		    const SummationSpace::Arc &refArc = *itRefArc;
		    if (hypArc.beginTime < refArc.endTime) {
			// initialize
			S.push_back(Closure::Path());
			Closure::Path &path = S.back();
			path.target = refArc.to;
			path.fbScore = sumSpace.states[refArc.from].fwdScore + refArc.score - refArc.normScore;
			const f64 overlapDuration = f64(overlap(hypArc, refArc));
			const f64 hypProp = overlapDuration / hypArc.duration;
			const f64 refProp = overlapDuration / refArc.duration;
			// risk
			scorer.initialize(path, hypArc, hypProp, refArc, refProp);
			// traverse overlapping reference closure
			while (!S.empty()) {
			    Closure::Path lastPath = S.back(); S.pop_back();
			    const SummationSpace::State &state = sumSpace.states[lastPath.target];
			    if (hypArc.endTime <= state.time) {
				lastPath.fbScore += state.bwdScore;
				Probability pathPosterior = ::exp(-lastPath.fbScore);
				scorer.addRiskAndConfidence(risk, confidence, hypArc, lastPath, pathPosterior);
			    } else {
				const u32 arcOffset = sumSpace.states[lastPath.target].beginArc;
				for (SummationSpace::ArcList::const_iterator itRefArcCtd = sumSpace.arcs.begin() + arcOffset, endRefArcCtd = sumSpace.arcs.begin() + arcOffset + state.nArcs;
				     itRefArcCtd != endRefArcCtd; ++itRefArcCtd) {
				    const SummationSpace::Arc &refArc = *itRefArcCtd;
				    verify((hypArc.beginTime < refArc.beginTime) && (refArc.beginTime < hypArc.endTime));
				    S.push_back(lastPath);
				    Closure::Path &path = S.back();
				    path.target = refArc.to;
				    path.fbScore += refArc.score;
				    if (refArc.beginTime < refArc.endTime) {
					const f64 overlapDuration = f64(overlap(hypArc, refArc));
					const f64 hypProp = overlapDuration / hypArc.duration;
					const f64 refProp = overlapDuration / refArc.duration;
					scorer.expand(path, hypArc, hypProp, refArc, refProp);
				    }
				}
			    }
			}
		    }
		}
		hypArc.risk = risk;
		hypArc.confidence = (confidence < 0.0) ? 0.0 : ((confidence < 1.0) ? confidence : 1.0);
	    }
	    pi.notify(u32(itHypArc - hypSpace.arcs.begin()));
	    lastHypArcPtr = &hypArc;
	}
	// finalize
	finalizeHypothesisSpace(hypSpaceL, sortedHypSpaceStates);
	pi.finish(false);
    }
    // -------------------------------------------------------------------------


    // -------------------------------------------------------------------------
    LocalCostDecoder::LocalCostDecoder(LocalCostDecoder::RiskBuilderRef riskBuilder, Score wordPenalty) :
	riskBuilder_(riskBuilder), wordPenalty_(wordPenalty) {
	verify(riskBuilder_);
	riskBuilder_->set(&offsets_, &risks_, &confidences_);
    }

    std::string LocalCostDecoder::name() const {
	return riskBuilder_->name();
    }

    void LocalCostDecoder::build(ConstLatticeRef hypSpaceL, ConstLatticeRef sumSpaceL, ConstFwdBwdRef sumSpaceFb) {
	verify(hypSpaceL && hypSpaceL->getBoundaries()->valid() && sumSpaceL && sumSpaceL->getBoundaries()->valid() && sumSpaceFb);
	l_ = hypSpaceL;
	riskBuilder_->build(hypSpaceL, sumSpaceL, sumSpaceFb);
    }

    namespace {
	struct TraceElement {
	    Score cost;
	    Fsa::StateId sid;
	    Fsa::StateId aid;
	    TraceElement() :
		cost(Core::Type<Score>::max),
		// hypCost(Core::Type<Score>::max), refCost(Core::Type<Score>::max),
		sid(Fsa::InvalidStateId), aid(Fsa::InvalidStateId) {}
	};
	typedef std::vector<TraceElement> Traceback;
    } // namespace
    std::pair<ConstLatticeRef, Score> LocalCostDecoder::best(ScoreId confidenceId) const {
	verify(l_);
	ConstStateMapRef topologicalSort = sortTopologically(l_);
	Traceback traceback(topologicalSort->maxSid + 1);
	const Fsa::StateId initialSid = topologicalSort->front();
	traceback[initialSid].cost = 0.0;
	//traceback[initialSid].hypCost = traceback[initialSid].refCost = 0.0;
	TraceElement bestTrace;
	// sssd
	for (StateMap::const_iterator itSid = topologicalSort->begin(), endSid = topologicalSort->end(); itSid != endSid; ++itSid) {
	    const Fsa::StateId sid = *itSid;
	    const Flf::State &s = *l_->getState(sid);
	    const TraceElement &currentTrace = traceback[sid];
	    if (s.isFinal()) {
		if (currentTrace.cost < bestTrace.cost) {
		    bestTrace.cost = currentTrace.cost;
		    // bestTrace.hypCost = currentTrace.hypCost;
		    // bestTrace.refCost = currentTrace.refCost;
		    bestTrace.sid = sid;
		}
	    }
	    ScoreList::const_iterator itRisk = beginRisk(sid);

	    // ScoreList::const_iterator itHypCost = hypCosts_.begin() + offsets_[sid], itRefCost = refCosts_.begin() + offsets_[sid];

	    Fsa::StateId aid = 0;
	    for (Flf::State::const_iterator a = s.begin(); a != s.end(); ++a, ++aid, ++itRisk) { // , ++itHypCost, ++itRefCost
		const Score cost = currentTrace.cost + *itRisk + ((a->input() == Fsa::Epsilon) ? 0.0 : wordPenalty_);
		TraceElement &trace = traceback[a->target()];
		if (cost < trace.cost) {
		    trace.cost = cost;
		    // trace.hypCost = currentTrace.hypCost + *itHypCost;
		    // trace.refCost = currentTrace.refCost + *itRefCost;
		    trace.sid = sid;
		    trace.aid = aid;
		}
	    }
	}
	verify(bestTrace.sid != Fsa::InvalidStateId);

	// dbg("cost=" << bestTrace.cost);
	// dbg("cost=" << bestTrace.cost << ", hyp-cost=" << bestTrace.hypCost << ", ref-cost=" << bestTrace.refCost);

	// trace
	StaticLattice *s = new StaticLattice(l_->type());
	s->setProperties(l_->knownProperties() | Fsa::PropertyLinear, l_->properties() | Fsa::PropertyLinear);
	s->setInputAlphabet(l_->getInputAlphabet());
	if (l_->type() != Fsa::TypeAcceptor)
	    s->setOutputAlphabet(l_->getOutputAlphabet());
	s->setSemiring(l_->semiring());
	s->setBoundaries(l_->getBoundaries());
	s->setDescription(Core::form("best-local-cn(%s)", l_->describe().c_str()));
	const Semiring &semiring = *l_->semiring();
	Fsa::StateId bestSid = bestTrace.sid;
	ConstStateRef sr = l_->getState(bestSid);
	Flf::State *sp = new Flf::State(sr->id(), sr->tags(), sr->weight()); s->setState(sp);
	verify(sp->isFinal());
	if (confidenceId != Semiring::InvalidId) {
	    ScoresRef scores = sp->weight_ = semiring.clone(sp->weight_);
	    scores->set(confidenceId, 0.0);
	}
	while (bestSid != initialSid) {
	    const TraceElement &trace = traceback[bestSid];
	    sr = l_->getState(trace.sid);
	    sp = new Flf::State(sr->id(), sr->tags(), sr->weight()); s->setState(sp);
	    Flf::Arc &bestArc = *sp->newArc() = *(sr->begin() + trace.aid);


	    // dbg("best: " << s->getInputAlphabet()->symbol(bestArc.input()) << ", acc=" << accuracy(trace.sid, trace.aid));

	    if (confidenceId != Semiring::InvalidId) {
		ScoresRef scores = bestArc.weight_ = semiring.clone(bestArc.weight_);
		scores->set(confidenceId, confidence(trace.sid, trace.aid));
	    }
	    bestSid = trace.sid;
	}
	verify(sp->id() == l_->initialStateId());
	s->setInitialStateId(sp->id());
	return std::make_pair(ConstLatticeRef(s), bestTrace.cost);
    }

    ConstLocalCostDecoderRef LocalCostDecoder::create(
	ConstLatticeRef hypSpaceL, ConstLatticeRef sumSpaceL, ConstFwdBwdRef sumSpaceFb,
	LocalCostDecoder::RiskBuilderRef riskBuilder,
	Score wordPenalty) {
	if (!hypSpaceL)
	    return ConstLocalCostDecoderRef();
	verify(sumSpaceL);
	if (!riskBuilder)
	    riskBuilder = RiskBuilderRef(new LocalOverlapRiskBuilder);
	LocalCostDecoder *scorer = new LocalCostDecoder(riskBuilder, wordPenalty);
	scorer->build(hypSpaceL, sumSpaceL, sumSpaceFb);
	return ConstLocalCostDecoderRef(scorer);
    }
    // -------------------------------------------------------------------------


    /*
    // -------------------------------------------------------------------------
    std::pair<ConstLatticeRef, Score> decodeByLocalRisk(ConstLatticeRef hypSpaceL, ConstLatticeRef sumSpaceL, ConstFwdBwdRef sumSpaceFb, Score wordPenalty, ScoreId confidenceId) {
	if (!hypSpaceL)
	    return std::make_pair(ConstLatticeRef(), Semiring::Invalid);
	verify(sumSpaceL);
	ConstLocalCostDecoderRef scorer =
	    LocalCostDecoder::create(hypSpaceL, sumSpaceL, sumSpaceFb, LocalCostDecoder::RiskBuilderRef(), wordPenalty);
	return scorer->best(confidenceId);
    }
    // -------------------------------------------------------------------------
    */


    // -------------------------------------------------------------------------
    /**
     * [*]
     * type                                  = overlap | local-alignment
     * overlap.alpha                         = 0.5 # [0.0,1.0]
     * local-alignment.scorer                = approximated-accuracy | continous-cost1 | continous-cost2 | discrete-cost
     * local-alignment.continous-cost1.alpha = 1.0 # [0.0,1.0]
     * local-alignment.continous-cost2.alpha = 0.5 # [0.0,0.5]
     * local-alignment.discrete-cost.alpha   = 0.5 # [0.0,0.5]
     * word-penalty                          = 0.0
     * search-space                          = union|mesh
     **/
    class LocalCostDecoderBuilder;
    typedef Core::Ref<const LocalCostDecoderBuilder> ConstLocalCostDecoderBuilderRef;

    class LocalCostDecoderBuilder : public Core::Component, public Core::ReferenceCounted {
    public:

	static const Core::ParameterFloat paramWordPenalty;
	static const Core::ParameterFloat paramAlpha;
	typedef enum {
	    RiskBuilderTypeOverlap,
	    RiskBuilderTypeLocalAlignment
	} RiskBuilderType;
	static const Core::Choice choiceRiskBuilderType;
	static const Core::ParameterChoice paramRiskBuilderType;
	typedef enum {
	    OverlapScorerTypePathSymetricFrameError,
	    OverlapScorerTypeArcSymetricFrameError
	} OverlapScorerType;
	static const Core::Choice choiceOverlapScorerType;
	static const Core::ParameterChoice paramOverlapScorerType;
	typedef enum {
	    LocalAlignmentScorerTypeLocalAccuracy,
	    LocalAlignmentScorerTypeContinousCost1,
	    LocalAlignmentScorerTypeContinousCost2,
	    LocalAlignmentScorerTypeDiscreteCost
	} LocalAlignmentScorerType;
	static const Core::Choice choiceLocalAlignmentScorerType;
	static const Core::ParameterChoice paramLocalAlignmentScorerType;
    private:
	LocalCostDecoder::RiskBuilderRef riskBuilder_;
	Score wordPenalty_;
    public:
	LocalCostDecoderBuilder(const Core::Configuration &config);

	LocalCostDecoder::RiskBuilderRef riskBuilder() const {
	    return riskBuilder_;
	}

	Score wordPenalty() const {
	    return wordPenalty_;
	}

	ConstLocalCostDecoderRef build() const {
	    return ConstLocalCostDecoderRef(new LocalCostDecoder(riskBuilder_, wordPenalty_));
	}

	ConstLocalCostDecoderRef build(ConstLatticeRef hypSpaceL, ConstLatticeRef sumSpaceL, ConstFwdBwdRef sumSpaceFb) const {
	    return LocalCostDecoder::create(hypSpaceL, sumSpaceL, sumSpaceFb, riskBuilder_, wordPenalty_);
	}

	static ConstLocalCostDecoderBuilderRef create(const Core::Configuration &config) {
	    return ConstLocalCostDecoderBuilderRef(new LocalCostDecoderBuilder(config));
	}
    };

    const Core::ParameterFloat LocalCostDecoderBuilder::paramWordPenalty(
	"word-penalty",
	"word penalty",
	0.0);

    const Core::ParameterFloat LocalCostDecoderBuilder::paramAlpha(
	"alpha",
	"alpha",
	0.0);

    const Core::Choice LocalCostDecoderBuilder::choiceRiskBuilderType(
	"overlap",         LocalCostDecoderBuilder::RiskBuilderTypeOverlap,
	"local-alignment", LocalCostDecoderBuilder::RiskBuilderTypeLocalAlignment,
	Core::Choice::endMark());
    const Core::ParameterChoice LocalCostDecoderBuilder::paramRiskBuilderType(
	"risk-builder",
	&LocalCostDecoderBuilder::choiceRiskBuilderType,
	"risk builder",
	LocalCostDecoderBuilder::RiskBuilderTypeOverlap);

    const Core::Choice LocalCostDecoderBuilder::choiceOverlapScorerType(
	"path-symetric", LocalCostDecoderBuilder::OverlapScorerTypePathSymetricFrameError,
	"arc-symetric",  LocalCostDecoderBuilder::OverlapScorerTypeArcSymetricFrameError,
	Core::Choice::endMark());
    const Core::ParameterChoice LocalCostDecoderBuilder::paramOverlapScorerType(
	"scorer",
	&LocalCostDecoderBuilder::choiceOverlapScorerType,
	"overlap scorer",
	LocalCostDecoderBuilder::OverlapScorerTypePathSymetricFrameError);

    const Core::Choice LocalCostDecoderBuilder::choiceLocalAlignmentScorerType(
	"approximated-accuracy", LocalCostDecoderBuilder::LocalAlignmentScorerTypeLocalAccuracy,
	"continous-cost1",       LocalCostDecoderBuilder::LocalAlignmentScorerTypeContinousCost1,
	"continous-cost2",       LocalCostDecoderBuilder::LocalAlignmentScorerTypeContinousCost2,
	"discrete-cost",         LocalCostDecoderBuilder::LocalAlignmentScorerTypeDiscreteCost,
	Core::Choice::endMark());
    const Core::ParameterChoice LocalCostDecoderBuilder::paramLocalAlignmentScorerType(
	"scorer",
	&LocalCostDecoderBuilder::choiceLocalAlignmentScorerType,
	"local alignment scorer",
	LocalCostDecoderBuilder::LocalAlignmentScorerTypeContinousCost2);

    LocalCostDecoderBuilder::LocalCostDecoderBuilder(const Core::Configuration &config) :
	Core::Component(config) {
	Core::Component::Message msg = log();
	Core::Choice::Value riskBuilderType = paramRiskBuilderType(config);
	if (riskBuilderType ==  Core::Choice::IllegalValue)
	    criticalError("Unknown risk builder type");
	const Core::Configuration riskBuilderConfig(config, choiceRiskBuilderType[riskBuilderType]);
	switch (RiskBuilderType(riskBuilderType)) {
	case RiskBuilderTypeOverlap: {
	    Core::Choice::Value scorerType = paramOverlapScorerType(riskBuilderConfig);
	    if (scorerType ==  Core::Choice::IllegalValue)
		criticalError("Unknown local alignment scorer");
	    const Core::Configuration scorerConfig(riskBuilderConfig, choiceOverlapScorerType[scorerType]);
	    switch (OverlapScorerType(scorerType)) {
	    case OverlapScorerTypePathSymetricFrameError:
		riskBuilder_ = LocalCostDecoder::RiskBuilderRef(
		    new LocalCostDecoder::LocalOverlapRiskBuilder(
			paramAlpha(scorerConfig, 0.5)));
		break;
	    case OverlapScorerTypeArcSymetricFrameError:
		riskBuilder_ = LocalCostDecoder::RiskBuilderRef(
		    new LocalCostDecoder::ArcSymetricFrameErrorRiskBuilder);
		break;
	    default:
		defect();
	    }
	} break;
	case RiskBuilderTypeLocalAlignment: {
	    LocalCostDecoder::LocalAlignmentRiskBuilder::ConstOverlapScorerRef scorer;
	    Core::Choice::Value scorerType = paramLocalAlignmentScorerType(riskBuilderConfig);
	    if (scorerType ==  Core::Choice::IllegalValue)
		criticalError("Unknown local alignment scorer");
	    const Core::Configuration scorerConfig(riskBuilderConfig, choiceLocalAlignmentScorerType[scorerType]);
	    switch (LocalAlignmentScorerType(scorerType)) {
	    case LocalAlignmentScorerTypeLocalAccuracy:
		scorer = LocalCostDecoder::LocalAlignmentRiskBuilder::ConstOverlapScorerRef(
		    new LocalCostDecoder::LocalAlignmentRiskBuilder::LocalAccuracyScorer);
		break;
	    case LocalAlignmentScorerTypeContinousCost1:
		scorer = LocalCostDecoder::LocalAlignmentRiskBuilder::ConstOverlapScorerRef(
		    new LocalCostDecoder::LocalAlignmentRiskBuilder::ContinousCostScorer1(
			paramAlpha(scorerConfig, 1.0)));
		break;
	    case LocalAlignmentScorerTypeContinousCost2:
		scorer = LocalCostDecoder::LocalAlignmentRiskBuilder::ConstOverlapScorerRef(
		    new LocalCostDecoder::LocalAlignmentRiskBuilder::ContinousCostScorer2(
			paramAlpha(scorerConfig, 0.5)));
		break;
	    case LocalAlignmentScorerTypeDiscreteCost:
		scorer = LocalCostDecoder::LocalAlignmentRiskBuilder::ConstOverlapScorerRef(
		    new LocalCostDecoder::LocalAlignmentRiskBuilder::DiscreteCostScorer(
			paramAlpha(scorerConfig, 0.5)));
		break;
	    default:
		defect();
	    }
	    riskBuilder_ = LocalCostDecoder::RiskBuilderRef(
		new LocalCostDecoder::LocalAlignmentRiskBuilder(scorer));
	} break;
	default:
	    defect();
	}
	msg << "risk-builder: " << riskBuilder_->name() << "\n";
	wordPenalty_ = paramWordPenalty(config);
	msg << "word-penalty: " << wordPenalty_ << "\n";
    }
    // -------------------------------------------------------------------------


    // -------------------------------------------------------------------------
    class ExtendByLocalRiskLattice : public RescoreLattice {
    private:
	ConstLocalCostDecoderRef scorer_;
	ScoreId scoreId_;
	ScoreId confidenceId_;

    public:
	ExtendByLocalRiskLattice(
	    ConstLatticeRef l, ConstLocalCostDecoderRef scorer,
	    ScoreId scoreId, ScoreId confidenceId, RescoreMode rescoreMode) :
	    RescoreLattice(l, rescoreMode), scorer_(scorer),
	    scoreId_(scoreId), confidenceId_(confidenceId) {
	    verify(scorer_);
	}
	virtual ~ExtendByLocalRiskLattice() {}

	virtual void rescore(State *sp) const {
	    const Score wordPenalty = scorer_->wordPenalty();
	    ScoreList::const_iterator itRisk = scorer_->beginRisk(sp->id());
	    ScoreList::const_iterator itConfidence = scorer_->beginConfidence(sp->id());
	    for (State::const_iterator a = sp->begin(); a != sp->end(); ++a, ++itRisk, ++itConfidence) {
		if (scoreId_ != Semiring::InvalidId)
		    a->weight_->add(scoreId_, *itRisk + wordPenalty);
		if (confidenceId_ != Semiring::InvalidId)
		    a->weight_->add(confidenceId_, *itConfidence);
	    }
	}
	virtual std::string describe() const {
	    std::string name = "extendByLocalRisk(" + fsa_->describe() + ",risk=" + scorer_->name();
	    if (scoreId_ != Semiring::InvalidId)
		name += Core::form("score-id=%zu", scoreId_);
	    if (confidenceId_ != Semiring::InvalidId)
		name += Core::form("confidence-id=%zu", confidenceId_);
	    return name;
	}
    };
    /*
    ConstLatticeRef extendByLocalRisk(
	ConstLatticeRef l, ConstFwdBwdRef fb,
	Score wordPenalty, ScoreId scoreId, ScoreId confidenceId,
	RescoreMode rescoreMode) {
	if (!l)
	    return ConstLatticeRef();
	ConstLocalCostDecoderRef scorer =
	    LocalCostDecoder::create(l, l, fb, LocalCostDecoder::RiskBuilderRef(), wordPenalty);
	return ConstLatticeRef(new ExtendByLocalRiskLattice(l, scorer, scoreId, confidenceId, rescoreMode));
    }
    */
    // -------------------------------------------------------------------------


    // -------------------------------------------------------------------------
    class LocalCostDecoderNode : public Node {
    public:
	typedef enum {
	    SearchSpaceTypeUnion,
	    SearchSpaceTypeMesh
	} SearchSpaceType;
	static const Core::Choice choiceSearchSpaceType;
	static const Core::ParameterString paramSearchSpaceType;
	static const Core::ParameterString paramScoreKey;
	static const Core::ParameterString paramConfidenceKey;
    private:
	u32 n_;
	Key scoreKey_;
	ScoreId scoreId_;
	Key confidenceKey_;
	ScoreId confidenceId_;
	RescoreMode rescoreMode_;
	SearchSpaceType ssType_;
	FwdBwdBuilderRef fbBuilder_;
	ConstLocalCostDecoderBuilderRef riskScorerBuilder_;

	ConstLatticeRef hypSpaceL_;
	ConstLatticeRef sumSpaceL_;
	ConstFwdBwdRef sumSpaceFb_;
	ConstLocalCostDecoderRef scorer_;
	ConstLatticeRef best_;
	ConstLatticeRef rescored_;

	ConstSemiringRef lastSemiring_;

    protected:
	void computeFwdBwd() {
	    if (!sumSpaceFb_) {
		ConstLatticeRefList lats(n_);
		for (u32 i = 0; i < n_; ++i)
		    lats[i] = requestLattice(i);
		std::pair<ConstLatticeRef, ConstFwdBwdRef> fbResult = (n_ == 1) ? fbBuilder_->build(lats.front()) : fbBuilder_->build(lats);
		sumSpaceL_ = fbResult.first; sumSpaceFb_ = fbResult.second;
		switch(ssType_) {
		case SearchSpaceTypeUnion:
		    hypSpaceL_ = sumSpaceL_;
		    break;
		case SearchSpaceTypeMesh:
		    hypSpaceL_ = persistent(mesh(sumSpaceL_, MeshTypeTimeBoundary));
		    break;
		default:
		    defect();
		}
	    }
	}

	void computeScorer() {
	    computeFwdBwd();
	    if (!scorer_ && hypSpaceL_) {
		computeFwdBwd();
		scorer_ = riskScorerBuilder_->build(hypSpaceL_, sumSpaceL_, sumSpaceFb_);
	    }
	}

	void computeIds() {
	    computeFwdBwd();
	    if (hypSpaceL_) {
		ConstSemiringRef semiring = hypSpaceL_->semiring();
		if (!lastSemiring_
		    || (semiring.get() != lastSemiring_.get())
		    || !(*semiring == *lastSemiring_)) {
		    lastSemiring_ = semiring;
		    if (!scoreKey_.empty()) {
			scoreId_ = semiring->id(scoreKey_);
			if (scoreId_ == Semiring::InvalidId)
			    warning("Semiring \"%s\" has no dimension labeled \"%s\".",
				    semiring->name().c_str(), scoreKey_.c_str());
		    }
		    if (!confidenceKey_.empty()) {
			confidenceId_ = semiring->id(confidenceKey_);
			if (confidenceId_ == Semiring::InvalidId)
			    warning("Semiring \"%s\" has no dimension labeled \"%s\".",
				    semiring->name().c_str(), confidenceKey_.c_str());
		    }
		}
	    }
	}

	ConstLatticeRef getBestLattice() {
	    computeScorer();
	    if (!best_ && scorer_) {
		computeIds();
		std::pair<ConstLatticeRef, Score> result = scorer_->best(confidenceId_);
		log("Minimum score for lattice \"%s\" is %f.",
		    hypSpaceL_->describe().c_str(), result.second);
		best_ = result.first;
	    }
	    return best_;
	}

	ConstLatticeRef getRescoredLattice() {
	    computeScorer();
	    if (!rescored_ && scorer_) {
		computeIds();
		rescored_ = ConstLatticeRef(
		    new ExtendByLocalRiskLattice(
			hypSpaceL_, scorer_, scoreId_, confidenceId_, rescoreMode_));
	    }
	    return rescored_;
	}

    public:
	LocalCostDecoderNode(const std::string &name, const Core::Configuration &config) :
	    Node(name, config), n_(0) { scoreId_ = confidenceId_ = Semiring::InvalidId; }
	~LocalCostDecoderNode() {}

	virtual void init(const std::vector<std::string> &arguments) {
	    for (n_ = 0; connected(n_); ++n_);
	    if (n_ == 0)
		criticalError("At least one incoming lattice at port 0 required.");
	    KeyList requiredKeys;
	    ScoreList requiredScales;
	    {
		Core::Component::Message msg = log();
		if (n_ > 1)
		    msg << "Combine " << n_ << " lattices.\n\n";
		scoreKey_ = paramScoreKey(config);
		if (!scoreKey_.empty()) {
		    msg << "score key is \"" << scoreKey_ << "\"\n";
		    requiredKeys.push_back(scoreKey_);
		    requiredScales.push_back(0.0);
		}
		confidenceKey_ = paramConfidenceKey(config);
		if (!confidenceKey_.empty()) {
		    msg << "confidence key is \"" << confidenceKey_ << "\"\n";
		    requiredKeys.push_back(confidenceKey_);
		    requiredScales.push_back(0.0);
		}
		Core::Choice::Value ssChoice = choiceSearchSpaceType[paramSearchSpaceType(config)];
		if (ssChoice == Core::Choice::IllegalValue)
		    criticalError("Unknown search space \"%s\"", paramSearchSpaceType(config).c_str());
		ssType_ = SearchSpaceType(ssChoice);
		msg << "Search space is \"" << choiceSearchSpaceType[ssType_] << "\"\n";
	    }
	    fbBuilder_ = FwdBwdBuilder::create(select("fb"), requiredKeys, requiredScales);
	    riskScorerBuilder_ = LocalCostDecoderBuilder::create(config);
	    rescoreMode_ = getRescoreMode(config);
	}

	virtual void finalize() {}

	virtual ConstLatticeRef sendLattice(Port to) {
	    switch (to) {
	    case 0:
		return getBestLattice();
	    case 1:
		return getRescoredLattice();
	    default:
		defect();
		return ConstLatticeRef();
	    }
	}

	virtual void sync() {
	    rescored_.reset();
	    best_.reset();
	    scorer_.reset();
	    sumSpaceFb_.reset();
	    sumSpaceL_.reset();
	    hypSpaceL_.reset();
	}
    };
    const Core::Choice LocalCostDecoderNode::choiceSearchSpaceType(
	"union", SearchSpaceTypeUnion,
	"mesh",  SearchSpaceTypeMesh,
	Core::Choice::endMark());
    const Core::ParameterString LocalCostDecoderNode::paramSearchSpaceType(
	"search-space",
	"search spaces: union or mesh",
	"mesh");
    const Core::ParameterString LocalCostDecoderNode::paramScoreKey(
	"score-key",
	"store arc risk",
	"");
    const Core::ParameterString LocalCostDecoderNode::paramConfidenceKey(
	"confidence-key",
	"store arc confidence",
	"");

    NodeRef createLocalCostDecoderNode(const std::string &name, const Core::Configuration &config) {
	return NodeRef(new LocalCostDecoderNode(name, config));
    }
    // -------------------------------------------------------------------------

} // namespace

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

#include "FlfCore/Basic.hh"
#include "FlfCore/Traverse.hh"
#include "FlfCore/Utility.hh"
#include "ConfusionNetwork.hh"
#include "FwdBwd.hh"
#include "Lexicon.hh"
#include "Map.hh"
#include "PivotArcConfusionNetworkBuilder.hh"


namespace Flf {
    class PivotArcCn;
    typedef Core::Ref<const PivotArcCn> ConstPivotArcCnRef;
    class PivotArcCnBuilder;
    typedef Core::Ref<const PivotArcCnBuilder> ConstPivotArcCnBuilderRef;


    class PivotArcCn : public Core::ReferenceCounted {
	friend class PivotArcCnBuilder;
    public:
	struct Arc;
	typedef std::vector<Arc> ArcList;
	typedef std::vector<Arc*> ArcPtrList;
	struct Arc {
	    Fsa::StateId sid;
	    u32 aid;
	    Fsa::LabelId label;
	    bool isNonWord;
	    Time start, end;
	    Score posteriorScore;
	    // nearest slot
	    Score slotDist;
	    u32 slotId;
	    // state: 0 pending, 1 pivot, 2 member
	    u8 state;
	    Arc() : slotDist(Core::Type<Score>::max), slotId(Core::Type<u32>::max), state(0) {}
	    struct SortByTime {
		bool operator() (const Arc *a1, const Arc *a2) const
		    { return a1->start < a2->start; }
	    };
	    struct SortByPosteriorScore {
		bool operator() (const Arc *a1, const Arc *a2) const
		    { return a1->posteriorScore < a2->posteriorScore; }
	    };
	    struct SortByDistance {
		bool operator() (const Arc *a1, const Arc *a2) const
		    { return a1->slotDist < a2->slotDist; }
	    };
	    struct SortByLabel {
		bool operator() (const Arc *a1, const Arc *a2) const
		    { return a1->label < a2->label; }
	    };
	};

	struct Slot;
	typedef Slot* SlotPtr;
	typedef std::vector<Slot> SlotList;
	typedef std::vector<SlotPtr> SlotPtrList;
	struct Slot {
	    Time start, end;
	    ArcPtrList arcs;
	};

    private:
	ConstLatticeRef l_;
	ArcPtrList arcs_;
	SlotPtrList slots_;

    private:
	PivotArcCn(ConstLatticeRef l) : l_(l) {}

    public:
	~PivotArcCn() {
	    for (SlotPtrList::iterator itSlotPtr = slots_.begin(); itSlotPtr != slots_.end(); ++itSlotPtr)
		delete *itSlotPtr;
	    slots_.clear();
	    for (ArcPtrList::iterator itArcPtr = arcs_.begin(); itArcPtr != arcs_.end(); ++itArcPtr)
		delete *itArcPtr;
	    arcs_.clear();
	}

	/*
	  The lattice
	*/
	ConstLatticeRef getLattice() const {
	    return l_;
	}

	/*
	  Create confusion network or sausage
	*/
    private:
	/*
	  Mapping between CN and lattice and vice versa
	*/
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
	ConfusionNetwork::ConstMapPropertiesRef getMapping(bool reduced) const {
	    ConfusionNetwork::MapProperties *mapProps = new ConfusionNetwork::MapProperties;

	    Core::Vector<Fsa::StateId> &stateIndex = mapProps->stateIndex;
	    ConfusionNetwork::MapProperties::Map &lat2cn = mapProps->lat2cn;
	    {
		StateIndexBuilder stateIndexBuilder(l_, stateIndex);
		stateIndexBuilder.build();
		lat2cn.resize(stateIndex.back(), ConfusionNetwork::MapProperties::Mapping(Fsa::InvalidStateId, 0));
	    }
	    if (reduced) {
		// lattice arc -> slot
		for (Fsa::StateId sid = 0; sid < slots_.size(); ++sid) {
		    Slot &pcnSlot = *slots_[sid];
		    for (ArcPtrList::const_iterator itArcPtr = pcnSlot.arcs.begin(), endArcPtr = pcnSlot.arcs.end();
			 itArcPtr != endArcPtr; ++itArcPtr) {
			const Arc &pcnArc = **itArcPtr;
			verify_(stateIndex[pcnArc.sid] + pcnArc.aid < lat2cn.size());
			lat2cn[stateIndex[pcnArc.sid] + pcnArc.aid] = ConfusionNetwork::MapProperties::Mapping(sid, Fsa::InvalidStateId);
		    }
		}
	    } else {
		// lattice arc <-> slot arc
		Core::Vector<Fsa::StateId> &slotIndex = mapProps->slotIndex;
		ConfusionNetwork::MapProperties::Map &cn2lat = mapProps->cn2lat;
		{
		    slotIndex.resize(slots_.size() + 1);
		    u32 nCnArcs = 0;
		    for (Fsa::StateId sid = 0; sid < slots_.size(); ++sid) {
			const Slot &pcnSlot = *slots_[sid];
			slotIndex[sid] = nCnArcs;
			nCnArcs += pcnSlot.arcs.size();
		    }
		    slotIndex.back() = nCnArcs;
		    cn2lat.resize(slotIndex.back(), ConfusionNetwork::MapProperties::Mapping(Fsa::InvalidStateId, 0));
		}
		for (Fsa::StateId sid = 0; sid < slots_.size(); ++sid) {
		    Slot &pcnSlot = *slots_[sid];
		    Fsa::StateId aid = 0;
		    for (ArcPtrList::const_iterator itArcPtr = pcnSlot.arcs.begin(), endArcPtr = pcnSlot.arcs.end();
			 itArcPtr != endArcPtr; ++itArcPtr, ++aid) {
			const Arc &pcnArc = **itArcPtr;
			verify_(slotIndex[sid] + aid < cn2lat.size());
			cn2lat[slotIndex[sid] + aid] = ConfusionNetwork::MapProperties::Mapping(pcnArc.sid, pcnArc.aid);
			verify_(stateIndex[pcnArc.sid] + pcnArc.aid < lat2cn.size());
			lat2cn[stateIndex[pcnArc.sid] + pcnArc.aid] = ConfusionNetwork::MapProperties::Mapping(sid, aid);
		    }
		}
	    }
	    return ConfusionNetwork::ConstMapPropertiesRef(mapProps);
	}
    public:
	ConstConfusionNetworkRef getCn(ScoreId posteriorId, bool mapping = false) const {
	    if (slots_.empty())
		return ConstConfusionNetworkRef();
	    ConstSemiringRef semiring = l_->semiring();

	    ConfusionNetwork *cn = new ConfusionNetwork(slots_.size());
	    cn->alphabet = l_->getInputAlphabet();
	    cn->semiring = semiring;
	    for (u32 i = 0; i < slots_.size(); ++i) {
		Slot &pcnSlot = *slots_[i];
		ConfusionNetwork::Slot &slot = (*cn)[i];
		for (ArcPtrList::const_iterator itArcPtr = pcnSlot.arcs.begin(), endArcPtr = pcnSlot.arcs.end(); itArcPtr != endArcPtr; ++itArcPtr) {
		    const Arc &pcnArc = **itArcPtr;
		    const State &state = *l_->getState(pcnArc.sid);
		    const Flf::Arc &arc = *(state.begin() + pcnArc.aid);
		    ScoresRef scores = arc.weight();
		    if (posteriorId != Semiring::InvalidId) {
			scores = semiring->clone(scores);
			scores->set(posteriorId, ::exp(-pcnArc.posteriorScore));
		    }
		    slot.push_back(ConfusionNetwork::Arc(
				       arc.input(), scores,
				       pcnArc.start, pcnArc.end - pcnArc.start));
		}
	    }
	    if (mapping)
		cn->mapProperties = getMapping(mapping);
	    return ConstConfusionNetworkRef(cn);
	}

	/*
	  Create confusion network, where per slot no label occurs twice and scores sum up to one,
	  and create lattice storing best result with time boundaries, confidence scores, and no two subsequent epsilon arcs.
	*/
    private:
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
    public:
	std::pair<ConstConfusionNetworkRef, ConstLatticeRef> getNormalizedCn(ScoreId confidenceId, bool mapping = false) const {
	    if (slots_.empty())
		return std::make_pair(ConstConfusionNetworkRef(), ConstLatticeRef());
	    ConstSemiringRef semiring = l_->semiring();

	    // Posterior CN
	    ConfusionNetwork *cn = new ConfusionNetwork(slots_.size());
	    cn->alphabet = l_->getInputAlphabet();
	    cn->semiring = semiring;
	    cn->normalizedProperties = ConfusionNetwork::ConstNormalizedPropertiesRef(
		new ConfusionNetwork::NormalizedProperties(confidenceId));
	    // best lattice
	    StaticBoundaries *b = new StaticBoundaries;
	    StaticLattice *s = new StaticLattice;
	    s->setDescription("decode-cn(" + l_->describe() + ",pivot-arc-cluster)");
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
		*colStart = createCollector(Fsa::SemiringTypeLog),
		*colEnd = createCollector(Fsa::SemiringTypeLog);
	    for (u32 i = 0; i < slots_.size(); ++i) {
		// distribute probabilities over words
		ConfusionNetwork::Slot &slot = (*cn)[i];
		ArcPtrList &arcs = slots_[i]->arcs;
		std::sort(arcs.begin(), arcs.end(), Arc::SortByLabel());
		const Arc *bestPcnArcPtr = 0;
		ScoresRef bestPcnArcScores = ScoresRef();
		f64 bestPosteriorScore = Core::Type<f64>::max;
		const Arc *bestCurrentLabelPcnArcPtr = arcs.front();
		Score epsConfidence = 0.0;
		for (ArcPtrList::const_iterator itArcPtr = arcs.begin(), endArcPtr = arcs.end(); itArcPtr != endArcPtr; ++itArcPtr) {
		    const Arc *pcnArcPtr = *itArcPtr;
		    if (pcnArcPtr->label != bestCurrentLabelPcnArcPtr->label) {
			f64 posteriorScore = col->get();
			col->reset();
			Score confidence = ::exp(-posteriorScore);
			if (bestCurrentLabelPcnArcPtr->label == Fsa::Epsilon)
			    epsConfidence = confidence;
			Score start = ::exp(posteriorScore - colStart->get()), end = ::exp(posteriorScore - colEnd->get());
			colStart->reset(); colEnd->reset();
			const Flf::State &state = *l_->getState(bestCurrentLabelPcnArcPtr->sid);
			const Flf::Arc &arc = *(state.begin() + bestCurrentLabelPcnArcPtr->aid);
			ScoresRef scores = arc.weight();
			if (confidenceId != Semiring::InvalidId) {
			    scores = semiring->clone(scores);
			    scores->set(confidenceId, confidence);
			}
			slot.push_back(ConfusionNetwork::Arc(
					   bestCurrentLabelPcnArcPtr->label,
					   scores,
					   Time(Core::round(start)), std::max(Time(1), Time(Core::round(end - start)))));
			if (posteriorScore < bestPosteriorScore) {
			    bestPosteriorScore = posteriorScore;
			    bestPcnArcPtr = bestCurrentLabelPcnArcPtr;
			    bestPcnArcScores = scores;
			}
			bestCurrentLabelPcnArcPtr = pcnArcPtr;
			sumCol->feed(posteriorScore);
		    } else if (pcnArcPtr->posteriorScore < bestCurrentLabelPcnArcPtr->posteriorScore) {
			bestCurrentLabelPcnArcPtr = pcnArcPtr;
		    }
		    col->feed(pcnArcPtr->posteriorScore);
		    colStart->feed(pcnArcPtr->posteriorScore - ::log(f64(pcnArcPtr->start)));
		    colEnd->feed(pcnArcPtr->posteriorScore - ::log(f64(pcnArcPtr->end)));
		}
		f64 posteriorScore = col->get();
		col->reset();
		{
		    const Flf::State &state = *l_->getState(bestCurrentLabelPcnArcPtr->sid);
		    const Flf::Arc &arc = *(state.begin() + bestCurrentLabelPcnArcPtr->aid);
		    Score confidence = ::exp(-posteriorScore);
		    if (bestCurrentLabelPcnArcPtr->label == Fsa::Epsilon)
			epsConfidence = confidence;
		    Score start = ::exp(posteriorScore - colStart->get()), end = ::exp(posteriorScore - colEnd->get());
		    colStart->reset(); colEnd->reset();
		    ScoresRef scores = arc.weight();
		    if (confidenceId != Semiring::InvalidId) {
			scores = semiring->clone(scores);
			scores->set(confidenceId, confidence);
		    }
		    slot.push_back(ConfusionNetwork::Arc(
				       bestCurrentLabelPcnArcPtr->label,
				       scores,
				       Time(Core::round(start)), std::max(Time(1), Time(Core::round(end - start)))));
		    if (posteriorScore < bestPosteriorScore) {
			bestPosteriorScore = posteriorScore;
			bestPcnArcPtr = bestCurrentLabelPcnArcPtr;
			bestPcnArcScores = scores;
		    }
		}
		sumCol->feed(posteriorScore);
		verify(bestPcnArcPtr);
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
				"PivotArcCnBuilder: Expected 1.0, got %f", (1.0 - dEpsConfidence));
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
			    bestPcnArcPtr = 0;
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
		if ((bestPcnArcPtr) && (bestPcnArcPtr->label != Fsa::Epsilon)) {
		    if (preferredEndTime < bestPcnArcPtr->start) {
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
			preferredEndTime = bestPcnArcPtr->start;
		    } else
			preferredEndTime = std::max(minEndTime ,(preferredEndTime + bestPcnArcPtr->start) / 2);

		    b->set(sp->id(), Boundary(preferredEndTime));
		    if (confidenceId != Semiring::InvalidId) {
			bestPcnArcScores = semiring->clone(bestPcnArcScores);
			bestPcnArcScores->set(confidenceId, bestConfidence);
		    }
		    sp->newArc(sp->id() + 1, bestPcnArcScores, bestPcnArcPtr->label, bestPcnArcPtr->label);
		    sp = new Flf::State(sp->id() + 1); s->setState(sp);
		    minEndTime = preferredEndTime + 1;
		    preferredEndTime = std::max(minEndTime, bestPcnArcPtr->end);
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
	    delete col; delete sumCol; delete colStart; delete colEnd;
	    if (mapping)
		cn->mapProperties = getMapping(true);
	    return std::make_pair(ConstConfusionNetworkRef(cn), ConstLatticeRef(s));
	}
    };



    class PivotArcCnBuilder : public Core::ReferenceCounted {
    public:
	class Distance;
	typedef Core::Ref<const Distance> ConstDistanceRef;
	class Distance : public Core::ReferenceCounted {
	public:
	    virtual ~Distance() {}
	    virtual std::string describe() const = 0;
	    virtual Score distance(const PivotArcCn::Arc &ref, const PivotArcCn::Arc &hyp) const = 0;
	    virtual Score distance(const PivotArcCn::Slot &slot, const PivotArcCn::Arc &arc) const {
		Score minDist = Core::Type<Score>::max;
		Time start = std::max(arc.start, slot.start), end = std::min(arc.end, slot.end);
		if (start < end) {
		    for (PivotArcCn::ArcPtrList::const_iterator itArcPtr = slot.arcs.begin(), endArcPtr = slot.arcs.end(); itArcPtr != endArcPtr; ++itArcPtr) {
			Score dist = distance(**itArcPtr, arc);
			if (dist < minDist) minDist = dist;
		    }
		}
		return minDist;
	    }
	};

	// min. distance over all arcs in slot
	class WeightedTimeDistance : public Distance {
	public:
	    Score impact;
	    ConstEditDistanceRef editDist;
	public:
	    WeightedTimeDistance(Score impact, ConstEditDistanceRef editDist) : impact(impact), editDist(editDist) {}
	    virtual std::string describe() const {
		return Core::form("weighted time, impact=%f, edit-distance=%s", impact, ((editDist) ? "yes" : "no"));
	    }
	    virtual Score distance(const PivotArcCn::Arc &ref, const PivotArcCn::Arc &hyp) const {
		Time start = std::max(ref.start, hyp.start), end = std::min(ref.end, hyp.end);
		verify(start < end);
		Score score = (impact + ref.posteriorScore * hyp.posteriorScore) / impact;
		if ((ref.isNonWord && hyp.isNonWord) || (!ref.isNonWord && !hyp.isNonWord)) {
		    Time dist =
			((ref.start < hyp.start) ? hyp.start - ref.start : ref.start - hyp.start) +
			((ref.end < hyp.end)     ? hyp.end - ref.end     : ref.end - hyp.end),
			norm =
			(ref.end - ref.start) + (hyp.end - hyp.start);
		    score *= Score(dist) / Score(norm);
		}
		if (ref.label != hyp.label)
		    score *= (editDist) ? (1.0 + editDist->normCost(ref.label, hyp.label)) : 2.0;
		return score;
	    }
	    static ConstDistanceRef create(Score impact = 10.0, ConstEditDistanceRef editDist = ConstEditDistanceRef())
		{ return ConstDistanceRef(new WeightedTimeDistance(impact, editDist)); }
	};

	// distance to pivot arc only
	class WeightedPivotTimeDistance : public WeightedTimeDistance {
	public:
	    WeightedPivotTimeDistance(Score impact, ConstEditDistanceRef editDist) : WeightedTimeDistance(impact, editDist) {}
	    virtual std::string describe() const {
		return Core::form("weighted pivot time, impact=%f, edit-distance=%s", impact, ((editDist) ? "yes" : "no"));
	    }
	    virtual Score distance(const PivotArcCn::Slot &slot, const PivotArcCn::Arc &arc) const {
		Time start = std::max(arc.start, slot.start), end = std::min(arc.end, slot.end);
		return (start < end) ? WeightedTimeDistance::distance(*slot.arcs.front(), arc) : Core::Type<Score>::max;
	    }
	    static ConstDistanceRef create(Score impact = 10.0, ConstEditDistanceRef editDist = ConstEditDistanceRef())
		{ return ConstDistanceRef(new WeightedPivotTimeDistance(impact, editDist)); }
	};


    private:
	ConstDistanceRef distFcn_;
	bool fast_;
	mutable PivotArcCn *pcn_;
	mutable PivotArcCn::ArcPtrList::iterator beginArcPtr_, endArcPtr_, endPivotArcPtr_;

    private:
	/**
	 * Start: For Debugging
	 **/
	void dump(PivotArcCn::SlotPtrList &slots) const {
	    Fsa::ConstAlphabetRef alphabet = pcn_->l_->getInputAlphabet();
	    u32 slotId = 0;
	    for (PivotArcCn::SlotPtrList::const_iterator itSlotPtr = slots.begin(), endSlotPtr = slots.end(); itSlotPtr != endSlotPtr; ++itSlotPtr, ++slotId) {
		const PivotArcCn::Slot &slot = **itSlotPtr;
		std::cout << slotId << ": " << slot.start << "-" << slot.end << std::endl;
		for (PivotArcCn::ArcPtrList::const_iterator itArcPtr = slot.arcs.begin(); itArcPtr != slot.arcs.end(); ++itArcPtr) {
		    const PivotArcCn::Arc &arc = **itArcPtr;
		    std::cout << "  " << alphabet->symbol(arc.label) << ": " << arc.start << "-" << arc.end << ", " << arc.posteriorScore << " ";
		    if (arc.slotDist == Core::Type<Score>::min)
			std::cout << "pivot";
		    else
			std::cout << arc.slotDist;
		    std::cout << std::endl;
		}
	    }
	    std::cout << std::endl;
	}

	bool isConsistent(PivotArcCn::SlotPtrList &slots) const {
	    bool consistent = true;
	    if (slots.size() > 1) {
		Time pStart = 0, pEnd = 0, tStart = slots.front()->start, tEnd = slots.front()->end;
		if (!(tStart < tEnd))
		    { dbg(1); consistent = false; }
		for (PivotArcCn::SlotPtrList::const_iterator itSlotPtr = slots.begin() + 1; (itSlotPtr != slots.end()) && consistent; ++itSlotPtr) {
		    const PivotArcCn::Slot &slot = **itSlotPtr;
		    pStart = tStart; pEnd = tEnd;
		    tStart = slot.start; tEnd = slot.end;
		    if (!(tStart < tEnd))
			{ consistent = false; break; }
		    if (!(pEnd <= tStart))
			{ consistent = false; break; }
		    if (slot.arcs.empty())
			{ consistent = false; break; }
		    for (PivotArcCn::ArcPtrList::const_iterator itArcPtr = slot.arcs.begin(); itArcPtr != slot.arcs.end(); ++itArcPtr) {
			const PivotArcCn::Arc &arc = **itArcPtr;
			if (!((arc.start <= tStart) && (tEnd <= arc.end)))
			    { consistent = false; break; }
		    }
		}
	    }
	    if (!consistent)
		dump(slots);
	    return consistent;
	}

	bool isArcListCleanedUp(u8 threshold) const {
	    PivotArcCn::ArcPtrList::iterator itArcPtr = beginArcPtr_;
	    for (; itArcPtr != endArcPtr_; ++itArcPtr) {
		verify((*itArcPtr)->state < threshold);
		verify(((*itArcPtr)->slotDist == Core::Type<Score>::max) && ((*itArcPtr)->slotId == Core::Type<u32>::max));
	    }
	    for (PivotArcCn::ArcPtrList::iterator endArcPtr = pcn_->arcs_.end(); itArcPtr != endArcPtr; ++itArcPtr) {
		verify((*itArcPtr)->state >= threshold);
		verify((*itArcPtr)->slotDist != Core::Type<Score>::max);
	    }
	    return true;
	}
	/**
	 * End: For Debugging
	 **/


	// move all arcs to the end of the arc vector, where state reaches or exceeds the threshold
	void cleanUpArcs(u8 threshold = 1) const {
	    for (; (endArcPtr_ != beginArcPtr_) && ((*(endArcPtr_ - 1))->state >= threshold); --endArcPtr_);
	    for (PivotArcCn::ArcPtrList::iterator itArcPtr = beginArcPtr_; itArcPtr != endArcPtr_; ++itArcPtr) {
		if ((*itArcPtr)->state >= threshold) {
		    std::swap(*itArcPtr, *(endArcPtr_ - 1));
		    for (; (*(endArcPtr_ - 1))->state >= threshold; --endArcPtr_);
		}
		(*itArcPtr)->state = 0;
		(*itArcPtr)->slotDist = Core::Type<Score>::max;
		(*itArcPtr)->slotId = Core::Type<u32>::max;
	    }
	    verify(isArcListCleanedUp(threshold));
	}

	void cleanUpArcsAndSlots() const {
	    endArcPtr_ = endPivotArcPtr_;
	    cleanUpArcs(2);
	    endPivotArcPtr_ = endArcPtr_;
	    for (PivotArcCn::SlotPtrList::iterator itSlotPtr = pcn_->slots_.begin(), endSlotPtr = pcn_->slots_.end(); itSlotPtr != endSlotPtr; ++itSlotPtr) {
		PivotArcCn::ArcPtrList &arcs = (*itSlotPtr)->arcs;
		arcs.erase(arcs.begin() + 1, arcs.end());
		verify(arcs.front()->state == 2);
	    }
	}

	struct TraceElement {
	    Score score;
	    Fsa::StateId bptr;
	    PivotArcCn::Arc *arc;
	    TraceElement() : score(Semiring::Max), bptr(Fsa::InvalidStateId), arc(0) {}
	};
	typedef std::vector<TraceElement> Traceback;
	u32 init(ConstLatticeRef l, ConstFwdBwdRef fb) const {
	    LabelMapRef nonWordToEpsilonMap = LabelMap::createNonWordToEpsilonMap(Lexicon::us()->alphabetId(l->getInputAlphabet(), true));
	    ConstStateMapRef topologicalSort = sortTopologically(l);
	    Fsa::StateId initialSid = topologicalSort->front();

	    ConstBoundariesRef b = l->getBoundaries();
	    ConstSemiringRef semiring = l->semiring();
	    PivotArcCn::ArcPtrList &arcs = pcn_->arcs_;
	    PivotArcCn::SlotPtrList &slots = pcn_->slots_;
	    Traceback traceback(topologicalSort->maxSid + 1);
	    traceback[initialSid].score = 0.0;
	    TraceElement bestTrace;
	    for (u32 i = 0; i < topologicalSort->size(); ++i) {
		Fsa::StateId sid = (*topologicalSort)[i];
		ConstStateRef sr = l->getState(sid);
		Time startTime = b->time(sid);
		const TraceElement &currentTrace = traceback[sid];
		if (sr->isFinal()) {
		    Score score = currentTrace.score + semiring->project(sr->weight());
		    if (score < bestTrace.score) {
			bestTrace.score = score;
			bestTrace.bptr = sid;
		    }
		}
		for (State::const_iterator a = sr->begin(), a_begin = sr->begin(), a_end = sr->end(); a != a_end; ++a) {
		    PivotArcCn::Arc *arc = 0;
		    Score posteriorScore = fb->arc(sr, a).score();
		    Time endTime = b->time(a->target());
		    /*
		      if ((startTime < endTime) && (a->input() != Fsa::Epsilon)) {

		      Seems to make not much a difference with the given distance function;
		      for other distance functions things might like different.

		      Todo: still crashes sometimes, especially for system combination
		      (the reason is probably that the algorithm assumes that the pivot elements/slots
		      occupy the complete time span ...)
		      perhaps it is interesting to make it run for system combination
		    */
		    if (startTime < endTime) {
			arc = new PivotArcCn::Arc;
			arc->sid = sid;
			arc->aid = a - a_begin;
			arc->posteriorScore = posteriorScore;
			arc->start = startTime;
			arc->end = endTime;
			arc->label = a->input();
			arc->isNonWord = ((arc->label != Fsa::Epsilon) && (*nonWordToEpsilonMap)[arc->label].empty()) ? false : true;
			arcs.push_back(arc);
		    } else
			if (a->input() != Fsa::Epsilon)
			    Core::Application::us()->warning(
				"Non-epsilon arc of length 0 detected: \"%s\", %d-%d, score=%f; arc is discarded",
				l->getInputAlphabet()->symbol(a->input()).c_str(), startTime, endTime, semiring->project(a->weight()));
		    TraceElement &trace = traceback[a->target()];
		    /*
		      Score score = currentTrace.score + semiring->project(a->weight());

		      Use best posterior sentence instead of Viterbi sentence for initialization.
		      Seems to make no difference no average;
		      sometimes some errors better, very sometimes a few errors worse.
		      But it works, too, for fwd/bwd combination experiments.
		    */
		    if (posteriorScore < trace.score) {
			trace.score = posteriorScore;
			trace.bptr = sid;
			trace.arc = arc;
		    }
		}
	    }
	    beginArcPtr_ = pcn_->arcs_.begin();
	    endArcPtr_ = pcn_->arcs_.end();
	    verify(bestTrace.bptr != Fsa::InvalidStateId);
	    u32 nAdded = 0;
	    Fsa::StateId bestSid = bestTrace.bptr;
	    while (bestSid != initialSid) {
		const TraceElement &trace = traceback[bestSid];
		if (trace.arc) {
		    PivotArcCn::Arc &arc = *trace.arc;
		    PivotArcCn::Slot *slot = new PivotArcCn::Slot;
		    slot->start = arc.start; slot->end = arc.end;
		    slot->arcs.push_back(&arc);
		    slots.push_back(slot);
		    arc.state = 2;
		    arc.slotDist = Core::Type<Score>::min;
		    ++nAdded;
		}
		bestSid = trace.bptr;
	    }
	    std::reverse(slots.begin(), slots.end());
	    verify(isConsistent(slots));
	    cleanUpArcs(2);
	    endPivotArcPtr_ = endArcPtr_;
	    return nAdded;
	}


	// find pivot arcs, initialize a new slot for each pivot arc, and insert new slot preserving time order
	u32 addPivotArcs(PivotArcCn::SlotPtrList &slots) const {
	    u32 nAdded = 0;
	    std::sort(beginArcPtr_, endArcPtr_, PivotArcCn::Arc::SortByPosteriorScore());
	    for (PivotArcCn::ArcPtrList::iterator itArcPtr = beginArcPtr_; itArcPtr != endArcPtr_; ++itArcPtr) {
		PivotArcCn::Arc &arc = **itArcPtr;
		// binary search for finding correct position for new pivot element; slots are sorted by time
		bool insertSlot = false;
		PivotArcCn::SlotPtrList::iterator itInsertSlot = slots.end();
		if (!slots.empty()) {
		    s32 l = 0, r = slots.end() - slots.begin() - 1, m, maxM = slots.size() - 1;
		    for (;;) {
			// m = (l + r) / 2;
			m = (s32)((u32)(l + r) >> 1);
			verify((l <= r) && (0 <= m) && (m <= maxM));
			const PivotArcCn::Slot &slot = **(slots.begin() + m);
			if (arc.start > slot.start) {
			    if (m == r) {
				if ((slot.end <= arc.start) && ((m == maxM) || (arc.end <= (*(slots.begin() + m + 1))->start))) {
				    insertSlot = true;
				    itInsertSlot = slots.begin() + m + 1;
				}
				break;
			    }
			    l = m + 1;
			} else if (arc.start < slot.start) {
			    if (l == m) {
				if ((arc.end <= slot.start) && ((m == 0) || ((*(slots.begin() + m - 1))->end <= arc.start))) {
				    insertSlot = true;
				    itInsertSlot = slots.begin() + m;
				}
				break;
			    }
			    r = m - 1;
			} else
			    break;
		    }
		} else
		    insertSlot = true;
		if (insertSlot) {
		    PivotArcCn::Slot *newSlot = new PivotArcCn::Slot;
		    newSlot->start = arc.start; newSlot->end = arc.end;
		    newSlot->arcs.push_back(&arc);
		    slots.insert(itInsertSlot, newSlot);
		    arc.slotDist = Core::Type<Score>::min;
		    arc.state = 2;
		    ++nAdded;
		}
	    }
	    verify_(!slots.empty() && isConsistent(slots));
	    return nAdded;
	}

	// find best matching slot for all arcs; some arcs might be left because of violation of time contraints
	u32 addArcs(PivotArcCn::SlotPtrList &slots) const {
	    const Distance &distFcn = *distFcn_;
	    u32 nAdded = 0, lastNAdded = 0;
	    do {
		lastNAdded = nAdded;
		std::sort(beginArcPtr_, endArcPtr_, PivotArcCn::Arc::SortByTime());
		PivotArcCn::ArcPtrList::iterator beginArcPtr = beginArcPtr_;
		u32 slotId = 0;
		for (PivotArcCn::SlotPtrList::const_iterator itSlotPtr = slots.begin(), endSlotPtr = slots.end(); itSlotPtr != endSlotPtr; ++itSlotPtr, ++slotId) {
		    const PivotArcCn::Slot &slot = **itSlotPtr;
		    for (; (beginArcPtr != endArcPtr_) && ((*beginArcPtr)->end <= slot.start); ++beginArcPtr);
		    for (PivotArcCn::ArcPtrList::iterator itArcPtr = beginArcPtr; (itArcPtr != endArcPtr_) && ((*itArcPtr)->start < slot.end); ++itArcPtr) {
			PivotArcCn::Arc &arc = **itArcPtr;
			Score slotDist = distFcn.distance(slot, arc);
			if (slotDist < arc.slotDist) {
			    arc.slotDist = slotDist;
			    arc.slotId = slotId;
			}
		    }
		}

		/* dbg
		   for (PivotArcCn::ArcPtrList::iterator itArcPtr = beginArcPtr_; itArcPtr != endArcPtr_; ++itArcPtr) {
		   PivotArcCn::Arc &arc = **itArcPtr;
		   if (arc.slotId < slots.size()) {
		   PivotArcCn::Slot &slot = *slots[arc.slotId];
		   verify(arc.slotDist != Core::Type<Score>::max);
		   verify(std::max(arc.start, slot.start) < std::min(arc.end, slot.end));
		   } else
		   verify(nAdded > 0);
		   }
		*/

		std::sort(beginArcPtr_, endArcPtr_, PivotArcCn::Arc::SortByDistance());
		for (PivotArcCn::ArcPtrList::iterator itArcPtr = beginArcPtr_; itArcPtr != endArcPtr_; ++itArcPtr) {
		    PivotArcCn::Arc &arc = **itArcPtr;
		    if (arc.slotId < slots.size()) {
			PivotArcCn::Slot &slot = *slots[arc.slotId];
			Time start = std::max(arc.start, slot.start), end = std::min(arc.end, slot.end);
			if (start < end) {
			    slot.arcs.push_back(&arc);
			    slot.start = start; slot.end = end;
			    arc.state = 1;
			    ++nAdded;
			}
		    }
		}

		/* dbg
		   for (PivotArcCn::ArcPtrList::iterator itArcPtr = beginArcPtr_; itArcPtr != endArcPtr_; ++itArcPtr) {
		   PivotArcCn::Arc &arc = **itArcPtr;
		   if ((arc.slotId < slots.size()) && (arc.state == 0)) {
		   PivotArcCn::Slot &slot = *slots[arc.slotId];
		   verify(std::max(arc.start, slot.start) >= std::min(arc.end, slot.end));
		   }
		   }
		*/

		cleanUpArcs();

	    } while(lastNAdded < nAdded);
	    verify_(isConsistent(slots));
	    return nAdded;
	}


	void addSlots(PivotArcCn::SlotPtrList &slots, PivotArcCn::SlotPtrList &addSlots) const {
	    PivotArcCn::SlotPtrList::iterator itSlotPtr = slots.begin(), itAddSlotPtr = addSlots.begin();
	    while ((itSlotPtr != slots.end()) && (itAddSlotPtr != addSlots.end())) {
		if ((*itAddSlotPtr)->start < (*itSlotPtr)->start) {
		    itSlotPtr = slots.insert(itSlotPtr, *itAddSlotPtr);
		    ++itSlotPtr; ++itAddSlotPtr;
		} else
		    ++itSlotPtr;
	    }
	    if (itAddSlotPtr != addSlots.end())
		slots.insert(slots.end(), itAddSlotPtr, addSlots.end());
	}


	/**
	 * Algorithm 1 (fast):
	 * 1) Initialize CN with top best arcs; remove top best aka pivot arcs
	 * 2) for all arcs finds best matching slot
	 * 3) sort arcs by similarity and assign arc to slot until all remaining arcs violate time contraints;
	 *    remove all assigned arcs
	 *    goto 2) until no more arcs are assigned
	 * 4) sort arcs by posterior and find new pivot arcs; remove pivot arcs
	 *    goto 2) until no more arcs are left
	 *
	 * Algorithm 2:
	 * 1) Initialize CN with top best arcs; remove top best aka pivot arcs
	 * 2) for all arcs finds best matching slot
	 * 3) sort arcs by similarity and assign arc to slot until all remaining arcs violate time contraints;
	 *    temporarily remove all assigned arcs
	 *    goto 2) until no more arcs are assigned
	 * 4) sort arcs by posterior and find new pivot arcs; remove pivot arcs, put back all temporarily removed arcs
	 *    goto 2) until no more arcs are left
	 **/
	void buildPivotArcCn(ConstLatticeRef l, ConstFwdBwdRef fb) const {
	    // initialize CN, use viterbi path for initialization
	    init(l, fb);
	    // build CN
	    PivotArcCn::SlotPtrList &slots = pcn_->slots_;
	    PivotArcCn::SlotPtrList newSlots;
	    //endPivotArcs = endArcs_;
	    addArcs(slots);
	    while (beginArcPtr_ != endArcPtr_) {

		verify_(isConsistent(slots));

		addPivotArcs(newSlots);
		if (fast_) {
		    // add only non-member arcs
		    cleanUpArcs();
		    addArcs(newSlots);
		    addSlots(slots, newSlots);
		} else {
		    // (re-)assign all member- and non-member-arcs
		    addSlots(slots, newSlots);
		    cleanUpArcsAndSlots();
		    addArcs(slots);
		}
		newSlots.clear();
	    }
	    verify(isConsistent(pcn_->slots_));
	}

    private:
	PivotArcCnBuilder(ConstDistanceRef distFcn, bool fast) : distFcn_(distFcn), fast_(fast) {}
	~PivotArcCnBuilder() {}

    public:
	void dump(std::ostream &os) const {
	    os << "Pivot-CN builder:" << std::endl;
	    os << "  fast: " << (fast_ ? "yes" : "no") << std::endl;
	    os << "  distance function: " << distFcn_->describe() << std::endl;
	}

	ConstPivotArcCnRef build(ConstLatticeRef l, ConstFwdBwdRef fb = ConstFwdBwdRef()) const {
	    if (!fb) {
		std::pair<ConstLatticeRef, ConstFwdBwdRef> result = FwdBwd::build(l);
		l = result.first; fb = result.second;
	    }
	    PivotArcCn *pcn = pcn_ = new PivotArcCn(l);
	    buildPivotArcCn(l, fb);
	    pcn_ = 0;
	    return ConstPivotArcCnRef(pcn);
	}

	static ConstPivotArcCnBuilderRef create(ConstDistanceRef distFcn = ConstDistanceRef(), bool fast = false) {
	    if (!distFcn)
		distFcn = PivotArcCnBuilder::WeightedTimeDistance::create();
	    return ConstPivotArcCnBuilderRef(new PivotArcCnBuilder(distFcn, fast));
	}
    };
    // -------------------------------------------------------------------------


    // -------------------------------------------------------------------------
    struct PivotArcCnFactory::Internal {
	ConstPivotArcCnBuilderRef pivotCnBuilder;
	ConstPivotArcCnRef pivotCn;
    };

    PivotArcCnFactory::PivotArcCnFactory() {
	internal_ = new Internal;
	internal_->pivotCnBuilder = PivotArcCnBuilder::create(PivotArcCnBuilder::WeightedPivotTimeDistance::create(10.0), false);
    }

    PivotArcCnFactory::~PivotArcCnFactory() {
	delete internal_;
    }

    void PivotArcCnFactory::dump(std::ostream &os) const {
	internal_->pivotCnBuilder->dump(os);
    }

    void PivotArcCnFactory::build(ConstLatticeRef l, ConstFwdBwdRef fb) {

	verify(l && fb);
	verify(internal_->pivotCnBuilder);

	internal_->pivotCn = internal_->pivotCnBuilder->build(l, fb);
    }

    void PivotArcCnFactory::reset() {
	internal_->pivotCn.reset();
    }

    ConstConfusionNetworkRef PivotArcCnFactory::getCn(ScoreId posteriorId, bool mapping) const {
	verify(internal_->pivotCn);
	return internal_->pivotCn->getCn(posteriorId, mapping);
    }

    std::pair<ConstConfusionNetworkRef, ConstLatticeRef> PivotArcCnFactory::getNormalizedCn(ScoreId confidenceId, bool mapping) const {
	verify(internal_->pivotCn);
	return internal_->pivotCn->getNormalizedCn(confidenceId, mapping);
    }
    // -------------------------------------------------------------------------


    // -------------------------------------------------------------------------
    class PivotArcCnBuilderNode : public Node {
    public:
	typedef enum {
	    WeightedTimeDistance,
	    WeightedPivotTimeDistance
	} DistanceFcn;
	static const Core::Choice choiceDistanceFcn;
	static const Core::ParameterChoice paramDistanceFcn;
	static const Core::ParameterFloat paramPosteriorImpact;
	static const Core::ParameterBool paramEditDistance;
	static const Core::ParameterBool paramFast;
	static const Core::ParameterBool paramApproximateLatticeBoundaryTimes;
	static const Core::ParameterString paramConfKey;
	static const Core::ParameterBool paramMap;

    private:
	u32 n_;
	Key confidenceKey_;
	FwdBwdBuilderRef fbBuilder_;
	ConstPivotArcCnBuilderRef cnBuilder_;
	bool map_;

	ConstLatticeRef union_;
	ConstFwdBwdRef fb_;
	ConstPivotArcCnRef pivotCn_;
	ConstConfusionNetworkRef posteriorCn_;
	ConstConfusionNetworkRef cn_;
	ConstLatticeRef best_;

	ConstSemiringRef lastSemiring_;
	ScoreId confidenceId_;

    private:
	ConstPivotArcCnBuilderRef getPivotArcCnBuilder(ConstLatticeRef l) {
	    if (!cnBuilder_) {
		Core::Choice::Value distChoice = paramDistanceFcn(config);
		if (distChoice == Core::Choice::IllegalValue)
		    criticalError("Unknwon distance fucntion.");
		const Core::Configuration distConfig = select(choiceDistanceFcn[distChoice]);
		Score impact = 1.0 / paramPosteriorImpact(distConfig);
		ConstEditDistanceRef editDist;
		if (paramEditDistance(distConfig))
		    editDist = EditDistance::create(Lexicon::us()->alphabetId(l->getInputAlphabet()));
		PivotArcCnBuilder::ConstDistanceRef distFcn;
		switch (DistanceFcn(distChoice)) {
		case WeightedTimeDistance:
		    distFcn = PivotArcCnBuilder::WeightedTimeDistance::create(impact, editDist);
		    break;
		case WeightedPivotTimeDistance:
		    distFcn = PivotArcCnBuilder::WeightedPivotTimeDistance::create(impact, editDist);
		    break;
		default:
		    defect();
		}
		cnBuilder_ = PivotArcCnBuilder::create(distFcn, paramFast(config));
		cnBuilder_->dump(log());
	    }
	    return cnBuilder_;
	}


	void computeFwdBwd() {
	    if (!fb_) {
		ConstLatticeRefList lats(n_);
		for (u32 i = 0; i < n_; ++i)
		    lats[i] = requestLattice(i);
		std::pair<ConstLatticeRef, ConstFwdBwdRef> fbResult = (n_ == 1) ? fbBuilder_->build(lats.front()) : fbBuilder_->build(lats);
		//std::pair<ConstLatticeRef, ConstFwdBwdRef> fbResult = fbBuilder_->build(lats);
		union_ = fbResult.first; fb_ = fbResult.second;
	    }
	}

	void computePivotArcCn() {
	    if (!pivotCn_) {
		computeFwdBwd();
		pivotCn_ = getPivotArcCnBuilder(union_)->build(union_, fb_);
	    }
	}

	void computeConfidenceId() {
	    computePivotArcCn();
	    ConstSemiringRef semiring = pivotCn_->getLattice()->semiring();
	    if (!lastSemiring_
		|| (semiring.get() != lastSemiring_.get())
		|| !(*semiring == *lastSemiring_)) {
		lastSemiring_ = semiring;
		if (!confidenceKey_.empty()) {
		    confidenceId_ = semiring->id(confidenceKey_);
		    if (confidenceId_ == Semiring::InvalidId)
			warning("Semiring \"%s\" has no dimension labeled \"%s\".",
				semiring->name().c_str(), confidenceKey_.c_str());
		}
	    }
	}

	ConstConfusionNetworkRef getCn() {
	    if (!cn_) {
		computePivotArcCn();
		computeConfidenceId();
		cn_ = pivotCn_->getCn(confidenceId_, map_);
	    }
	    return cn_;
	}

	ConstConfusionNetworkRef getNormalizedCn() {
	    if (!posteriorCn_) {
		computePivotArcCn();
		computeConfidenceId();
		std::pair<ConstConfusionNetworkRef, ConstLatticeRef> result = pivotCn_->getNormalizedCn(confidenceId_, map_);
		posteriorCn_ = result.first; best_ = result.second;
		verify(posteriorCn_->isNormalized());
	    }
	    return posteriorCn_;
	}

	ConstLatticeRef getBestLattice() {
	    if (!best_) {
		computePivotArcCn();
		computeConfidenceId();
		std::pair<ConstConfusionNetworkRef, ConstLatticeRef> result = pivotCn_->getNormalizedCn(confidenceId_, map_);
		posteriorCn_ = result.first; best_ = result.second;
		verify(posteriorCn_->isNormalized());
	    }
	    return best_;
	}

	ConstLatticeRef getUnionLattice() {
	    computeFwdBwd();
	    return union_;
	}

    public:
	PivotArcCnBuilderNode(const std::string &name, const Core::Configuration &config) :
	    Node(name, config), n_(0) {  confidenceId_ = Semiring::InvalidId; }
	virtual ~PivotArcCnBuilderNode() {}

	virtual void init(const std::vector<std::string> &arguments) {
	    for (n_ = 0; connected(n_); ++n_);
	    if (n_ == 0)
		criticalError("At least one incoming lattice at port 0 required.");
	    Core::Component::Message msg = log();
	    if (n_ > 1)
		msg << "Combine " << n_ << " lattices.\n\n";
	    KeyList requiredKeys;
	    ScoreList requiredScales;
	    confidenceKey_ = paramConfKey(config);
	    if (!confidenceKey_.empty()) {
		msg << "Confidence key is \"" << confidenceKey_ << "\"\n";
		requiredKeys.push_back(confidenceKey_);
		requiredScales.push_back(0.0);
	    }
	    fbBuilder_ = FwdBwdBuilder::create(select("fb"), requiredKeys, requiredScales);
	    map_ = paramMap(config);
	    if (map_)
		msg << "Build lattice-to-CN resp. CN-to-lattice mapping\n";
	}

	virtual void finalize() {}

	virtual ConstLatticeRef sendLattice(Port to) {
	    switch (to) {
	    case 0:
		return getBestLattice();
	    case 2:
		return cn2lattice(getNormalizedCn(), paramApproximateLatticeBoundaryTimes(config));
	    case 4:
		return cn2lattice(getCn(), paramApproximateLatticeBoundaryTimes(config));
	    case 6:
		return getUnionLattice();
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
	    best_.reset();
	    pivotCn_.reset();
	    posteriorCn_.reset();
	    cn_.reset();
	    fb_.reset();
	    union_.reset();
	}
    };
    const Core::Choice PivotArcCnBuilderNode::choiceDistanceFcn(
	"weighted-time",       PivotArcCnBuilderNode::WeightedTimeDistance,
	"weighted-pivot-time", PivotArcCnBuilderNode::WeightedPivotTimeDistance,
	Core::Choice::endMark());
    const Core::ParameterChoice PivotArcCnBuilderNode::paramDistanceFcn(
	"distance",
	&PivotArcCnBuilderNode::choiceDistanceFcn,
	"distance function",
	PivotArcCnBuilderNode::WeightedTimeDistance);
    const Core::ParameterFloat PivotArcCnBuilderNode::paramPosteriorImpact(
	"posterior-impact",
	"impact of posterior score on arc distance",
	0.1);
    const Core::ParameterBool PivotArcCnBuilderNode::paramEditDistance(
	"edit-distance",
	"calculate edit distance between different arc labels",
	false);
    const Core::ParameterBool PivotArcCnBuilderNode::paramFast(
	"fast",
	"use fast algorithm",
	true);
    const Core::ParameterBool PivotArcCnBuilderNode::paramApproximateLatticeBoundaryTimes(
	"approximate-lattice-boundary-times",
	"create approximative lattice boundary times when converting the confusion network back to a lattice:\
	 inherit the boundary time from the adjacent confusion network arc with highest probability",
	false);
    const Core::ParameterString PivotArcCnBuilderNode::paramConfKey(
	"confidence-key",
	"store confidence score",
	"");
    const Core::ParameterBool PivotArcCnBuilderNode::paramMap(
	"map",
	"map lattice to CN and vice versa",
	false);
    NodeRef createPivotArcCnBuilderNode(const std::string &name, const Core::Configuration &config) {
	return NodeRef(new PivotArcCnBuilderNode(name, config));
    }
    // -------------------------------------------------------------------------
} // namespace

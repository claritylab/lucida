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
#include <Core/ProgressIndicator.hh>
#include <Math/Utilities.hh>

#include "FlfCore/Basic.hh"
#include "FlfCore/Traverse.hh"
#include "FlfCore/Utility.hh"
#include "CenterFrameConfusionNetworkBuilder.hh"
#include "ConfusionNetwork.hh"
#include "Copy.hh"
#include "FwdBwd.hh"
#include "TimeframeConfusionNetworkBuilder.hh"

namespace Flf {

    typedef s32 Id;
    typedef std::vector<Id> IdList;

    typedef std::vector<Time> FrameList;

    class CenterFrameCn;
    typedef Core::Ref<const CenterFrameCn> ConstCenterFrameCnRef;
    class CenterFrameCnBuilder;
    typedef Core::Ref<CenterFrameCnBuilder> CenterFrameCnBuilderRef;


    /*
      Inernal data structure storing all information necessary for building a CN
      plus algorithm-individual information available from the CN construction algorithm
    */
    class CenterFrameCn : public Core::ReferenceCounted {
	friend class CenterFrameCnBuilder;
    public:
	struct Arc {
	    Fsa::StateId sid, aid;
	    s32 start, end;
	    Fsa::LabelId label;
	    ScoresRef scores;
	    Score posteriorScore;
	    IdList frameIds;
	};
	typedef std::vector<Arc> ArcList;

	struct Slot {
	    Time t;
	    IdList arcIds;
	    Slot(Time t) : t(t) {}
	};
	typedef std::vector<Slot*> SlotPtrList;

    private:
	ConstLatticeRef l_;
	ArcList arcs_;
	SlotPtrList slotPtrs_;

    private:
	CenterFrameCn(ConstLatticeRef l) : l_(l) {}

    public:
	~CenterFrameCn() {
	    for (SlotPtrList::iterator itSlotPtr = slotPtrs_.begin(), endSlotPtr = slotPtrs_.end();
		 itSlotPtr != endSlotPtr; ++itSlotPtr) delete *itSlotPtr;
	}

	/*
	  The lattice
	*/
	ConstLatticeRef getLattice() const {
	    return l_;
	}

	/*
	  The support frames
	*/
	FrameList getSupportFrames() const {
	    FrameList supportFrames(slotPtrs_.size());
	    FrameList::iterator itFrame = supportFrames.begin();
	    for (u32 i = 0; i < slotPtrs_.size(); ++i, ++itFrame) {
		const Slot &frSlot = *slotPtrs_[i];
		*itFrame = frSlot.t;
	    }
	    return supportFrames;
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
		for (Fsa::StateId sid = 0; sid < slotPtrs_.size(); ++sid) {
		    Slot &frSlot = *slotPtrs_[sid];
		    for (IdList::const_iterator itArcId = frSlot.arcIds.begin(), endArcId = frSlot.arcIds.end();
			 itArcId != endArcId; ++itArcId) {
			const Arc &frArc = arcs_[*itArcId];
			verify_(stateIndex[frArc.sid] + frArc.aid < lat2cn.size());
			lat2cn[stateIndex[frArc.sid] + frArc.aid] = ConfusionNetwork::MapProperties::Mapping(sid, Fsa::InvalidStateId);
		    }
		}
	    } else {
		// lattice arc <-> slot arc
		Core::Vector<Fsa::StateId> &slotIndex = mapProps->slotIndex;
		ConfusionNetwork::MapProperties::Map &cn2lat = mapProps->cn2lat;
		{
		    slotIndex.resize(slotPtrs_.size() + 1);
		    u32 nCnArcs = 0;
		    for (Fsa::StateId sid = 0; sid < slotPtrs_.size(); ++sid) {
			const Slot &frSlot = *slotPtrs_[sid];
			slotIndex[sid] = nCnArcs;
			nCnArcs += frSlot.arcIds.size();
		    }
		    slotIndex.back() = nCnArcs;
		    cn2lat.resize(slotIndex.back(), ConfusionNetwork::MapProperties::Mapping(Fsa::InvalidStateId, 0));
		}
		for (Fsa::StateId sid = 0; sid < slotPtrs_.size(); ++sid) {
		    Slot &frSlot = *slotPtrs_[sid];
		    Fsa::StateId aid = 0;
		    for (IdList::const_iterator itArcId = frSlot.arcIds.begin(), endArcId = frSlot.arcIds.end();
			 itArcId != endArcId; ++itArcId, ++aid) {
			const Arc &frArc = arcs_[*itArcId];
			verify_(slotIndex[sid] + aid < cn2lat.size());
			cn2lat[slotIndex[sid] + aid] = ConfusionNetwork::MapProperties::Mapping(frArc.sid, frArc.aid);
			verify_(stateIndex[frArc.sid] + frArc.aid < lat2cn.size());
			lat2cn[stateIndex[frArc.sid] + frArc.aid] = ConfusionNetwork::MapProperties::Mapping(sid, aid);
		    }
		}
	    }
	    return ConfusionNetwork::ConstMapPropertiesRef(mapProps);
	}
    public:
	ConstConfusionNetworkRef getCn(ScoreId posteriorId, bool mapping = false) const {
	    if (slotPtrs_.empty())
		return ConstConfusionNetworkRef();
	    ConstSemiringRef semiring = l_->semiring();

	    ConfusionNetwork *cn = new ConfusionNetwork(slotPtrs_.size());
	    cn->alphabet = l_->getInputAlphabet();
	    cn->semiring = semiring;
	    for (u32 i = 0; i < slotPtrs_.size(); ++i) {
		Slot &frSlot = *slotPtrs_[i];
		ConfusionNetwork::Slot &slot = (*cn)[i];
		for (IdList::const_iterator itArcId = frSlot.arcIds.begin(), endArcId = frSlot.arcIds.end();
		     itArcId != endArcId; ++itArcId) {
		    const Arc &frArc = arcs_[*itArcId];
		    // const Flf::State &state = *l_->getState(frArc.sid);
		    // const Flf::Arc &arc = *(state.begin() + frArc.aid);
		    ScoresRef scores = frArc.scores;
		    if (posteriorId != Semiring::InvalidId) {
			scores = semiring->clone(scores);
			scores->set(posteriorId, ::exp(-frArc.posteriorScore));
		    }
		    slot.push_back(ConfusionNetwork::Arc(
				       frArc.label, scores,
				       frArc.start, frArc.end - frArc.start));
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
	    if (slotPtrs_.empty())
		return std::make_pair(ConstConfusionNetworkRef(), ConstLatticeRef());
	    ConstSemiringRef semiring = l_->semiring();
	    // Posterior CN
	    ConfusionNetwork *cn = new ConfusionNetwork(slotPtrs_.size());
	    cn->alphabet = l_->getInputAlphabet();
	    cn->semiring = semiring;
	    cn->normalizedProperties = ConfusionNetwork::ConstNormalizedPropertiesRef(
		new ConfusionNetwork::NormalizedProperties(confidenceId));
	    // best lattice
	    StaticBoundaries *b = new StaticBoundaries;
	    StaticLattice *s = new StaticLattice;
	    s->setDescription("decode-cn(" + l_->describe() + ",center-frame)");
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
	    for (u32 i = 0; i < slotPtrs_.size(); ++i) {
		// distribute probabilities over words
		ConfusionNetwork::Slot &slot = (*cn)[i];
		Slot &frcnSlot = *slotPtrs_[i];
		const Arc *bestFrcnArcPtr = 0;
		ScoresRef bestFrcnArcScores = ScoresRef();
		f64 bestPosteriorScore = Core::Type<f64>::max;
		const Arc *bestCurrentLabelFrcnArcPtr = &arcs_[frcnSlot.arcIds.front()];
		Score epsConfidence = 0.0;
		for (IdList::const_iterator itArcId = frcnSlot.arcIds.begin(), endArcId = frcnSlot.arcIds.end(); itArcId != endArcId; ++itArcId) {
		    const Arc *frcnArcPtr = &arcs_[*itArcId];
		    if (frcnArcPtr->label != bestCurrentLabelFrcnArcPtr->label) {
			verify(bestCurrentLabelFrcnArcPtr->label < frcnArcPtr->label);
			f64 posteriorScore = col->get();
			col->reset();
			Score confidence = ::exp(-posteriorScore);

			// dbg
			verify(!Math::isnan(confidence));

			if (bestCurrentLabelFrcnArcPtr->label == Fsa::Epsilon)
			    epsConfidence = confidence;
			Score start = ::exp(posteriorScore - colStart->get()), end = ::exp(posteriorScore - colEnd->get());
			colStart->reset(); colEnd->reset();
			// const Flf::State &state = *l_->getState(bestCurrentLabelFrcnArcPtr->sid);
			// const Flf::Arc &arc = *(state.begin() + bestCurrentLabelFrcnArcPtr->aid);
			ScoresRef scores = bestCurrentLabelFrcnArcPtr->scores;
			if (confidenceId != Semiring::InvalidId) {
			    scores = semiring->clone(scores);
			    scores->set(confidenceId, confidence);
			}
			slot.push_back(ConfusionNetwork::Arc(
					   bestCurrentLabelFrcnArcPtr->label,
					   scores,
					   Time(Core::round(start)), std::max(Time(1), Time(Core::round(end - start)))));
			if (posteriorScore < bestPosteriorScore) {
			    bestPosteriorScore = posteriorScore;
			    bestFrcnArcPtr = bestCurrentLabelFrcnArcPtr;
			    bestFrcnArcScores = scores;
			}
			bestCurrentLabelFrcnArcPtr = frcnArcPtr;
			sumCol->feed(posteriorScore);
		    } else {
			if (frcnArcPtr->posteriorScore < bestCurrentLabelFrcnArcPtr->posteriorScore)
			    bestCurrentLabelFrcnArcPtr = frcnArcPtr;
		    }
		    col->feed(frcnArcPtr->posteriorScore);
		    colStart->feed(frcnArcPtr->posteriorScore - ::log(f64(frcnArcPtr->start)));
		    colEnd->feed(frcnArcPtr->posteriorScore - ::log(f64(frcnArcPtr->end)));
		}
		f64 posteriorScore = col->get();
		col->reset();
		{
		    const Flf::State &state = *l_->getState(bestCurrentLabelFrcnArcPtr->sid);
		    const Flf::Arc &arc = *(state.begin() + bestCurrentLabelFrcnArcPtr->aid);
		    Score confidence = ::exp(-posteriorScore);

		    // dbg
		    verify(!std::isnan(confidence));

		    if (bestCurrentLabelFrcnArcPtr->label == Fsa::Epsilon)
			epsConfidence = confidence;
		    Score start = ::exp(posteriorScore - colStart->get()), end = ::exp(posteriorScore - colEnd->get());
		    colStart->reset(); colEnd->reset();
		    ScoresRef scores = arc.weight();
		    if (confidenceId != Semiring::InvalidId) {
			scores = semiring->clone(scores);
			scores->set(confidenceId, confidence);
		    }
		    slot.push_back(ConfusionNetwork::Arc(
				       bestCurrentLabelFrcnArcPtr->label,
				       scores,
				       Time(Core::round(start)), std::max(Time(1), Time(Core::round(end - start)))));
		    if (posteriorScore < bestPosteriorScore) {
			bestPosteriorScore = posteriorScore;
			bestFrcnArcPtr = bestCurrentLabelFrcnArcPtr;
			bestFrcnArcScores = scores;
		    }
		}
		sumCol->feed(posteriorScore);
		verify(bestFrcnArcPtr);
		// add remaining probability mass to eps arc
		Probability bestConfidence = ::exp(-bestPosteriorScore);
		Probability sum = ::exp(-sumCol->get());

		// dbg
		verify(!std::isnan(sum));

		sumCol->reset();
		if (sum != 1.0) {
		    bool normalize = false;
		    Probability dEpsConfidence = 1.0 - sum;
		    if (dEpsConfidence < 0.0) {
			// if (dEpsConfidence < -0.001)
			if (dEpsConfidence < -0.01)
			    Core::Application::us()->warning(
				"CenterFrameCnBuilder: Expected 1.0, got %f", (1.0 - dEpsConfidence));
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
			    bestFrcnArcPtr = 0;
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
		if ((bestFrcnArcPtr) && (bestFrcnArcPtr->label != Fsa::Epsilon)) {
		    if (preferredEndTime < Time(bestFrcnArcPtr->start)) {
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
			preferredEndTime = bestFrcnArcPtr->start;
		    } else
			preferredEndTime = std::max(minEndTime ,(preferredEndTime + bestFrcnArcPtr->start) / 2);

		    b->set(sp->id(), Boundary(preferredEndTime));
		    if (confidenceId != Semiring::InvalidId) {
			bestFrcnArcScores = semiring->clone(bestFrcnArcScores);
			bestFrcnArcScores->set(confidenceId, bestConfidence);
		    }
		    sp->newArc(sp->id() + 1, bestFrcnArcScores, bestFrcnArcPtr->label, bestFrcnArcPtr->label);
		    sp = new Flf::State(sp->id() + 1); s->setState(sp);
		    minEndTime = preferredEndTime + 1;
		    preferredEndTime = std::max(minEndTime, Time(bestFrcnArcPtr->end));
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


    /*
      CN builder
    */
    class CenterFrameCnBuilder : public Core::ReferenceCounted {
    private:
	class Frame {
	public:
	    struct Properties {
		Score nonEpsScore;
		Score nonEpsCenterDevScore;
	    };
	public:
	    s32 t;
	    bool updated;
	    Properties props;
	    IdList arcIds;
	    Frame() : t(0.0), updated(true) {}
	};
	typedef std::vector<Frame> FrameList;

	struct SortArcIdByLabel {
	    const CenterFrameCn::ArcList &arcs;
	    SortArcIdByLabel(const CenterFrameCn::ArcList &arcs) : arcs(arcs) {}
	    bool operator() (const Id aid1, const Id aid2) const {
		return arcs[aid1].label < arcs[aid2].label;
	    }
	};

	struct SortSlotPtrByTime {
	    bool operator() (const CenterFrameCn::Slot *slot1, const CenterFrameCn::Slot *slot2) const {
		return slot1->t < slot2->t;
	    }
	};

    private:
	StaticLattice *staticL_;
	StaticBoundaries *staticB_;
	CenterFrameCn *feCn_;
	FrameList frames_;

    private:
	/*
	  initialize data structure
	*/
	u32 initialize(ConstFwdBwdRef fb, ConstPosteriorCnRef fCn) {
	    CenterFrameCn::ArcList &arcs = feCn_->arcs_;
	    // collect some lattice statistics
	    u32 nArcs = 0;
	    Time nFrames = 0;
	    for (Fsa::StateId sid = 0, endSid = staticL_->size(); sid < endSid; ++sid) {
		Flf::State *sp = staticL_->fastState(sid);
		if (sp) {
		    nArcs += sp->nArcs();
		    Time t = staticB_->time(sid);
		    if (t > nFrames)
			nFrames = t;
		}
	    }
	    arcs.resize(nArcs);
	    frames_.resize(nFrames);
	    // fill data structure
	    ProbabilityList confidences;
	    Id arcId = 0;
	    for (Fsa::StateId sid = 0, endSid = staticL_->size(); sid < endSid; ++sid) {
		Flf::State *sp = staticL_->fastState(sid);
		if (sp) {
		    Fsa::StateId aid = 0;
		    Time tBegin = staticB_->time(sid), tEnd, dur;
		    for (State::const_iterator a = sp->begin(), a_end = sp->end(); a != a_end; ++a, ++aid, ++arcId) {
			CenterFrameCn::Arc &arc = arcs[arcId];
			Fsa::LabelId label = a->input();
			if (label == Fsa::Epsilon)
			    continue;
			tEnd = staticB_->time(a->target());
			arc.sid = sid; arc.aid = aid;
			arc.start = tBegin; arc.end = tEnd;
			arc.label = label;
			arc.scores = a->weight();
			arc.posteriorScore = (fb->state(sid).begin() + aid)->score();
			dur = tEnd - tBegin;
			if (dur > 0) {
			    if (confidences.size() < dur)
				confidences.resize(dur);
			    fCn->scores(confidences.begin(), confidences.begin() + dur, tBegin, label);
			    Probability maxConf = 0.0;
			    for (ProbabilityList::const_iterator itConf = confidences.begin(), endConf = confidences.begin() + dur;
				 itConf != endConf; ++itConf)
				if (*itConf > maxConf) maxConf = *itConf;
			    verify_(0.0 < maxConf  && maxConf <= 1.01);
			    Id frameId = tBegin;
			    FrameList::iterator itFrame = frames_.begin() + tBegin;
			    for (ProbabilityList::const_iterator itConf = confidences.begin(), endConf = confidences.begin() + dur;
				 itConf != endConf; ++itConf, ++frameId, ++itFrame)
				if (maxConf - *itConf <= 0.01) {

				    // dbg("+ " << frameId << ", " << *itConf << ", " << staticL_->getInputAlphabet()->symbol(label));

				    arc.frameIds.push_back(frameId);
				    itFrame->arcIds.push_back(arcId);
				}
			} else
			    Core::Application::us()->warning(
				"Non-epsilon arc of length 0 detected: \"%s\", %d-%d, score=%f; arc is discarded",
				staticL_->getInputAlphabet()->symbol(arc.label).c_str(),
				arc.start, arc.end,
				staticL_->semiring()->project(arc.scores));
		    }
		}
	    }
	    // data structure post-processing
	    SortArcIdByLabel sortArcIdByLabel(arcs);
	    s32 t = 0;
	    for (FrameList::iterator itFrame = frames_.begin(), endFrame = frames_.begin() + nFrames; itFrame != endFrame; ++itFrame, ++t) {
		Frame &frame = *itFrame;
		frame.t = t;
		std::sort(frame.arcIds.begin(), frame.arcIds.end(), sortArcIdByLabel);
	    }
	    return nFrames;
	}

	/*
	  Alogrithm:
	  1) C <- []
	  for each frame f do ;
	  for each label l do ;
	  for each arc a in lattice L with label l and crossing frame f do ;
	  if argmax(fCN(arc)) == f then ;
	  C <- (fCN(arc), arc, f)
	  2) sort C by criterion
	  3) nextF <- C[0].f
	  for each (p, a, f) in C if f == nextF do ;
	  S <- arc
	  remove arc a from lattice L
	  4) if lattice L has arcs then goto 1)
	*/
	/*
	  find the next best frame; discard all frames not storing an arc anymore
	*/
	inline bool less(const Frame::Properties &p1, const Frame::Properties &p2) const {
	    if (Core::isAlmostEqualUlp(p1.nonEpsScore, p2.nonEpsScore, 2048))
		return p1.nonEpsCenterDevScore > p2.nonEpsCenterDevScore;
	    else
		return p1.nonEpsScore < p2.nonEpsScore;
	}
	Id updateFrames(IdList::iterator beginFrameId, IdList::iterator &endFrameId) {
	    const CenterFrameCn::ArcList &arcs = feCn_->arcs_;
	    Id bestFrameId = -1;
	    Frame::Properties bestProps; bestProps.nonEpsScore = Core::Type<Score>::max; bestProps.nonEpsCenterDevScore = Core::Type<Score>::max;
	    Collector *nonEpsCol = createCollector(Fsa::SemiringTypeLog), *centerDevCol = createCollector(Fsa::SemiringTypeLog);
	    for (IdList::const_iterator itFrameId = beginFrameId; itFrameId != endFrameId; ++itFrameId) {
		Frame &frame = frames_[*itFrameId];
		bool keep = true;
		if (frame.updated) {
		    keep = false;
		    for (IdList::const_iterator itArcId = frame.arcIds.begin(), endArcId = frame.arcIds.end();
			 itArcId != endArcId; ++itArcId) {
			if (*itArcId != -1) {
			    keep = true;
			    const CenterFrameCn::Arc &arc = arcs[*itArcId];
			    nonEpsCol->feed(arc.posteriorScore);
			    const Score centerDev = Score(u32(Core::abs(frame.t + frame.t - arc.start - arc.end + 1)) >> 1);
			    if (centerDev > 0.0) {
				// quadratic version
				centerDevCol->feed(arc.posteriorScore - 2.0 * ::log(centerDev));
				// normalized, quadratic version
				/*
				const Score normDev = Score((arc.end - arc.start - 1) >> 1);
				verify(normDev > 0.0);
				centerDevCol->feed(arc.posteriorScore - 2.0 * (::log(centerDev) - ::log(normDev)));
				*/

			    }
			}
		    }
		    if (keep) {
			frame.props.nonEpsScore = nonEpsCol->get();
			nonEpsCol->reset();
			if (frame.props.nonEpsScore < 0.0010005003335835344) // -log(0.995)=0.0050125418235442863, -log(0.999)=0.0010005003335835344
			    frame.props.nonEpsScore = 0.0;
			frame.props.nonEpsCenterDevScore = centerDevCol->get();
			centerDevCol->reset();
		    } else
			frame.arcIds.clear();
		    frame.updated = false;
		}
		if (keep) {
		    if (less(frame.props, bestProps)) {
			bestFrameId = *itFrameId;
			bestProps = frame.props;
		    }
		    *beginFrameId = *itFrameId;
		    ++beginFrameId;
		}
	    }
	    endFrameId = beginFrameId;
	    delete nonEpsCol; delete centerDevCol;
	    return bestFrameId;
	}

	/*
	  Remove frame from hypotheses list and convert frame into slot
	*/
	void buildSlot(Id frameId) {
	    // const CenterFrameCn::ArcList &arcs = feCn_->arcs_;
	    CenterFrameCn::ArcList &arcs = feCn_->arcs_;
	    Frame &frame = frames_[frameId];
	    feCn_->slotPtrs_.push_back(new CenterFrameCn::Slot(frameId));
	    CenterFrameCn::Slot &slot = *feCn_->slotPtrs_.back();
	    for (IdList::const_iterator itArcId = frame.arcIds.begin(), endArcId = frame.arcIds.end();
		 itArcId != endArcId; ++itArcId) {
		Id arcId = *itArcId;
		if (arcId != -1) {
		    slot.arcIds.push_back(arcId);
		    const CenterFrameCn::Arc &arc = arcs[arcId];
		    for (IdList::const_iterator itDiscardFrameId = arc.frameIds.begin(), endDiscardFrameId = arc.frameIds.end();
			 itDiscardFrameId != endDiscardFrameId; ++itDiscardFrameId) if (*itDiscardFrameId != frameId) {
			    Frame &discardFrame = frames_[*itDiscardFrameId];
			    // delete arc from frame
			    IdList::iterator itDiscardArcId = discardFrame.arcIds.begin();
			    for (; *itDiscardArcId != arcId; ++itDiscardArcId) verify_(itDiscardArcId != discardFrame.arcIds.end());
			    *itDiscardArcId = -1;
			    discardFrame.updated = true;
			}
		}
	    }
	    frame.arcIds.clear();
	    frame.updated = true;
	}

	/*
	  build the internal CN data structure
	*/
	void buildCenterFrameCn(ConstFwdBwdRef fb, ConstPosteriorCnRef fCn) {
	    Core::ProgressIndicator pi("initialize");
	    pi.start();
	    u32 nFrames = initialize(fb, fCn);
	    pi.setTask(Core::form("#frames=%d", nFrames));
	    pi.setTotal(nFrames);
	    verify(feCn_);
	    IdList frameIdList(nFrames);
	    {
		IdList::iterator itFrameId = frameIdList.begin();
		for (u32 frameId = 0; frameId < nFrames; ++frameId, ++itFrameId)
		    *itFrameId = frameId;
	    }
	    // build CN
	    IdList::iterator beginFrameId = frameIdList.begin(), endFrameId = frameIdList.begin() + nFrames;
	    Id frameId = updateFrames(beginFrameId, endFrameId);
	    pi.notify(nFrames - (endFrameId - beginFrameId));
	    while (frameId != -1) {
		buildSlot(frameId);
		frameId = updateFrames(beginFrameId, endFrameId);
		pi.notify(nFrames - (endFrameId - beginFrameId));
	    }
	    pi.finish(false);
	    std::sort(feCn_->slotPtrs_.begin(), feCn_->slotPtrs_.end(), SortSlotPtrByTime());
	}

	CenterFrameCnBuilder() : staticL_(0), staticB_(0), feCn_(0) {}

    public:
	~CenterFrameCnBuilder() {}

	void dump(std::ostream &os) const {
	    os << "Frame-Example-CN builder" << std::endl;
	}

	ConstCenterFrameCnRef build(ConstLatticeRef l, ConstFwdBwdRef fb = ConstFwdBwdRef(), ConstPosteriorCnRef fCn = ConstPosteriorCnRef()) {
	    staticL_ = new StaticLattice;
	    staticB_ = new StaticBoundaries;
	    persistent(l, staticL_, staticB_);
	    staticL_->setBoundaries(ConstBoundariesRef(staticB_));
	    l = ConstLatticeRef(staticL_);
	    if (!fb)
		fb = FwdBwd::build(l).second;
	    if (!fCn)
		fCn = buildFramePosteriorCn(l, fb);
	    CenterFrameCn *feCn = feCn_ = new CenterFrameCn(l);
	    buildCenterFrameCn(fb, fCn);
	    staticL_ = 0;
	    staticB_ = 0;
	    feCn_ = 0;
	    frames_.clear();
	    return ConstCenterFrameCnRef(feCn);
	}

	static CenterFrameCnBuilderRef create() {
	    return CenterFrameCnBuilderRef(new CenterFrameCnBuilder);
	}
    };
    // -------------------------------------------------------------------------


    // -------------------------------------------------------------------------
    /*
      // deprecated
    namespace {
	typedef enum {
	    FrameOrderByCenterDeviation,
	    FrameOrderByLabelEntropy,
	    FrameOrderByMinLabelScore
	} FrameOrderType;
	const Core::Choice choiceFrameOrderType(
	    "center-deviation",      FrameOrderByCenterDeviation,
	    "label-entropy",         FrameOrderByLabelEntropy,
	    "max-label-probability", FrameOrderByMinLabelScore,
	    Core::Choice::endMark());
	const Core::ParameterChoice paramFrameOrderType(
	    "frame-order",
	    &choiceFrameOrderType,
	    "risk builder",
	    FrameOrderByLabelEntropy);
	CenterFrameCnBuilder::Frame::ConstLessRef buildFrameOrder(const Core::Configuration &config) {
	    Core::Choice::Value frameOrderType = paramFrameOrderType(config);
	    if (frameOrderType == Core::Choice::IllegalValue)
		Core::Application::us()->criticalError("Unknown frame order type");
	    switch (FrameOrderType(frameOrderType)) {
	    case FrameOrderByCenterDeviation:
		return CenterFrameCnBuilder::Frame::ConstLessRef(new CenterFrameCnBuilder::LessByCenterDeviation());
	    case FrameOrderByLabelEntropy:
		return CenterFrameCnBuilder::Frame::ConstLessRef(new CenterFrameCnBuilder::LessByLabelEntropy());
	    case FrameOrderByMinLabelScore:
		return CenterFrameCnBuilder::Frame::ConstLessRef(new CenterFrameCnBuilder::LessByMinLabelScore());
	    default:
		defect();
	    }
	    return CenterFrameCnBuilder::Frame::ConstLessRef();
	}
    } // namespace
    */
    // -------------------------------------------------------------------------


    // -------------------------------------------------------------------------
    struct CenterFrameCnFactory::Internal {
	CenterFrameCnBuilderRef feCnBuilder;
	ConstCenterFrameCnRef feCn;
    };

    CenterFrameCnFactory::CenterFrameCnFactory(const Core::Configuration &config) {
	internal_ = new Internal;
	internal_->feCnBuilder = CenterFrameCnBuilder::create();
    }

    CenterFrameCnFactory::CenterFrameCnFactory() {
	internal_ = new Internal;
	internal_->feCnBuilder = CenterFrameCnBuilder::create();
    }

    CenterFrameCnFactory::~CenterFrameCnFactory() {
	delete internal_;
    }

    void CenterFrameCnFactory::dump(std::ostream &os) const {
	internal_->feCnBuilder->dump(os);
    }

    void CenterFrameCnFactory::build(ConstLatticeRef l, ConstFwdBwdRef fb) {
	internal_->feCn = internal_->feCnBuilder->build(l, fb);
    }

    void CenterFrameCnFactory::reset() {
	internal_->feCn.reset();
    }

    ConstConfusionNetworkRef CenterFrameCnFactory::getCn(ScoreId posteriorId, bool mapping) const {
	verify(internal_->feCn);
	return internal_->feCn->getCn(posteriorId, mapping);
    }

    std::pair<ConstConfusionNetworkRef, ConstLatticeRef> CenterFrameCnFactory::getNormalizedCn(ScoreId confidenceId, bool mapping) const {
	verify(internal_->feCn);
	return internal_->feCn->getNormalizedCn(confidenceId, mapping);
    }
    // -------------------------------------------------------------------------


    // -------------------------------------------------------------------------
    class CenterFrameCnBuilderNode : public Node {
    public:
	static const Core::ParameterString paramConfKey;
	static const Core::ParameterBool paramMap;
    private:
	u32 n_;
	Key confidenceKey_;
	FwdBwdBuilderRef fbBuilder_;
	CenterFrameCnBuilderRef cnBuilder_;
	bool map_;

	ConstLatticeRef union_;
	ConstFwdBwdRef fb_;
	ConstPosteriorCnRef fCn_;
	ConstCenterFrameCnRef feCn_;
	ConstConfusionNetworkRef posteriorCn_;
	ConstConfusionNetworkRef cn_;
	ConstLatticeRef best_;

	ConstSemiringRef lastSemiring_;
	ScoreId confidenceId_;
    private:
	CenterFrameCnBuilderRef getCenterFrameCnBuilder(ConstLatticeRef l) {
	    if (!cnBuilder_) {
		cnBuilder_ = CenterFrameCnBuilder::create();
		cnBuilder_->dump(log());
	    }
	    return cnBuilder_;
	}

	void computeConfidenceId(ConstSemiringRef semiring) {
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

	void computeFwdBwd() {
	    if (!fb_) {
		ConstLatticeRefList lats(n_);
		for (u32 i = 0; i < n_; ++i)
		    lats[i] = requestLattice(i);
		std::pair<ConstLatticeRef, ConstFwdBwdRef> fbResult = (n_ == 1) ? fbBuilder_->build(lats.front()) : fbBuilder_->build(lats);
		union_ = fbResult.first; fb_ = fbResult.second;
	    }
	}

	void computeFCn() {
	    if (!fCn_) {
		computeFwdBwd();
		fCn_ = buildFramePosteriorCn(union_, fb_);
	    }
	}

	void computeCenterFrameCn() {
	    if (!feCn_) {
		computeFwdBwd();
		computeFCn();
		feCn_ = getCenterFrameCnBuilder(union_)->build(union_, fb_, fCn_);

		/*
		FrameList supportFrames = feCn_->getSupportFrames();
		Core::Component::Message msg = log();
		if (supportFrames.empty()) {
		    msg << Core::XmlEmpty("support-frames");
		} else {
		    msg << Core::XmlOpen("support-frames");
		    msg << supportFrames.front();
		    for (FrameList::const_iterator itFrame = supportFrames.begin() + 1; itFrame != supportFrames.end(); ++itFrame)
			msg << ", " << *itFrame;
		    msg << Core::XmlClose("support-frames");
		}
		*/

	    }
	}

	ConstConfusionNetworkRef getCn() {
	    if (!cn_) {
		computeCenterFrameCn();
		computeConfidenceId(feCn_->getLattice()->semiring());
		cn_ = feCn_->getCn(confidenceId_, map_);
	    }
	    return cn_;
	}

	ConstConfusionNetworkRef getNormalizedCn() {
	    if (!posteriorCn_) {
		computeCenterFrameCn();
		computeConfidenceId(feCn_->getLattice()->semiring());
		std::pair<ConstConfusionNetworkRef, ConstLatticeRef> result = feCn_->getNormalizedCn(confidenceId_, map_);
		posteriorCn_ = result.first; best_ = result.second;
		verify(posteriorCn_->isNormalized());
	    }
	    return posteriorCn_;
	}

	ConstLatticeRef getBestLattice() {
	    if (!best_) {
		computeCenterFrameCn();
		computeConfidenceId(feCn_->getLattice()->semiring());
		std::pair<ConstConfusionNetworkRef, ConstLatticeRef> result = feCn_->getNormalizedCn(confidenceId_, map_);
		posteriorCn_ = result.first; best_ = result.second;
		verify(posteriorCn_->isNormalized());
	    }
	    return best_;
	}

	ConstLatticeRef getUnionLattice() {
	    computeFwdBwd();
	    return union_;
	}

	ConstPosteriorCnRef getFCn() {
	    computeFCn();
	    return fCn_;
	}

    public:
	CenterFrameCnBuilderNode(const std::string &name, const Core::Configuration &config) :
	    Node(name, config), n_(0) {  confidenceId_ = Semiring::InvalidId; }
	virtual ~CenterFrameCnBuilderNode() {}

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
		return cn2lattice(getNormalizedCn());
	    case 4:
		return cn2lattice(getCn());
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

	virtual ConstPosteriorCnRef sendPosteriorCn(Port to) {
	    switch (to) {
	    case 5:
		return getFCn();
	    default:
		defect();
		return ConstPosteriorCnRef();
	    }
	}

	virtual void sync() {
	    best_.reset();
	    feCn_.reset();
	    posteriorCn_.reset();
	    cn_.reset();
	    fCn_.reset();
	    fb_.reset();
	    union_.reset();
	    // dbg
	    // cnBuilder_.reset();
	}
    };
    const Core::ParameterString CenterFrameCnBuilderNode::paramConfKey(
	"confidence-key",
	"store confidence score",
	"");
    const Core::ParameterBool CenterFrameCnBuilderNode::paramMap(
	"map",
	"map lattice to CN and vice versa",
	false);

    NodeRef createCenterFrameCnBuilderNode(const std::string &name, const Core::Configuration &config) {
	return NodeRef(new CenterFrameCnBuilderNode(name, config));
    }
    // -------------------------------------------------------------------------

} // namespace

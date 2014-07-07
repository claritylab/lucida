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
#include <Core/Vector.hh>
#include <Fsa/Types.hh>

#include "FlfCore/Basic.hh"
#include "FlfCore/LatticeInternal.hh"
#include "FlfCore/TopologicalOrderQueue.hh"
#include "FlfCore/Traverse.hh"
#include "Lexicon.hh"
#include "Map.hh"
#include "Miscellaneous.hh"
#include "Segment.hh"

namespace Flf {

    // -------------------------------------------------------------------------
    /**
     * Fits the lattice into [begin, end]:
     * - begin and end are relative to the lattices initial state
     * - fitting lattice has exactly one initial state with time 0 and one
     *   final state with time end-begin, i.e. all pathes have the same length
     * - the time of all states but the final state is less than end (and >= 0)
     * - for each path the in origianl lattice there exist a path with
     *   the same score in the fitting lattice; those are the only pathes
     *   in the fitting lattice (i.e. according to the begin and end times
     *   words at the beginning or end of a path might be discarded).
     * - optional: the last arc carries the sentence-end-symbol
     * - if begin and end time are not given, then begin is 0 and end time
     *   is the max time over all states.
     **/
    class FittingLattice : public SlaveLattice {
    public:
	class Boundaries : public Flf::Boundaries {
	    friend class FittingLattice;
	private:
	    ConstBoundariesRef srcBoundaries_;
	    s32 startTime_, endTime_;
	    mutable Core::Vector<Boundary> boundaries_;
	    Boundary initialBoundary_;
	    Boundary finalBoundary_;
	    Boundary finalTm1Boundary_;

	private:
	    Boundaries(
		ConstBoundariesRef srcBoundaries,
		s32 startTime, s32 endTime) :
		srcBoundaries_(srcBoundaries),
		startTime_(startTime),
		endTime_(endTime),
		boundaries_(0) {
		initialBoundary_.setTime(0);
		finalBoundary_.setTime(endTime_ - startTime_);
		finalTm1Boundary_.setTime((startTime_ == endTime_) ? InvalidTime : Time(endTime_ - startTime_ - 1));
	    }
	    virtual bool valid() const {
		return true;
	    }
	    virtual bool valid(Fsa::StateId sid) const {
		bool isValid = false;
		if ((sid == 0) || (sid == 1))
		    isValid = true;
		else
		    isValid = srcBoundaries_->valid(sid - 2);
		return isValid;
	    }
	    virtual const Boundary & get(Fsa::StateId sid) const {
		switch (sid) {
		case 0:
		    return initialBoundary_;
		case 1:
		    return finalBoundary_;
		default:
		    Fsa::StateId srcSid = sid - 2;
		    const Boundary &srcB = srcBoundaries_->get(srcSid);
		    s32 srcT = srcB.time();
		    if (srcT < endTime_) {
			verify(srcT >= startTime_);
			if (startTime_ != 0) {
			    boundaries_.grow(srcSid);
			    Boundary &b = boundaries_[srcSid];
			    b = srcB; b.setTime(srcT - startTime_);
			    return b;
			} else
			    return srcB;
		    } else
			return finalTm1Boundary_;
		}
	    }
	};
	friend class Boundaries;

	typedef enum {
	    ActionNone,
	    ActionAddArc,
	    ActionAddFinalArc,
	    ActionAddSentenceEndArc
	} Action;

    private:
	s32 startTime_, endTime_;
	bool forceSentenceEndSymbol_;

	ConstStateMapRef topologicalOrderMap_;
	ConstSemiringRef semiring_;
	ConstStateRef initialSr_, finalSr_;
	Fsa::LabelId sentenceEndInput_, sentenceEndOutput_;

	Flf::ConstBoundariesRef srcBoundaries_;

	mutable TopologicalOrderQueueRef topologicalQeue_;
	mutable Core::Vector<ScoresRef> ssspScores_;

    protected:
	ScoresRef sssp(Fsa::StateId initSid) const {
	    ScoresRef ssspScore = semiring_->zero();
	    ssspScores_.grow(topologicalOrderMap_->maxSid);
	    TopologicalOrderQueue &Q = *topologicalQeue_;
	    Q.insert(initSid);
	    ssspScores_[initSid] = semiring_->one();
	    const Lattice &l = *fsa_;
	    while (!Q.empty()) {
		Fsa::StateId sid = Q.top(); Q.pop();
		ConstStateRef sr = l.getState(sid);
		ScoresRef score = ssspScores_[sid];
		ssspScores_[sid].reset();
		if (sr->isFinal())
		    ssspScore = semiring_->collect(
			ssspScore, semiring_->extend(score, sr->weight()));
		for (State::const_iterator a = sr->begin(), a_end = sr->end(); a != a_end; ++a) {
		    if (!ssspScores_[a->target()]) {
			ssspScores_[a->target()] = semiring_->extend(score, a->weight());
			Q.insert(a->target());
		    } else
			ssspScores_[a->target()] = semiring_->collect(
			    ssspScores_[a->target()], semiring_->extend(score, a->weight()));
		}
	    }
	    return ssspScore;
	}

	inline void addArc(Action action, State *from, const State *to, ScoresRef score, Fsa::LabelId input, Fsa::LabelId output) const {
	    switch (action) {
	    case ActionAddArc:
		from->newArc(to->id() + 2, score, input, output);
		break;
	    case ActionAddFinalArc: {
		ScoresRef finalScore = to->hasArcs() ? sssp(to->id()) : (to->isFinal() ? to->weight() : semiring_->zero());
		// true if lattice is trim
		// verify(semiring_->compare(finalScore, semiring_->zero()) != 0);
		from->newArc(1, semiring_->extend(score, finalScore), input, output);
		break; }
	    case ActionAddSentenceEndArc: {
		ScoresRef finalScore = to->hasArcs() ? sssp(to->id()) : (to->isFinal() ? to->weight() : semiring_->zero());
		// true if lattice is trim
		// verify(semiring_->compare(finalScore, semiring_->zero()) != 0);
		from->newArc(1, semiring_->extend(score, finalScore), sentenceEndInput_, sentenceEndOutput_);
		break; }
	    default:
		defect();
	    }
	}

	State * buildInitialState(State *sp) const {
	    Fsa::StateId srcInitialSid = fsa_->initialStateId();
	    s32 initialT = srcBoundaries_->time(srcInitialSid);
	    verify(initialT == 0);
	    if (startTime_ < 0) {
		sp->newArc(srcInitialSid + 2, semiring_->clone(semiring_->one()), Fsa::Epsilon, Fsa::Epsilon);
	    } else if (startTime_ == 0) {
		buildState(sp, srcInitialSid);
	    } else {
		TopologicalOrderQueueRef initialTopologicalQeue = createTopologicalOrderQueue(fsa_, topologicalOrderMap_);
		TopologicalOrderQueue &Q = *initialTopologicalQeue;
		Core::Vector<ScoresRef> initialSsspScores(topologicalOrderMap_->maxSid + 1);
		Q.insert(srcInitialSid);
		initialSsspScores[srcInitialSid] = semiring_->one();
		const Lattice &l = *fsa_;
		bool hasFinal = false;
		ScoresRef finalScore = semiring_->zero();
		while (!Q.empty()) {
		    Fsa::StateId sid = Q.top(); Q.pop();
		    ConstStateRef sr = l.getState(sid);
		    s32 t = srcBoundaries_->time(sid);
		    if (t >= endTime_)
			t = endTime_ - 1;
		    ScoresRef score = initialSsspScores[sid];
		    initialSsspScores[sid].reset();
		    if (sr->isFinal()) {
			hasFinal = true;
			finalScore = semiring_->collect(
			    finalScore, semiring_->extend(score, sr->weight()));
		    }
		    for (State::const_iterator a = sr->begin(), a_end = sr->end(); a != a_end; ++a) {
			ConstStateRef targetSr = fsa_->getState(a->target());
			s32 targetT = srcBoundaries_->time(a->target());
			if (targetT > startTime_) {
			    ScoresRef arcScore = semiring_->extend(score, a->weight());
			    Action action = ActionNone;
			    if (forceSentenceEndSymbol_) {
				if (t == endTime_ - 1)
				    action = ActionAddSentenceEndArc;
				else if (((targetT >= endTime_) || (targetSr->isFinal() && !targetSr->hasArcs())) &&
					 ((a->input() == sentenceEndInput_) || (a->output() == sentenceEndOutput_)))
				    action = ActionAddFinalArc;
				else
				    action = ActionAddArc;
			    } else {
				if ((targetT >= endTime_) ||
				    (targetSr->isFinal() && !targetSr->hasArcs() && ((a->input() == Fsa::Epsilon) && (a->output() == Fsa::Epsilon))))
				    action = ActionAddFinalArc;
				else
				    action = ActionAddArc;
			    }
			    addArc(action, sp, targetSr.get(), arcScore, a->input(), a->output());
			} else {
			    if (!initialSsspScores[a->target()]) {
				initialSsspScores[a->target()] = semiring_->extend(score, a->weight());
				Q.insert(a->target());
			    } else
				initialSsspScores[a->target()] = semiring_->collect(
				    initialSsspScores[a->target()], semiring_->extend(score, a->weight()));
			}
		    }
		}
		if (hasFinal)
		    sp->newArc(1, finalScore, sentenceEndInput_, sentenceEndOutput_);
	    }
	    return sp;
	}

	State * buildFinalState(State *sp) const {
	    sp->setFinal(semiring_->one());
	    return sp;
	}

	State * buildState(State *sp, Fsa::StateId srcSid) const {
	    const Lattice &srcL = *fsa_;
	    ConstStateRef srcSr = srcL.getState(srcSid);
	    s32 srcT = srcBoundaries_->time(srcSid);
	    if (srcT >= endTime_)
		srcT = endTime_ - 1;
	    if (srcSr->isFinal())
		sp->newArc(1, srcSr->weight(), sentenceEndInput_, sentenceEndOutput_);
	    for (State::const_iterator a = srcSr->begin(), a_end = srcSr->end(); a != a_end; ++a) {
		ConstStateRef targetSr = srcL.getState(a->target());
		s32 targetT = srcBoundaries_->time(a->target());
		Action action = ActionNone;
		if (forceSentenceEndSymbol_) {
		    if (srcT == endTime_ - 1)
			action = ActionAddSentenceEndArc;
		    else if (((targetT >= endTime_) || (targetSr->isFinal() && !targetSr->hasArcs())) &&
			     ((a->input() == sentenceEndInput_) && (a->output() == sentenceEndOutput_)))
			action = ActionAddFinalArc;
		    else
			action = ActionAddArc;
		} else {
		    if ((targetT >= endTime_) ||
			(targetSr->isFinal() && !targetSr->hasArcs() && ((a->input() == Fsa::Epsilon) && (a->output() == Fsa::Epsilon))))
			action = ActionAddFinalArc;
		    else
			action = ActionAddArc;
		}
		addArc(action, sp, targetSr.get(), a->weight(), a->input(), a->output());
	    }
	    return sp;
	}

    public:
	FittingLattice(ConstLatticeRef l, s32 startTime, s32 endTime, bool forceSentenceEndSymbol) :
	    SlaveLattice(l), startTime_(startTime), endTime_(endTime), forceSentenceEndSymbol_(forceSentenceEndSymbol) {
	    topologicalOrderMap_ = findTopologicalOrder(l);
	    verify(topologicalOrderMap_ && (topologicalOrderMap_->maxSid != Fsa::InvalidStateId));
	    if (startTime_ == Core::Type<s32>::max)
		startTime_ = 0;
	    if (endTime_ == Core::Type<s32>::max) {
		endTime_ = 0;
		Fsa::StateId sid = 0;
		for (StateMap::const_iterator it = topologicalOrderMap_->begin(), it_end = topologicalOrderMap_->end();
		     it != it_end; ++it, ++sid)
		    if (*it != Fsa::InvalidStateId)
			endTime_ = std::max(endTime_, s32(srcBoundaries_->time(sid)));
	    }
	    verify((0 <= endTime_) && (startTime_ <= endTime_));
	    if (forceSentenceEndSymbol_) {
		switch (Lexicon::us()->alphabetId(l->getInputAlphabet())) {
		case Lexicon::LemmaAlphabetId:
		    sentenceEndInput_ = Lexicon::us()->sentenceEndLemmaId();
		    break;
		case Lexicon::LemmaPronunciationAlphabetId:
		    sentenceEndInput_ = Lexicon::us()->sentenceEndLemmaPronunciationId();
		    break;
		default:
		    sentenceEndInput_ = Fsa::Epsilon;
		}
		sentenceEndOutput_ = (l->type() == Fsa::TypeAcceptor) ? sentenceEndInput_ : Fsa::Epsilon;
	    } else
		sentenceEndInput_ = sentenceEndOutput_ = Fsa::Epsilon;
	    semiring_ = l->semiring();
	    topologicalQeue_ = createTopologicalOrderQueue(l, topologicalOrderMap_);
	    srcBoundaries_ = l->getBoundaries();
	    setBoundaries(Flf::ConstBoundariesRef(new Boundaries(srcBoundaries_, startTime_, endTime_)));
	    if (startTime_ == endTime_) {
		ScoresRef finalScores = sssp(fsa_->initialStateId());
		if (semiring_->compare(finalScores, semiring_->one()) != 0)
		    Core::Application::us()->error(
			"Fitting lattice: Result is a single state lattice with final weight %s != %s;\n"
			"discard final weight.",
			semiring_->describe(finalScores, Fsa::HintShowDetails | HintUnscaled).c_str(),
			semiring_->describe(semiring_->one(), Fsa::HintShowDetails | HintUnscaled).c_str());
		initialSr_ = ConstStateRef(new State(0, Fsa::StateTagFinal, semiring_->one()));
	    } else if ((startTime_ + 1 == endTime_) && forceSentenceEndSymbol_) {
		State *sp = new State(0);
		sp->newArc(1, sssp(fsa_->initialStateId()), sentenceEndInput_, sentenceEndOutput_);
		initialSr_ = ConstStateRef(sp);
		finalSr_ = ConstStateRef(buildFinalState(new State(1)));
	    } else {
		initialSr_ = ConstStateRef(buildInitialState(new State(0)));
		finalSr_ = ConstStateRef(buildFinalState(new State(1)));
	    }
	}
	virtual ~FittingLattice() {}

	virtual Fsa::StateId initialStateId() const {
	    return 0;
	}

	virtual ConstStateRef getState(Fsa::StateId sid) const {
	    switch (sid) {
	    case 0:
		return initialSr_;
	    case 1:
		return finalSr_;
	    default:
		Fsa::StateId srcSid = sid - 2;
		s32 t = srcBoundaries_->time(srcSid);
		State * sp = new State(sid);
		if (t >= endTime_) {
		    ScoresRef finalScore = sssp(srcSid);
		    ConstStateRef srcSr = fsa_->getState(srcSid);
		    if (srcSr->isFinal())
			finalScore = semiring_->collect(srcSr->weight(), finalScore);
		    sp->newArc(1, finalScore, sentenceEndInput_, sentenceEndOutput_);
		} else {
		    buildState(sp, srcSid);
		}
		verify(!sp->isFinal());
		return ConstStateRef(sp);
	    }
	}

	virtual std::string describe() const {
	    return Core::form("fit-lattice(%s,%d-%d)", fsa_->describe().c_str(), startTime_, endTime_);
	}
    };

    ConstLatticeRef fit(ConstLatticeRef l, bool forceSentenceEndLabels) {
	return fit(l, Core::Type<s32>::max, Core::Type<s32>::max, forceSentenceEndLabels);
    }

    ConstLatticeRef fit(ConstLatticeRef l, s32 startTime, s32 endTime, bool forceSentenceEndLabels) {
	verify_(l->hasProperty(Fsa::PropertyAcyclic));
	return ConstLatticeRef(new FittingLattice(l, startTime, endTime, forceSentenceEndLabels));
    }
    // -------------------------------------------------------------------------



    // -------------------------------------------------------------------------
    class FitLatticeNode : public FilterNode {
	friend class Network;
    public:
	static const Core::ParameterBool paramForceSentenceEndLabels;
    private:
	bool forceSentenceEndLabels_;
    protected:
	virtual ConstLatticeRef filter(ConstLatticeRef l) {
	    if (!l)
		return ConstLatticeRef();
	    s32 startTime = Core::Type<s32>::max, endTime = Core::Type<s32>::max;
	    if (connected(1)) {
		ConstSegmentRef segment = requestSegment(1);
		if (!segment->hasStartTime() || !segment->hasEndTime()) {
		    criticalError("Segment \"%s\" has no start and end time.",
				  segment->segmentId().c_str());
		} else {
		    startTime = 0;
		    endTime = nFrames(segment);
		}
	    }
	    return fit(l, startTime, endTime, forceSentenceEndLabels_);
	}
    public:
	FitLatticeNode(const std::string &name, const Core::Configuration &config) :
	    FilterNode(name, config) {}
	virtual ~FitLatticeNode() {}

	void init(const std::vector<std::string> &arguments) {
	    forceSentenceEndLabels_ = paramForceSentenceEndLabels(config);
	    if (connected(1))
		log("Get lattice end time from segment at port 1.");
	}

	virtual ConstSegmentRef sendSegment(Port to) {
	    verify((to == 1) && connected(1));
	    return requestSegment(1);
	}
    };
    const Core::ParameterBool FitLatticeNode::paramForceSentenceEndLabels(
	"force-sentence-end-labels",
	"put sentence end labels at final arcs",
	false);
    NodeRef createFitLatticeNode(const std::string &name, const Core::Configuration &config) {
	return NodeRef(new FitLatticeNode(name, config));
    }
    // -------------------------------------------------------------------------


    // -------------------------------------------------------------------------
    bool isValidTarget(Fsa::StateId sid) {
	if (sid == Fsa::InvalidStateId)
	    return false;
	return true;
    }
    bool isValidLabel(Fsa::ConstAlphabetRef alphabet, Fsa::LabelId label) {
	if (label == Fsa::InvalidLabelId)
	    return false;
	return true;
    }
    bool isFinite(ConstSemiringRef semiring, ScoresRef scores) {
	for (Scores::const_iterator w = scores->begin(), w_end = scores->begin() + semiring->size(); w != w_end; ++w)
	    if ((*w == Semiring::Zero) || (*w == Semiring::Invalid) || (std::isfinite(*w) == 0))
		return false;
	return true;
    }
    namespace { typedef enum { None, White, Gray, Black } Color; }
    StaticLatticeRef cleanUp(ConstLatticeRef l) {
	if (l->initialStateId() == Fsa::InvalidStateId)
	    return StaticLatticeRef();

	StaticLattice *s = new StaticLattice;
	s->setDescription("cleaned-up(" + l->describe() + ")");
	s->setType(l->type());
	s->setProperties(l->knownProperties(), l->properties());
	s->addProperties(Fsa::PropertyAcyclic);
	s->setInputAlphabet(l->getInputAlphabet());
	if (l->type() == Fsa::TypeTransducer)
	    s->setOutputAlphabet(l->getOutputAlphabet());
	s->setSemiring(l->semiring());
	s->setBoundaries(l->getBoundaries());
	s->setInitialStateId(l->initialStateId());

	const bool isAcceptor = (l->type() == Fsa::TypeAcceptor);
	Fsa::ConstAlphabetRef
	    inputA = l->getInputAlphabet(),
	    outputA = isAcceptor ? l->getInputAlphabet() : l->getOutputAlphabet();
	ConstSemiringRef semiring = l->semiring();

	Core::Vector<Color> C;
	Core::Vector<Fsa::StateId> S;
	C.grow(l->initialStateId(), None);
	S.push_back(l->initialStateId());
	C[l->initialStateId()] = White;
	std::vector<bool> isValidArc;
	while (!S.empty()) {
	    Fsa::StateId sid = S.back();
	    if (C[sid] == White) {
		C[sid] = Gray;
		ConstStateRef sr = l->getState(sid);
		if (!sr)
		    Core::Application::us()->criticalError("State %d does not exist.", sid);
		State *sp = 0;
		bool isValidArc = true;
		for (State::const_iterator a = sr->begin(); a != sr->end(); ++a) {
		    if (isValidTarget(a->target())) {
			C.grow(a->target(), None);
			if (C[a->target()] == None) {
			    S.push_back(a->target());
			    C[a->target()] = White;
			}
			if (isValidArc) {
			    if ((C[a->target()] == Gray)
				|| !isValidLabel(inputA, a->input())
				|| !isValidLabel(outputA, a->output())
				|| (isAcceptor && (a->input() != a->output()))
				|| !isFinite(semiring, a->weight()))
				isValidArc = false;
			}
		    } else
			isValidArc = false;
		}
		if (!isValidArc) {
		    u32 nDiscarded = 0, nCorrected = 0;
		    sp = new State(sr->id(), sr->tags(), sr->weight());
		    for (State::const_iterator a = sr->begin(); a != sr->end(); ++a) {
			bool discard = false;
			if (!isValidTarget(a->target())) {
			    Core::Application::us()->warning("Arc has invalid target.");
			    discard = true;
			} else if (C[a->target()] == Gray) {
			    Core::Application::us()->warning("Arc closes a cycle.");
			    discard = true;
			}
			if (!isValidLabel(inputA, a->input())) {
			    Core::Application::us()->warning("Arc has invalid input.");
			    discard = true;
			}
			if (!isValidLabel(outputA, a->output())) {
			    Core::Application::us()->warning("Arc has invalid ouput.");
			    discard = true;
			}
			if (!isFinite(semiring, a->weight())) {
			    Core::Application::us()->warning("Arc weight is not finite.");
			    discard = true;
			}
			if (discard) {
			    ++nDiscarded;
			    Core::Application::us()->warning(
				"Discard arc from=%d,to=%d,i/o=%s(%d):%s(%d),w=%s",
				sr->id(), a->target(),
				inputA->symbol(a->input()).c_str(), a->input(),
				outputA->symbol(a->output()).c_str(), a->output(),
				semiring->describe(a->weight(), Fsa::HintShowDetails | HintUnscaled).c_str());
			} else {
			    Arc &newArc = (*sp->newArc() = *a);
			    if (isAcceptor && (a->input() != a->output())) {
				++nCorrected;
				Core::Application::us()->warning("Acceptor arc has different input and output; correct.");
				newArc.output_ = newArc.input_;
			    }
			}
		    }
		    verify(nDiscarded + nCorrected > 0);
		    Core::Application::us()->log(
			"For state %d: %d arcs discarded and %d arcs corrected of %d original arcs",
			sr->id(), sr->nArcs(), nDiscarded, nCorrected);
		} else
		    sp = new State(*sr);
		s->setState(sp);
	    } else {
		verify(C[sid] == Gray);
		C[sid] = Black;
		S.pop_back();
	    }
	}
	StaticLatticeRef sRef = StaticLatticeRef(s);
	trimInPlace(sRef);
	if (sRef->initialStateId() == Fsa::InvalidStateId)
	    return StaticLatticeRef();
	return sRef;
    }

    class CleanUpNode : public FilterNode {
	friend class Network;
    protected:
	virtual ConstLatticeRef filter(ConstLatticeRef l) {
	    if (!l)
		return ConstLatticeRef();
	    return cleanUp(l);
	}
    public:
	CleanUpNode(const std::string &name, const Core::Configuration &config) :
	    FilterNode(name, config) {}
	virtual ~CleanUpNode() {}
	void init(const std::vector<std::string> &arguments) {}
    };
    NodeRef createCleanUpNode(const std::string &name, const Core::Configuration &config) {
	return NodeRef(new CleanUpNode(name, config));
    }
    // -------------------------------------------------------------------------

} // namespace Flf

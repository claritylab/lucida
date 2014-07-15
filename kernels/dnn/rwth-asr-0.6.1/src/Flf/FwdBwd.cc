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
#include <Core/Component.hh>
#include <Core/Parameter.hh>
#include <Core/Utility.hh>
#include <Lm/ScaledLanguageModel.hh>

#include "FlfCore/Utility.hh"
#include "FlfCore/Traverse.hh"
#include "FwdBwd.hh"
#include "Lexicon.hh"


#include <Fsa/Basic.hh>
#include <Fsa/Sssp.hh>
#include <Fsa/Static.hh>
#include "FlfCore/Basic.hh"
#include "Copy.hh"
#include "Filter.hh"


namespace Flf {

    // -------------------------------------------------------------------------
    typedef Core::Ref<FwdBwd> FwdBwdRef;

    FwdBwd::State::State() : begin_(0), end_(0) {}

    FwdBwd::Arc::Arc() {}

    struct FwdBwd::Internal {
	ConstSemiringRefList semirings;
	FwdBwd::State *states;
	FwdBwd::Arc *arcs;
	f64 min, max, sum;
	Internal(const ConstSemiringRefList &semirings) :
	    semirings(semirings), states(0), arcs(0),
	    min(Core::Type<f64>::max),
	    max(Core::Type<f64>::min),
	    sum(Core::Type<f64>::min) {}
	~Internal() {
	    delete [] states;
	    delete [] arcs;
	}
    };

    FwdBwd::FwdBwd() :
	internal_(0) {}

    FwdBwd::~FwdBwd() {
	delete internal_;
    }

    s32 FwdBwd::n() const {
	return internal_->semirings.size();
    }

    const ConstSemiringRefList & FwdBwd::semirings() const {
	return internal_->semirings;
    }

    f64 FwdBwd::min() const {
	return internal_->min;
    }

    f64 FwdBwd::max() const {
	return internal_->max;
    }

    f64 FwdBwd::sum() const {
	return internal_->sum;
    }

    const FwdBwd::State & FwdBwd::state(Fsa::StateId sid) const {
	return internal_->states[sid];
    }

    class FwdBwd::Builder {
    protected:
	// negative logs of [0.99, 1.01]
	static const std::pair<f64, f64> OOneInterval;
	// negative logs of [0.95, 1.05]
	static const std::pair<f64, f64> OFiveInterval;

	struct Properties {
	    Fsa::StateId offset;
	    u32 nInitialArcs, nArcs;
	    Time startTime, endTime;
	    Core::Vector<std::pair<u32, u32> > fanInOut;
	    Core::Vector<Fsa::StateId> finalStateIds;
	    Core::Ref<StateMap> topologicalSort;
	    Properties() : offset(0), nInitialArcs(0), nArcs(0), startTime(0), endTime(0) {}
	};

	class TraverseLatticeProperties : protected DfsState {
	protected:
	    ConstLatticeRef l;
	    ConstBoundariesRef b;
	    Properties &properties;
	protected:
	    virtual void discoverState(ConstStateRef sr) {
		Fsa::StateId sid = sr->id();
		if (sr->isFinal()) {
		    if (sr->hasArcs())
			Core::Application::us()->error(
			    "In Fwd./Bwd. Algorithm: Lattice \"%s\"has final state with outgoing arcs, "
			    "ignore final status", fsa_->describe().c_str());
		    else
			properties.finalStateIds.push_back(sid);
		    Time t = b->time(sid);
		    if (t > properties.endTime) properties.endTime = t;
		} else if (!sr->hasArcs())
		    Core::Application::us()->criticalError(
			"In Fwd./Bwd. Algorithm: Lattice \"%s\"is not trim.", fsa_->describe().c_str());
		properties.nArcs += sr->nArcs();
		verify_(sid < properties.fanInOut.size());
		properties.fanInOut[sid].first = sr->nArcs();
		for (Flf::State::const_iterator a = sr->begin(), a_end = sr->end(); a != a_end; ++a) {
		    properties.fanInOut.grow(a->target(), std::make_pair(0, 0));
		    ++properties.fanInOut[a->target()].second;
		}
	    }

	    virtual void exploreNonTreeArc(Flf::ConstStateRef from, const Flf::Arc &a) {
		if (color(a.target()) == Gray)
		    Core::Application::us()->criticalError(
			"In Fwd./Bwd. Algorithm: Lattice \"%s\"is not acyclic.",
			fsa_->describe().c_str());
	    }

	    virtual void finishState(Fsa::StateId sid) {
		properties.topologicalSort->push_back(sid);
		properties.topologicalSort->maxSid = std::max(properties.topologicalSort->maxSid, sid);
	    }

	    void traverse() {
		Fsa::StateId initialSid = l->initialStateId();
		verify(initialSid != Fsa::InvalidStateId);
		properties.topologicalSort = Core::Ref<StateMap>(new StateMap);
		properties.topologicalSort->maxSid = initialSid;
		properties.fanInOut.grow(initialSid, std::make_pair(0, 0));
		properties.nInitialArcs = fsa_->getState(initialSid)->nArcs();
		properties.nArcs = 0;
		properties.startTime = b->time(initialSid);
		dfs();
		if (properties.finalStateIds.empty())
		    Core::Application::us()->criticalError(
			"In Fwd./Bwd. Algorithm: Lattice \"%s\" has no final state.",
			fsa_->describe().c_str());
		std::reverse(properties.topologicalSort->begin(), properties.topologicalSort->end());
		fsa_->setTopologicalSort(properties.topologicalSort);
	    }

	public:
	    TraverseLatticeProperties(
		ConstLatticeRef l,
		Properties &properties) :
		DfsState(l),
		l(l), b(l->getBoundaries()),
		properties(properties) {
		verify(!l->knowsProperty(Fsa::PropertyAcyclic) ||
		       l->hasProperty(Fsa::PropertyAcyclic));
	    }
	    virtual ~TraverseLatticeProperties() {}
	};

	class TraverseLattice : public TraverseLatticeProperties {
	    typedef TraverseLatticeProperties Precursor;
	protected:
	    StaticLattice &staticL;
	    StaticBoundaries &staticB;
	protected:
	    virtual void discoverState(ConstStateRef sr) {
		Precursor::discoverState(sr);
		staticL.setState(new Flf::State(*sr));
		staticB.set(sr->id(), b->get(sr->id()));
	    }
	public:
	    TraverseLattice(
		ConstLatticeRef l,
		Properties &properties,
		StaticLattice &staticL,
		StaticBoundaries &staticB) :
		Precursor(l, properties),
		staticL(staticL), staticB(staticB) {
		traverse();
	    }
	    virtual ~TraverseLattice() {}
	};

	class TraverseSubLattice : public TraverseLatticeProperties {
	    typedef TraverseLatticeProperties Precursor;
	protected:
	    StaticLattice &staticL;
	    StaticBoundaries &staticB;
	protected:
	    virtual void discoverState(ConstStateRef sr) {
		Precursor::discoverState(sr);
		Flf::State *sp = new Flf::State(*sr);
		Fsa::StateId unionSid = sr->id() + properties.offset;
		sp->setId(unionSid);
		for (Flf::State::iterator a = sp->begin(), a_end = sp->end(); a != a_end; ++a)
		    a->target_ += properties.offset;
		staticL.setState(sp);
		staticB.set(unionSid, b->get(sr->id()));
	    }
	public:
	    TraverseSubLattice(
		ConstLatticeRef l,
		Properties &properties,
		StaticLattice &staticL,
		StaticBoundaries &staticB) :
		Precursor(l, properties),
		staticL(staticL),
		staticB(staticB) {
		traverse();
	    }
	    virtual ~TraverseSubLattice() {}
	};

	struct ScoreArc {
	    Fsa::LabelId target;
	    f64 score;
	    f64 cost;
	    ScoreArc() {}
	};
	struct ScoreState {
	    f64 score;
	    f64 fwdScore, bwdScore;
	    f64 cost;
	    f64 genFwdScore, genBwdScore;
	    ScoreArc *fwdBegin, *fwdEnd;
	    ScoreArc *bwdBegin, *bwdEnd;
	    ScoreState() {}
	};

    public:
	/*
	  Make static copy of lattice and
	  calculate normalized fwd./bwd. scores.
	*/
	ConstLatticeRef operator() (ConstLatticeRef l, const FwdBwd::Parameters &params, FwdBwd *fb = 0) {
	    if (!l || (l->initialStateId() == Fsa::InvalidStateId))
		return ConstLatticeRef();
	    ConstSemiringRef posteriorSemiring = params.posteriorSemiring;
	    if (!posteriorSemiring)
		posteriorSemiring = toLogSemiring(l->semiring(), params.alpha);

	    //dbg("Lattice:  " << l->describe());
	    //dbg("Semiring: " << posteriorSemiring->name());

	    bool hasRisk = (params.riskId != Semiring::InvalidId);
	    bool hasScores = ((params.scoreId != Semiring::InvalidId) || (params.riskId != Semiring::InvalidId));
	    /*
	      Static lattice properties and skeleton
	    */
	    StaticLatticeRef s = StaticLatticeRef(new StaticLattice(l->type()));
	    s->setSemiring(l->semiring());
	    s->setInputAlphabet(l->getInputAlphabet());
	    if (l->type() != Fsa::TypeAcceptor)
		s->setOutputAlphabet(l->getOutputAlphabet());
	    StaticBoundariesRef b = StaticBoundariesRef(new StaticBoundaries);
	    s->setBoundaries(b);
	    s->setDescription("static(" + l->describe() + ")");
	    s->setProperties(l->knownProperties(), l->properties());
	    /*
	      Make lattice persistent and calculate some statistics
	    */
	    Properties properties;
	    TraverseLattice traverse(l, properties, *s, *b);
	    ConstStateMapRef topologicalSort = properties.topologicalSort;
	    s->setInitialStateId(topologicalSort->front());
	    /*
	      Data structures
	    */
	    // normalized fwd./bwd. scores
	    FwdBwd::Internal *fbScores = 0;
	    FwdBwd::Arc *nextFwdBwdArc = 0, *endFwdBwdArc = 0;
	    if (fb) {
		fbScores = fb->internal_ = new FwdBwd::Internal(ConstSemiringRefList(1, posteriorSemiring));
		fbScores->states = new FwdBwd::State[topologicalSort->maxSid + 1];
		fbScores->arcs = new FwdBwd::Arc[properties.nArcs];
		nextFwdBwdArc = fbScores->arcs; endFwdBwdArc = fbScores->arcs + properties.nArcs;
	    }
	    // temporary data structure
	    ScoreState *stateScores = new ScoreState[topologicalSort->maxSid + 1];
	    ScoreArc *fwdArcScores = new ScoreArc[properties.nArcs];
	    ScoreArc *bwdArcScores = new ScoreArc[properties.nArcs];
	    /*
	      Calculate fwd./bwd. probabilities
	    */
	    Collector *col = createCollector(Fsa::SemiringTypeLog);
	    CostCollector *colCost = (hasRisk) ? CostCollector::create() : 0;
	    ScoreArc *nextFwdArcScores = fwdArcScores, *endFwdArcScores = fwdArcScores + properties.nArcs;
	    ScoreArc *nextBwdArcScores = bwdArcScores, *endBwdArcScores = bwdArcScores + properties.nArcs;
	    /*
	      bwd. scores; build up bwd arc structure
	    */
	    for (StateMap::const_reverse_iterator itSid = topologicalSort->rbegin(),
		     endSid = topologicalSort->rend(); itSid != endSid; ++itSid) {
		Fsa::StateId sid = *itSid;
		ScoreState &stateScore = stateScores[sid];
		std::pair<u32, u32> &fanInOut = properties.fanInOut[sid];
		stateScore.fwdBegin = stateScore.fwdEnd = nextFwdArcScores;
		nextFwdArcScores += fanInOut.first; verify(nextFwdArcScores <= endFwdArcScores);
		stateScore.bwdBegin = stateScore.bwdEnd = nextBwdArcScores;
		nextBwdArcScores += fanInOut.second; verify(nextBwdArcScores <= endBwdArcScores);
		const Flf::State *sp = s->fastState(sid);
		if (!sp->hasArcs()) {
		    verify(sp->isFinal());
		    stateScore.bwdScore = posteriorSemiring->project(sp->weight());
		    if (hasRisk) stateScore.cost = stateScore.genBwdScore = sp->weight()->get(params.costId);
		} else {
		    stateScore.bwdScore = stateScore.cost = 0.0;
		    for (Flf::State::const_iterator a = sp->begin(), a_end = sp->end(); a != a_end; ++a) {
			Fsa::StateId targetSid = a->target();
			stateScore.fwdEnd->target = targetSid;
			ScoreState &targetStateScore = stateScores[targetSid];
			targetStateScore.bwdEnd->target = sid;
			f64 score = posteriorSemiring->project(a->weight());
			// verify(score != Semiring::Zero);
			stateScore.fwdEnd->score = targetStateScore.bwdEnd->score = score;
			f64 bwdScore = targetStateScore.bwdScore + score;

			// f64 bwdScore = ((targetStateScore.bwdScore == Semiring::Zero) || (score == Semiring::Zero)) ?
			// Semiring::Zero : targetStateScore.bwdScore + score;

			col->feed(bwdScore);
			if (hasRisk) {
			    f64 cost = a->weight()->get(params.costId);
			    stateScore.fwdEnd->cost = targetStateScore.bwdEnd->cost = cost;
			    f64 genBwdScore = targetStateScore.genBwdScore + cost;
			    colCost->feed(bwdScore, genBwdScore);
			}
			++stateScore.fwdEnd;
			++targetStateScore.bwdEnd;
		    }
		    stateScore.bwdScore = col->get();
		    col->reset();
		    if (hasRisk) {
			stateScore.genBwdScore = colCost->get(stateScore.bwdScore);
			colCost->reset();
		    }
		}
	    }
	    /*
	      fwd. scores
	    */
	    stateScores[topologicalSort->front()].fwdScore = 0.0;
	    for (StateMap::const_iterator itSid = topologicalSort->begin() + 1,
		     endSid = topologicalSort->end(); itSid != endSid; ++itSid) {
		Fsa::StateId sid = *itSid;
		ScoreState &stateScore = stateScores[sid];
		for (const ScoreArc *a = stateScore.bwdBegin, *a_end = stateScore.bwdEnd; a != a_end; ++a) {
		    ScoreState &targetStateScore = stateScores[a->target];
		    f64 fwdScore = targetStateScore.fwdScore + a->score;

		    // f64 fwdScore = ((targetStateScore.fwdScore == Semiring::Zero) || (a->score == Semiring::Zero)) ?
		    // Semiring::Zero : targetStateScore.fwdScore + a->score;

		    col->feed(fwdScore);
		    if (hasRisk) {
			f64 genFwdScore = targetStateScore.genFwdScore + a->cost;
			colCost->feed(fwdScore, genFwdScore);
		    }
		}
		stateScore.fwdScore = col->get();
		col->reset();
		if (hasRisk) {
		    stateScore.genFwdScore = colCost->get(stateScore.fwdScore);
		    colCost->reset();
		}
	    }
	    /*
	      complete fwd. sums for final states;
	      calculate fwd./bwd. sum
	    */
	    f64 bwdSum = stateScores[topologicalSort->front()].bwdScore;
	    f64 genBwdSum = (hasRisk) ? stateScores[topologicalSort->front()].genBwdScore : 0.0;
	    for (Core::Vector<Fsa::StateId>::const_iterator itSid = properties.finalStateIds.begin(), endSid = properties.finalStateIds.end();
		 itSid != endSid; ++itSid) {
		ScoreState &stateScore = stateScores[*itSid];
		stateScore.fwdScore += stateScore.bwdScore;

		// stateScore.fwdScore = ((stateScore.fwdScore == Semiring::Zero) || (stateScore.bwdScore == Semiring::Zero)) ?
		// Semiring::Zero : stateScore.fwdScore + stateScore.bwdScore;

		col->feed(stateScore.fwdScore);
		if (hasRisk) {
		    stateScore.genFwdScore += stateScore.cost;
		    colCost->feed(stateScore.fwdScore, stateScore.genFwdScore);
		}
	    }

	    f64 fwdSum = col->get();
	    col->reset();
	    {
		f64 deviation = 0.5 * (fwdSum - bwdSum);
		//if (!Core::isAlmostEqualUlp(f32(fwdSum), f32(bwdSum), 100)) {
		if ((deviation <= OOneInterval.first) || (OOneInterval.second <= deviation))
		    Core::Application::us()->warning(
			"Numerical instability: fwd./bwd. scores are not equal; "
			"fwd-score=%f, bwd-score=%f, probability quotient is %f and exceeds [0.99, 1.01]; ",
			fwdSum, bwdSum, ::exp(-deviation));
	    }
	    f64 genFwdSum = 0.0;
	    if (hasRisk) {
		genFwdSum = colCost->get(fwdSum);
		colCost->reset();
		f64 deviation = 0.5 * (genFwdSum - genBwdSum);
		//if (!Core::isAlmostEqualUlp(f32(genFwdSum), f32(genBwdSum), 200)) {
		if ((deviation <= OOneInterval.first) || (OOneInterval.second <= deviation))
		    Core::Application::us()->warning(
			"Numerical instability: generalized fwd./bwd. scores are not equal; "
			"gen-fwd-score=%f, gen-bwd-score=%f, probability quotient is %f and exceeds [0.99, 1.01]; ",
			genFwdSum, genBwdSum, ::exp(-deviation));
	    }

	    /*
	      fwd./bwd. probabilities
	    */
	    f64 normScore = 0.5 * (fwdSum + bwdSum);

	    // dbg("FB-sum:  " << normScore);

	    if (hasRisk) {
		Core::Application::us()->log(
		    "Expected cost for lattice \"%s\" is %f.",
		    l->describe().c_str(), 0.5 * (genFwdSum + genBwdSum));
	    }
	    f64 genNormScore =  params.normRisk ? 0.5 * (genFwdSum + genBwdSum) : 0.0;
	    if (hasScores) {
		const Semiring &semiring = *s->semiring();
		for (StateMap::const_iterator itSid = topologicalSort->begin(), endSid = topologicalSort->end();
		     itSid != endSid; ++itSid) {
		    Fsa::StateId sid = *itSid;
		    ScoreState &stateScore = stateScores[sid];
		    Flf::State::iterator a = s->fastState(sid)->begin();
		    for (const ScoreArc *fa = stateScore.fwdBegin, *fa_end = stateScore.fwdEnd; fa != fa_end; ++fa, ++a) {
			ScoreState &targetStateScore = stateScores[fa->target];
			ScoresRef scores = a->weight_ = semiring.clone(a->weight());
			Score fbScore = stateScore.fwdScore + fa->score + targetStateScore.bwdScore - normScore;
			if (params.scoreId != Semiring::InvalidId)
			    scores->set(params.scoreId, fbScore);
			if (hasRisk) {
			    f64 genFbScore = stateScore.genFwdScore + fa->cost + targetStateScore.genBwdScore - genNormScore;
			    Score risk = (genFbScore >= 0.0) ? ::exp(::log(genFbScore) - fbScore) : -::exp(::log(-genFbScore) - fbScore);
			    scores->set(params.riskId, risk);
			}
		    }
		    verify_(a == s->getState(sid)->end());
		}
	    }
	    if (fbScores) {
		fbScores->sum = normScore;
		for (StateMap::const_iterator itSid = topologicalSort->begin(), endSid = topologicalSort->end();
		     itSid != endSid; ++itSid) {
		    Fsa::StateId sid = *itSid;
		    ScoreState &stateScore = stateScores[sid];
		    FwdBwd::State &fbState = fbScores->states[sid];
		    fbState.fwdScore = stateScore.fwdScore;
		    fbState.bwdScore = stateScore.bwdScore;
		    fbState.normScore = normScore;
		    fbState.begin_ = fbState.end_ = nextFwdBwdArc;
		    for (const ScoreArc *fa = stateScore.fwdBegin, *fa_end = stateScore.fwdEnd; fa != fa_end; ++fa, ++fbState.end_) {
			fbState.end_->arcScore = fa->score;
			fbState.end_->fbScore = stateScore.fwdScore + fa->score + stateScores[fa->target].bwdScore;
			fbState.end_->normScore = normScore;
			Score posteriorScore = fbState.end_->fbScore - fbState.end_->normScore;
			if (posteriorScore > fbScores->max) fbScores->max = posteriorScore;
			if (posteriorScore < fbScores->min) fbScores->min = posteriorScore;
		    }
		    nextFwdBwdArc = fbState.end_; verify(nextFwdBwdArc <= endFwdBwdArc);
		}
	    }
	    /*
	      clean up temporary data structure
	    */
	    delete [] stateScores; stateScores = 0;
	    delete [] fwdArcScores; fwdArcScores = 0;
	    delete [] bwdArcScores; bwdArcScores = 0;
	    delete col; col = 0;
	    delete colCost; colCost = 0;
	    /*
	      we are done
	    */
	    return s;
	}


	/*
	  Build union of all lattices and
	  calculate combined, normalized fwd./bwd. scores.
	*/
	ConstLatticeRef operator() (const ConstLatticeRefList &_lats, const FwdBwd::CombinationParameters &params, FwdBwd *fb = 0) {
	    std::vector<u32> indexMap;
	    ConstLatticeRefList lats;
	    ConstSemiringRefList posteriorSemirings;
	    ScoreList weights;
	    /*
	      Normalize weights, exclude lattices with weight zero; find posterior semiring
	    */
	    {
		Score normWeight = 0.0;
		for (u32 i = 0; i < _lats.size(); ++i) {
		    Score weight = params.weights.empty() ? 1.0 : params.weights[i];
		    if ((weight > 0.0)  && _lats[i] && (_lats[i]->initialStateId() != Fsa::InvalidStateId)) {
			indexMap.push_back(i);
			lats.push_back(_lats[i]);
			weights.push_back(weight);
			posteriorSemirings.push_back(
			    (!params.posteriorSemirings.empty() && params.posteriorSemirings[i]) ?
			    params.posteriorSemirings[i] :
			    toLogSemiring(_lats[i]->semiring(), (params.alphas.empty() ? Score(0.0) : params.alphas[i])));
		    }
		    normWeight += weight;
		}
		if (lats.empty())
		    return ConstLatticeRef();
		verify(normWeight != 0.0);
		normWeight = 1.0 / normWeight;
		for (ScoreList::iterator itWeight = weights.begin(); itWeight != weights.end(); ++itWeight)
		    *itWeight *= normWeight;
	    }
	    const SemiringCombinationHelper &semiringCombo = *params.combination->semiringCombination();
	    /*
	      Union lattice properties and skeleton
	    */
	    // params.combination->update(lats);
	    bool hasSystemLabels = bool(params.systemAlphabet);
	    StaticLatticeRef unionL = StaticLatticeRef(new StaticLattice);
	    if (hasSystemLabels) {
		unionL->setType(Fsa::TypeTransducer);
		unionL->setInputAlphabet(params.combination->inputAlphabet());
		unionL->setOutputAlphabet(params.systemAlphabet);
	    } else {
		unionL->setType(params.combination->type());
		unionL->setInputAlphabet(params.combination->inputAlphabet());
		if (params.combination->type() != Fsa::TypeAcceptor)
		    unionL->setOutputAlphabet(params.combination->outputAlphabet());
	    }
	    unionL->setSemiring(semiringCombo.semiring());
	    unionL->setProperties(Fsa::PropertyAll, Fsa::PropertyAcyclic);
	    unionL->setDescription(Core::form("union(n=%zu)", lats.size()));
	    StaticBoundariesRef unionB = StaticBoundariesRef(new StaticBoundaries);
	    unionL->setBoundaries(unionB);

	    /*
	      Build unified lattice and calculate some statistics
	    */
	    Core::Vector<Properties> propertiesList(lats.size());
	    u32 maxStates = 0, maxArcs = 0;
	    u32 nUnionStates = 2, nInitialUnionArcs = lats.size(), nUnionArcs = lats.size();
	    Time unionStartTime = Core::Type<Time>::max, unionEndTime = Core::Type<Time>::min;
	    for (u32 i = 0; i < lats.size(); ++i) {

		// dbg("Semiring[" << i << "]: " << posteriorSemirings[i]->name());

		Properties &properties = propertiesList[i];
		properties.offset = nUnionStates;
		TraverseSubLattice traverse(lats[i], properties, *unionL, *unionB);
		maxStates = std::max(maxStates, properties.topologicalSort->maxSid + 1);
		maxArcs = std::max(maxArcs, properties.nArcs);
		nUnionStates += properties.topologicalSort->maxSid + 1;
		nUnionArcs += properties.nArcs + properties.finalStateIds.size();
		if (properties.startTime < unionStartTime)
		    unionStartTime = properties.startTime;
		if (properties.endTime > unionEndTime)
		    unionEndTime = properties.endTime;
	    }
	    Flf::State *unionInitialSp = new Flf::State(0);
	    unionL->setState(unionInitialSp);
	    unionL->setInitialStateId(unionInitialSp->id());
	    unionB->set(unionInitialSp->id(), unionStartTime);
	    Flf::State *unionFinalSp = new Flf::State(1);
	    unionFinalSp->setFinal(unionL->semiring()->clone(unionL->semiring()->one()));
	    unionL->setState(unionFinalSp);
	    unionB->set(unionFinalSp->id(), unionEndTime);

	    /*
	      Fwd/Bwd data structures
	    */
	    // normalized fwd./bwd. scores
	    FwdBwd::Internal *fbScores = 0;
	    FwdBwd::Arc *nextFwdBwdArc = 0, *endFwdBwdArc = 0;
	    if (fb) {
		fbScores = fb->internal_ = new FwdBwd::Internal(posteriorSemirings);
		fbScores->sum = 0.0;
		fbScores->states = new FwdBwd::State[nUnionStates];
		fbScores->arcs = new FwdBwd::Arc[nUnionArcs];
		FwdBwd::State &initialFbState = fbScores->states[unionInitialSp->id()];
		initialFbState.fwdScore = 0.0;
		initialFbState.bwdScore = 0.0;
		initialFbState.normScore = 0.0;
		initialFbState.begin_ = fbScores->arcs;
		initialFbState.end_ = fbScores->arcs + nInitialUnionArcs;
		FwdBwd::State &finalFbState = fbScores->states[unionFinalSp->id()];
		finalFbState.fwdScore = 0.0;
		finalFbState.bwdScore = 0.0;
		finalFbState.normScore = 0.0;
		nextFwdBwdArc = fbScores->arcs + nInitialUnionArcs;
		endFwdBwdArc = fbScores->arcs + nUnionArcs;
	    }

	    /*
	      Temporary data structure
	    */
	    ScoreState *stateScores = new ScoreState[maxStates];
	    ScoreArc *fwdArcScores = new ScoreArc[maxArcs];
	    ScoreArc *bwdArcScores = new ScoreArc[maxArcs];

	    /*
	      Calculate fwd./bwd. probabilities
	    */
	    Collector *col = createCollector(Fsa::SemiringTypeLog);
	    for (u32 i = 0; i < lats.size(); ++i) {
		Properties &properties = propertiesList[i];
		Fsa::StateId offsetSid = properties.offset;
		ConstStateMapRef topologicalSort = properties.topologicalSort;
		ScoreArc *nextFwdArcScores = fwdArcScores, *endFwdArcScores = fwdArcScores + properties.nArcs;
		ScoreArc *nextBwdArcScores = bwdArcScores, *endBwdArcScores = bwdArcScores + properties.nArcs;
		ConstSemiringRef posteriorSemiring = posteriorSemirings[i];
		const Semiring &semiring = *semiringCombo.semiring();
		u32 paramIndex = indexMap[i];
		Fsa::LabelId systemLabel = hasSystemLabels ? params.systemLabels[paramIndex] : Fsa::Epsilon;
		// DEPR begin
		ScoreId normId = params.normIds[paramIndex];
		ScoreId weightId = params.weightIds[paramIndex];
		Score normT = Score(1.0) / (Score(properties.endTime - unionStartTime));
		// DEPR end

		/*
		  bwd. scores; build up bwd arc structure
		*/
		for (StateMap::const_reverse_iterator itSid = topologicalSort->rbegin(),
			 endSid = topologicalSort->rend(); itSid != endSid; ++itSid) {
		    Fsa::StateId sid = *itSid;
		    ScoreState &stateScore = stateScores[sid];
		    std::pair<u32, u32> &fanInOut = properties.fanInOut[sid];
		    stateScore.fwdBegin = stateScore.fwdEnd = nextFwdArcScores;
		    nextFwdArcScores += fanInOut.first; verify(nextFwdArcScores <= endFwdArcScores);
		    stateScore.bwdBegin = stateScore.bwdEnd = nextBwdArcScores;
		    nextBwdArcScores += fanInOut.second; verify(nextBwdArcScores <= endBwdArcScores);
		    Fsa::StateId unionSid = sid + offsetSid;
		    const Flf::State *unionSp = unionL->fastState(unionSid);
		    if (!unionSp->hasArcs()) {
			verify(unionSp->isFinal());
			stateScore.bwdScore = posteriorSemiring->project(unionSp->weight());
		    } else {
			stateScore.score = 0.0;
			for (Flf::State::const_iterator a = unionSp->begin(), a_end = unionSp->end(); a != a_end; ++a) {
			    Fsa::StateId targetSid = a->target() - offsetSid;
			    f64 score = posteriorSemiring->project(a->weight());
			    stateScore.fwdEnd->target = targetSid;
			    stateScore.fwdEnd->score = score;
			    ++stateScore.fwdEnd;
			    ScoreState &targetStateScore = stateScores[targetSid];
			    col->feed(targetStateScore.bwdScore + score);
			    targetStateScore.bwdEnd->target = sid;
			    targetStateScore.bwdEnd->score = score;
			    ++targetStateScore.bwdEnd;
			}
			stateScore.bwdScore = col->get();
			col->reset();
		    }
		}
		/*
		  fwd. scores
		*/
		stateScores[topologicalSort->front()].fwdScore = 0.0;
		for (StateMap::const_iterator itSid = topologicalSort->begin() + 1, endSid = topologicalSort->end();
		     itSid != endSid; ++itSid) {
		    Fsa::StateId sid = *itSid;
		    ScoreState &stateScore = stateScores[sid];
		    for (const ScoreArc *a = stateScore.bwdBegin, *a_end = stateScore.bwdEnd; a != a_end; ++a)
			col->feed(stateScores[a->target].fwdScore + a->score);
		    stateScore.fwdScore = col->get();
		    col->reset();
		}
		/*
		  complete final state;
		  calculate fwd./bwd. sums
		*/
		for (Core::Vector<Fsa::StateId>::const_iterator itSid = properties.finalStateIds.begin(), endSid = properties.finalStateIds.end();
		     itSid != endSid; ++itSid) {
		    ScoreState &stateScore = stateScores[*itSid];
		    col->feed(stateScore.fwdScore + stateScore.bwdScore);
		    // stateScore.fwdScore += stateScore.score; // DEPR
		}
		f64 fwdSum = col->get();
		col->reset();
		f64 bwdSum = stateScores[topologicalSort->front()].bwdScore;
		{
		    f64 deviation = 0.5 * (fwdSum - bwdSum);
		    if ((deviation <= OOneInterval.first) || (OOneInterval.second <= deviation))
			Core::Application::us()->warning(
			    "Numerical instability: fwd./bwd. scores are not equal; "
			    "fwd-score=%f, bwd-score=%f, probability quotient is %f and exceeds [0.99, 1.01]; ",
			    fwdSum, bwdSum, ::exp(-deviation));
		}
		/*
		  fwd./bwd. probabilities
		*/
		f64 fbSum = 0.5 * (fwdSum + bwdSum);
		f64 normScore = fbSum + ::log(weights[i]);


		// dbg("FB-sum[" << i << "]: " << fbSum);

		// DEPR begin
		f64 systemNormScore = 0.0;
		if (normId != Semiring::InvalidId) {
		    if (params.fsaNorms[paramIndex]) {
			Fsa::ConstAutomatonRef total = Fsa::staticCopy(
			    Fsa::changeSemiring(
				toFsa(
				    lats[i],
				    posteriorSemiring->scales()),
				Fsa::getSemiring(Fsa::SemiringTypeLog)));
			Fsa::Weight totalInv;
			Fsa::posterior64(total, totalInv);
			systemNormScore = -f64(totalInv);
		    } else
			systemNormScore = fbSum;
		}
		// DEPR end

		if (fbScores) {
		    fbScores->sum += normScore;
		    // union initial state
		    {
			FwdBwd::State &fbState = fbScores->states[unionInitialSp->id()];
			FwdBwd::Arc &fbArc = fbState.begin_[i];
			fbArc.arcScore = 0.0;
			fbArc.fbScore = fbSum;
			fbArc.normScore = normScore;
		    }
		    // sub-lattice states
		    for (StateMap::const_iterator itSid = topologicalSort->begin(),
			     endSid = topologicalSort->end(); itSid != endSid; ++itSid) {
			Fsa::StateId sid = *itSid;
			ScoreState &stateScore = stateScores[sid];
			FwdBwd::State &fbState = fbScores->states[sid + offsetSid];
			fbState.fwdScore = stateScore.fwdScore;
			fbState.bwdScore = stateScore.bwdScore;
			fbState.normScore = normScore;
			fbState.begin_ = fbState.end_ = nextFwdBwdArc;
			for (const ScoreArc *fa = stateScore.fwdBegin, *fa_end = stateScore.fwdEnd; fa != fa_end; ++fa, ++fbState.end_) {
			    fbState.end_->arcScore = fa->score;
			    fbState.end_->fbScore = stateScore.fwdScore + fa->score + stateScores[fa->target].bwdScore;
			    fbState.end_->normScore = normScore;
			    Score posteriorScore = fbState.end_->fbScore - fbState.end_->normScore;
			    if (posteriorScore > fbScores->max) fbScores->max = posteriorScore;
			    if (posteriorScore < fbScores->min) fbScores->min = posteriorScore;
			}
			nextFwdBwdArc = fbState.end_; verify(nextFwdBwdArc <= endFwdBwdArc);
		    }
		    // final states
		    for (Core::Vector<Fsa::StateId>::const_iterator itSid = properties.finalStateIds.begin(), endSid = properties.finalStateIds.end();
			 itSid != endSid; ++itSid) {
			Fsa::StateId sid = *itSid;
			// ScoreState &stateScore = stateScores[sid];
			FwdBwd::State &fbState = fbScores->states[sid + offsetSid];
			fbState.begin_ = nextFwdBwdArc;
			fbState.begin_->arcScore = fbState.bwdScore; // i.e. final state score
			fbState.begin_->fbScore = fbState.fwdScore + fbState.bwdScore;
			fbState.begin_->normScore = normScore;
			fbState.end_ = ++nextFwdBwdArc;
			verify(nextFwdBwdArc <= endFwdBwdArc);
		    }
		}
		/*
		  Connect union initial state to sub-initial state and set fwd/bwd scores for initial arcs
		*/
		{
		    ScoresRef scores = semiring.clone(semiring.one());
		    if (params.scoreId != Semiring::InvalidId)
			scores->set(params.scoreId, -::log(weights[i]));
		    unionInitialSp->newArc(topologicalSort->front() + offsetSid, scores, Fsa::Epsilon, systemLabel);
		}
		/*
		  Connect sub-final states to union final state and set fwd/bwd scores for remaining arcs
		*/
		for (StateMap::const_iterator itSid = topologicalSort->begin(),
			 endSid = topologicalSort->end(); itSid != endSid; ++itSid) {
		    Fsa::StateId sid = *itSid;
		    ScoreState &stateScore = stateScores[sid];
		    Time beginT = unionB->time(sid + offsetSid);
		    Flf::State *sp = unionL->fastState(sid + offsetSid);
		    Flf::State::iterator a = sp->begin();
		    if (sp->isFinal()) {
			verify(!sp->hasArcs());
			ScoresRef scores = semiring.clone(semiring.one());
			semiringCombo.set(scores, i, sp->weight());
			sp->unsetFinal();
			FwdBwd::State &fbState = fbScores->states[sid + offsetSid];
			if (params.scoreId != Semiring::InvalidId)
			    scores->set(params.scoreId, fbState.fwdScore + fbState.bwdScore - normScore);
			sp->newArc(unionFinalSp->id(), scores, Fsa::Epsilon, systemLabel);
		    } else {
			for (const ScoreArc *fa = stateScore.fwdBegin, *fa_end = stateScore.fwdEnd; fa != fa_end; ++fa, ++a) {
			    if (hasSystemLabels)
				a->output_ = systemLabel;
			    ScoresRef scores = semiring.clone(semiring.one());
			    semiringCombo.set(scores, i, a->weight());
			    if (params.scoreId != Semiring::InvalidId) {
				Score fbScore = stateScore.fwdScore + fa->score + stateScores[fa->target].bwdScore - normScore;
				scores->set(params.scoreId, fbScore);
			    }

			    // DEPR begin
			    if ((normId != Semiring::InvalidId) || (weightId != Semiring::InvalidId)) {
				Score normDuration = normT * Score(unionB->time(a->target()) - beginT);
				if (normId != Semiring::InvalidId)
				    scores->set(normId,  -systemNormScore * normDuration);
				if (weightId != Semiring::InvalidId)
				    scores->set(weightId, normDuration);
			    }
			    // DEPR end

			    a->weight_ = scores;
			}
			verify_(a == unionL->fastState(sid + offsetSid)->end());
		    }
		}
	    }
	    /*
	      clean up temporary data structure
	    */
	    delete [] stateScores; stateScores = 0;
	    delete [] fwdArcScores; fwdArcScores = 0;
	    delete [] bwdArcScores; bwdArcScores = 0;
	    /*
	      Verify union fwd./bwd. sum
	    */
	    if (fbScores) {
		f64 unionFwdBwdSum = 0.0;
		{
		    FwdBwd::State &fbState = fbScores->states[unionInitialSp->id()];
		    verify(u32(fbState.end_ - fbState.begin_) == unionInitialSp->nArcs());
		    for (FwdBwd::Arc *itArc = fbState.begin_; itArc != fbState.end_; ++itArc)
			col->feed(itArc->fbScore - itArc->normScore);
		}
		unionFwdBwdSum = col->get();
		col->reset();
		if ((unionFwdBwdSum <= OOneInterval.first) || (OOneInterval.second <= unionFwdBwdSum))
		    Core::Application::us()->warning(
			"Numerical instability: sum over initial arcs exceeds [0.99, 1.01]; "
			"normalized fwd. sum is %f.", ::exp(-unionFwdBwdSum));
		for (u32 i = 0; i < lats.size(); ++i) {
		    Properties &properties = propertiesList[i];
		    for (Core::Vector<Fsa::StateId>::const_iterator itSid = properties.finalStateIds.begin(), endSid = properties.finalStateIds.end();
			 itSid != endSid; ++itSid) {
			const Flf::State *sp = unionL->fastState(*itSid + properties.offset);
			verify(!sp->isFinal() && (sp->nArcs() == 1));
			FwdBwd::State &fbState = fbScores->states[sp->id()];
			verify(fbState.end_ - fbState.begin_ == 1);
			col->feed(fbState.begin_->fbScore - fbState.begin_->normScore);
		    }
		}
		unionFwdBwdSum = col->get();
		col->reset();
		if ((unionFwdBwdSum <= OOneInterval.first) || (OOneInterval.second <= unionFwdBwdSum))
		    Core::Application::us()->warning(
			"Numerical instability: sum over final arcs exceeds [0.99, 1.01]; "
			"normalized fwd. sum is %f.", ::exp(-unionFwdBwdSum));
	    }
	    /*
	      Change to posterior semiring, preserving the scales used for FB calculation
	    */
	    if (params.setPosteriorSemiring) {
		ScoreList unionPosteriorScales(unionL->semiring()->size(), 0.0);
		for (ScoreId i = 0; i < lats.size(); ++i) {
		    u32 paramIndex = indexMap[i];
		    for (ScoreId j = 0; j < posteriorSemirings[i]->size(); ++j) {
			ScoreId unionId = semiringCombo.subId(i, j);
			if (unionId != Semiring::InvalidId)
			    unionPosteriorScales[unionId] = posteriorSemirings[i]->scale(j);
		    }

		    // DEPR begin
		    ScoreId normId = params.normIds[paramIndex];
		    if (normId != Semiring::InvalidId)
			unionPosteriorScales[normId] = 1.0;
		    ScoreId weightId = params.weightIds[paramIndex];
		    if (weightId != Semiring::InvalidId)
			unionPosteriorScales[weightId] = -::log(weights[i]);
		    // DEPR end

		}
		ConstSemiringRef unionPosteriorSemiring = Semiring::create(
		    Fsa::SemiringTypeLog, unionL->semiring()->size(), unionPosteriorScales, unionL->semiring()->keys());
		unionL->setSemiring(unionPosteriorSemiring);


		//dbg begin
		/* {
		   for (u32 i = 0; i < lats.size(); ++i) {
		   u32 paramIndex = indexMap[i];
		   unionPosteriorScales[params.normIds[paramIndex]] = 0.0;
		   unionPosteriorScales[params.weightIds[paramIndex]] = 0.0;
		   }
		   ConstSemiringRef singlePosteriorSemiring = Semiring::create(
		   Fsa::SemiringTypeLog, unionL->semiring()->size(), unionPosteriorScales, unionL->semiring()->keys());
		   //dbg("Union posterior semiring:  " << unionPosteriorSemiring->name());
		   //dbg("Single posterior semiring: " << singlePosteriorSemiring->name());
		   //dbg("Check log-linear models ...");
		   std::pair<ConstLatticeRef, ConstFwdBwdRef> result = FwdBwd::build(unionL, unionPosteriorSemiring);
		   //dbg("Union: " << result.second->sum() << " = 0.0");
		   std::vector<f64> Z(lats.size()), Zp(lats.size());
		   for (u32 i = 0; i < lats.size(); ++i) {
		   u32 paramIndex = indexMap[i];
		   ConstLatticeRef l = persistent(filterByOutput(unionL, params.systemLabels[paramIndex]));
		   result = FwdBwd::build(l, unionPosteriorSemiring);
		   //dbg("Weight[" << i << "]:  " << ::exp(-result.second->sum()) << " = " << weights[i]);
		   result = FwdBwd::build(l, singlePosteriorSemiring);
		   Z[i] = 0.0;
		   for (ConstStateRef sr = l->getState(l->initialStateId()); sr->hasArcs(); sr = l->getState(sr->begin()->target()))
		   Z[i] += sr->begin()->weight()->get(params.normIds[paramIndex]);
		   Zp[i] = -result.second->sum();
		   Fsa::ConstAutomatonRef total = Fsa::staticCopy(
		   Fsa::changeSemiring(
		   toFsa(
		   l,
		   unionPosteriorScales),
		   Fsa::getSemiring(Fsa::SemiringTypeLog)));
		   Fsa::Weight totalInv;
		   Fsa::posterior64(total, totalInv);
		   dbg(Core::form("FB-Norm[%d], norm-dimension: %.10f", i, Z[i]));
		   dbg(Core::form("FB-Norm[%d], Flf:            %.10f", i, Zp[i]));
		   dbg(Core::form("FB-Norm[%d], Fsa:            %.10f", i, f64(totalInv)));
		   }
		   //dbg("Check weights ...");
		   std::vector<f64> w(lats.size());
		   f64 wNorm = 0.0;
		   for (u32 i = 0; i < lats.size(); ++i) {
		   u32 paramIndex = indexMap[i];
		   wNorm += w[i] = ::exp(
		   - unionPosteriorSemiring->scale(params.weightIds[paramIndex])
		   - unionPosteriorSemiring->scale(params.normIds[paramIndex]) * Z[i]
		   + Zp[i]);
		   }
		   for (u32 i = 0; i < lats.size(); ++i)
		   dbg("Weight[" << i << "]:  " << w[i] << "/" << wNorm << " = " << (w[i] / wNorm) << " = " << weights[i]);
		   } */
		// dbg end


	    }
	    delete col; col = 0;
	    return unionL;
	}
    } buildFwdBwd;
    const std::pair<f64, f64> FwdBwd::Builder::OOneInterval(-0.00995033085316808285, 0.01005033585350144118);
    const std::pair<f64, f64> FwdBwd::Builder::OFiveInterval(-0.04879016416943200307, 0.05129329438755053343);

    FwdBwd::Parameters::Parameters() :
	alpha(0.0),
	posteriorSemiring(),
	costId(Semiring::InvalidId),
	scoreId(Semiring::InvalidId),
	riskId(Semiring::InvalidId),
	normRisk(false) {}

    void FwdBwd::Parameters::verifyConsistency() const {
	if ((riskId != Semiring::InvalidId) && (costId == Semiring::InvalidId))
	    Core::Application::us()->criticalError("Risk calculation requires a dimension storing the arc-wise cost.");
    }

    std::pair<ConstLatticeRef, ConstFwdBwdRef> FwdBwd::build(
	ConstLatticeRef l,
	const FwdBwd::Parameters &params) {
	params.verifyConsistency();
	FwdBwdRef fb = FwdBwdRef(new FwdBwd);
	l = buildFwdBwd(l, params, fb.get());
	return std::make_pair(l, fb);
    }

    std::pair<ConstLatticeRef, ConstFwdBwdRef> FwdBwd::build(
	ConstLatticeRef l,
	ConstSemiringRef posteriorSemiring) {
	Parameters params;
	params.posteriorSemiring = posteriorSemiring;
	return build(l, params);
    }

    FwdBwd::CombinationParameters::CombinationParameters() :
	weights(),
	alphas(),
	posteriorSemirings(),
	systemAlphabet(),
	systemLabels(),
	combination(),
	scoreId(Semiring::InvalidId) {}

    void FwdBwd::CombinationParameters::verifyConsistency(u32 n) const {
	if (n == 0)
	    Core::Application::us()->criticalError("Cannot combine zero systems.");
	verify(weights.empty() || (weights.size() == n));
	verify(alphas.empty() || (alphas.size() == n));
	verify(posteriorSemirings.empty() || (posteriorSemirings.size() == n));
	if (systemAlphabet) {
	    verify(systemLabels.size() == n);
	    for (LabelIdList::const_iterator itLabel = systemLabels.begin(); itLabel != systemLabels.end(); ++itLabel)
		if (*itLabel == Fsa::InvalidLabelId)
		    Core::Application::us()->criticalError("System label must not be invalid.");
	}
	verify((normIds.size() == n) && (fsaNorms.size() == n));
	verify(weightIds.size() == n);
	verify(combination);
    }

    std::pair<ConstLatticeRef, ConstFwdBwdRef> FwdBwd::build(
	const ConstLatticeRefList &lats,
	const FwdBwd::CombinationParameters &params) {
	params.verifyConsistency(lats.size());
	FwdBwdRef fb = FwdBwdRef(new FwdBwd);
	ConstLatticeRef l = buildFwdBwd(lats, params, fb.get());
	return std::make_pair(l, fb);
    }

    std::pair<ConstLatticeRef, ConstFwdBwdRef> FwdBwd::build(
	const ConstLatticeRefList &lats,
	const ScoreList &weights,
	const ConstSemiringRefList &posteriorSemirings) {
	CombinationParameters params;
	params.weights = weights;
	params.posteriorSemirings = posteriorSemirings;
	return build(lats, params);
    }
    // -------------------------------------------------------------------------


    // -------------------------------------------------------------------------
    namespace {
	const Core::ParameterString paramKey(
	    "key",
	    "dimension to store fwd./bwd. score",
	    "");
	const Core::ParameterString paramLabel(
	    "label",
	    "label/symbol/word",
	    "");
	const Core::ParameterFloat paramWeight(
	    "weight",
	    "weight",
	    1.0);
	const Core::ParameterFloat paramAlpha(
	    "alpha",
	    "fwd./bwd. score alpha",
	    0.0);
	const Core::ParameterBool paramFsa(
	    "fsa",
	    "use fsa algorithm",
	    false);
	const Core::ParameterBool paramNormalize(
	    "normalize",
	    "normalize",
	    false);
	const Core::ParameterBool paramSystemLabels(
	    "system-labels",
	    "store system labels as output",
	    false);
	const Core::ParameterBool paramSetPosteriorSemiring(
	    "set-posterior-semiring",
	    "set posterior semiring at resulting lattice",
	    false);
    } // namespace

    class FwdBwdBuilder::Internal : public Core::Component {
    private:
	struct SingleConfiguration {
	    ConstSemiringRef semiring;
	    FwdBwd::Parameters params;
	    void dump(std::ostream &os) const {
		if (semiring) {
		    os << "Target semiring is \"" << semiring->name() << "\"." << std::endl;
		    if (params.scoreId != Semiring::InvalidId)
			os << "Store fwd./bwd. scores in dimension \"" << semiring->key(params.scoreId) << "\"." << std::endl;
		    if (params.riskId != Semiring::InvalidId) {
			os << "Store risk in dimension \"" << semiring->key(params.riskId) << "\"." << std::endl;
			if (params.normRisk)
			    os << "Risk is normalized." << std::endl;
		    }
		}
		os << "1. lattice:"  << std::endl;
		if (params.posteriorSemiring) {
		    os << "\tsemiring = " << params.posteriorSemiring->name() << std::endl;
		} else {
		    if (params.alpha == Core::Type<Score>::max)
			os << "\talpha    = infinity" << std::endl;
		    else if (params.alpha == 0.0)
			os << "\talpha    = <1/max-scale>" << std::endl;
		    else
			os << "\talpha    = " << params.alpha << std::endl;
		}
		if (params.costId != Semiring::InvalidId)
		    os << "\tcost-key = \"" << semiring->key(params.costId) << "\"" << std::endl;
	    }
	};

	struct CombinationConfiguration {
	    FwdBwd::CombinationParameters params;
	    void dump(std::ostream &os) const {
		if (params.combination) {
		    os << "Target semiring is \"" << params.combination->semiring()->name() << "\"." << std::endl;
		    if (params.scoreId != Semiring::InvalidId)
			os << "Store fwd./bwd. scores in dimension \"" << params.combination->semiring()->key(params.scoreId) << "\"."  << std::endl;
		}
		if (params.setPosteriorSemiring)
		    os << "Set semiring used for fwd/bwd-score calculation on resulting lattice." << std::endl;
		for (u32 i = 0; i < params.weights.size(); ++i) {
		    os << (i + 1) << ". lattice:"  << std::endl;
		    os << "\tweight     = " << params.weights[i] << std::endl;
		    if (params.posteriorSemirings[i]) {
			os << "\tsemiring   = " << params.posteriorSemirings[i]->name() << std::endl;
		    } else {
			if (params.alphas[i] == Core::Type<Score>::max)
			    os << "\talpha      = infinity" << std::endl;
			else if (params.alphas[i] == 0.0)
			    os << "\talpha      = <1/max-scale>" << std::endl;
			else
			    os << "\talpha      = " << params.alphas[i] << std::endl;
		    }
		    if (params.systemLabels[i] != Fsa::InvalidLabelId)
			os << "\tlabel      = " << params.systemAlphabet->symbol(params.systemLabels[i]) << std::endl;
		    if (params.normIds[i] != Semiring::InvalidId) {
			os << "\tnorm-key   = " << params.combination->semiring()->key(params.normIds[i]);
			if (params.fsaNorms[i])
			    os << " (fsa norm)";
			os << std::endl;
		    }
		    if (params.weightIds[i] != Semiring::InvalidId)
			os << "\tweight-key = " << params.combination->semiring()->key(params.weightIds[i]) << std::endl;
		}
	    }
	};

    public:
	Core::XmlChannel configurationChannel;
	Core::XmlChannel statisticsChannel;
	SingleConfiguration *singleConfig;
	CombinationConfiguration *comboConfig;
	KeyList extensionKeys;
	ScoreList extensionScales;
	ScoreId scoreExtensionId;

    public:
	Internal(const Core::Configuration &config, const KeyList &extensionKeys, const ScoreList &extensionScales) :
	    Core::Component(config),
	    configurationChannel(config, "configuration"),
	    statisticsChannel(config, "statistics"),
	    singleConfig(0), comboConfig(0),
	    extensionKeys(extensionKeys), extensionScales(extensionScales),
	    scoreExtensionId(Semiring::InvalidId) {
	    Key scoreKey = paramKey(select("score"));
	    if (!scoreKey.empty()) {
		scoreExtensionId = this->extensionKeys.size();
		this->extensionKeys.push_back(scoreKey);
		this->extensionScales.push_back(0.0);
	    }
	}

	virtual ~Internal() {
	    delete singleConfig;
	    delete comboConfig;
	}

	void update(ConstLatticeRef l) {
	    if (!singleConfig)
		singleConfig = new SingleConfiguration;
	    else if ((singleConfig->semiring.get() == l->semiring().get()) || (*singleConfig->semiring == *l->semiring()))
		return;
	    singleConfig->semiring = l->semiring();
	    FwdBwd::Parameters &params = singleConfig->params;
	    Core::ParameterFloat::Value alpha = paramAlpha(config);
	    params.alpha = (alpha == Core::Type<Core::ParameterFloat::Value>::max) ? Core::Type<Score>::max : Score(alpha);
	    params.posteriorSemiring = Semiring::create(Core::Configuration(config, "semiring"));
	    Key scoreKey = paramKey(select("score"));
	    for (KeyList::const_iterator itExtKey = extensionKeys.begin(); itExtKey != extensionKeys.end(); ++itExtKey)
		if (singleConfig->semiring->id(*itExtKey) == Semiring::InvalidId)
		    criticalError(
			"Subsequent processings require a dimension \"%s\", which is not provided by semiring \"%s\".",
			itExtKey->c_str(), singleConfig->semiring->name().c_str());
	    if (scoreExtensionId != Semiring::InvalidId)
		singleConfig->params.scoreId = SemiringCombinationHelper::getIdOrDie(singleConfig->semiring, extensionKeys[scoreExtensionId]);
	    Key riskKey = paramKey(select("risk"));
	    if (!riskKey.empty())
		singleConfig->params.riskId = SemiringCombinationHelper::getIdOrDie(singleConfig->semiring, riskKey);
	    singleConfig->params.normRisk = paramNormalize(select("risk"));
	    Key costKey = paramKey(select("cost"));
	    if (!costKey.empty())
		singleConfig->params.costId = SemiringCombinationHelper::getIdOrDie(singleConfig->semiring, costKey);
	    singleConfig->params.verifyConsistency();
	    if (configurationChannel.isOpen()) {
		configurationChannel << Core::XmlOpen("configuration")
		    + Core::XmlAttribute("component", this->name())
		    + Core::XmlAttribute("name", "FB");
		singleConfig->dump(configurationChannel);
		configurationChannel << Core::XmlClose("configuration");
	    }
	}

	void update(const ConstLatticeRefList &lats) {
	    if (!comboConfig) {
		comboConfig = new CombinationConfiguration;
		comboConfig->params.combination = CombinationHelper::create(
		    SemiringCombinationHelper::getType(SemiringCombinationHelper::paramType(select("score-combination"))),
		    extensionKeys, extensionScales);
	    }
	    if (comboConfig->params.combination->update(lats)) {
		FwdBwd::CombinationParameters &params = comboConfig->params;
		SemiringCombinationHelper &semiringCombo = *params.combination->semiringCombination();
		if (scoreExtensionId != Semiring::InvalidId)
		    params.scoreId = semiringCombo.combinationId(scoreExtensionId);
		Lexicon::SymbolMap systemSymbolMap;
		if (paramSystemLabels(config)) {
		    params.systemAlphabet = Lexicon::us()->alphabet(Lexicon::LemmaAlphabetId);
		    systemSymbolMap = Lexicon::us()->symbolMap(Lexicon::LemmaAlphabetId);
		}
		for (u32 i = params.weights.size(); i < lats.size(); ++i) {
		    const Core::Configuration subConfig = select(Core::form("lattice-%d", i));
		    params.weights.push_back(paramWeight(subConfig));
		    params.posteriorSemirings.push_back(Semiring::create(Core::Configuration(subConfig, "semiring")));
		    Core::ParameterFloat::Value alpha = paramAlpha(subConfig);
		    params.alphas.push_back((alpha == Core::Type<Core::ParameterFloat::Value>::max) ? Core::Type<Score>::max : Score(alpha));
		    if (params.systemAlphabet) {
			std::string systemLabel = paramLabel(subConfig);
			if (systemLabel.empty()) systemLabel = Core::form("system-%d", i);
			params.systemLabels.push_back(systemSymbolMap.index(systemLabel));
		    } else
			params.systemLabels.push_back(Fsa::InvalidLabelId);
		    Key normKey = paramKey(Core::Configuration(subConfig, "norm"));
		    if (!normKey.empty()) {
			params.normIds.push_back(semiringCombo.subId(i, normKey));
			params.fsaNorms.push_back(paramFsa(Core::Configuration(subConfig, "norm")));
		    } else {
			params.normIds.push_back(Semiring::InvalidId);
			params.fsaNorms.push_back(false);
		    }
		    Key weightKey = paramKey(Core::Configuration(subConfig, "weight"));
		    if (!weightKey.empty())
			params.weightIds.push_back(semiringCombo.subId(i, weightKey));
		    else
			params.weightIds.push_back(Semiring::InvalidId);
		}
		params.setPosteriorSemiring = paramSetPosteriorSemiring(config);
		comboConfig->params.verifyConsistency(lats.size());
		if (configurationChannel.isOpen()) {
		    configurationChannel << Core::XmlOpen("configuration")
			+ Core::XmlAttribute("component", this->name())
			+ Core::XmlAttribute("name", "FB-combination");
		    comboConfig->dump(configurationChannel);
		    configurationChannel << Core::XmlClose("configuration");
		}
	    }
	}

	void dumpStatistics(ConstFwdBwdRef fb, const std::string &name) {
	    if (statisticsChannel.isOpen()) {
		statisticsChannel << Core::XmlOpen("statistics")
		    + Core::XmlAttribute("component", this->name())
		    + Core::XmlAttribute("name", name);
		statisticsChannel << Core::XmlFull("lower-bound", fb->min());
		statisticsChannel << Core::XmlFull("upper-bound", fb->max());
		statisticsChannel << Core::XmlClose("statistics");
	    }
	}
    };

    FwdBwdBuilder::FwdBwdBuilder() :internal_(0)  {}

    FwdBwdBuilder::~FwdBwdBuilder() {
	delete internal_;
    }

    std::pair<ConstLatticeRef, ConstFwdBwdRef> FwdBwdBuilder::build(ConstLatticeRef l) {
	internal_->update(l);
	FwdBwdRef fb = FwdBwdRef(new FwdBwd);
	l = buildFwdBwd(l, internal_->singleConfig->params, fb.get());
	internal_->dumpStatistics(fb, "FB");
	return std::make_pair(l, fb);
    }

    std::pair<ConstLatticeRef, ConstFwdBwdRef> FwdBwdBuilder::build(const ConstLatticeRefList &lats) {
	internal_->update(lats);
	FwdBwdRef fb = FwdBwdRef(new FwdBwd);
	ConstLatticeRef l = buildFwdBwd(lats, internal_->comboConfig->params, fb.get());
	internal_->dumpStatistics(fb, "FB-combination");
	return std::make_pair(l, fb);
    }

    FwdBwdBuilderRef FwdBwdBuilder::create(const Core::Configuration &config, const KeyList &extensionKeys, const ScoreList &extensionScales) {
	FwdBwdBuilder *fwdBwdBuilder = new FwdBwdBuilder;
	fwdBwdBuilder->internal_ = new FwdBwdBuilder::Internal(config, extensionKeys, extensionScales);
	return FwdBwdBuilderRef(fwdBwdBuilder);
    }
    // -------------------------------------------------------------------------



    // -------------------------------------------------------------------------
    class FwdBwdBuilderNode : public Node {
    public:
	static const Core::ParameterBool paramForce;

    private:
	u32 n_;
	bool multiLatticeAlgorithm_;
	FwdBwdBuilderRef fbBuilder_;
	ConstLatticeRef l_;
	ConstFwdBwdRef fb_;
	bool hasNext_;

    private:
	void next() {
	    if (!hasNext_) {
		ConstLatticeRefList lats(n_);
		for (u32 i = 0; i < n_; ++i) {
		    ConstLatticeRef l = requestLattice(i);
		    if (!l) {
			warning("No lattice provided at port %d; discard", i);
			lats.clear();
			break;
		    }
		    lats[i] = l;
		}
		if (!lats.empty()) {
		    std::pair<ConstLatticeRef, ConstFwdBwdRef> fbResult = (multiLatticeAlgorithm_) ?
			fbBuilder_->build(lats) : fbBuilder_->build(lats.front());
		    l_ = fbResult.first; fb_ = fbResult.second;
		}
		hasNext_ = true;
	    }
	}

    public:
	FwdBwdBuilderNode(const std::string &name, const Core::Configuration &config) :
	    Node(name, config), n_(0) {}
	virtual ~FwdBwdBuilderNode() {}

	virtual void init(const std::vector<std::string> &arguments) {
	    for (n_ = 0; connected(n_); ++n_);
	    if (n_ == 0)
		criticalError("At least one incoming lattice at port 0 required.");
	    if (n_ == 1)
		multiLatticeAlgorithm_ = paramForce(select("multi-lattice-algorithm"));
	    else
		multiLatticeAlgorithm_ = true;
	    if (multiLatticeAlgorithm_)
		log() << "Combine " << n_ << " lattices.\n\n";
	    fbBuilder_ = FwdBwdBuilder::create(select("fb"));
	    hasNext_ = false;
	}

	virtual ConstLatticeRef sendLattice(Port to) {
	    verify(to == 0);
	    next();
	    return l_;
	}

	virtual const void *sendData(Port to) {
	    verify(to == 1);
	    next();
	    return fb_.get();
	}

	virtual void sync() {
	    l_.reset();
	    fb_.reset();
	    hasNext_ = false;
	}
    };
    const Core::ParameterBool FwdBwdBuilderNode::paramForce(
	"force",
	"forc",
	false);
    NodeRef createFwdBwdBuilderNode(const std::string &name, const Core::Configuration &config) {
	return NodeRef(new FwdBwdBuilderNode(name, config));
    }
    // -------------------------------------------------------------------------

} // namespace Flf

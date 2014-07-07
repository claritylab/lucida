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
#include <Core/Hash.hh>
#include <Core/Parameter.hh>

#include "FlfCore/Basic.hh"
#include "FlfCore/Ftl.hh"
#include "FlfCore/Utility.hh"
#include "Best.hh"

#include "Lexicon.hh"


namespace Flf {

    // -------------------------------------------------------------------------
    namespace {
	struct TraceElement {
	    Score score;
	    Fsa::StateId sid;
	    Fsa::StateId aid;
	    TraceElement() :
		score(Semiring::Max),
		sid(Fsa::InvalidStateId),
		aid(Fsa::InvalidStateId) {}
	};
	typedef std::vector<TraceElement> Traceback;
    } // namespace
    std::pair<ConstLatticeRef, Score> bestProjection(ConstLatticeRef l) {
	const Semiring &semiring = *l->semiring();
	ConstStateMapRef topologicalSort = sortTopologically(l);
	Fsa::StateId initialSid = topologicalSort->front();
	Traceback traceback(topologicalSort->maxSid + 1);
	traceback[initialSid].score = 0.0;
	TraceElement bestTrace;
	for (u32 i = 0; i < topologicalSort->size(); ++i) {
	    const Fsa::StateId sid = (*topologicalSort)[i];
	    ConstStateRef sr = l->getState(sid);

	    // dbg
	    /*
	    std::cout << "state: " << sr->id() << std::endl;
	    */

	    const TraceElement &currentTrace = traceback[sid];
	    if (sr->isFinal()) {
		const Score score = add(currentTrace.score, semiring.project(sr->weight()));
		if (score < bestTrace.score) {
		    bestTrace.score = score;
		    bestTrace.sid = sid;
		}
	    }
	    Fsa::StateId aid = 0;
	    for (State::const_iterator a = sr->begin(), end = sr->end(); a != end; ++a, ++aid) {


		// dbg
		/*
		const Boundary &leftBoundary = l->getBoundaries()->get(sr->id());
		const Boundary &rightBoundary = l->getBoundaries()->get(a->target());
		std::cout << Lexicon::us()->phonemeInventory()->phonemeAlphabet()->symbol(leftBoundary.transit().initial)
			  << ((leftBoundary.transit().boundary == AcrossWordBoundary) ? " | " : " + ")
			  << "\"" << l->inputAlphabet()->symbol(a->input()) << "\""
			  << ((rightBoundary.transit().boundary == AcrossWordBoundary) ? " | " : " + ")
			  << Lexicon::us()->phonemeInventory()->phonemeAlphabet()->symbol(rightBoundary.transit().final)
			  << std::endl;
		*/

		const Score score = add(currentTrace.score, semiring.project(a->weight()));
		TraceElement &trace = traceback[a->target()];
		if (score < trace.score) {
		    trace.score = score;
		    trace.sid = sid;
		    trace.aid = aid;
		}
	    }
	}
	if (bestTrace.sid == Fsa::InvalidStateId) {
	    Core::Application::us()->error("No best path found; all pathes have infinite score.");
	    return std::make_pair(ConstLatticeRef(), Semiring::Max);
	}
	StaticLattice *s = new StaticLattice(l->type());
	s->setProperties(l->knownProperties() | Fsa::PropertyLinear, l->properties() | Fsa::PropertyLinear);
	s->setInputAlphabet(l->getInputAlphabet());
	if (l->type() != Fsa::TypeAcceptor)
		s->setOutputAlphabet(l->getOutputAlphabet());
	s->setSemiring(l->semiring());
	s->setBoundaries(l->getBoundaries());
	s->setDescription(Core::form("best-projection(%s)", l->describe().c_str()));
	Fsa::StateId bestSid = bestTrace.sid;
	ConstStateRef sr = l->getState(bestSid);
	State *sp = new State(sr->id(), sr->tags(), sr->weight()); s->setState(sp);
	verify(sp->isFinal());
	while (bestSid != initialSid) {
	    const TraceElement &trace = traceback[bestSid];
	    sr = l->getState(trace.sid);
	    sp = new State(sr->id(), sr->tags(), sr->weight()); s->setState(sp);
	    *sp->newArc() = *(sr->begin() + trace.aid);
	    bestSid = trace.sid;
	}
	s->setInitialStateId(sp->id());
	return std::make_pair(ConstLatticeRef(s), bestTrace.score);
    }
    // -------------------------------------------------------------------------


    // -------------------------------------------------------------------------
    void dumpBest(ConstLatticeRef l, std::ostream &os) {
	ConstSemiringRef semiring = l->semiring();
	ScoresRef sum = semiring->one();
	os << "\"";
	if (l && (l->initialStateId() != Fsa::InvalidStateId)) {
	    ConstLatticeRef b = best(l);
	    Fsa::ConstAlphabetRef alphabet = b->getInputAlphabet();
	    ConstStateRef sr = b->getState(b->initialStateId());
	    for (; sr->hasArcs(); sr = b->getState(sr->begin()->target())) {
		sum = semiring->extend(sum, sr->begin()->weight());
		os << alphabet->symbol(sr->begin()->input()) << " ";
	    }
	    sum = semiring->extend(sum, sr->weight());
	}
	os << "\""
	   << ", score=" << semiring->project(sum)
	   << ", scores=" << semiring->describe(sum, Fsa::HintShowDetails | HintUnscaled)
	   << std::endl;
    }
    // -------------------------------------------------------------------------


    // -------------------------------------------------------------------------
    ConstLatticeRef best(ConstLatticeRef l, SingleSourceShortestPathAlgorithm algo) {
	if (!l || (l->initialStateId() == Fsa::InvalidStateId))
	    return ConstLatticeRef();
	switch (algo) {
	case Dijkstra:
	    return FtlWrapper::firstbest(l);
	case BellmanFord:
	    return FtlWrapper::best(l);
	case ProjectingBellmanFord:
	    return bestProjection(l).first;
	default:
	    defect();
	    return ConstLatticeRef();
	}
    }
    // -------------------------------------------------------------------------


    // -------------------------------------------------------------------------
    namespace {
	const Core::Choice choiceSsspAlgorithm(
	    "dijkstra",                Dijkstra,
	    "bellman-ford",            BellmanFord,
	    "projecting-bellman-ford", ProjectingBellmanFord,
	    Core::Choice::endMark());
	const Core::ParameterString paramSsspAlgorithm(
	    "algorithm",
	    "single source shortest path algorithm",
	    "dijkstra");
	SingleSourceShortestPathAlgorithm getSsspAlgorithm(const Core::Configuration &config) {
	    Core::Choice::Value ssspChoice = choiceSsspAlgorithm[paramSsspAlgorithm(config)];
	    if (ssspChoice == Core::Choice::IllegalValue)
		Core::Application::us()->criticalError(
		    "SingleSourceShortestPathAlgorithm: Unknown algorithm \"%s\"",
		    paramSsspAlgorithm(config).c_str());
	    return SingleSourceShortestPathAlgorithm(ssspChoice);
	}
    } // namespace

    class BestNode : public FilterNode {
	typedef FilterNode Precursor;
    private:
	SingleSourceShortestPathAlgorithm ssspAlg_;

    protected:
	virtual ConstLatticeRef filter(ConstLatticeRef l) {
	    return best(l, ssspAlg_);
	}
    public:
	BestNode(const std::string &name, const Core::Configuration &config) :
	    Precursor(name, config) {}
	virtual ~BestNode() {}
	virtual void init(const std::vector<std::string> &arguments) {
	    ssspAlg_ = getSsspAlgorithm(config);
	}
    };

    NodeRef createBestNode(const std::string &name, const Core::Configuration &config) {
	return NodeRef(new BestNode(name, config));
    }
    // -------------------------------------------------------------------------


    // -------------------------------------------------------------------------
    struct StateIdPair : public std::pair<Fsa::StateId, Fsa::StateId> {
	typedef std::pair<Fsa::StateId, Fsa::StateId> Precursor;
	StateIdPair(Fsa::StateId first, Fsa::StateId second) :
	    Precursor(first, second) {}
	struct Hash : public Core::hash<Fsa::StateId> {
	    typedef Core::hash<Fsa::StateId> Precursor;
	    size_t operator() (const StateIdPair &pair) const
		{ return Precursor::operator()(pair.first | ~pair.second); }
	};
    };

    class AllPairsShortestDistance::Internal :
	    public Core::hash_map<StateIdPair, Score, StateIdPair::Hash> {
	typedef Core::hash_map<StateIdPair, Score, StateIdPair::Hash> Precursor;
    private:
	struct SortDistanceByState {
	    bool operator() (const AllPairsShortestDistance::Distance &d1, const AllPairsShortestDistance::Distance &d2) const {
		return (d1.from != d2.from) ? (d1.from < d2.from) : (d1.to < d2.to);
	    }
	};
	mutable AllPairsShortestDistance::Distance *distance_;

    public:
	Internal() : Precursor(), distance_(0) {}
	~Internal() { delete [] distance_; }

	Score get(const StateIdPair &key) const {
	    Precursor::const_iterator it = Precursor::find(key);
	    return (it != Precursor::end()) ? it->second : Semiring::Invalid;
	}

	AllPairsShortestDistance::const_iterator begin() const {
	    if (!distance_) {
		AllPairsShortestDistance::Distance *itDist = distance_ = new Distance[size()];
		for (const_iterator itMap = Precursor::begin(), endMap = Precursor::end(); itMap != endMap; ++itMap, ++itDist) {
		    itDist->from  = itMap->first.first;
		    itDist->to    = itMap->first.second;
		    itDist->score = itMap->second;
		}
		std::sort(distance_, distance_ + size(), SortDistanceByState());
	    }
	    return distance_;
	}

	AllPairsShortestDistance::const_iterator end() const {
	    return begin() + size();
	}
    };

    AllPairsShortestDistance::AllPairsShortestDistance(const Internal *internal) :
	internal_(internal) {}

    AllPairsShortestDistance::~AllPairsShortestDistance() {
	delete internal_;
    }

    Score AllPairsShortestDistance::get(Fsa::StateId from, Fsa::StateId to) const {
	return internal_->get(StateIdPair(from, to));
    }

    AllPairsShortestDistance::const_iterator AllPairsShortestDistance::begin() const {
	return internal_->begin();
    }

    AllPairsShortestDistance::const_iterator AllPairsShortestDistance::end() const {
	return internal_->end();
    }

    namespace {
	struct StateIdScorePair {
	    Fsa::StateId sid;
	    Score score;
	    StateIdScorePair(Fsa::StateId sid, Score score) :
		sid(sid), score(score) {}
	    bool operator< (const StateIdScorePair &pair) const
		{ return sid < pair.sid; }
	};
	typedef std::vector<StateIdScorePair> StateIdScorePairList;
    } // namespace

    ConstAllPairsShortestDistanceRef AllPairsShortestDistance::create(ConstLatticeRef lRef, Time timeThreshold) {
	Internal *internal = new Internal;
	ConstStateMapRef topologicalSort = sortTopologically(lRef);
	const Lattice &l = *lRef;
	const Semiring &semiring = *l.semiring();
	const Boundaries &b = *l.getBoundaries();
	std::vector<StateIdScorePairList> D(topologicalSort->maxSid + 1);
	for (StateMap::const_iterator itSid = topologicalSort->begin(),
		     endSid = topologicalSort->end(); itSid != endSid; ++itSid) {
	    Fsa::StateId sid = *itSid;
	    ConstStateRef sr = l.getState(sid);
	    StateIdScorePairList &from = D[sid];
	    // update self
	    StateIdScorePair d(sid, Semiring::One);
	    StateIdScorePairList::iterator itPair = std::lower_bound(from.begin(), from.end(), d);
	    from.insert(itPair, d);
	    // update successors
	    for (State::const_iterator a = sr->begin(), a_begin = sr->begin(), a_end = sr->end(); a != a_end; ++a) {
		StateIdScorePairList &to = D[a->target()];
		Time toTime = b.time(sr->id());
		Score score = semiring.project(a->weight());
		for (StateIdScorePairList::const_iterator itFrom = from.begin(), endFrom = from.end(); itFrom != endFrom; ++itFrom) {
		    Time fromTime = b.time(itFrom->sid);
		    verify(fromTime <= toTime);
		    if (toTime - fromTime <= timeThreshold) {
			d.sid = itFrom->sid;
			d.score = itFrom->score + score;
			itPair = std::lower_bound(to.begin(), to.end(), d);
			if ((itPair == to.end()) || (itPair->sid != d.sid))
			    to.insert(itPair, d);
			else
			    if (d.score < itPair->score) itPair->score = d.score;
		    }
		}
	    }
	    // store into final data structure
	    for (StateIdScorePairList::const_iterator itFrom = from.begin(), endFrom = from.end(); itFrom != endFrom; ++itFrom)
		internal->insert(std::make_pair(StateIdPair(itFrom->sid, sid), itFrom->score));
	}
	return ConstAllPairsShortestDistanceRef(new AllPairsShortestDistance(internal));
    }
    // -------------------------------------------------------------------------


    // -------------------------------------------------------------------------
    class DumpAllPairsShortestDistanceNode : public FilterNode {
	typedef FilterNode Precursor;
    public:
	static const Core::ParameterFloat paramTimeThreshold;
    private:
	Core::Channel dump_;
	bool hasDumped_;
	Time timeThreshold_;
    protected:
	virtual ConstLatticeRef filter(ConstLatticeRef l) {
	    if (!hasDumped_) {
		ConstAllPairsShortestDistanceRef D = AllPairsShortestDistance::create(l, timeThreshold_);
		if (connected(1))
		    printSegmentHeader(dump_, requestSegment(1));
		dump_ << "# from\tto\tscore\n";
		for (AllPairsShortestDistance::const_iterator itD = D->begin(), endD = D->end(); itD != endD; ++itD)
		    dump_ << itD->from << "\t" << itD->to << "\t" << itD->score << "\n";
		dump_ << "\n";
		hasDumped_ = true;
	    }
	    return l;
	}
    public:
	DumpAllPairsShortestDistanceNode(const std::string &name, const Core::Configuration &config) :
	    Precursor(name, config), dump_(config, "dump"), hasDumped_(false) {}
	~DumpAllPairsShortestDistanceNode() {}
	virtual void init(const std::vector<std::string> &arguments) {
	    f32 tmp = paramTimeThreshold(config);
	    if (tmp != Core::Type<f32>::max) {
		timeThreshold_ = Time(Core::round(tmp * 100.0));
		verify(timeThreshold_ >= 0);
		log("time threshold is %.2fsec", (f32(timeThreshold_) / 100.0));
	    } else
		timeThreshold_ = Core::Type<Time>::max;
	}
	virtual void sync() {
	    hasDumped_ = false;
	}
    };
    const Core::ParameterFloat DumpAllPairsShortestDistanceNode::paramTimeThreshold(
	"time-threshold",
	"time threshold",
	Core::Type<f32>::max);
    NodeRef createDumpAllPairsShortestDistanceNode(const std::string &name, const Core::Configuration &config) {
	return NodeRef(new DumpAllPairsShortestDistanceNode(name, config));
    }
    // -------------------------------------------------------------------------

} // namespace Flf

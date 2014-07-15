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
#include <Core/Vector.hh>
#include <Fsa/hSort.hh>
#include <Fsa/Hash.hh>

#include "FlfCore/Basic.hh"
#include "FlfCore/LatticeInternal.hh"
#include "FlfCore/TopologicalOrderQueue.hh"
#include "FlfCore/Utility.hh"
#include "Cache.hh"
#include "Copy.hh"
#include "EpsilonRemoval.hh"
#include "Lexicon.hh"
#include "Map.hh"
#include "NonWordFilter.hh"


namespace Flf {

    // -------------------------------------------------------------------------
    struct Hyp {
	bool visited;
	Score score;
	Fsa::StateId bptr;
	State::const_iterator a;
	Hyp() : visited(false), score(Semiring::Max), bptr(Fsa::InvalidStateId) {}
    };
    typedef Core::Vector<Hyp> HypList;
    typedef Core::Vector<Fsa::StateId> StateIdList;
    // -------------------------------------------------------------------------


    // -------------------------------------------------------------------------
    StaticLatticeRef applyEpsClosureFilter(ConstLatticeRef l) {
	verify_(l->hasProperty(Fsa::PropertyAcyclic));
	l = sort(l, Fsa::SortTypeByInputAndTarget);
	l = persistent(l);
	ConstStateMapRef topologicalOrderMap = findTopologicalOrder(l);
	require(topologicalOrderMap);
	TopologicalOrderQueueRef nonEpsQueue =
	    createTopologicalOrderQueue(l, topologicalOrderMap);
	TopologicalOrderQueue &Q = *nonEpsQueue;
	TopologicalOrderQueueRef epsQueue =
	    createTopologicalOrderQueue(l, topologicalOrderMap);
	TopologicalOrderQueue &epsQ = *epsQueue;
	StateIdList epsClosure, epsHull;
	const ScoreList &scales = l->semiring()->scales();

	StaticLattice *s = new StaticLattice(l->type());
	s->setProperties(l->knownProperties(), l->properties());
	s->setInputAlphabet(l->getInputAlphabet());
	if (s->type() != Fsa::TypeAcceptor)
	    s->setOutputAlphabet(l->getOutputAlphabet());
	s->setSemiring(l->semiring());
	s->setInitialStateId(l->initialStateId());
	s->setBoundaries(l->getBoundaries());
	s->setProperties(Fsa::PropertySortedByInputAndTarget, Fsa::PropertyAll);
	s->setDescription(Core::form("eps-closure-filter(%s)",
				     l->describe().c_str()));

	HypList hyps(topologicalOrderMap->maxSid + 1);
	Core::Vector<bool> visited(topologicalOrderMap->maxSid + 1, false);
	Q.insert(l->initialStateId());
	while (!Q.empty()) {
	    Fsa::StateId sid = Q.top(); Q.pop();
	    if (visited[sid])
		continue;
	    visited[sid] = true;
	    verify((hyps[sid].score == Semiring::Max) && (hyps[sid].bptr == Fsa::InvalidStateId));
	    // filter epsilon closure
	    hyps[sid].score = Semiring::One;
	    epsQ.insert(sid);
	    while (!epsQ.empty()) {
		Fsa::StateId epsSid = epsQ.top(); epsQ.pop();
		ConstStateRef epsSr = l->getState(epsSid);
		Score fwdScore = hyps[epsSid].score;
		State::const_iterator epsA = epsSr->begin(), epsEnd = epsSr->end();
		for (; (epsA != epsEnd) && (epsA->input() == Fsa::Epsilon); ++epsA) {
		    Score score = fwdScore + epsA->weight()->project(scales);
		    Hyp &hyp = hyps[epsA->target()];
		    if (!hyp.visited || (score < hyp.score)) {
			if (!hyp.visited) {
			    hyp.visited = true;
			    epsQ.insert(epsA->target());
			}
			hyp.score = score;
			hyp.bptr = epsSid;
			hyp.a = epsA;
		    }
		}
		if (epsA != epsEnd) {
		    Q.insert(epsSid);
		    epsHull.push_back(epsSid);
		} else if (epsSr->isFinal()){
		    epsHull.push_back(epsSid);
		}
		epsClosure.push_back(epsSid);
	    }
	    // add epsilon arcs; reset traceback arrays
	    for (StateIdList::const_iterator itSid = epsHull.begin(), endSid = epsHull.end(); itSid != endSid; ++itSid) {
		Fsa::StateId epsSid = *itSid;
		if (!s->hasState(epsSid)) {
		    ConstStateRef epsSr = l->getState(epsSid);
		    s->setState(new State(epsSr->id(), epsSr->tags(), epsSr->weight()));
		}
		for (;;) {
		    Hyp &hyp = hyps[epsSid];
		    if (hyp.bptr == Fsa::InvalidStateId)
			break;
		    if (!s->hasState(hyp.bptr)) {
			ConstStateRef bptrSr = l->getState(hyp.bptr);
			s->setState(new State(bptrSr->id(), bptrSr->tags(), bptrSr->weight()));
		    }
		    State *sp = s->fastState(hyp.bptr);
		    const Arc &a = *hyp.a;
		    State::iterator pos = sp->lower_bound(a, Ftl::byInputAndTarget<Lattice>());
		    if ((pos == sp->end())
			|| (a.target() != pos->target()) || (a.input() != pos->input()))
			sp->insert(pos, a);
		    epsSid = hyp.bptr;
		    hyp.bptr = Fsa::InvalidStateId;
		}
	    }
	    epsHull.clear();
	    for (StateIdList::const_iterator itSid = epsClosure.begin(), endSid = epsClosure.end(); itSid != endSid; ++itSid) {
		Hyp &hyp = hyps[*itSid];
		hyp.visited = false;
		hyp.score = Semiring::Max;
		hyp.bptr = Fsa::InvalidStateId;
	    }
	    epsClosure.clear();
	    // add non-epsilon arcs
	    ConstStateRef sr = l->getState(sid);
	    State::const_iterator a = sr->begin(), a_end = sr->end();
	    verify(s->hasState(sid));
	    State *sp = s->fastState(sid);
	    for (; (a != a_end) && (a->input() == Fsa::Epsilon); ++a);
	    for (; a != a_end; ++a) {
		*sp->newArc() = *a;
		Q.insert(a->target());
	    }
	    verify(!hyps[sid].visited && (hyps[sid].bptr == Fsa::InvalidStateId));
	    // hyps[sid].score = Semiring::Max; // should not be necessary
	    verify(s->hasState(sid));
	}

	return StaticLatticeRef(s);
    }
    // -------------------------------------------------------------------------



    // -------------------------------------------------------------------------
    StaticLatticeRef applyEpsClosureWeakDeterminizationFilter(ConstLatticeRef l) {
	verify_(l->hasProperty(Fsa::PropertyAcyclic));
	l = sort(l, Fsa::SortTypeByInputAndTarget);
	l = persistent(l);
	ConstStateMapRef topologicalOrderMap = findTopologicalOrder(l);
	require(topologicalOrderMap);
	TopologicalOrderQueueRef nonEpsQueue =
	    createTopologicalOrderQueue(l, topologicalOrderMap);
	TopologicalOrderQueue &Q = *nonEpsQueue;
	TopologicalOrderQueueRef epsQueue =
	    createTopologicalOrderQueue(l, topologicalOrderMap);
	TopologicalOrderQueue &epsQ = *epsQueue;
	StateIdList epsClosure, epsHull;
	const ScoreList &scales = l->semiring()->scales();

	StaticLattice *s = new StaticLattice(l->type());
	s->setProperties(l->knownProperties(), l->properties());
	s->setInputAlphabet(l->getInputAlphabet());
	if (s->type() != Fsa::TypeAcceptor)
	    s->setOutputAlphabet(l->getOutputAlphabet());
	s->setSemiring(l->semiring());
	s->setInitialStateId(l->initialStateId());
	s->setBoundaries(l->getBoundaries());
	s->setProperties(Fsa::PropertySortedByInputAndTarget, Fsa::PropertyAll);
	s->setDescription(Core::form("eps-closure-weak-determinization-filter(%s)",
				     l->describe().c_str()));

	HypList hyps(topologicalOrderMap->maxSid + 1);
	Core::Vector<bool> visited(topologicalOrderMap->maxSid + 1, false);
	Q.insert(l->initialStateId());
	while (!Q.empty()) {
	    Fsa::StateId sid = Q.top(); Q.pop();
	    if (visited[sid])
		continue;
	    visited[sid] = true;
	    // initialize source state
	    verify((hyps[sid].score == Semiring::Max) && (hyps[sid].bptr == Fsa::InvalidStateId));
	    hyps[sid].score = Semiring::One;
	    ConstStateRef sr = l->getState(sid);
	    State::const_iterator a = sr->begin(), a_end = sr->end();
	    if (sid != l->initialStateId())
		for (; (a != a_end) && (a->input() == Fsa::Epsilon); ++a);
	    while (a != a_end) {
		// initialize label-epsilon closure
		for(Fsa::LabelId label = a->input(); (a != a_end) && (a->input() == label); ++a) {
		    Score score = a->weight()->project(scales);
		    Hyp &hyp = hyps[a->target()];
		    if (!hyp.visited || (score < hyp.score)) {
			if (!hyp.visited) {
			    hyp.visited = true;
			    epsQ.insert(a->target());
			}
			hyp.score = score;
			hyp.bptr = sid;
			hyp.a = a;
		    }
		}
		verify_(!epsQ.empty());
		// filter label-epsilon closure
		while (!epsQ.empty()) {
		    Fsa::StateId epsSid = epsQ.top(); epsQ.pop();
		    ConstStateRef epsSr = l->getState(epsSid);
		    Score fwdScore = hyps[epsSid].score;
		    State::const_iterator epsA = epsSr->begin(), epsEnd = epsSr->end();
		    for (; (epsA != epsEnd) && (epsA->input() == Fsa::Epsilon); ++epsA) {
			Score score = fwdScore + epsA->weight()->project(scales);
			Hyp &hyp = hyps[epsA->target()];
			if (!hyp.visited || (score < hyp.score)) {
			    if (!hyp.visited) {
				hyp.visited = true;
				epsQ.insert(epsA->target());
			    }
			    hyp.score = score;
			    hyp.bptr = epsSid;
			    hyp.a = epsA;
			}
		    }
		    if (epsA != epsEnd) {
			Q.insert(epsSid);
			epsHull.push_back(epsSid);
		    } else if (epsSr->isFinal()){
			epsHull.push_back(epsSid);
		    }
		    epsClosure.push_back(epsSid);
		}
		// add label and epsilon arcs; reset traceback arrays
		for (StateIdList::const_iterator itSid = epsHull.begin(), endSid = epsHull.end(); itSid != endSid; ++itSid) {
		    Fsa::StateId epsSid = *itSid;
		    if (!s->hasState(epsSid)) {
			ConstStateRef epsSr = l->getState(epsSid);
			s->setState(new State(epsSr->id(), epsSr->tags(), epsSr->weight()));
		    }
		    for (;;) {
			Hyp &hyp = hyps[epsSid];
			if (hyp.bptr == Fsa::InvalidStateId)
			    break;
			if (!s->hasState(hyp.bptr)) {
			    ConstStateRef bptrSr = l->getState(hyp.bptr);
			    s->setState(new State(bptrSr->id(), bptrSr->tags(), bptrSr->weight()));
			}
			State *sp = s->fastState(hyp.bptr);
			const Arc &a = *hyp.a;
			State::iterator pos = sp->lower_bound(a, Ftl::byInputAndTarget<Lattice>());
			if ((pos == sp->end())
			    || (a.target() != pos->target()) || (a.input() != pos->input()))
			    sp->insert(pos, a);
			epsSid = hyp.bptr;
			hyp.bptr = Fsa::InvalidStateId;
		    }
		}
		verify_(s->hasState(sid) && (s->fastState(sid)->hasArcs()));
		epsHull.clear();
		for (StateIdList::const_iterator itSid = epsClosure.begin(), endSid = epsClosure.end(); itSid != endSid; ++itSid) {
		    Hyp &hyp = hyps[*itSid];
		    hyp.visited = false;
		    hyp.score = Semiring::Max;
		    hyp.bptr = Fsa::InvalidStateId;
		}
		epsClosure.clear();
	    }
	    if (!s->hasState(sid)) {
		ConstStateRef sr = l->getState(sid);
		s->setState(new State(sr->id(), sr->tags(), sr->weight()));
	    }
	    verify(!hyps[sid].visited && (hyps[sid].bptr == Fsa::InvalidStateId));
	    hyps[sid].score = Semiring::Max;
	    verify(s->hasState(sid));
	}

	return StaticLatticeRef(s);
    }
    // -------------------------------------------------------------------------



    // -------------------------------------------------------------------------
    StaticLatticeRef applyEpsClosureStrongDeterminizationFilter(ConstLatticeRef l) {
	verify_(l->hasProperty(Fsa::PropertyAcyclic));
	l = sort(l, Fsa::SortTypeByInputAndTarget);
	l = persistent(l);
	ConstStateMapRef topologicalOrderMap = findTopologicalOrder(l);
	require(topologicalOrderMap);
	TopologicalOrderQueueRef nonEpsQueue =
	    createTopologicalOrderQueue(l, topologicalOrderMap);
	TopologicalOrderQueue &Q = *nonEpsQueue;
	TopologicalOrderQueueRef epsQueue =
	    createTopologicalOrderQueue(l, topologicalOrderMap);
	TopologicalOrderQueue &epsQ = *epsQueue;
	StateIdList leftEpsClosure, leftEpsFinalsHull, rightEpsClosure, rightEpsHull;
	typedef Core::Vector<std::pair<Fsa::StateId, std::pair<State::const_iterator, State::const_iterator> > > StateArcRangeList;
	StateArcRangeList leftEpsExtendedHull;
	const ScoreList &scales = l->semiring()->scales();

	StaticLattice *s = new StaticLattice(l->type());
	s->setProperties(l->knownProperties(), l->properties());
	s->setInputAlphabet(l->getInputAlphabet());
	if (s->type() != Fsa::TypeAcceptor)
	    s->setOutputAlphabet(l->getOutputAlphabet());
	s->setSemiring(l->semiring());
	s->setInitialStateId(l->initialStateId());
	s->setBoundaries(l->getBoundaries());
	s->setProperties(Fsa::PropertySortedByInputAndTarget, Fsa::PropertyAll);
	s->setDescription(Core::form("eps-closure-strong-determinization-filter(%s)",
				     l->describe().c_str()));

	HypList
	    leftHyps(topologicalOrderMap->maxSid + 1),
	    rightHyps(topologicalOrderMap->maxSid + 1);
	Core::Vector<bool> visited(topologicalOrderMap->maxSid + 1, false);
	Q.insert(l->initialStateId());
	while (!Q.empty()) {
	    Fsa::StateId sid = Q.top(); Q.pop();
	    if (visited[sid])
		continue;
	    visited[sid] = true;
	    // initialize left epsilon closure
	    verify((leftHyps[sid].score == Semiring::Max) && (leftHyps[sid].bptr == Fsa::InvalidStateId));
	    leftHyps[sid].score = Semiring::One;
	    Fsa::LabelId nextLabel = Core::Type<Fsa::LabelId>::max;
	    epsQ.insert(sid);
	    while (!epsQ.empty()) {
		Fsa::StateId epsSid = epsQ.top(); epsQ.pop();
		ConstStateRef epsSr = l->getState(epsSid);
		Score fwdScore = leftHyps[epsSid].score;
		State::const_iterator epsA = epsSr->begin(), epsEnd = epsSr->end();
		for (; (epsA != epsEnd) && (epsA->input() == Fsa::Epsilon); ++epsA) {
		    Score score = fwdScore + epsA->weight()->project(scales);
		    Hyp &leftHyp = leftHyps[epsA->target()];
		    if (!leftHyp.visited || (score < leftHyp.score)) {
			if (!leftHyp.visited) {
			    leftHyp.visited = true;
			    epsQ.insert(epsA->target());
			}
			leftHyp.score = score;
			leftHyp.bptr = epsSid;
			leftHyp.a = epsA;
		    }
		}
		if (epsA != epsEnd) {
		    leftEpsExtendedHull.push_back(std::make_pair(epsSid, std::make_pair(epsA, epsEnd)));
		    if (epsA->input() < nextLabel) nextLabel = epsA->input();
		}
		if (epsSr->isFinal())
		    leftEpsFinalsHull.push_back(epsSid);
		leftEpsClosure.push_back(epsSid);
	    }
	    // add left finals
	    for (StateIdList::const_iterator itSid = leftEpsFinalsHull.begin(), endSid = leftEpsFinalsHull.end(); itSid != endSid; ++itSid) {
		Fsa::StateId epsSid = *itSid;
		if (!s->hasState(epsSid)) {
		    ConstStateRef epsSr = l->getState(epsSid);
		    s->setState(new State(epsSr->id(), epsSr->tags(), epsSr->weight()));
		}
		for (;;) {
		    Hyp &leftHyp = leftHyps[epsSid];
		    if (leftHyp.bptr == Fsa::InvalidStateId)
			break;
		    if (!s->hasState(leftHyp.bptr)) {
			ConstStateRef bptrSr = l->getState(leftHyp.bptr);
			s->setState(new State(bptrSr->id(), bptrSr->tags(), bptrSr->weight()));
		    }
		    State *sp = s->fastState(leftHyp.bptr);
		    const Arc &a = *leftHyp.a;
		    State::iterator pos = sp->lower_bound(a, Ftl::byInputAndTarget<Lattice>());
		    if ((pos == sp->end())
			|| (a.target() != pos->target()) || (a.input() != pos->input()))
			sp->insert(pos, a);
		    epsSid = leftHyp.bptr;
		    leftHyp.bptr = Fsa::InvalidStateId;
		}
	    }
	    leftEpsFinalsHull.clear();
	    // filter right label-epsilon closure
	    while (nextLabel != Core::Type<Fsa::LabelId>::max) {
		Fsa::LabelId label = nextLabel;
		nextLabel = Core::Type<Fsa::LabelId>::max;
		for (StateArcRangeList::iterator itSidRange = leftEpsExtendedHull.begin(), endSidRange = leftEpsExtendedHull.end(); itSidRange != endSidRange; ++itSidRange) {
		    Fsa::StateId epsSid = itSidRange->first;
		    Score fwdScore = leftHyps[epsSid].score;
		    std::pair<State::const_iterator, State::const_iterator> &epsRange = itSidRange->second;
		    for (; (epsRange.first != epsRange.second) && (epsRange.first->input() == label); ++epsRange.first) {
			Score score = fwdScore + epsRange.first->weight()->project(scales);
			Hyp &rightHyp = rightHyps[epsRange.first->target()];
			if (!rightHyp.visited || (score < rightHyp.score)) {
			    if (!rightHyp.visited) {
				rightHyp.visited = true;
				epsQ.insert(epsRange.first->target());
			    }
			    rightHyp.score = score;
			    rightHyp.bptr = epsSid;
			    rightHyp.a = epsRange.first;
			}
		    }
		    if ((epsRange.first != epsRange.second) && (epsRange.first->input() < nextLabel))
			nextLabel = epsRange.first->input();
		}
		while (!epsQ.empty()) {
		    Fsa::StateId epsSid = epsQ.top(); epsQ.pop();
		    ConstStateRef epsSr = l->getState(epsSid);
		    Score fwdScore = rightHyps[epsSid].score;
		    State::const_iterator epsA = epsSr->begin(), epsEnd = epsSr->end();
		    for (; (epsA != epsEnd) && (epsA->input() == Fsa::Epsilon); ++epsA) {
			Score score = fwdScore + epsA->weight()->project(scales);
			Hyp &rightHyp = rightHyps[epsA->target()];
			if (!rightHyp.visited || (score < rightHyp.score)) {
			    if (!rightHyp.visited) {
				rightHyp.visited = true;
				epsQ.insert(epsA->target());
			    }
			    rightHyp.score = score;
			    rightHyp.bptr = epsSid;
			    rightHyp.a = epsA;
			}
		    }
		    if (epsA != epsEnd) {
			Q.insert(epsSid);
			rightEpsHull.push_back(epsSid);
		    } else if (epsSr->isFinal())
			rightEpsHull.push_back(epsSid);
		    rightEpsClosure.push_back(epsSid);
		}
		// add label and epsilon arcs from right and left closures; reset traceback arrays
		for (StateIdList::const_iterator itSid = rightEpsHull.begin(), endSid = rightEpsHull.end(); itSid != endSid; ++itSid) {
		    // right epsilon closure
		    Fsa::StateId epsSid = *itSid;
		    if (!s->hasState(epsSid)) {
			ConstStateRef epsSr = l->getState(epsSid);
			s->setState(new State(epsSr->id(), epsSr->tags(), epsSr->weight()));
		    }
		    for (;;) {
			Hyp &rightHyp = rightHyps[epsSid];
			if (rightHyp.bptr == Fsa::InvalidStateId) {
			    // left epsilon closure
			    for (;;) {
				Hyp &leftHyp = leftHyps[epsSid];
				if (leftHyp.bptr == Fsa::InvalidStateId)
				    break;
				if (!s->hasState(leftHyp.bptr)) {
				    ConstStateRef bptrSr = l->getState(leftHyp.bptr);
				    s->setState(new State(bptrSr->id(), bptrSr->tags(), bptrSr->weight()));
				}
				State *sp = s->fastState(leftHyp.bptr);
				const Arc &a = *leftHyp.a;
				State::iterator pos = sp->lower_bound(a, Ftl::byInputAndTarget<Lattice>());
				if ((pos == sp->end())
				    || (a.target() != pos->target()) || (a.input() != pos->input()))
				    sp->insert(pos, a);
				epsSid = leftHyp.bptr;
				leftHyp.bptr = Fsa::InvalidStateId;
			    }
			    break;
			}
			if (!s->hasState(rightHyp.bptr)) {
			    ConstStateRef bptrSr = l->getState(rightHyp.bptr);
			    s->setState(new State(bptrSr->id(), bptrSr->tags(), bptrSr->weight()));
			}
			State *sp = s->fastState(rightHyp.bptr);
			const Arc &a = *rightHyp.a;
			State::iterator pos = sp->lower_bound(a, Ftl::byInputAndTarget<Lattice>());
			if ((pos == sp->end())
			    || (a.target() != pos->target()) || (a.input() != pos->input()))
			    sp->insert(pos, a);
			epsSid = rightHyp.bptr;
			rightHyp.bptr = Fsa::InvalidStateId;
		    }
		}
		rightEpsHull.clear();
		for (StateIdList::const_iterator itSid = rightEpsClosure.begin(), endSid = rightEpsClosure.end(); itSid != endSid; ++itSid) {
		    Hyp &rightHyp = rightHyps[*itSid];
		    rightHyp.visited = false;
		    rightHyp.score = Semiring::Max;
		    rightHyp.bptr = Fsa::InvalidStateId;
		}
		rightEpsClosure.clear();
	    }
	    for (StateIdList::const_iterator itSid = leftEpsClosure.begin(), endSid = leftEpsClosure.end(); itSid != endSid; ++itSid) {
		Hyp &leftHyp = leftHyps[*itSid];
		leftHyp.score = Semiring::Max;
		leftHyp.bptr = Fsa::InvalidStateId;
	    }
	    leftEpsClosure.clear();
	    verify(!leftHyps[sid].visited && (leftHyps[sid].bptr == Fsa::InvalidStateId));
	    leftHyps[sid].score = Semiring::Max;
	    verify(s->hasState(sid));
	}

	return StaticLatticeRef(s);
    }
    // -------------------------------------------------------------------------



    // -------------------------------------------------------------------------
    class NonWordClosureFilterNode : public FilterNode {
    protected:
	virtual ConstLatticeRef filter(ConstLatticeRef l) {
	    if (!l)
		return ConstLatticeRef();
	    if (l->type() != Fsa::TypeAcceptor) {
		warning("%s: \"%s\" is a transducer, but result will be an acceptor, i.e. output will be lost.",
			name.c_str(), l->describe().c_str());
		l = projectInput(l);
	    }
	    l = transducer(l);
	    l = applyOneToOneLabelMap(l, LabelMap::createNonWordToEpsilonMap(Lexicon::us()->alphabetId(l->getInputAlphabet())));
	    l = applyEpsClosureFilter(l);
	    l = projectOutput(l);
	    l->setProperties(Fsa::PropertySorted, 0);
	    verify(l->type() == Fsa::TypeAcceptor);
	    return l;
	}
    public:
	NonWordClosureFilterNode(const std::string &name, const Core::Configuration &config) :
	    FilterNode(name, config) {}
	~NonWordClosureFilterNode() {}
    };
    NodeRef createNonWordClosureFilterNode(
	const std::string &name, const Core::Configuration &config) {
	return NodeRef(new NonWordClosureFilterNode(name, config));
    }
    // -------------------------------------------------------------------------



    // -------------------------------------------------------------------------
    class NonWordClosureWeakDeterminizationFilterNode : public FilterNode {
    protected:
	virtual ConstLatticeRef filter(ConstLatticeRef l) {
	    if (!l)
		return ConstLatticeRef();
	    if (l->type() != Fsa::TypeAcceptor) {
		warning("%s: \"%s\" is a transducer, but result will be an acceptor, i.e. output will be lost.",
			name.c_str(), l->describe().c_str());
		l = projectInput(l);
	    }
	    l = transducer(l);
	    l = applyOneToOneLabelMap(l, LabelMap::createNonWordToEpsilonMap(Lexicon::us()->alphabetId(l->getInputAlphabet())));
	    l = applyEpsClosureWeakDeterminizationFilter(l);
	    l = projectOutput(l);
	    l->setProperties(Fsa::PropertySorted, 0);
	    verify(l->type() == Fsa::TypeAcceptor);
	    return l;
	}
    public:
	NonWordClosureWeakDeterminizationFilterNode(const std::string &name, const Core::Configuration &config) :
	    FilterNode(name, config) {}
	~NonWordClosureWeakDeterminizationFilterNode() {}
    };
    NodeRef createNonWordClosureWeakDeterminizationFilterNode(
	const std::string &name, const Core::Configuration &config) {
	return NodeRef(new NonWordClosureWeakDeterminizationFilterNode(name, config));
    }
    // -------------------------------------------------------------------------



    // -------------------------------------------------------------------------
    class NonWordClosureStrongDeterminizationFilterNode : public FilterNode {
    protected:
	virtual ConstLatticeRef filter(ConstLatticeRef l) {
	    if (!l)
		return ConstLatticeRef();
	    if (l->type() != Fsa::TypeAcceptor) {
		warning("%s: \"%s\" is a transducer, but result will be an acceptor, i.e. output will be lost.",
			name.c_str(), l->describe().c_str());
		l = projectInput(l);
	    }
	    l = transducer(l);
	    l = applyOneToOneLabelMap(l, LabelMap::createNonWordToEpsilonMap(Lexicon::us()->alphabetId(l->getInputAlphabet())));
	    l = applyEpsClosureStrongDeterminizationFilter(l);
	    l = projectOutput(l);
	    l->setProperties(Fsa::PropertySorted, 0);
	    verify(l->type() == Fsa::TypeAcceptor);
	    return l;
	}
    public:
	NonWordClosureStrongDeterminizationFilterNode(const std::string &name, const Core::Configuration &config) :
	    FilterNode(name, config) {}
	~NonWordClosureStrongDeterminizationFilterNode() {}
    };
    NodeRef createNonWordClosureStrongDeterminizationFilterNode(
	const std::string &name, const Core::Configuration &config) {
	return NodeRef(new NonWordClosureStrongDeterminizationFilterNode(name, config));
    }
    // -------------------------------------------------------------------------



    // -------------------------------------------------------------------------
    class NormalizeEpsilonClosureLattice : public SlaveLattice {
    private:
	ConstSemiringRef semiring_;
	mutable TopologicalOrderQueueRef epsQeue_;
	mutable Core::Vector<ScoresRef> epsClosureScores_;

    public:
	NormalizeEpsilonClosureLattice(ConstLatticeRef l) :
	    SlaveLattice(cache(sort(l, Fsa::SortTypeByInputAndOutputAndTarget))),
	    semiring_(l->semiring()) {
	    ConstStateMapRef topologicalOrderMap = findTopologicalOrder(l);
	    verify(topologicalOrderMap && (topologicalOrderMap->maxSid != Fsa::InvalidStateId));
	    epsQeue_ = createTopologicalOrderQueue(l, topologicalOrderMap);
	    epsClosureScores_.grow(topologicalOrderMap->maxSid);
	}
	virtual ~NormalizeEpsilonClosureLattice() {}

	virtual ConstStateRef getState(Fsa::StateId sid) const {
	    ConstStateRef sr = fsa_->getState(sid);
	    State::const_iterator a = sr->begin(), a_end = sr->end();

	    // Check in O(1), if eps/eps arcs exist
	    if ((a == a_end)
		|| (a->input() != Fsa::Epsilon)
		|| (a->output() != Fsa::Epsilon))
		return sr;

	    // Initialize epsilon closure
	    verify(sid < epsClosureScores_.size());
	    TopologicalOrderQueue &epsQ(*epsQeue_);
	    State *sp = new State(sr->id());
	    for (; (a != a_end) && (a->input() == Fsa::Epsilon) && (a->output() == Fsa::Epsilon); ++a)
		if (!epsClosureScores_[a->target()]) {
		    epsClosureScores_[a->target()] = a->weight();
		    epsQ.insert(a->target());
		} else
		    epsClosureScores_[a->target()] =
			semiring_->collect(epsClosureScores_[a->target()], a->weight());
	    for (; a != a_end; ++a)
		*sp->newArc() = *a;
	    // Initialize (potential) final weight
	    ScoresRef finalWeight;
	    if (sr->isFinal()) {
		sp->addTags(Fsa::StateTagFinal);
		finalWeight = sr->weight();
	    } else
		finalWeight = semiring_->zero();
	    // Process epsilon closure
	    while (!epsQ.empty()) {
		Fsa::StateId epsSid = epsQ.top(); epsQ.pop();
		ConstStateRef epsSr = fsa_->getState(epsSid);
		ScoresRef score = epsClosureScores_[epsSid];
		epsClosureScores_[epsSid].reset();
		State::const_iterator a = epsSr->begin(), a_end = epsSr->end();
		if ((a != a_end) && ((a_end - 1)->input() == Fsa::Epsilon) && ((a_end - 1)->output() == Fsa::Epsilon)) {
		    if (epsSr->isFinal()) {
			sp->addTags(Fsa::StateTagFinal);
			finalWeight = semiring_->collect(
			    finalWeight, semiring_->extend(score, epsSr->weight()));
		    }
		    for (; a != a_end; ++a) {
			verify((a->input() == Fsa::Epsilon) && (a->output() == Fsa::Epsilon));
			if (!epsClosureScores_[a->target()]) {
			    epsClosureScores_[a->target()] = semiring_->extend(score, a->weight());
			    epsQ.insert(a->target());
			} else
			    epsClosureScores_[a->target()] =
				semiring_->collect(
				    epsClosureScores_[a->target()], semiring_->extend(
					score, a->weight()));
		    }
		} else
		    sp->newArc(epsSid, score, Fsa::Epsilon, Fsa::Epsilon);
	    }
	    if (sp->isFinal())
		sp->setWeight(finalWeight);
	    return ConstStateRef(sp);
	}
	virtual std::string describe() const {
	    return Core::form("normalize-epsilon-closure(%s)", fsa_->describe().c_str());
	}
    };

    ConstLatticeRef normalizeEpsClosure(ConstLatticeRef l) {
	verify_(l->hasProperty(Fsa::PropertyAcyclic));
	return ConstLatticeRef(new NormalizeEpsilonClosureLattice(l));
    }
    // -------------------------------------------------------------------------



    // -------------------------------------------------------------------------
    class NonWordClosureNormalizationFilterNode : public FilterNode {
    protected:
	virtual ConstLatticeRef filter(ConstLatticeRef l) {
	    if (!l)
		return ConstLatticeRef();
	    if (l->type() != Fsa::TypeAcceptor) {
		warning("%s: \"%s\" is a transducer, but result will be an acceptor, i.e. output will be lost.",
			name.c_str(), l->describe().c_str());
		l = projectInput(l);
	    }
	    l = applyOneToOneLabelMap(
		l, LabelMap::createNonWordToEpsilonMap(Lexicon::us()->alphabetId(l->getInputAlphabet())));
	    return normalizeEpsClosure(l);
	}
    public:
	NonWordClosureNormalizationFilterNode(const std::string &name, const Core::Configuration &config) :
	    FilterNode(name, config) {}
	~NonWordClosureNormalizationFilterNode() {}
    };
    NodeRef createNonWordClosureNormalizationFilterNode(const std::string &name, const Core::Configuration &config) {
	return NodeRef(new NonWordClosureNormalizationFilterNode(name, config));
    }
    // -------------------------------------------------------------------------



    // -------------------------------------------------------------------------
    class NonWordClosureRemovalFilterNode : public FilterNode {
    protected:
	virtual ConstLatticeRef filter(ConstLatticeRef l) {
	    if (!l)
		return ConstLatticeRef();
	    if (l->type() != Fsa::TypeAcceptor) {
		warning("%s: \"%s\" is a transducer, but result will be an acceptor, i.e. output will be lost.",
			name.c_str(), l->describe().c_str());
		l = projectInput(l);
	    }
	    l = applyOneToOneLabelMap(
		l, LabelMap::createNonWordToEpsilonMap(Lexicon::us()->alphabetId(l->getInputAlphabet())));
	    return fastRemoveEpsilons(l);
	}
    public:
	NonWordClosureRemovalFilterNode(const std::string &name, const Core::Configuration &config) :
	    FilterNode(name, config) {}
	~NonWordClosureRemovalFilterNode() {}
    };
    NodeRef createNonWordClosureRemovalFilterNode(const std::string &name, const Core::Configuration &config) {
	return NodeRef(new NonWordClosureRemovalFilterNode(name, config));
    }

} // namespace Flf

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
#include <Fsa/tBasic.hh>

#include "Basic.hh"
#include "Ftl.hh"
#include "LatticeInternal.hh"
#include "Traverse.hh"


namespace Flf {


    // -------------------------------------------------------------------------
    ConstLatticeRef changeSemiring(ConstLatticeRef l, ConstSemiringRef targetSemiring) {
	if (l->semiring()->size() != targetSemiring->size())
	    Core::Application::us()->criticalError(
		"Cannot replace semiring \"%s\" by \"%s\"; semirings differ in size.",
		l->semiring()->name().c_str(), targetSemiring->name().c_str());
	return FtlWrapper::changeSemiring(l, targetSemiring);
    }

    ConstLatticeRef rescale(ConstLatticeRef l, const ScoreList &scales, const KeyList &keys) {
	return changeSemiring(l, rescaleSemiring(l->semiring(), scales, keys));
    }

    ConstLatticeRef rescale(ConstLatticeRef l, ScoreId id, Score scale, const Key &key) {
	return changeSemiring(l, rescaleSemiring(l->semiring(), id, scale, key));
    }

    class ProjectSemiringLattice : public ModifyLattice {
	typedef ModifyLattice Precursor;
    private:
	ConstSemiringRef semiring_;
	const ProjectionMatrix mapping_;
    protected:
	ScoresRef project(ScoresRef src) const {
	    ScoresRef score = semiring_->create();
	    Scores::iterator itScore = score->begin();
	    for (ProjectionMatrix::const_iterator itScales = mapping_.begin();
		 itScales != mapping_.end(); ++itScales, ++itScore)
		*itScore = src->project(*itScales);
	    return score;
	}
    public:
	ProjectSemiringLattice(ConstLatticeRef l, ConstSemiringRef targetSemiring, const ProjectionMatrix &mapping) :
	    Precursor(l), semiring_(targetSemiring), mapping_(mapping) {
	    verify(mapping_.size() <= targetSemiring->size());
	    ConstSemiringRef sourceSemiring = l->semiring();
	    for (ProjectionMatrix::const_iterator itScales = mapping_.begin(); itScales != mapping_.end(); ++itScales)
		verify(itScales->size() == sourceSemiring->size());
	}
	virtual ~ProjectSemiringLattice() {}

	virtual ConstSemiringRef semiring() const {
	    return semiring_;
	}

	virtual void modifyState(State *sp) const {
	    if (sp->isFinal())
		sp->setWeight(project(sp->weight()));
	    else
		sp->setWeight(semiring_->one());
	    for (State::iterator a = sp->begin(); a != sp->end(); ++a)
		a->setWeight(project(a->weight()));
	}

	virtual std::string describe() const {
	    return Core::form("projectSemiring(%s;%s,%s)",
			      fsa_->describe().c_str(), fsa_->semiring()->name().c_str(), semiring_->name().c_str());
	}
    };
    ConstLatticeRef projectSemiring(ConstLatticeRef l, ConstSemiringRef targetSemiring, const ProjectionMatrix &mapping) {
	return ConstLatticeRef(new ProjectSemiringLattice(l, targetSemiring, mapping));
    }
    // -------------------------------------------------------------------------



    // -------------------------------------------------------------------------
    struct FsaWeightToScores {
	ConstSemiringRef semiring;
	ScoresRef defaultScore;
	ScoreId id;
	FsaWeightToScores(ConstSemiringRef semiring, ScoresRef defaultScore, ScoreId id) :
	    semiring(semiring), defaultScore(defaultScore), id(id) {}
	ScoresRef operator()(const Fsa::Weight &w) const {
	    ScoresRef score = semiring->clone(defaultScore);
	    score->set(id, Score(w));
	    return score;
	}
    };
    ConstLatticeRef fromFsa(Fsa::ConstAutomatonRef f, ConstSemiringRef semiring, ScoreId id, ScoresRef defaultScore) {
	Fsa::SemiringType type = Fsa::getSemiringType(f->semiring());
	if ((type != Fsa::SemiringTypeTropical) && (type != Fsa::SemiringTypeLog))
	    Core::Application::us()->warning(
		"In \"fromFsa\": expected tropical or log semiring, found %s semiring.",
		f->semiring()->name().c_str());
	if (!defaultScore)
	    defaultScore = semiring->one();
	return Ftl::convert<Fsa::Automaton, Lattice, FsaWeightToScores>(
	    f, semiring, FsaWeightToScores(semiring, defaultScore, id));
    }

    class FsaVectorLattice : public Lattice {
	typedef Lattice Precursor;
    private:
	const std::vector<Fsa::ConstAutomatonRef> fsas_;
	ConstSemiringRef semiring_;
	mutable std::vector<Fsa::ConstStateRef> tmpStates_;
	mutable std::vector<Fsa::State::const_iterator> tmpArcIterators_;
    public:
	FsaVectorLattice(const std::vector<Fsa::ConstAutomatonRef> &fsas, ConstSemiringRef semiring) :
	    Precursor(), fsas_(fsas), semiring_(semiring), tmpStates_(0), tmpArcIterators_(0) {
	    verify((fsas_.size() > 0) && (fsas_.size() == semiring_->size()));
	    tmpStates_.resize(fsas_.size());
	    tmpArcIterators_.resize(fsas_.size());
	}
	virtual ~FsaVectorLattice() {}

	virtual Fsa::Type type() const {
	    return fsas_.front()->type();
	}

	virtual ConstSemiringRef semiring() const {
	    return semiring_;
	}

	virtual Fsa::StateId initialStateId() const {
	    return fsas_.front()->initialStateId();
	}

	virtual Fsa::ConstAlphabetRef getInputAlphabet() const {
	    return fsas_.front()->getInputAlphabet();
	}

	virtual Fsa::ConstAlphabetRef getOutputAlphabet() const {
	    return fsas_.front()->getOutputAlphabet();
	}

	virtual ConstStateRef getState(Fsa::StateId sid) const {
	    std::vector<Fsa::ConstStateRef>::iterator itSr = tmpStates_.begin();
	    std::vector<Fsa::State::const_iterator>::iterator itA = tmpArcIterators_.begin();
	    for (std::vector<Fsa::ConstAutomatonRef>::const_iterator itFsa = fsas_.begin(); itFsa != fsas_.end(); ++itFsa, ++itSr, itA++) {
		*itSr = (*itFsa)->getState(sid);
		*itA = (*itSr)->begin();
	    }
	    State *sp = new State(sid);
	    if (tmpStates_.front()->isFinal()) {
		ScoresRef scores = semiring_->create();
		sp->setFinal(scores);
		Scores::iterator itScore = scores->begin();
		for (std::vector<Fsa::ConstStateRef>::const_iterator itSr = tmpStates_.begin(); itSr != tmpStates_.end(); ++itSr, ++itScore)
		    *itScore = Score((*itSr)->weight());
	    }
	    for (Fsa::State::const_iterator a = tmpArcIterators_.front(), a_end = tmpStates_.front()->end(); a != a_end; ++a) {
		ScoresRef scores = semiring_->create();
		sp->newArc(a->target(), scores, a->input(), a->output());
		Scores::iterator itScore = scores->begin();
		for (std::vector<Fsa::State::const_iterator>::iterator itA = tmpArcIterators_.begin(); itA != tmpArcIterators_.end(); ++itA, ++itScore) {
		    *itScore = Score((*itA)->weight());
		    ++(*itA);
		}
	    }
	    return ConstStateRef(sp);
	}

	virtual std::string describe() const {
	    std::string name = "(fsas-to-flf(";
	    std::vector<Fsa::ConstAutomatonRef>::const_iterator it = fsas_.begin();
	    name += (*it)->describe();
	    for (++it; it != fsas_.end(); ++it)
		name += "," + (*it)->describe();
	    name += ")";
	    return name;
	}
    };
    ConstLatticeRef fromFsaVector(const std::vector<Fsa::ConstAutomatonRef> &fsas, ConstSemiringRef semiring) {
	if (fsas.empty())
	    return ConstLatticeRef();
	return ConstLatticeRef(new FsaVectorLattice(fsas, semiring));
    }

    struct FsaWeightToConstant {
	ConstSemiringRef semiring;
	ScoresRef constScore;
	FsaWeightToConstant(ConstSemiringRef semiring, ScoresRef constScore) :
	    semiring(semiring), constScore(constScore) {}
	ScoresRef operator()(const Fsa::Weight &w) const {
	    return constScore;
	}
    };
    ConstLatticeRef fromUnweightedFsa(Fsa::ConstAutomatonRef f, ConstSemiringRef semiring, ScoresRef defaultScore) {
	Fsa::SemiringType type = Fsa::getSemiringType(f->semiring());
	if ((type != Fsa::SemiringTypeTropical) && (type != Fsa::SemiringTypeLog))
	    Core::Application::us()->warning(
		"In \"fromUnweightedFsa\": expected tropical or log semiring, found %s semiring.",
		f->semiring()->name().c_str());
	if (!defaultScore)
	    defaultScore = semiring->one();
	return Ftl::convert<Fsa::Automaton, Lattice, FsaWeightToConstant>(
	    f, semiring, FsaWeightToConstant(semiring, defaultScore));
    }
    // -------------------------------------------------------------------------



    // -------------------------------------------------------------------------
    struct LinearCombination {
	ScoreList scales;
	LinearCombination(const ScoreList &scales) : scales(scales) {}
	Fsa::Weight operator()(const ScoresRef &a) const {
	    return Fsa::Weight(a->project(scales));
	}
    };
    Fsa::ConstAutomatonRef toFsa(ConstLatticeRef l, const ScoreList &scales) {
	LinearCombination conv(scales);
	if (conv.scales.empty())
	    conv.scales = l->semiring()->scales();
	else
	    require(conv.scales.size() == l->semiring()->size());
	Fsa::ConstSemiringRef semiring = Fsa::getSemiring(l->semiring()->type());
	require(semiring);
	return Ftl::convert<Lattice, Fsa::Automaton, LinearCombination>(
	    l, semiring, conv);
    }

    struct Projection {
	ScoreId id;
	Score scale;
	Projection(ScoreId id, Score scale = Score(1)) : id(id), scale(scale) {}
	Fsa::Weight operator()(const ScoresRef &a) const
	    { return Fsa::Weight(scale * a->get(id)); }
    };
    Fsa::ConstAutomatonRef toFsa(ConstLatticeRef l, ScoreId id, bool scaled) {
	require(l->semiring()->hasId(id));
	Projection conv(id, (scaled ? l->semiring()->scale(id) : 1.0));
	return Ftl::convert<Lattice, Fsa::Automaton, Projection>(
	    l, Fsa::getSemiring(l->semiring()->type()), conv);
    }
    std::vector<Fsa::ConstAutomatonRef> toFsaVector(ConstLatticeRef l, bool scaled) {
	std::vector<Fsa::ConstAutomatonRef> fsas(l->semiring()->size());
	Fsa::ConstSemiringRef semiring = Fsa::getSemiring(l->semiring()->type());
	for (ScoreId id = 0; id < l->semiring()->size(); ++id) {
	    Projection conv(id, (scaled ? l->semiring()->scale(id) : 1.0));
	    fsas[id] = Ftl::convert<Lattice, Fsa::Automaton, Projection>(
		l, semiring, conv);
	}
	return fsas;
    }

    struct Constant{
	Fsa::Weight constWeight;
	Constant(const Fsa::Weight &constWeight) : constWeight(constWeight) {}
	Fsa::Weight operator()(const ScoresRef &a) const { return constWeight; }
    };
    Fsa::ConstAutomatonRef toUnweightedFsa(ConstLatticeRef l, const Fsa::Weight &constWeight) {
	Constant conv(constWeight);
	return Ftl::convert<Lattice, Fsa::Automaton, Constant>(
	    l, Fsa::getSemiring(l->semiring()->type()), conv);
    }
    // -------------------------------------------------------------------------



    // -------------------------------------------------------------------------
    class TopologicalOrderBuilder : protected DfsState {
    protected:
	Core::Ref<StateMap> map_;
    private:
	bool isCyclic_;
    protected:
	virtual void exploreNonTreeArc(ConstStateRef from, const Arc &a) {
	    if (color(a.target()) == Gray) isCyclic_ = true;
	}
    public:
	TopologicalOrderBuilder(ConstLatticeRef l, Core::Ref<StateMap> map) :
	    DfsState(l), map_(map), isCyclic_(false) {}
	bool isCyclic() const { return isCyclic_; }
	virtual ~TopologicalOrderBuilder() {}
    };


    class SortInTopologicalOrder : public TopologicalOrderBuilder {
    protected:
	virtual void finishState(Fsa::StateId sid) {
	    ensure(sid != Fsa::InvalidStateId);
	    map_->push_back(sid);
	    map_->maxSid = std::max(map_->maxSid, sid);
	}
    public:
	SortInTopologicalOrder(ConstLatticeRef l, Core::Ref<StateMap> map) :
	    TopologicalOrderBuilder(l, map) {
	    map_->maxSid = 0;
	    dfs();
	    if (!isCyclic()) {
		if (map_->empty())
		    map_->maxSid = Fsa::InvalidStateId;
		else
		    std::reverse(map_->begin(), map_->end());
	    } else {
		map_->maxSid = Fsa::InvalidStateId;
		map_->clear();
	    }
	}
	virtual ~SortInTopologicalOrder() {}
    };
    ConstStateMapRef sortTopologically(ConstLatticeRef l) {
	if (!l->getTopologicalSort()) {
	    Core::Ref<StateMap> map = Core::Ref<StateMap>(new StateMap());
	    SortInTopologicalOrder(l, map);
	    if (map->maxSid != Fsa::InvalidStateId)
		l->setTopologicalSort(map);
	}
	return l->getTopologicalSort();
    }


    class FindTopologicalOrder : public TopologicalOrderBuilder {
    private:
	Fsa::StateId time_;
    protected:
	virtual void finishState(Fsa::StateId sid) {
	    ensure(sid != Fsa::InvalidStateId);
	    map_->grow(sid, Fsa::InvalidStateId);
	    (*map_)[sid] = time_++;
	}
    public:
	FindTopologicalOrder(ConstLatticeRef l, Core::Ref<StateMap> map) :
	    TopologicalOrderBuilder(l, map), time_(0) {
	    dfs();
	    if (time_ > 0) {
		if (!isCyclic()) {
		    --time_;
		    for (StateMap::iterator itOrder = map_->begin(); itOrder != map_->end(); ++itOrder)
			if (*itOrder != Fsa::InvalidStateId)
			    *itOrder = time_ - *itOrder;
		    map_->maxSid = map_->size() - 1;
		} else
		    map_->clear();
	    }
	}
	virtual ~FindTopologicalOrder() {}
    };
    ConstStateMapRef findTopologicalOrder(ConstLatticeRef l) {
	Core::Ref<StateMap> map;
	if (!l->getTopologicalSort()) {
	    map = Core::Ref<StateMap>(new StateMap);
	    FindTopologicalOrder(l, map);
	    if (map->maxSid == Fsa::InvalidStateId)
		map.reset();
	} else {
	    const StateMap &topologicalSort = *l->getTopologicalSort();
	    if (topologicalSort.maxSid == Fsa::InvalidStateId) {
		map = Core::Ref<StateMap>(new StateMap(0));
		map->maxSid = Fsa::InvalidStateId;
	    } else {
		map = Core::Ref<StateMap>(new StateMap(topologicalSort.maxSid + 1, Fsa::InvalidStateId));
		StateMap &topologicalOrder = *map;
		Fsa::StateId order = 0;
		for (StateMap::const_iterator itSid = topologicalSort.begin(), endSid = topologicalSort.end();
		     itSid != endSid; ++itSid, ++order) topologicalOrder[*itSid] = order;
		topologicalOrder.maxSid = topologicalSort.maxSid;
	    }
	}
	return map;
    }


    struct ChronologicalWeakOrder {
	const Boundaries &boundaries;
	const StateMap &topologicalOrder;
	bool operator() (Fsa::StateId sid1, Fsa::StateId sid2) const {
	    if (boundaries.time(sid1) == boundaries.time(sid2))
		return topologicalOrder[sid1] < topologicalOrder[sid2];
	    else
		return boundaries.time(sid1) < boundaries.time(sid2);
	}
	ChronologicalWeakOrder(const Boundaries &boundaries, const StateMap &topologicalOrder) :
	    boundaries(boundaries), topologicalOrder(topologicalOrder) {}
    };
    ConstStateMapRef sortChronologically(ConstLatticeRef l) {
	ConstStateMapRef topologicalSort = sortTopologically(l);
	verify(topologicalSort);
	StateMap *chronologicalSort = new StateMap(*topologicalSort);
	// a stable sort w/o taking the topological order (explicitly) into account should be enough here
	ConstStateMapRef topologicalOrder = findTopologicalOrder(l);
	ChronologicalWeakOrder lessThan(*l->getBoundaries(), *topologicalOrder);
	std::sort(chronologicalSort->begin(), chronologicalSort->end(), lessThan);
	return ConstStateMapRef(chronologicalSort);
    }
    // -------------------------------------------------------------------------


    // -------------------------------------------------------------------------
    class TopologicalOrderLattice : public SlaveLattice {
	typedef SlaveLattice Precursor;
    public:
	class TopologicalOrderBoundaries : public Boundaries {
	private:
	    ConstBoundariesRef boundaries_;
	    ConstStateMapRef topologicalSort_; // order = new-sid -> old-sid
	public:
	    TopologicalOrderBoundaries(ConstBoundariesRef boundaries, ConstStateMapRef topologicalSort) :
	    boundaries_(boundaries), topologicalSort_(topologicalSort) {}
	    virtual ~TopologicalOrderBoundaries() {}
	    bool valid() const
		{ return boundaries_->valid(); }
	    bool valid(Fsa::StateId sid) const
		{ return boundaries_->valid((*topologicalSort_)[sid]); }
	    const Boundary& get(Fsa::StateId sid) const
		{ return boundaries_->get((*topologicalSort_)[sid]); }
	};
    private:
	ConstStateMapRef topologicalSort_;   // order = new-sid -> old-sid
	ConstStateMapRef topologicalOrder_;  // old-sid -> order = new-sid
    public:
	TopologicalOrderLattice(ConstLatticeRef l) : Precursor(l) {
	    topologicalSort_ = sortTopologically(l);
	    topologicalOrder_ = findTopologicalOrder(l);
	    verify(((*topologicalSort_)[0] == fsa_->initialStateId()) && ((*topologicalOrder_)[fsa_->initialStateId()] == 0));
	    setBoundaries(ConstBoundariesRef(new TopologicalOrderBoundaries(fsa_->getBoundaries(), topologicalSort_)));
	}
	virtual ~TopologicalOrderLattice() {}
	Fsa::StateId initialStateId() const
	    { return 0; }
	ConstStateRef getState(Fsa::StateId sid) const {
	    State *sp = new State(*fsa_->getState((*topologicalSort_)[sid]));
	    sp->setId(sid);
	    const StateMap &topologicalOrder = *topologicalOrder_;
	    for (State::iterator a = sp->begin(), end = sp->end(); a != end; ++a)
		a->target_ = topologicalOrder[a->target_];
	    return ConstStateRef(sp);
	}
	std::string describe() const
	    { return Core::form("topologically-ordered(%s)", fsa_->describe().c_str()); }
    };
    ConstLatticeRef sortByTopologicalOrder(ConstLatticeRef l) {
	return ConstLatticeRef(new TopologicalOrderLattice(l));
    }
    // -------------------------------------------------------------------------


    // -------------------------------------------------------------------------
    bool isAcyclic(ConstLatticeRef l) {
	return FtlWrapper::isAcyclic(l);
    }

    /*
    bool isLinear(ConstLatticeRef l) {
	return FtlWrapper::isLinear(l);
    }
    */

    /*
    Fsa::Property getProperties(ConstLatticeRef l, Fsa::Property properties) {
	return FtlWrapper::getProperties(l, properties);
    }
    */
    // -------------------------------------------------------------------------


    // -------------------------------------------------------------------------
    ConstLatticeRef partial(ConstLatticeRef l, Fsa::StateId initial) {
	return FtlWrapper::partial(l, initial);
    }

    ConstLatticeRef sort(ConstLatticeRef l, Fsa::SortType type) {
	return FtlWrapper::sort(l, type);
    }

    ConstLatticeRef trim(ConstLatticeRef l, bool progress) {
	return FtlWrapper::trim(l, progress);
    }

    void trimInPlace(StaticLatticeRef l) {
	l->setDescription("trim-in-place(" + l->describe() + ")");
	FtlWrapper::trimInPlace(l);
    }
    // -------------------------------------------------------------------------

    // -------------------------------------------------------------------------
    class ScoreAppendLattice : public SlaveLattice {
	typedef SlaveLattice Precursor;
    private:
	ConstLatticeRef appendL_;
	u32 n1_, n2_;
	ConstSemiringRef semiring_;

    protected:
	ScoresRef concatenate(const Scores &scores1, const Scores &scores2) const {
	    ScoresRef scores = semiring_->create();
	    Scores::iterator itScore = scores->begin();
	    for (Scores::const_iterator itScore1 = scores1.begin(), endScore1 = scores1.begin() + n1_;
		 itScore1 != endScore1; ++itScore1, ++itScore) *itScore = *itScore1;
	    for (Scores::const_iterator itScore2 = scores2.begin(), endScore2 = scores2.begin() + n2_;
		 itScore2 != endScore2; ++itScore2, ++itScore) *itScore = *itScore2;
	    return scores;
	}

    public:
	ScoreAppendLattice(ConstLatticeRef l1, ConstLatticeRef l2, ConstSemiringRef semiring) :
	    Precursor(l1), appendL_(l2), semiring_(semiring)  {
	    const Semiring &semiring1 = *l1->semiring();
	    n1_ = semiring1.size();
	    const Semiring &semiring2 = *l2->semiring();
	    n2_ = semiring2.size();
	    if (!semiring) {
		verify(semiring1.type() == semiring2.type());
		ScoreList scales(semiring1.scales());
		scales.insert(scales.end(), semiring2.scales().begin(), semiring2.scales().end());
		KeyList keys(semiring1.keys());
		keys.insert(keys.end(), semiring2.keys().begin(), semiring2.keys().end());
		semiring_ = Semiring::create(semiring1.type(), n1_ + n2_, scales, keys);
	    } else
		verify(semiring_->size() == n1_ + n2_);
	}
	virtual ~ScoreAppendLattice() {}

	ConstSemiringRef semiring() const {
	    return semiring_;
	}

	ConstStateRef getState(Fsa::StateId sid) const {
	    State *sp = new State(*fsa_->getState(sid));
	    ConstStateRef appendSp = appendL_->getState(sid);
	    verify(appendSp);
	    if (sp->isFinal()) {
		verify(appendSp->isFinal());
		sp->weight_ = concatenate(*sp->weight(), *appendSp->weight());
	    }
	    State::const_iterator a_append = appendSp->begin(), end_append = appendSp->end();
	    for (State::iterator a = sp->begin(), end = sp->end(); a != end; ++a, ++a_append) {
		verify(a_append != end_append);
		verify(a->target() == a_append->target());
		verify_(a->input() == a_append->input());
		verify_(a->output() == a_append->output());
		a->weight_ = concatenate(*a->weight(), *a_append->weight());
	    }
	    return ConstStateRef(sp);
	}
	std::string describe() const
	    { return Core::form("append-score(%s,%s)", fsa_->describe().c_str(), appendL_->describe().c_str()); }
    };
    ConstLatticeRef appendScores(ConstLatticeRef l1, ConstLatticeRef l2, ConstSemiringRef semiring) {
	return ConstLatticeRef(new ScoreAppendLattice(l1, l2, semiring));
    }
    // -------------------------------------------------------------------------

} // namespace Flf

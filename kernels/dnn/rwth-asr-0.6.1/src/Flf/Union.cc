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
#include <Core/Parameter.hh>
#include <Fsa/Hash.hh>

#include "FlfCore/Basic.hh"
#include "FlfCore/LatticeInternal.hh"
#include "Convert.hh"
#include "Union.hh"
#include "Copy.hh"

namespace Flf {

    // -------------------------------------------------------------------------
    class UnionBoundaries : public Boundaries {
    private:
	Fsa::StateId nLats_;
	ConstBoundariesRefList boundaries_;
	Boundary initialBoundary_;

    private:
	std::pair<Fsa::StateId, Fsa::StateId> fromUnionSid(Fsa::StateId unionSid) const {
	    Fsa::StateId sid = (unionSid - 1) / nLats_;
	    return std::make_pair(unionSid - 1 - sid * nLats_, sid);
	}

    public:
	UnionBoundaries(const ConstLatticeRefList &lats) :
	    Boundaries(), nLats_(lats.size()),
	    boundaries_(lats.size()) {
	    boundaries_[0] = lats[0]->getBoundaries();
	    initialBoundary_ = boundaries_[0]->get(lats[0]->initialStateId());
	    for (u32 i = 1; i < nLats_; ++i) {
		boundaries_[i] = lats[i]->getBoundaries();
		if (initialBoundary_.valid()) {
		    const Boundary &currentInitialBoundary(
			boundaries_[i]->get(lats[i]->initialStateId()));
		    if (currentInitialBoundary.time() != initialBoundary_.time())
			initialBoundary_ = InvalidBoundary;
		    else if (!(currentInitialBoundary.transit() == initialBoundary_.transit()))
			initialBoundary_.setTransit(Boundary::Transit());
		}
	    }
	}
	virtual ~UnionBoundaries() {}

	virtual bool valid() const {
	    return true;
	}

	virtual bool valid(Fsa::StateId unionSid) const {
	    return get(unionSid).valid();
	}

	virtual const Boundary& get(Fsa::StateId unionSid) const {
	    if (unionSid == 0)
		return initialBoundary_;
	    else {
		std::pair<Fsa::StateId, Fsa::StateId> sid = fromUnionSid(unionSid);
		return boundaries_[sid.first]->get(sid.second);
	    }
	}
    };


    class UnionLattice : public SlaveLattice {
    private:
	Fsa::StateId nLats_;
	ConstLatticeRefList lats_;
	ConstSemiringRef semiring_;

    private:
	Fsa::StateId toUnionSid(Fsa::StateId i, Fsa::StateId sid) const {
	    return sid * nLats_ + i + 1;
	}

	std::pair<Fsa::StateId, Fsa::StateId> fromUnionSid(Fsa::StateId unionSid) const {
	    Fsa::StateId sid = (unionSid - 1) / nLats_;
	    return std::make_pair(unionSid - 1 - sid * nLats_, sid);
	}

    public:
	UnionLattice(const ConstLatticeRefList &lats, ConstSemiringRef semiring) :
	    SlaveLattice(lats[0]), nLats_(lats.size()),
	    lats_(lats), semiring_(semiring) {
	    verify(semiring);
	    bool hasBoundaries = false;
	    for (u32 i = 0; i < nLats_; ++i) {
		ConstLatticeRef currentLattice = lats[i];
		verify(fsa_->type() == currentLattice->type());
		verify(fsa_->getInputAlphabet().get() == currentLattice->getInputAlphabet().get());
		if (fsa_->type() != Fsa::TypeAcceptor)
		    verify(fsa_->getOutputAlphabet().get() ==
			   currentLattice->getOutputAlphabet().get());
		if (lats[i]->getBoundaries()->valid())
		    hasBoundaries = true;
	    }
	    if (hasBoundaries)
		setBoundaries(ConstBoundariesRef(new UnionBoundaries(lats)));
	}
	virtual ~UnionLattice() {}

	virtual ConstSemiringRef semiring() const {
	    return semiring_;
	}

	virtual Fsa::StateId initialStateId() const {
	    return 0;
	}

	virtual ConstStateRef getState(Fsa::StateId unionSid) const {
	    State *sp;
	    if (unionSid == 0) {
		sp = new State(0);
		for (u32 i = 0; i < nLats_; ++i) {
		    ConstStateRef sr = lats_[i]->getState(lats_[i]->initialStateId());
		    if (sr->isFinal())
			sp->newArc(toUnionSid(i, lats_[i]->initialStateId()),
				   semiring_->one(), Fsa::Epsilon);
		    for (State::const_iterator a = sr->begin(); a != sr->end(); ++a)
			sp->newArc(toUnionSid(i, a->target()), a->weight(),
				   a->input(), a->output());
		}
	    } else {
		std::pair<Fsa::StateId, Fsa::StateId> sid = fromUnionSid(unionSid);
		ConstStateRef sr = lats_[sid.first]->getState(sid.second);
		sp = new State(*sr);
		sp->setId(unionSid);
		for (State::iterator a = sp->begin(); a != sp->end(); ++a)
		    a->target_ = toUnionSid(sid.first, a->target());
	    }
	    return ConstStateRef(sp);
	}

	virtual std::string describe() const {
	    std::string desc = "union(";
	    for (u32 i = 0; i < nLats_; ++i)
		desc += lats_[i]->describe() + ",";
	    desc.at(desc.size() - 1) = ')';
	    return desc;
	}
    };

    ConstLatticeRef unite(const ConstLatticeRefList &lats, ConstSemiringRef semiring) {
	switch (lats.size()) {
	case 0:
	    return ConstLatticeRef();
	case 1:
	    if (semiring)
		return changeSemiring(lats[0], semiring);
	    else
		return lats[0];
	default:
	    if (!semiring) {
		semiring = lats[0]->semiring();
		for (u32 i = 1; i < lats.size(); ++i)
		    if (!Semiring::equal(semiring, lats[0]->semiring()))
			Core::Application::us()->criticalError(
			    "Lattice union requires equal semirings, but \"%s\" != \"%s\".",
			    semiring->name().c_str(), lats[i]->semiring()->name().c_str());
	    }
	    return ConstLatticeRef(new UnionLattice(lats, semiring));
	}
    }

    ConstLatticeRef unite(ConstLatticeRef l1, ConstLatticeRef l2, ConstSemiringRef semiring) {
	ConstLatticeRefList lats(2);
	lats[0] = l1;
	lats[1] = l2;
	return unite(lats, semiring);
    }
    // -------------------------------------------------------------------------


    // -------------------------------------------------------------------------
    class UnionNode : public Node {
    private:
	u32 n_;
	ConstSemiringRef unionSemiring_;
	ConstLatticeRef union_;

    private:
	void buildUnion() {
	    if (!union_) {
		ConstLatticeRefList lats(n_);
		for (u32 i = 0; i < n_; ++i)
		    lats[i] = requestLattice(i);
		union_ = unite(lats, unionSemiring_);
	    }
	}

    public:
	UnionNode(const std::string &name, const Core::Configuration &config) :
	    Node(name, config),
	    n_(0) {}
	virtual ~UnionNode() {}

	virtual void init(const std::vector<std::string> &arguments) {
	    for (n_ = 0; connected(n_); ++n_);
	    if (n_ == 0)
		criticalError("At least one incoming lattice at port 0 required.");
	    Core::Component::Message msg = log();
	    if (n_ > 1)
		msg << "Combine " << n_ << " lattices.\n\n";
	    unionSemiring_ = Semiring::create(select("semiring"));
	    if (unionSemiring_)
		msg << "Union semiring:\n\t" << unionSemiring_->name();
	}

	virtual void finalize() {}

	virtual ConstLatticeRef sendLattice(Port to) {
	    verify(to == 0);
	    buildUnion();
	    return union_;
	}

	virtual void sync() {
	    union_.reset();
	}
    };
    NodeRef createUnionNode(const std::string &name, const Core::Configuration &config) {
	return NodeRef(new UnionNode(name, config));
    }
    // -------------------------------------------------------------------------


    // -------------------------------------------------------------------------
    namespace {
	struct MeshedFullBoundaryBuilder {
	    typedef Fsa::Hash<Boundary, Boundary::Hash, Boundary::Equal> HashList;
	    Boundary operator()(const Boundary &b) const {
		return b;
	    }
	};

	struct MeshedTimeBoundaryBuilder {
	    struct BoundaryTimeHash {
		size_t operator() (const Boundary &b) const {
		    return size_t(b.time());
		}
	    };
	    struct BoundaryTimeEqual {
		bool operator() (const Boundary &b1, const Boundary &b2) const {
		    return b1.time() == b2.time();
		}
	    };
	    typedef Fsa::Hash<Boundary, Boundary::Hash, Boundary::Equal> HashList;
	    Boundary operator()(const Boundary &b) const {
		return Boundary(b.time());
	    }
	};
    } // namespace

    ConstLatticeRef expandTransits(ConstLatticeRef lat, Bliss::Phoneme::Id leftContext, Bliss::Phoneme::Id rightContext) {
	LexiconRef lexicon = Lexicon::us();
	verify(lat->getBoundaries()->valid());
	lat = sortByTopologicalOrder(lat);
	StaticBoundaries *b = new StaticBoundaries;
	StaticLattice *s = new StaticLattice(Fsa::TypeAcceptor);
	s->setProperties(lat->knownProperties(), lat->properties());
	s->setInputAlphabet(lat->getInputAlphabet());
	s->setSemiring(lat->semiring());
	s->setBoundaries(ConstBoundariesRef(b));

	verify(Lexicon::us()->alphabetId(lat->getInputAlphabet()) == Lexicon::LemmaPronunciationAlphabetId);
	Core::Ref<const Bliss::LemmaPronunciationAlphabet> lpAlphabet = Lexicon::us()->lemmaPronunciationAlphabet();

	typedef std::pair<Fsa::StateId, Flf::Arc> InArc;
	typedef std::multimap<Fsa::StateId, InArc> Hypotheses;
	Hypotheses inArcs;
	inArcs.insert(std::make_pair(lat->initialStateId(), InArc(Fsa::InvalidStateId, Flf::Arc())));

	while (!inArcs.empty())
	{
	    Fsa::StateId oldStateId = inArcs.begin()->first;
	    ConstStateRef oldState = lat->getState(oldStateId);

	    Hypotheses::iterator inHypEnd = inArcs.upper_bound(oldStateId);

	    std::set<std::pair<int, int> > transits;
	    typedef std::multimap<int, InArc> FinalPhoneHyps;
	    FinalPhoneHyps finalPhoneHyps;
	    std::multimap<int, u32> initialPhoneArcs;
	    std::set<int> finalPhones, initialPhones;

	    bool hasRealInArcs = false;

	    for (Hypotheses::const_iterator inHypIt = inArcs.begin();
		inHypIt != inHypEnd; ++inHypIt)
	    {
		Bliss::Phoneme::Id final = Bliss::Phoneme::term;
		if (inHypIt->second.first == Fsa::InvalidStateId || inHypIt->second.second.input() == Fsa::Epsilon)
		{
		    final = leftContext;
		}
		else
		{
		    const Bliss::LemmaPronunciation* pron = lpAlphabet->lemmaPronunciation(inHypIt->second.second.input());
		    if (pron && pron->pronunciation()->length()) {
			hasRealInArcs = true;
			final = pron->pronunciation()->phonemes()[pron->pronunciation()->length() - 1];
		    }
		}
		if (final != Bliss::Phoneme::term && lexicon->phonemeInventory()->phoneme(final)->isContextDependent())
		{
		    finalPhones.insert(final);
		    finalPhoneHyps.insert(std::make_pair((int)final, inHypIt->second));
		}else{
		    finalPhoneHyps.insert(std::make_pair((int)Bliss::Phoneme::term, inHypIt->second));
		}
	    }

	    bool epsilonToFinal = false;

	    for (u32 arcI = 0; arcI < oldState->nArcs(); ++arcI)
	    {
		Bliss::Phoneme::Id initial = Bliss::Phoneme::term;
		if (oldState->getArc(arcI)->input() == Fsa::Epsilon) {
		    if (lat->getState(oldState->getArc(arcI)->target())->isFinal())
			epsilonToFinal = true;
		    else
			Core::Application::us()->warning() << "Found a non-initial epsilon arc, transit boundaries will not be correct, target " << oldState->getArc(arcI)->target() << " am score " << oldState->getArc(arcI)->score(0) << " lm score " << oldState->getArc(arcI)->score(1);
		}
		const Bliss::LemmaPronunciation* pron = lpAlphabet->lemmaPronunciation(oldState->getArc(arcI)->input());
		if (pron && pron->pronunciation()->length())
		    initial = pron->pronunciation()->phonemes()[0];
		if (initial != Bliss::Phoneme::term && lexicon->phonemeInventory()->phoneme(initial)->isContextDependent())
		{
		    initialPhones.insert(initial);
		    initialPhoneArcs.insert(std::make_pair((int)initial, arcI));
		}else{
		    initialPhoneArcs.insert(std::make_pair((int)Bliss::Phoneme::term, arcI));
		}
	    }

	    if (rightContext != Bliss::Phoneme::term && (oldState->isFinal() || epsilonToFinal))
		initialPhones.insert(rightContext);

	    for (std::set<int>::const_iterator finalIt = finalPhones.begin(); finalIt != finalPhones.end(); ++finalIt)
		for (std::set<int>::const_iterator initialIt = initialPhones.begin(); initialIt != initialPhones.end(); ++initialIt)
		    transits.insert(std::make_pair(*finalIt, *initialIt));

	    if (transits.empty() || finalPhoneHyps.count(Bliss::Phoneme::term) || initialPhoneArcs.count(Bliss::Phoneme::term))
	    {
		transits.clear();
		transits.insert(std::make_pair<int, int>(Bliss::Phoneme::term, Bliss::Phoneme::term));
	    }

	    verify(!transits.empty());

	    for(std::set<std::pair<int, int> >::iterator transitIt = transits.begin(); transitIt != transits.end(); ++transitIt)
	    {
		bool isContextDependent = transitIt->first != Bliss::Phoneme::term;

		State* newState = s->newState(oldState->tags(), oldState->weight());
		b->set(newState->id(), Flf::Boundary(lat->boundary(oldStateId).time(), Flf::Boundary::Transit(transitIt->first, transitIt->second)));

		if (oldState->id() == lat->initialStateId())
		{
		    verify(s->initialStateId() == Fsa::InvalidStateId);
		    s->setInitialStateId(newState->id());
		}

		FinalPhoneHyps::const_iterator finalHypsBegin = isContextDependent ? finalPhoneHyps.lower_bound(transitIt->first) : finalPhoneHyps.begin();
		FinalPhoneHyps::const_iterator finalHypsEnd = isContextDependent ? finalPhoneHyps.upper_bound(transitIt->first) : finalPhoneHyps.end();
		for (FinalPhoneHyps::const_iterator finalHypIt = finalHypsBegin;
		    finalHypIt != finalHypsEnd; ++finalHypIt)
		    if (finalHypIt->second.first != Fsa::InvalidStateId)
			const_cast<State&>(*s->getState(finalHypIt->second.first)).newArc(newState->id(), finalHypIt->second.second.weight(), finalHypIt->second.second.input(), finalHypIt->second.second.output());

		std::multimap<int, u32>::const_iterator initialArcsBegin = isContextDependent ? initialPhoneArcs.lower_bound(transitIt->second) : initialPhoneArcs.begin();
		std::multimap<int, u32>::const_iterator initialArcsEnd = isContextDependent ? initialPhoneArcs.upper_bound(transitIt->second) : initialPhoneArcs.end();
		for (std::multimap<int, u32>::const_iterator initialArcIt = initialArcsBegin;
		    initialArcIt != initialArcsEnd; ++initialArcIt)
		{
		    InArc nextHyp;
		    nextHyp.first = newState->id();
		    nextHyp.second = *oldState->getArc(initialArcIt->second);
		    inArcs.insert(std::make_pair(nextHyp.second.target(), nextHyp));
		    verify(nextHyp.second.target() > oldStateId);
		}
	    }

	    verify(inArcs.begin()->first == oldStateId);
	    inArcs.erase(inArcs.begin(), inArcs.upper_bound(oldStateId));
	}

	s->setDescription("expand_transits(" + lat->describe() + ")");

	return ConstLatticeRef(s);
    }

    // -------------------------------------------------------------------------
    class ExpandTransitsNode : public FilterNode
    {
    public:
	static const Core::ParameterFloat paramWordEndBeam;
    private:
	ConstLatticeRef latL_;
    protected:
	ConstLatticeRef filter(ConstLatticeRef l) {
	    if (!l)
		return ConstLatticeRef();
	    if (!latL_)
		latL_ = expandTransits(l);

	    return latL_;
	}
    public:
	ExpandTransitsNode(const std::string &name, const Core::Configuration &config) :
	    FilterNode(name, config) {}
	virtual ~ExpandTransitsNode() {}

	virtual void sync() {
	    latL_.reset();
	}
    };

    NodeRef createExpandTransitsNode(const std::string &name, const Core::Configuration &config) {
	return NodeRef(new ExpandTransitsNode(name, config));
    }

    template<class MeshedBoundaryBuilder>
    ConstLatticeRef buildMesh(const std::vector<MeshEntry>& entries, ConstSemiringRef semiring, const MeshedBoundaryBuilder &meshedBoundaryBuilder = MeshedBoundaryBuilder()) {
	StaticBoundaries *b = new StaticBoundaries;
	StaticLattice *s = new StaticLattice(Fsa::TypeAcceptor);
	StaticLatticeRef staticLattice(s);
	ConstLatticeRef firstLat = entries[0].lattice;
	s->setProperties(firstLat->knownProperties(), firstLat->properties());
	s->setInputAlphabet(firstLat->getInputAlphabet());
	s->setSemiring(semiring);
	s->setBoundaries(ConstBoundariesRef(b));

	ScoreId amScoreId = semiring->id("am");

	s->addProperties(Fsa::PropertySortedByInputAndTarget);

	typedef typename MeshedBoundaryBuilder::HashList BoundaryHashList;
	BoundaryHashList meshSids;
	for (u32 i = 0; i < entries.size(); ++i) {
	    s32 timeOffset = entries[i].timeOffset;
	    ConstLatticeRef l = entries[i].lattice;
	    ConstStateMapRef topologicalSort = sortTopologically(l);
	    verify(topologicalSort && topologicalSort->size());
	    if (entries[i].reverseOffset != Speech::InvalidTimeframeIndex)
	    {
		// backward lattice
		Speech::TimeframeIndex reverseOffset = entries[i].reverseOffset;

		for (s32 j = ((s32)topologicalSort->size())-1; j >= 0; --j) {

		    Fsa::StateId sid = (*topologicalSort)[j];
		    ConstStateRef sr = l->getState(sid);
		    Boundary meshB = meshedBoundaryBuilder(l->boundary(sid));

		    {
			// reverse boundary
			if(meshB.time() > reverseOffset)
			{
			    std::cout << "skipping state " << sid << " which can not be reversed. state time: " << meshB.time() << " reverse offset " << reverseOffset << std::endl;
			    continue;
			}
			meshB.setTime((reverseOffset - meshB.time()) + timeOffset);
			Flf::Boundary::Transit transit = meshB.transit();
			std::swap(transit.final, transit.initial);
			meshB.setTransit(transit);
		    }

		    std::pair<Fsa::StateId, bool> meshSource = meshSids.insertExisting(meshB);
		    if (!meshSource.second) {
			State *meshSourceSp = new State(meshSource.first);
			verify(!s->hasState(meshSourceSp->id()));
			s->setState(meshSourceSp);
			b->set(meshSourceSp->id(), meshB);
		    }

		    State *meshSp = s->fastState(meshSource.first);

		    for (State::const_iterator a = sr->begin(); a != sr->end(); ++a) {
			if (a->input() == Fsa::Epsilon)
			    continue; // Skip non-acoustic arcs

			Boundary targetB = meshedBoundaryBuilder(l->boundary(a->target()));

			{
			    // reverse boundary
			    verify(targetB.time() <= reverseOffset);
			    targetB.setTime((reverseOffset - targetB.time()) + timeOffset);
			    Flf::Boundary::Transit transit = targetB.transit();
			    std::swap(transit.final, transit.initial);
			    targetB.setTransit(transit);
			}

			Fsa::StateId targetSid = meshSids.find(targetB);
			verify(targetSid != BoundaryHashList::InvalidCursor);

			if (meshSp->id() != targetSid) {
			    if ((meshB.time() <= targetB.time()))
			    {
				std::cout << "skipping backward arc" << std::endl;
				Core::Application::us()->warning(
				    "Reversed lattice \"%s\" contains null/negative-length arcs; Skipping arc.",
				    l->describe().c_str()) << " " << targetB.time() << " -> " << meshB.time() << " (label=" << a->input() << ")";
				    continue;
			    }
			    verify(targetB.time() < meshB.time());
			    State *targetSp = s->fastState(targetSid);
			    Flf::Arc arc(meshSp->id(), a->weight(), a->input(), a->output());
			    State::iterator pos = targetSp->lower_bound(arc, Ftl::byInputAndTarget<Lattice>());
			    if ((pos == targetSp->end())
				|| (arc.target() != pos->target()) || (arc.input() != pos->input())) {
				targetSp->insert(pos, arc);
			    }else{
				if (semiring->project(arc.weight()) < semiring->project(pos->weight()))
				    pos->setWeight(arc.weight());
			    }
			}
		    }
		}
	    }else{
		// forward lattice
		Boundary initialB = meshedBoundaryBuilder(l->boundary(topologicalSort->front()));
		initialB.setTime(initialB.time() + timeOffset);
		std::pair<Fsa::StateId, bool> meshInitial = meshSids.insertExisting(initialB);
		if (!meshInitial.second) {
		    State *meshInitialSp = new State(meshInitial.first);
		    verify(!s->hasState(meshInitialSp->id()));
		    s->setState(meshInitialSp);
		    b->set(meshInitialSp->id(), initialB);
		}

		for (u32 j = 0; j < topologicalSort->size(); ++j) {

		    Fsa::StateId sid = (*topologicalSort)[j];

		    ConstStateRef sr = l->getState(sid);
		    bool hasNonEpsilonArc = false;
		    for (State::const_iterator a = sr->begin(); a != sr->end(); ++a)
			if (a->input() != Fsa::Epsilon)
			    hasNonEpsilonArc = true;

		    if (!hasNonEpsilonArc)
			continue;

		    Boundary meshB = meshedBoundaryBuilder(l->boundary(sid));
		    meshB.setTime(meshB.time() + timeOffset);

		    State *meshSp = 0;
		    std::pair<Fsa::StateId, bool> meshS = meshSids.insertExisting(meshB);
		    if (!meshS.second) {
			meshSp = new State(meshS.first);
			verify(!s->hasState(meshSp->id()));
			s->setState(meshSp);
			b->set(meshSp->id(), meshB);
		    }else{
			meshSp = s->fastState(meshS.first);
		    }

		    for (State::const_iterator a = sr->begin(); a != sr->end(); ++a) {
			if (a->input() == Fsa::Epsilon)
			    continue; // Skip non-acoustic arcs
			Boundary targetB = meshedBoundaryBuilder(l->boundary(a->target()));
			targetB.setTime(targetB.time() + timeOffset);
			std::pair<Fsa::StateId, bool> meshTarget = meshSids.insertExisting(targetB);
			if (!meshTarget.second) {
			    State *meshTargetSp = new State(meshTarget.first);
			    verify(!s->hasState(meshTargetSp->id()));
			    s->setState(meshTargetSp);
			    b->set(meshTargetSp->id(), targetB);
			}

			if (meshSp->id() != meshTarget.first) {
			    if ((meshB.time() >= targetB.time()))
			    {
				std::cout << "skipping forward arc" << std::endl;
				Core::Application::us()->warning(
				    "Lattice \"%s\" contains null/negative-length arcs; skipping arc.",
				    l->describe().c_str()) << " " << meshB.time() << " -> " << targetB.time() << "(label=" << a->input() << ")";
				continue;
			    }

			    verify(meshB.time() < targetB.time());
			    Flf::Arc arc(meshTarget.first, a->weight(), a->input(), a->output());
			    State::iterator pos = meshSp->lower_bound(arc, Ftl::byInputAndTarget<Lattice>());
			    if ((pos == meshSp->end())
				|| (arc.target() != pos->target()) || (arc.input() != pos->input())) {
				meshSp->insert(pos, arc);
			    }else{
				if (semiring->project(arc.weight()) < semiring->project(pos->weight()))
				    pos->setWeight(arc.weight());
			    }
			}
		    }
		}
	    }
	}

	Core::Vector<Fsa::StateId> initialSids;
	Flf::Boundary initialB;
	initialB.setTime(InvalidTime);

	for (Fsa::StateId stateId = 0; stateId <= s->maxStateId(); ++stateId)
	{
	    if (!s->hasState(stateId) || (s->fastState(stateId)->nArcs() == 0 && s->size() > 1))
		continue;
	    Flf::Boundary bound(b->get(stateId));
	    if (initialB.time() == InvalidTime || bound.time() < initialB.time())
	    {
		initialSids.clear();
		initialSids.push_back(stateId);
		initialB = bound;
	    }
	    else if(bound.time() == initialB.time())
	    {
		initialSids.push_back(stateId);
	    }
	}

	if (initialB.time() == InvalidTime)
	{
	    for (std::vector<Fsa::StateId>::iterator it = initialSids.begin(); it != initialSids.end(); ++it)
		b->set(*it, Flf::Boundary(0, b->get(*it).transit()));
	    std::cout << "initial time was invalid, resetting to 0" << std::endl;
	    initialB.setTime(0);
	}

	if (initialSids.size() == 1) {
	    s->setInitialStateId(initialSids.front());
	} else
	{
	    State *meshInitialSp;
	    std::pair<Fsa::StateId, bool> meshInitial = meshSids.insertExisting(initialB);
	    if (!meshInitial.second) {
		meshInitialSp = new State(meshInitial.first);
		verify(!s->hasState(meshInitialSp->id()));
		s->setState(meshInitialSp);
		b->set(meshInitialSp->id(), initialB);
	    } else
		meshInitialSp = s->fastState(meshInitial.first);
	    Speech::TimeframeIndex firstTime = Speech::InvalidTimeframeIndex;
	    for (Core::Vector<Fsa::StateId>::const_iterator itSid = initialSids.begin();
		 itSid != initialSids.end(); ++itSid)
		if (*itSid != meshInitialSp->id())
		    meshInitialSp->newArc(*itSid, semiring->one(), Fsa::Epsilon, Fsa::Epsilon);
	    s->setInitialStateId(meshInitialSp->id());
	}

	Core::Vector<Fsa::StateId> finalSids;
	Flf::Boundary finalB;
	finalB.setTime(InvalidTime);

	ConstStateMapRef topologicalSort = sortTopologically(staticLattice);

	for (s32 i = ((s32)topologicalSort->size())-1; i >= 0; --i)
	{
	    State *sp = s->fastState(topologicalSort->operator[](i));
	    if (!sp->nArcs())
	    {
		const Flf::Boundary& stateB = b->get(sp->id()).time();
		if (stateB.time() >= finalB.time() || finalB.time() == InvalidTime)
		{
		    if (stateB.time() > finalB.time() || finalB.time() == InvalidTime)
		    {
			finalSids.clear();
			finalB = stateB;
		    }
		    finalSids.push_back(sp->id());
		}
	    }
	}

	for (u32 i = 0; i < finalSids.size(); ++i)
	    s->fastState(finalSids[i])->setFinal(semiring->one());

	std::string desc = "mesh(" + entries[0].lattice->describe();
	for (u32 i = 1; i < entries.size(); ++i)
	    desc += "," + entries[i].lattice->describe();
	desc.at(desc.size() - 1) = ';';
	desc += semiring->name() + ")";
	s->setDescription(desc);

	    // Trim, as some paths may have become unreachable
	trimInPlace(staticLattice);
	((ConstLatticeRef)staticLattice)->setTopologicalSort(ConstStateMapRef());
	verify(staticLattice->initialStateId() != Fsa::InvalidStateId);
	verify(staticLattice->hasState(staticLattice->initialStateId()));
	return copyBoundaries(staticLattice);
    }

    ConstLatticeRef mesh(const std::vector<MeshEntry>& lattices, ConstSemiringRef semiring, MeshType meshType) {
	if (lattices.empty())
	    return ConstLatticeRef();
	if (!semiring) {
	    semiring = lattices[0].lattice->semiring();
	    for (u32 i = 1; i < lattices.size(); ++i)
		if ((semiring.get() != lattices[i].lattice->semiring().get()) &&
		    !(*semiring == *lattices[i].lattice->semiring()))
		    Core::Application::us()->criticalError(
			"mesh: Mesh requires equal semirings, but \"%s\" != \"%s\"",
			semiring->name().c_str(), lattices[i].lattice->semiring()->name().c_str());
	}

	switch (meshType) {
	case MeshTypeFullBoundary:
	    return buildMesh<MeshedFullBoundaryBuilder>(lattices, semiring);
	case MeshTypeTimeBoundary:
	    return buildMesh<MeshedTimeBoundaryBuilder>(lattices, semiring);
	default:
	    defect();
	}
	return ConstLatticeRef();
    }

    ConstLatticeRef mesh(ConstLatticeRef l1, ConstLatticeRef l2, ConstSemiringRef semiring, MeshType meshType) {
	std::vector<MeshEntry> lattices;
	MeshEntry e;
	e.lattice = l1;
	lattices.push_back(e);
	e.lattice = l2;
	lattices.push_back(e);
	return mesh(lattices, semiring, meshType);
    }

    ConstLatticeRef mesh(ConstLatticeRef l, MeshType meshType) {
	std::vector<MeshEntry> lattices;
	MeshEntry e;
	e.lattice = l;
	lattices.push_back(e);
	return mesh(lattices, l->semiring(), meshType);
    }
    // -------------------------------------------------------------------------


    // -------------------------------------------------------------------------
    class MeshNode : public FilterNode {
    public:
	static const Core::Choice choiceMeshType;
	static const Core::ParameterChoice paramMeshType;
    private:
	MeshType meshType_;
	ConstLatticeRef meshL_;
    protected:
	ConstLatticeRef filter(ConstLatticeRef l) {
	    if (!l)
		return ConstLatticeRef();
	    if (!meshL_)
		meshL_ = mesh(l, meshType_);
	    return meshL_;
	}
    public:
	MeshNode(const std::string &name, const Core::Configuration &config) :
	    FilterNode(name, config) {}
	virtual ~MeshNode() {}

	virtual void init(const std::vector<std::string> &arguments) {
	    Core::Component::Message msg = log();
	    Core::Choice::Value meshType = paramMeshType(config);
	    if (meshType ==  Core::Choice::IllegalValue)
		criticalError("Unknown mesh type");
	    meshType_ = MeshType(meshType);
	    msg << "mesh type is \"" << choiceMeshType[meshType_] << "\"\n";
	}

	virtual void sync() {
	    meshL_.reset();
	}
    };
    const Core::Choice MeshNode::choiceMeshType(
	"full", MeshTypeFullBoundary,
	"time", MeshTypeTimeBoundary,
	Core::Choice::endMark());
    const Core::ParameterChoice MeshNode::paramMeshType(
	"mesh-type",
	&MeshNode::choiceMeshType,
	"type of mesh",
	MeshTypeFullBoundary);
    NodeRef createMeshNode(const std::string &name, const Core::Configuration &config) {
	return NodeRef(new MeshNode(name, config));
    }
    // -------------------------------------------------------------------------

} // namespace Flf

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

#include "FlfCore/Basic.hh"
#include "FlfCore/LatticeInternal.hh"
#include "FlfCore/Utility.hh"
#include "Best.hh"
#include "Compose.hh"
#include "Copy.hh"
#include "Lexicon.hh"
#include "Map.hh"
#include "Union.hh"
#include "TimeAlignment.hh"
#include "TimeframeConfusionNetworkBuilder.hh"


namespace Flf {

    // -------------------------------------------------------------------------
    class StaticScoreLattice : public ModifyLattice {
	typedef ModifyLattice Precursor;
    private:
	ConstSemiringRef semiring_;
	ScoresRef weight_;
    public:
	StaticScoreLattice(ConstLatticeRef l, ConstSemiringRef semiring, ScoresRef weight) :
	    Precursor(l), semiring_(semiring), weight_(weight) {}
	virtual ~StaticScoreLattice() {}

	virtual ConstSemiringRef semiring() const {
	    return semiring_;
	}

	virtual void modifyState(State *sp) const {
	    if (sp->isFinal())
		sp->setWeight(weight_);
	    else
		sp->setWeight(semiring_->one());
	    for (State::iterator a = sp->begin(); a != sp->end(); ++a)
		a->setWeight(weight_);
	}

	virtual std::string describe() const {
	    return Core::form("static-score(%s;%s)",
			      fsa_->describe().c_str(),
			      semiring_->describe(weight_, Fsa::HintShowDetails).c_str());
	}
    };
    ConstLatticeRef staticScore(ConstLatticeRef l, ConstSemiringRef semiring, ScoresRef weight) {
	return ConstLatticeRef(new StaticScoreLattice(l, semiring, weight));
    }
    // -------------------------------------------------------------------------



    // -------------------------------------------------------------------------
    class TimeAlignmentBuilder::Internal : public Core::Component {
    public:
	static const Core::ParameterBool paramTryIntersection;
    private:
	static const u16 AlignedFlag = 0x8000;
	static const u16 BptrMask    = 0x7fff;

	struct Statistics {
	    u32 n;
	    u32 nIntersections;
	    u32 nFrameAlignments;
	    Statistics() : n(0), nIntersections(0), nFrameAlignments(0) {}
	};

    private:
	Core::XmlChannel statisticsChannel_;
	FwdBwdBuilderRef fbBuilder_;
	Statistics statistics_;
	bool tryIntersection_;
	Core::Ref<const Bliss::LemmaAlphabet> lAlphabet_;

    private:
	void distributeTimeOverUnalignedWords(StaticBoundaries *b, const std::vector<std::pair<Fsa::StateId, Fsa::LabelId> > &wordStarts) const {
	    if (wordStarts.size() < 3)
		return;
	    Time startTime = (*b)[wordStarts.back().first].time();
	    Time duration = ((*b)[wordStarts.front().first].time() - startTime) / (wordStarts.size() - 1);
	    Core::Component::Message msg(log("Could not align:\n"));
	    msg << "\t" << startTime << "\t" << lAlphabet_->symbol(wordStarts.back().second) << "\n";
	    startTime += duration;
	    for (u32 i = wordStarts.size() - 2; 0 < i; --i) {
		msg << "\t" << startTime << "\t" << lAlphabet_->symbol(wordStarts[i].second) << "\n";
		(*b)[wordStarts[i].first].setTime(startTime);
		startTime += duration;
	    }
	}

    public:
	Internal(const Core::Configuration &config, FwdBwdBuilderRef fbBuilder) :
	    Core::Component(config), statisticsChannel_(config, "statistics"), fbBuilder_(fbBuilder) {
	    if (!fbBuilder_)
		fbBuilder_ = FwdBwdBuilder::create(select("fb"));
	    tryIntersection_ = paramTryIntersection(config);
	    lAlphabet_ = Lexicon::us()->lemmaAlphabet();
	}
	~Internal() {}

	void dump(std::ostream &os) const {
	    os << "Alignment parameters:" << std::endl;
	    if (tryIntersection_)
		os << "Try lattice intersection first,\n"
		   << "do fCN alignment if intersection is empty.\n";
	    else
		os << "Do always fCN alignment.\n";
	}

	void dumpStatistics() {
	    if (statisticsChannel_.isOpen()) {
		statisticsChannel_ << Core::XmlOpen("statistics")
		    + Core::XmlAttribute("component", this->name())
		    + Core::XmlAttribute("name", "alignment");
		statisticsChannel_ << Core::XmlFull("count", statistics_.n)
		    + Core::XmlAttribute("name", "all");
		statisticsChannel_ << Core::XmlFull("count", statistics_.nIntersections)
		    + Core::XmlAttribute("name", "intersections");
		statisticsChannel_ << Core::XmlFull("count", statistics_.nFrameAlignments)
		    + Core::XmlAttribute("name", "fCN alignments");
		statisticsChannel_ << Core::XmlClose("statistics");
	    }
	}

    protected:
	ConstLatticeRef align(ConstLatticeRef lHyp, ConstPosteriorCnRef cnRef) {
	    verify((lHyp->type() == Fsa::TypeAcceptor)
		   && (Lexicon::us()->alphabetId(lHyp->getInputAlphabet()) == Lexicon::LemmaAlphabetId));
	    ConstSemiringRef hypSemiring = lHyp->semiring();
	    ScoresRef One = hypSemiring->one();
	    Core::Vector<std::pair<Fsa::LabelId, ScoresRef> > hyp(1, std::make_pair(Fsa::Epsilon, One));
	    for (ConstStateRef sr = lHyp->getState(lHyp->initialStateId()); sr->hasArcs();
		 sr = lHyp->getState(sr->begin()->target())) {
		if (sr->begin()->input() != Fsa::Epsilon) {
		    hyp.push_back(std::make_pair(sr->begin()->input(), sr->begin()->weight()));
		    hyp.push_back(std::make_pair(Fsa::Epsilon, One));
		} else {
		    hyp.back().second = hypSemiring->extend(hyp.back().second, sr->begin()->weight());
		}
	    }

	    verify(hyp.size() <= BptrMask);
	    verify(hyp.size() / 2 <= cnRef->size());

	    const Score unkScore = (Semiring::Max - 1.0) / Score(cnRef->size());
	    Core::Vector<Score> scores1(hyp.size() + 1, Semiring::Max), scores2(hyp.size() + 1, Semiring::Max);
	    Core::Vector<Score> *lastScoresPtr = &scores1, *currentScoresPtr = &scores2;
	    (*lastScoresPtr)[0] = 0.0;
	    Core::Vector<u16> bptr(hyp.size() * cnRef->size());

	    for (u32 t = 0; t < cnRef->size(); ++t) {
		const u32 offset = t * hyp.size();
		const PosteriorCn::Slot &pdf = (*cnRef)[t];
		Core::Vector<Score> &lastScores = *lastScoresPtr, &currentScores = *currentScoresPtr;
		currentScores[0] = Semiring::Max;
		for (s32 i = 0; size_t(i) < hyp.size(); ++i) {
		    currentScores[i + 1] = Semiring::Max;
		    const PosteriorCn::Arc cnArc(hyp[i].first, 0);
		    PosteriorCn::Slot::const_iterator itPdf = std::lower_bound(pdf.begin(), pdf.end(), cnArc);
		    Score localScore = unkScore;
		    u16 flag = 0;
		    if ((itPdf != pdf.end()) && (itPdf->label == cnArc.label)) {
			localScore = -::log(itPdf->score);
			flag = AlignedFlag;
		    } else if (cnArc.label == Fsa::Epsilon)
			continue;
		    s32 maxJ = (cnArc.label == Fsa::Epsilon) ? 1 : 2;
		    for (s32 j = 0; j <= maxJ; ++j) {
			if (lastScores[i - j + 1] == Semiring::Max)
			    continue;
			Score score = lastScores[i - j + 1] + localScore;
			if (score < currentScores[i + 1]) {
			    currentScores[i + 1] = score;
			    bptr[offset + i] = (i - j) | flag;
			}
		    }
		}
		std::swap(lastScoresPtr, currentScoresPtr);
	    }
	    verify(((*lastScoresPtr)[hyp.size() - 1] < Semiring::Max)
		   || ((*lastScoresPtr)[hyp.size()] < Semiring::Max));

	    std::vector<std::pair<Fsa::StateId, Fsa::LabelId> > wordStarts;
	    StaticBoundaries *b = new StaticBoundaries;
	    StaticLattice *s = new StaticLattice;
	    s->setDescription(Core::form("aligned(%s)", lHyp->describe().c_str()));
	    s->setType(Fsa::TypeAcceptor);
	    s->setProperties(Fsa::PropertyAcyclic | Fsa::PropertyLinear, Fsa::PropertyAll);
	    s->setInputAlphabet(lHyp->getInputAlphabet());
	    s->setSemiring(lHyp->semiring());
	    s->setBoundaries(ConstBoundariesRef(b));
	    State *lastSp = s->newState();
	    b->set(lastSp->id(), Boundary(cnRef->size()));
	    lastSp->setFinal(One);
	    wordStarts.push_back(std::make_pair(lastSp->id(), Fsa::InvalidLabelId));
	    u32 j = ((*lastScoresPtr)[hyp.size() - 1] < (*lastScoresPtr)[hyp.size()]) ?
		hyp.size() - 2 : hyp.size() - 1;
	    u32 lastJ = j;
	    std::pair<Fsa::LabelId, ScoresRef>
		currentHyp = std::make_pair(Fsa::InvalidLabelId, One),
		lastHyp = hyp[lastJ];
	    bool isLastAligned = false, hasUnaligned = false;
	    for (s32 t = cnRef->size() - 1; 0 <= t; --t) {
		const u32 offset(t * hyp.size());
		currentHyp = hyp[j];
		if (j < lastJ) {
		    State *sp = s->newState();
		    sp->newArc(lastSp->id(), lastHyp.second, lastHyp.first);
		    b->set(sp->id(), Boundary(t + 1));
		    if (isLastAligned) {
			if (lastHyp.first != Fsa::Epsilon) {
			    if (hasUnaligned)
				distributeTimeOverUnalignedWords(b, wordStarts);
			    wordStarts.clear();
			    hasUnaligned = false;
			}
		    } else
			hasUnaligned = true;
		    wordStarts.push_back(std::make_pair(sp->id(), lastHyp.first));
		    lastJ = j;
		    lastSp = sp;
		    lastHyp = currentHyp;
		    isLastAligned = false;
		} else
		    verify(j == lastJ);
		if (bptr[offset + j] & AlignedFlag) isLastAligned = true;
		j = bptr[offset + j] & BptrMask;
	    }
	    verify(currentHyp.first != Fsa::InvalidLabelId);
	    State *sp = s->newState();
	    sp->newArc(lastSp->id(), lastHyp.second, lastHyp.first);
	    b->set(sp->id(), Boundary(0));
	    if (isLastAligned && hasUnaligned)
		distributeTimeOverUnalignedWords(b, wordStarts);
	    else if (!isLastAligned) {
		wordStarts.push_back(std::make_pair(sp->id(), lastHyp.first));
		distributeTimeOverUnalignedWords(b, wordStarts);
	    }
	    s->setInitialStateId(sp->id());

	    verify(s->getBoundaries()->valid());

	    return ConstLatticeRef(s);
	}

	ConstLatticeRef align(ConstLatticeRef lHyp, ConstLatticeRef lRef) {
	    ensure(Lexicon::us()->alphabetId(lRef->getInputAlphabet()) == Lexicon::LemmaAlphabetId);
	    ConstLatticeRef lMeshedRef = persistent(staticScore(mesh(lRef, MeshTypeTimeBoundary), lHyp->semiring(), lHyp->semiring()->one()));
	    ConstLatticeRef lAlignedHyp = best(composeSequencing(lMeshedRef, lHyp), BellmanFord);

	    if (!lAlignedHyp || (lAlignedHyp->initialStateId() == Fsa::InvalidStateId))
		return ConstLatticeRef();
	    lAlignedHyp->addProperties(Fsa::PropertyLinear);
	    return lAlignedHyp;
	}

    public:
	ConstLatticeRef align(ConstLatticeRef lHyp, ConstLatticeRef lRef, ConstPosteriorCnRef cnRef) {
	    ++statistics_.n;
	    if (!lHyp || (lHyp->initialStateId() == Fsa::InvalidStateId))
		return ConstLatticeRef();
	    if (!lHyp->hasProperty(Fsa::PropertyLinear)) {
		for (ConstStateRef sr = lHyp->getState(lHyp->initialStateId()); sr->hasArcs();
		     sr = lHyp->getState(sr->begin()->target()))
		    if (sr->nArcs() != 1) criticalError(
			"Cannot align \"%s\" is not linear; lattice is not linear.",
			lHyp->describe().c_str());
		lHyp->addProperties(Fsa::PropertyLinear);
	    }
	    lHyp->setBoundaries(InvalidBoundaries);
	    ConstLatticeRef l;
	    if (lRef && tryIntersection_) {
		l = align(lHyp, lRef);
		if (l)
		    ++statistics_.nIntersections;
	    }
	    if (!l && (lRef || cnRef)) {
		if (!cnRef) {
		    std::pair<ConstLatticeRef, ConstFwdBwdRef> fbResult = fbBuilder_->build(lRef);
		    lRef = fbResult.first;
		    cnRef = buildFramePosteriorCn(lRef, fbResult.second);
		}
		l = align(lHyp, cnRef);
		if (l)
		    ++statistics_.nFrameAlignments;
	    }
	    return l;
	}
    };
    const Core::ParameterBool TimeAlignmentBuilder::Internal::paramTryIntersection(
	"intersection",
	"try alignment by intersection",
	true);
    // -------------------------------------------------------------------------


    // -------------------------------------------------------------------------
    TimeAlignmentBuilder::TimeAlignmentBuilder(const Core::Configuration &config, FwdBwdBuilderRef fbBuilder) {
	internal_ = new Internal(config, fbBuilder);
    }

    TimeAlignmentBuilder::~TimeAlignmentBuilder() {
	delete internal_;
    }

    void TimeAlignmentBuilder::dump(std::ostream &os) const {
	internal_->dump(os);
    }

    void TimeAlignmentBuilder::dumpStatistics() {
	internal_->dumpStatistics();
    }

    ConstLatticeRef TimeAlignmentBuilder::align(ConstLatticeRef lHyp, ConstLatticeRef lRef) {
	return internal_->align(lHyp, lRef, ConstPosteriorCnRef());
    }

    ConstLatticeRef TimeAlignmentBuilder::align(ConstLatticeRef lHyp, ConstPosteriorCnRef cnRef) {
	return internal_->align(lHyp, ConstLatticeRef(), cnRef);
    }

    ConstLatticeRef TimeAlignmentBuilder::align(ConstLatticeRef lHyp, ConstLatticeRef lRef, ConstPosteriorCnRef cnRef) {
	return internal_->align(lHyp, lRef, cnRef);
    }

    TimeAlignmentBuilderRef TimeAlignmentBuilder::create(const Core::Configuration &config, FwdBwdBuilderRef fbBuilder) {
	return TimeAlignmentBuilderRef(new TimeAlignmentBuilder(config, fbBuilder));
    }
    // -------------------------------------------------------------------------


    // -------------------------------------------------------------------------
    class TimeAlignmentNode : public FilterNode {
    private:
	LabelMapRef nonWordToEpsilonMap_;
	TimeAlignmentBuilderRef aligner_;
    protected:
	virtual ConstLatticeRef filter(ConstLatticeRef lHyp) {
	    if (!lHyp ||
		(lHyp->initialStateId() == Fsa::InvalidStateId) ||
		lHyp->getState(lHyp->initialStateId())->isFinal())
		return ConstLatticeRef();
	    verify((lHyp->type() == Fsa::TypeAcceptor)
		   && (Lexicon::us()->alphabetId(lHyp->getInputAlphabet()) == Lexicon::LemmaAlphabetId));
	    lHyp = applyOneToOneLabelMap(projectInput(lHyp), nonWordToEpsilonMap_);
	    ConstPosteriorCnRef cnRef;
	    if (connected(1)) {
		cnRef = requestPosteriorCn(1);
	    }
	    ConstLatticeRef lRef;
	    if (connected(2)) {
		lRef = requestLattice(2);
		lRef = applyOneToOneLabelMap(projectInput(mapInput(lRef, MapToLemma)), nonWordToEpsilonMap_);
	    }
	    return aligner_->align(lHyp, lRef, cnRef);
	}
    public:
	TimeAlignmentNode(const std::string &name, const Core::Configuration &config) :
	    FilterNode(name, config), aligner_() {}
	virtual ~TimeAlignmentNode() {}
	virtual void init(const std::vector<std::string> &arguments) {
	    if (!connected(1) && !connected(2))
		criticalError("Reference fCN at port 1 or reference lattice at port 2 required.");
	    nonWordToEpsilonMap_ = LabelMap::createNonWordToEpsilonMap(Lexicon::LemmaAlphabetId);
	    aligner_ = TimeAlignmentBuilder::create(config);
	    aligner_->dump(log());
	}
	virtual void finalize() {
	    aligner_->dumpStatistics();
	}
    };
    NodeRef createTimeAlignmentNode(const std::string &name, const Core::Configuration &config) {
	return NodeRef(new TimeAlignmentNode(name, config));
    }
    // -------------------------------------------------------------------------

} // namespace Flf

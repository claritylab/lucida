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
#include <Core/Channel.hh>
#include <Fsa/Hash.hh>

#include "FlfCore/Basic.hh"

#include "FwdBwd.hh"
#include "Lexicon.hh"
#include "TimeframeConfusionNetwork.hh"
#include "TimeframeConfusionNetworkBuilder.hh"
#include "TimeframeError.hh"

namespace Flf {

    // -------------------------------------------------------------------------
    TimeframeError::TimeframeError(Score alpha) :
	alpha(alpha),
	nWords(0),
	nFrames(0), nFrameErrors(0),
	nSmoothedFrames(0), nSmoothedFrameErrors(0) {}

    void TimeframeError::add(const TimeframeError &e) {
	verify(alpha == e.alpha);
	nWords += e.nWords;
	nFrames += e.nFrames;
	nFrameErrors += e.nFrameErrors;
	nSmoothedFrames += e.nSmoothedFrames;
	nSmoothedFrameErrors += e.nSmoothedFrameErrors;
    }

    void TimeframeError::add(ConstLatticeRef lHyp, ConstLatticeRef lRef) {
	ConstBoundariesRef bHyp = lHyp->getBoundaries();
	ConstStateRef srHyp = lHyp->getState(lHyp->initialStateId());
	Time hypBegin, hypEnd = bHyp->time(srHyp->id());
	Fsa::LabelId hypLabel = Fsa::Epsilon;

	ConstBoundariesRef bRef = lRef->getBoundaries();
	ConstStateRef srRef = lRef->getState(lRef->initialStateId());
	Time refBegin, refEnd = bRef->time(lRef->initialStateId());
	Fsa::LabelId refLabel = Fsa::Epsilon;

	if (hypEnd != refEnd)
	    Core::Application::us()->warning(
		"Hypothesis starts at %.3f and reference at %.3f; assume epsilon for preceeding time frames",
		(f32(hypEnd) / 100.0), (f32(refEnd) / 100.0));
	hypBegin = refBegin = std::min(hypEnd, refEnd);
	for (;;) {
	    verify(refBegin <= hypBegin);
	    Time nFrames = hypEnd - hypBegin;
	    Time nErrors = 0;
	    if (nFrames > 0) {
		while (refEnd < hypEnd) {
		    // add error
		    if (hypLabel != refLabel)
			nErrors += refEnd - std::max(hypBegin, refBegin);
		    refBegin = refEnd;
		    if (srRef->hasArcs()) {
			if (srRef->nArcs() > 1)
			    Core::Application::us()->criticalError(
				"Reference lattice \"%s\" is not linear.", lRef->describe().c_str());
			const Arc &refArc = *srRef->begin();
			srRef = lRef->getState(refArc.target());
			refEnd = bRef->time(srRef->id());
			refLabel = refArc.input();
		    } else if (hypEnd == Core::Type<Time>::max) {
			hypEnd = refEnd;
		    } else {
			Core::Application::us()->warning(
			    "Reference ends at %.3f before hypothesis; assume epsilon for remaining time frames",
			    (f32(refEnd) / 100.0));
			refEnd = Core::Type<Time>::max;
			refLabel = Fsa::Epsilon;
		    }
		}
		verify((refBegin <= hypEnd) && (hypEnd <= refEnd));
		// add error
		if (hypLabel != refLabel)
		    nErrors += hypEnd - std::max(hypBegin, refBegin);
		// count statistics
		verify(nErrors <= nFrames);
		Score error = Score(nErrors);
		Score duration = Score(nFrames);
		this->nWords++;
		this->nFrames += duration;
		this->nFrameErrors += error;
		// see TimeframeConfusionNetwork.hh -> FramePosteriorCnFeatures
		this->nSmoothedFrames += FramePosteriorCnFeatures::smooth(duration, duration, alpha);
		this->nSmoothedFrameErrors += FramePosteriorCnFeatures::smooth(error, duration, alpha);
	    }
	    if ((hypEnd == refEnd) && !srHyp->hasArcs() && !srRef->hasArcs())
		break;
	    {
		hypBegin = hypEnd;
		if (srHyp->hasArcs()) {
		    if (srHyp->nArcs() > 1)
			Core::Application::us()->criticalError(
			    "Hypothesis lattice \"%s\" is not linear.", lHyp->describe().c_str());
		    const Arc &hypArc = *srHyp->begin();
		    srHyp = lHyp->getState(hypArc.target());
		    hypEnd = bHyp->time(srHyp->id());
		    hypLabel = hypArc.input();
		} else if (refEnd == Core::Type<Time>::max) {
		    refEnd = hypEnd;
		} else {
		    Core::Application::us()->warning(
			"Hypothesis ends at %.3f before reference; assume epsilon for remaining time frames",
			(f32(hypEnd) / 100.0));
		    hypEnd = Core::Type<Time>::max;
		    hypLabel = Fsa::Epsilon;
		}
		verify(hypBegin <= refEnd);
	    }
	}
    }

    void TimeframeError::add(ConstLatticeRef lHyp, ConstPosteriorCnRef cnRef) {
	FramePosteriorCnFeaturesRef features = FramePosteriorCnFeatures::create(cnRef);
	ConstBoundariesRef bHyp = lHyp->getBoundaries();
	Time begin, end = bHyp->time(lHyp->initialStateId());
	for (ConstStateRef sr = lHyp->getState(lHyp->initialStateId()), nextSr; sr->hasArcs(); sr = nextSr) {
	    if (sr->nArcs() > 1)
		Core::Application::us()->criticalError(
		    "Hypothesis lattice \"%s\" is not linear.", lHyp->describe().c_str());
	    const Arc &arc = *sr->begin();
	    nextSr = lHyp->getState(arc.target());
	    begin = end;
	    end = bHyp->time(nextSr->id());
	    if (begin < end) {
		Time duration = end - begin;
		features->update(arc.input(), begin, duration);
		nWords++;
		nFrames += features->norm(0.0);
		nFrameErrors += features->error(0.0);
		nSmoothedFrames += features->norm(alpha);
		nSmoothedFrameErrors += features->error(alpha);
	    }
	}
    }

    Score TimeframeError::error() const {
	return nFrameErrors / nFrames;
    }

    Score TimeframeError::smoothedError() const {
	return nSmoothedFrameErrors / nSmoothedFrames;
    }

    TimeframeErrorRef TimeframeError::create(Score alpha) {
	return TimeframeErrorRef(new TimeframeError(alpha));
    }

    TimeframeErrorRef TimeframeError::create(ConstLatticeRef lHyp, ConstLatticeRef lRef, Score alpha) {
	TimeframeErrorRef e = TimeframeErrorRef(new TimeframeError(alpha));
	e->add(lHyp, lRef);
	return e;
    }

    TimeframeErrorRef TimeframeError::create(ConstLatticeRef lHyp, ConstPosteriorCnRef cnRef, Score alpha) {
	TimeframeErrorRef e = TimeframeErrorRef(new TimeframeError(alpha));
	e->add(lHyp, cnRef);
	return e;
    }
    // -------------------------------------------------------------------------


    // -------------------------------------------------------------------------
    class TimeframeErrorNode : public FilterNode {
    private:
	Core::XmlChannel xml_;
	TimeframeErrorRef sumError_;
	bool hasAdded_;

    protected:
	void dump(TimeframeErrorRef error) {
	    xml_ << Core::XmlFull("nWords", error->nWords);
	    xml_ << Core::XmlFull("nFrames", error->nFrames);
	    xml_ << Core::XmlFull("nFrameErrors", error->nFrameErrors);
	    xml_ << Core::XmlFull("error", error->error());
	    xml_ << Core::XmlFull("nSmoothedFrames", error->nSmoothedFrames);
	    xml_ << Core::XmlFull("nSmoothedFrameErrors", error->nSmoothedFrameErrors);
	    xml_ << Core::XmlFull("smoothed-error", error->smoothedError());
	}

	ConstLatticeRef filter(ConstLatticeRef lHyp) {
	    if (!lHyp) {
		warning("Empty hypothesis lattice, ignore for error counting.");
		return ConstLatticeRef();
	    }
	    if (!hasAdded_) {
		TimeframeErrorRef error;
		if (connected(1)) {
		    ConstLatticeRef lRef = requestLattice(1);
		    verify(lRef);
		    error = TimeframeError::create(lHyp, lRef, sumError_->alpha);
		} else {
		    ConstPosteriorCnRef cnRef = requestPosteriorCn(2);
		    verify(cnRef);
		    error = TimeframeError::create(lHyp, cnRef, sumError_->alpha);
		}
		sumError_->add(*error);
		xml_ << (Core::XmlOpen("time-frame-error") + Core::XmlAttribute("a", error->alpha));
		dump(error);
		xml_ << Core::XmlClose("time-frame-error");
		hasAdded_ = true;
	    }
	    return lHyp;
	}

    public:
	TimeframeErrorNode(const std::string &name, const Core::Configuration &config) :
	    FilterNode(name, config), xml_(config, "dump") {}
	virtual ~TimeframeErrorNode() {}

	virtual void init(const std::vector<std::string> &arguments) {
	    Core::Component::Message msg(log());
	    if (connected(1)) {
		msg << "Read reference lattice from port 1.\n";
	    } else if (connected(2)) {
		msg << "Read reference fCN from port 2.\n";
	    } else
		criticalError("Either reference lattice at port 1 or reference fCN at port 2 required.");
	    sumError_ = TimeframeError::create(FramePosteriorCnFeatures::paramAlpha(config, 0.05));
	    msg << "Error smoothing alpha is " << sumError_->alpha << ".";
	    hasAdded_ = false;
	}

	virtual void sync() {
	    hasAdded_ = false;
	}

	virtual void finalize() {
	    xml_ << (Core::XmlOpen("sum-time-frame-error") + Core::XmlAttribute("a", sumError_->alpha));
	    dump(sumError_);
	    xml_ << Core::XmlClose("sum-time-frame-error");
	}
    };

    NodeRef createTimeframeErrorNode(const std::string &name, const Core::Configuration &config) {
	return NodeRef(new TimeframeErrorNode(name, config));
    }
    // -------------------------------------------------------------------------


    // -------------------------------------------------------------------------
    class MinimumFrameWerDecoder : public Core::Component {
    public:
	typedef enum {
	    SearchSpaceTypeUnion,
	    SearchSpaceTypeMesh,
	    SearchSpaceTypeFrameCn
	} SearchSpaceType;
	static const Core::Choice choiceSearchSpaceType;
	static const Core::ParameterString paramSearchSpaceType;

	/**
	 * The real Decoder
	 *
	 * find best hypothesis given the lattice(s) and the
	 * (combined) timeframe word posterior distributions
	 **/
	class SearchSpace : public Core::Component {
	public:
	    static const ScoreId ConfidenceId = 0;
	private:
	    ConstSemiringRef confidenceSemiring_;
	protected:
	    inline ConstSemiringRef confidenceSemiring() const { return confidenceSemiring_; }
	public:
	    SearchSpace(const Core::Configuration &config) : Core::Component(config) {
		confidenceSemiring_ = Semiring::create(Fsa::SemiringTypeTropical, 1);
		confidenceSemiring_->setKey(ConfidenceId, "confidence");
		confidenceSemiring_->setScale(ConfidenceId, 1.0);
	    }
	    virtual ~SearchSpace() {}
	    virtual ConstLatticeRef best(
		const ConstLatticeRefList &lats,
		ConstPosteriorCnRef cn) = 0;
	};

    protected:
	SearchSpace * initSearchSpace(const Core::Configuration &config);

    private:
	SearchSpace *ss_;

    public:
	MinimumFrameWerDecoder(const Core::Configuration &config) :
	    Core::Component(config) {
	    ss_ = initSearchSpace(config);
	}
	~MinimumFrameWerDecoder() {
	    delete ss_;
	}

	ConstLatticeRef decode(const ConstLatticeRefList &lats, ConstPosteriorCnRef cn) {
	    ensure(!lats.empty());
	    ensure(cn);
	    ConstLatticeRef result = ss_->best(lats, cn);
	    return result;
	}
    };
    const Core::Choice MinimumFrameWerDecoder::choiceSearchSpaceType = Core::Choice(
	"union", SearchSpaceTypeUnion,
	"mesh",  SearchSpaceTypeMesh,
	"cn",    SearchSpaceTypeFrameCn,
	Core::Choice::endMark());
    const Core::ParameterString MinimumFrameWerDecoder::paramSearchSpaceType(
	"search-space",
	"search spaces: union, mesh or cn(=fCN)",
	"mesh");
    // -------------------------------------------------------------------------


    // -------------------------------------------------------------------------
    /**
     * min. potential based max. posterior decoding
     *
     * Standard lattice decoder; best hypothesis present in one of the
     * given lattices.
     **/
    class UnionSearchSpace :
	    public MinimumFrameWerDecoder::SearchSpace {
    public:
	static const Core::ParameterFloat paramAlpha;
	static const Core::ParameterFloat paramNonWordAlpha;
	static const Core::ParameterString paramConfKey;
    private:
	Score alpha_;
	Score nonWordAlpha_;
	std::vector<Fsa::LabelId> nonWords_;
	Fsa::LabelId lastNonWord_;
	Key confidenceKey_;

    protected:
	Score alpha(Fsa::LabelId word) const {
	    if (word == Fsa::Epsilon)
		return nonWordAlpha_;
	    if (word > lastNonWord_)
		return alpha_;
	    std::vector<Fsa::LabelId>::const_reverse_iterator itNonWord = nonWords_.rbegin();
	    for (; *itNonWord > word; ++itNonWord);
	    return (*itNonWord == word) ? nonWordAlpha_ : alpha_;
	}


	/*
	  Cost function

	  S(w; t_start, t_end) = sum_{t \in [t_start, t_end[} (1 - p(w|t))
				 / (1 + alpha (t_end - t_start - 1))

	*/
	Score calcScore(const PosteriorCn &cn, Fsa::LabelId label,
			Time start, Time end) const {
	    if (start == end)
		return 0.0;
	    const PosteriorCn::Arc cnArc(label, 0.0);
	    Score sum = 0.0;
	    for (Time t = start; t < end; ++t)
		sum += cn.score(t, label);
	    Score duration = end - start;

	    /*
	    if (!((0.0 <= sum) && (sum <= duration + 0.001)))
		dbg("not 0.0 <= " << sum << " <= " << duration);
	    */
	    //verify((0.0 <= sum) && (sum <= duration + 0.001));
	    return (duration - sum) / (1.0 + alpha(label) * (duration - 1.0));
	    //Score s = (length - sum) / (1.0 + alpha(label) * (length - 1.0));
	    //return s * s;
	}

	/*
	  Playing around with position-dependent weighted time frames
	  seems not to help - but perhaps i did not play around long
	  enough ...

	Score calcScore(const PosteriorCn &cn, Fsa::LabelId label,
			Time start, Time end) const {
	    if (start == end)
		return 0.0;
	    const PosteriorCn::Arc cnArc(label, 0.0);
	    Probability sum = 0.0;

	    Score duration = 0.0;
	    for (Time t = start; t < end; ++t) {

		Score w = 1.0;
		if ((t == start + 1) || (t == end - 2)) w = 0.75;
		if ((t == start) || (t == end - 1)) w = 0.25;

		const PosteriorCn::Slot &pdf(cn[t]);
		PosteriorCn::Slot::const_iterator itPdf = std::lower_bound(
		    pdf.begin(), pdf.end(), cnArc);
		// verify((itPdf != pdf.end()) && (itPdf->label == cnArc.label));
		if ((itPdf != pdf.end()) && (itPdf->label == cnArc.label))
		    sum += w * itPdf->score;
		duration += w;
	    }
	    verify((0.0 <= sum) && (sum <= duration));

	    // return (duration - sum) / (1.0 + alpha_ * (duration - 1.0));
	    Score length = end - start;
	    sum *= length / duration;
	    return (length - sum) / (1.0 + alpha_ * (length - 1.0));
	}
	*/

	/*
	  Confidence score
	*/
	Score calcConfidence(const PosteriorCn &cn, Fsa::LabelId label,
			     Time start, Time end) const {
	    if (start == end)
		return 1.0;
	    const PosteriorCn::Arc cnArc(label, 0.0);
	    Probability maxP = 0.0;
	    for (Time t = start; t < end; ++t)
		maxP = std::max(maxP, cn.score(t, label));
	    return maxP;
	}

	struct Hyp {
	    Score score;
	    Fsa::LabelId label;
	    ScoresRef weight;
	    Fsa::StateId bptr;
	    Hyp() :
		score(Semiring::Max),
		label(Fsa::InvalidLabelId),
		bptr(Fsa::InvalidStateId) {}
	};
	struct Trace : Core::Vector<Hyp>, public Core::ReferenceCounted {
	    Score score;
	    Fsa::StateId final;
	    u32 system;
	    Trace():
		Core::Vector<Hyp>(),
		score(Semiring::Max),
		final(Fsa::InvalidStateId),
		system(Core::Type<u32>::max) {}
	};
	typedef Core::Ref<const Trace> ConstTraceRef;

	ConstTraceRef search(
	    const ConstLatticeRefList &lats,
	    const PosteriorCn &cn) const {
	    Trace *currentTrace = new Trace, *bestTrace = new Trace;
	    Score bestScore = Semiring::Max;
	    Fsa::StateId bestFinal = Fsa::InvalidStateId;
	    ScoresRef bestFinalWeight = lats.front()->semiring()->one();
	    u32 bestSystem = Core::Type<u32>::max;
	    for (u32 i = 0; i < lats.size(); ++i) {
		ConstLatticeRef l = lats[i];
		ConstStateMapRef topologicalSort = sortTopologically(l);
		verify(topologicalSort);
		currentTrace->clear();
		currentTrace->grow((*topologicalSort)[0]);
		(*currentTrace)[(*topologicalSort)[0]].score = 0.0;
		for (u32 j = 0; j < topologicalSort->size(); ++j) {
		    Fsa::StateId sid = (*topologicalSort)[j];
		    ConstStateRef sr = l->getState(sid);
		    Time startTime = l->boundary(sid).time();
		    Score fwdScore = (*currentTrace)[sid].score;
		    if (sr->isFinal() && (fwdScore < bestScore)) {
			bestScore = fwdScore;
			bestFinal = sid;
			bestFinalWeight = sr->weight();
			bestSystem = i;
		    }
		    for (State::const_iterator a = sr->begin(); a != sr->end(); ++a) {
			Time endTime = l->boundary(a->target()).time();
			Score currentScore = calcScore(cn, a->input(), startTime, endTime);
			currentTrace->grow(a->target());
			Hyp &currentHyp = (*currentTrace)[a->target()];
			Score score = fwdScore + currentScore;
			if (score < currentHyp.score) {
			    currentHyp.score = score;
			    currentHyp.label = a->input();
			    currentHyp.weight = a->weight();
			    currentHyp.bptr = sid;
			}
		    }
		}
		if (bestSystem == i)
		    std::swap(bestTrace, currentTrace);
	    }
	    delete currentTrace;
	    bestTrace->score = bestScore;
	    bestTrace->final = bestFinal;
	    bestTrace->system = bestSystem;
	    return ConstTraceRef(bestTrace);
	}

	ConstLatticeRef trace(
	    const ConstLatticeRefList &lats,
	    const PosteriorCn &cn,
	    ConstTraceRef trace) const {
	    ConstSemiringRef semiring = lats[trace->system]->semiring();
	    ScoreId confidenceId =  Semiring::InvalidId;
	    if (semiring.get() == confidenceSemiring().get())
		confidenceId = ConfidenceId;
	    else if (!confidenceKey_.empty()) {
		confidenceId = semiring->id(confidenceKey_);
		if (confidenceId == Semiring::InvalidId)
		    warning("Semiring \"%s\" has no dimension labeled \"%s\".",
			    semiring->name().c_str(), confidenceKey_.c_str());
	    }
	    ConstBoundariesRef boundaries = lats[trace->system]->getBoundaries();
	    StaticBoundaries *b = new StaticBoundaries;
	    StaticLattice *s = new StaticLattice;
	    s->setDescription("min-fWER(union," + semiring->name() + ")");
	    s->setType(Fsa::TypeAcceptor);
	    s->setProperties(Fsa::PropertyAcyclic | Fsa::PropertyLinear, Fsa::PropertyAll);
	    s->setInputAlphabet(lats[0]->getInputAlphabet());
	    s->setSemiring(semiring);
	    s->setBoundaries(ConstBoundariesRef(b));
	    State *lastSp = s->newState();
	    lastSp->setFinal(semiring->clone(semiring->one()));
	    b->set(lastSp->id(), boundaries->get(trace->final));
	    for (Fsa::StateId sourceSid = (*trace)[trace->final].bptr, targetSid = trace->final;
		 sourceSid != Fsa::InvalidStateId; sourceSid = (*trace)[targetSid].bptr) {
		const Hyp &target((*trace)[targetSid]);
		State *sp = s->newState();
		b->set(sp->id(), boundaries->get(sourceSid));
		ScoresRef score;
		if (confidenceId != Semiring::InvalidId) {
		    score = semiring->clone(target.weight);
		    score->set(confidenceId, calcConfidence(
				   cn, target.label, b->time(sp->id()), b->time(lastSp->id())));
		} else
		    score = target.weight;
		sp->newArc(lastSp->id(), score, target.label);
		lastSp = sp;
		targetSid = sourceSid;
	    }
	    s->setInitialStateId(lastSp->id());
	    return ConstLatticeRef(s);
	}

    public:
	UnionSearchSpace(const Core::Configuration &config) :
	    MinimumFrameWerDecoder::SearchSpace(config) {
	    alpha_ = paramAlpha(config);
	    nonWordAlpha_ = paramNonWordAlpha(config, alpha_);
	    verify(Fsa::Epsilon < Fsa::FirstLabelId);
	    nonWords_ = Lexicon::us()->nonWordLemmaIds();
	    nonWords_.push_back(Fsa::Epsilon);
	    std::sort(nonWords_.begin(), nonWords_.end());
	    lastNonWord_ = nonWords_.back();
	    confidenceKey_ = paramConfKey(config);
	    Core::Component::Message msg(log());
	    msg << "Word arc normalization alpha is " << alpha_ << ".\n";
	    msg << "Non word arc normalization alpha is " << nonWordAlpha_ << ";\n"
		<< (nonWords_.size() - 1) << " non words found.\n";
	    if (!confidenceKey_.empty())
		msg << "Confidence key is \"" << confidenceKey_ << "\".";
	}
	virtual ~UnionSearchSpace() {}

	virtual ConstLatticeRef best(
	    const ConstLatticeRefList &lats,
	    ConstPosteriorCnRef cn) {
	    if (lats.empty())
		return ConstLatticeRef();
	    ConstTraceRef best = search(lats, *cn);
	    log("Best system is %d, min. score is %f.", best->system, best->score);
	    ConstLatticeRef result = trace(lats, *cn, best);
	    return result;
	}
    };
    const Core::ParameterFloat UnionSearchSpace::paramAlpha(
	"alpha",
	"smooth time frame error normalization",
	0.05);
    const Core::ParameterFloat UnionSearchSpace::paramNonWordAlpha(
	"non-word-alpha",
	"smooth time frame error normalization for non words",
	0.0);
    const Core::ParameterString UnionSearchSpace::paramConfKey(
	"confidence-key",
	"store confidence score",
	"");
    // -------------------------------------------------------------------------


    // -------------------------------------------------------------------------
    /**
     * min. potential based max. posterior decoding
     *
     * Lattices are transformed to the union of their time conditioned forms.
     * A standard lattice decoding is done on the resulting mesh.
     **/
    class MeshSearchSpace :
	public UnionSearchSpace {
	typedef UnionSearchSpace Precursor;
    public:
	/*
	  Attention: the usage of more boundary information than time causes problems
	  in conjunction with null-length arcs: loops cannot be detected/avoided easily.
	  Allowing more boundary information would require a two-step procedure:
	  1. build mesh, allow loops
	  2. detect loops and remove arcs closing a loop (which arc to remove might be arbitrary)
	*/
	struct BoundaryTimeHash {
	    size_t operator() (const Boundary &b) const
		{ return size_t(b.time()); }
	};
	struct BoundaryTimeEqual {
	    bool operator() (const Boundary &b1, const Boundary &b2) const
		{ return b1.time() == b2.time(); }
	};
	typedef Fsa::Hash<Boundary, BoundaryTimeHash, BoundaryTimeEqual> BoundaryTimeHashList;
    protected:
	ConstLatticeRef buildMesh(const ConstLatticeRefList &lats) {
	    ScoresRef defaultScore;
	    ConstSemiringRef semiring = lats[0]->semiring();
	    for (u32 i = 1; i < lats.size(); ++i)
		if ((semiring.get() != lats[i]->semiring().get()) &&
		    !(semiring == lats[i]->semiring())) {
		    semiring = confidenceSemiring();
		    defaultScore = confidenceSemiring()->one();
		    log("Building time conditioned search space: Semirings are not equal (%s != %s), use %s",
			lats[0]->semiring()->name().c_str(), lats[i]->semiring()->name().c_str(), semiring->name().c_str());
		    break;
		}
	    StaticBoundaries *b = new StaticBoundaries;
	    StaticLattice *s = new StaticLattice(Fsa::TypeAcceptor);
	    s->setProperties(lats[0]->knownProperties(), lats[0]->properties());
	    s->setDescription(Core::form("mesh-search-space(%s)", semiring->name().c_str()));
	    s->setInputAlphabet(lats[0]->getInputAlphabet());
	    s->setSemiring(semiring);
	    s->setBoundaries(ConstBoundariesRef(b));
	    Core::Vector<Fsa::StateId> initialSids;
	    BoundaryTimeHashList meshSids;
	    for (u32 i = 0; i < lats.size(); ++i) {
		ConstLatticeRef l = lats[i];
		ConstStateMapRef topologicalSort = sortTopologically(l);
		verify(topologicalSort);
		const Boundary &initialB = l->boundary(topologicalSort->front());
		std::pair<Fsa::StateId, bool> meshInitial = meshSids.insertExisting(initialB);
		if (!meshInitial.second) {
		    State *meshInitialSp = new State(meshInitial.first);
		    s->setState(meshInitialSp);
		    initialSids.push_back(meshInitialSp->id());
		    b->set(meshInitialSp->id(), Boundary(initialB.time()));
		}
		for (u32 j = 0; j < topologicalSort->size(); ++j) {
		    Fsa::StateId sid = (*topologicalSort)[j];
		    ConstStateRef sr = l->getState(sid);
		    const Boundary &meshB = l->boundary(sid);
		    Fsa::StateId meshSid = meshSids.find(meshB);
		    verify(meshSid != BoundaryTimeHashList::InvalidCursor);
		    State *meshSp = s->fastState(meshSid);
		    if (sr->isFinal()) {
			if (defaultScore)
			    meshSp->setFinal(defaultScore);
			else {
			    if (meshSp->isFinal()) {
				if (semiring->compare(meshSp->weight(), sr->weight()) != 0)
				    warning("Building time conditioned search space: Final scores at time %d are not equal",
					    meshB.time());
			    } else
				meshSp->setFinal(sr->weight());
			}
		    }
		    for (State::const_iterator a = sr->begin(); a != sr->end(); ++a) {
			const Boundary &targetB = l->boundary(a->target());
			if (meshB.time() < targetB.time()) {
			    std::pair<Fsa::StateId, bool> meshTarget = meshSids.insertExisting(targetB);
			    if (!meshTarget.second) {
				State *meshTargetSp = new State(meshTarget.first);
				s->setState(meshTargetSp);
				b->set(meshTargetSp->id(), Boundary(targetB.time()));
			    }
			    meshSp->newArc(meshTarget.first, (defaultScore ? defaultScore : a->weight()), a->input());
			} else
			    verify(meshB.time() == targetB.time());
		    }
		}
	    }
	    if (initialSids.size() == 1)
		s->setInitialStateId(initialSids.front());
	    else {
		State *meshInitialSp;
		Boundary initialB(0, Boundary::Transit(Bliss::Phoneme::term, Bliss::Phoneme::term));
		std::pair<Fsa::StateId, bool> meshInitial = meshSids.insertExisting(initialB);
		if (!meshInitial.second) {
		    meshInitialSp = new State(meshInitial.first);
		    s->setState(meshInitialSp);
		    b->set(meshInitialSp->id(), Boundary(initialB.time()));
		} else
		    meshInitialSp = s->fastState(meshInitial.first);
		for (Core::Vector<Fsa::StateId>::const_iterator itSid = initialSids.begin();
		     itSid != initialSids.end(); ++itSid)
		    if (*itSid != meshInitialSp->id())
			meshInitialSp->newArc(*itSid, s->semiring()->one(), Fsa::Epsilon, Fsa::Epsilon);
		s->setInitialStateId(meshInitialSp->id());
	    }
	    return ConstLatticeRef(s);
	}

    public:
	MeshSearchSpace(const Core::Configuration &config) :
	    Precursor(config) {
	}
	virtual ~MeshSearchSpace() {}

	virtual ConstLatticeRef best(
	    const ConstLatticeRefList &lats,
	    ConstPosteriorCnRef cn) {
	    if (lats.empty())
		return ConstLatticeRef();
	    ConstLatticeRefList meshedLats(1, buildMesh(lats));
	    ConstTraceRef best = Precursor::search(meshedLats, *cn);
	    log("Min. score is %f.", best->score);
	    ConstLatticeRef result = Precursor::trace(meshedLats, *cn, best);
	    dynamic_cast<StaticLattice*>(const_cast<Lattice*>(result.get()))
		->setDescription(Core::form("min-fWER(mesh,%s)", result->semiring()->name().c_str()));
	    return result;
	}
    };
    // -------------------------------------------------------------------------


    // -------------------------------------------------------------------------
    class FrameCnSearchSpace :
	    public MinimumFrameWerDecoder::SearchSpace {
    public:
	static const Core::ParameterFloat paramWordPenalty;

    private:
	Score wordPenalty_;

    protected:
	struct Hyp {
	    Score score;
	    Fsa::LabelId label;
	    u32 bptr;
	};
	typedef Core::Vector<Core::Vector<Hyp> > Trace;

    public:
	virtual ConstLatticeRef best(
	    const ConstLatticeRefList &lats,
	    ConstPosteriorCnRef cnRef) {
	    const PosteriorCn &cn(*cnRef);
	    Trace trace(cn.size());
	    trace[0].resize(cn.front().size());
	    for (u32 j = 0; j < trace.front().size(); ++j) {
		Hyp &hyp(trace.front()[j]);
		hyp.score = -::log(cn.front()[j].score);
		hyp.label = cn.front()[j].label;
		hyp.bptr = Core::Type<u32>::max;
	    }
	    for (u32 t = 1; t < cn.size(); ++t) {
		const PosteriorCn::Slot &prePdf(cn[t - 1]);
		const Core::Vector<Hyp> &preHyps(trace[t - 1]);
		const PosteriorCn::Slot &pdf(cn[t]);
		Core::Vector<Hyp> &hyps(trace[t]);
		hyps.resize(pdf.size());
		for (u32 j = 0; j < hyps.size(); ++j) {
		    const PosteriorCn::Arc &cnArc(pdf[j]);
		    Hyp &hyp(hyps[j]);
		    hyp.score = Semiring::Max;
		    for (u32 k = 0; k < preHyps.size(); ++k) {
			Score score = preHyps[k].score - Score(::log(cnArc.score));
			if (prePdf[k].label != cnArc.label)
			    score += wordPenalty_;
			if (score < hyp.score) {
			    hyp.score = score;
			    hyp.label = cnArc.label;
			    hyp.bptr = k;
			}
		    }
		}
	    }
	    Hyp bestHyp;
	    bestHyp.score = Semiring::Max;
	    bestHyp.label = Fsa::InvalidLabelId;
	    bestHyp.bptr = Core::Type<u32>::max;
	    for (u32 j = 0; j < trace.back().size(); ++j) {
		const Hyp &hyp(trace.back()[j]);
		if (hyp.score < bestHyp.score) {
		    bestHyp.score = hyp.score;
		    bestHyp.bptr = j;
		}
	    }
	    verify(bestHyp.score != Semiring::Max);

	    log("Min. score is %f.", bestHyp.score);

	    ConstSemiringRef semiring = confidenceSemiring();
	    StaticBoundaries *b = new StaticBoundaries;
	    StaticLattice *s = new StaticLattice;
	    s->setType(Fsa::TypeAcceptor);
	    s->setProperties(Fsa::PropertyAcyclic | Fsa::PropertyLinear, Fsa::PropertyAll);
	    s->setInputAlphabet(lats.front()->getInputAlphabet());
	    s->setSemiring(semiring);
	    s->setDescription(Core::form("min-fWER(fCN,%s)", s->semiring()->name().c_str()));
	    s->setBoundaries(ConstBoundariesRef(b));
	    State *lastSp = s->newState();
	    b->set(lastSp->id(), Boundary(trace.size()));
	    lastSp->setFinal(semiring->clone(semiring->one()));
	    u32 k = bestHyp.bptr;
	    Hyp lastHyp(trace.back()[k]);
	    Probability confScore = Semiring::Max;
	    for (s32 t = trace.size() - 1; 0 <= t; --t) {
		const Hyp &hyp(trace[t][k]);
		if (hyp.label != lastHyp.label) {
		    ScoresRef score = semiring->create();
		    //score->set(PosteriorId, ::exp(hyp.score - lastHyp.score));
		    score->set(ConfidenceId, ::exp(-confScore));
		    confScore = Semiring::Max;
		    State *sp = s->newState();
		    b->set(sp->id(), Boundary(t + 1));
		    sp->newArc(lastSp->id(), score, lastHyp.label);
		    lastHyp = hyp;
		    lastSp = sp;
		}
		confScore = std::min(confScore, -::log(cn[t][k].score));
		k = hyp.bptr;
	    }
	    ScoresRef score = semiring->create();
	    score->set(ConfidenceId, ::exp(-confScore));
	    State *sp = s->newState();
	    b->set(sp->id(), Boundary(0));
	    sp->newArc(lastSp->id(), score, lastHyp.label);
	    s->setInitialStateId(sp->id());
	    return ConstLatticeRef(s);
	}

    public:
	FrameCnSearchSpace(const Core::Configuration &config) :
	    MinimumFrameWerDecoder::SearchSpace(config) {
	    wordPenalty_ = paramWordPenalty(config);
	    log("word-penalty=%f", wordPenalty_);
	}
	virtual ~FrameCnSearchSpace() {}
    };
    const Core::ParameterFloat FrameCnSearchSpace::paramWordPenalty(
	"word-penalty",
	"penalty per word change",
	0.0);
    // -------------------------------------------------------------------------


    // -------------------------------------------------------------------------
    MinimumFrameWerDecoder::SearchSpace * MinimumFrameWerDecoder::initSearchSpace(
	const Core::Configuration &config) {
	Core::Choice::Value ssChoice = choiceSearchSpaceType[paramSearchSpaceType(config)];
	if (ssChoice == Core::Choice::IllegalValue) {
	    criticalError("Unknown search space \"%s\"",
			  paramSearchSpaceType(config).c_str());
	    return 0;
	}
	log() << "Search space is \"" <<
	    choiceSearchSpaceType[SearchSpaceType(ssChoice)] << "\".";
	switch (SearchSpaceType(ssChoice)) {
	case SearchSpaceTypeUnion:
	    return new UnionSearchSpace(
		Core::Configuration(config, "union"));
	case SearchSpaceTypeMesh:
	    return new MeshSearchSpace(
		Core::Configuration(config, "mesh"));
	case SearchSpaceTypeFrameCn:
	    return new FrameCnSearchSpace(
		Core::Configuration(config, "cn"));
	default:
	    return 0;
	}
    }
    // -------------------------------------------------------------------------


    // -------------------------------------------------------------------------
    class MinimumFrameWerDecoderNode : public Node {
    private:
	u32 n_;
	FwdBwdBuilderRef fbBuilder_;
	MinimumFrameWerDecoder *decoder_;
	ConstLatticeRef best_;

    public:
	MinimumFrameWerDecoderNode(const std::string &name, const Core::Configuration &config) :
	    Node(name, config), n_(0), decoder_(0) {}
	virtual ~MinimumFrameWerDecoderNode() {
	    delete decoder_;
	}

	virtual void init(const std::vector<std::string> &arguments) {
	    if (!connected(1))
		criticalError("Input at port 1 required.");
	    for (n_ = 1; connected(n_ + 1); ++n_);
	    Core::Component::Message msg(log("Search %d lattice(s). ", n_));
	    if (connected(0))
		msg << "Get fCN at port 0.";
	    else
		msg << "Calculate fCN from lattice(s)";
	    fbBuilder_ = FwdBwdBuilder::create(select("fb"));
	    decoder_ = new MinimumFrameWerDecoder(config);
	}

	virtual ConstLatticeRef sendLattice(Port to) {
	    verify(to == 0);
	    if (!best_) {
		ConstLatticeRefList lats(n_);
		for (u32 i = 0; i < n_; ++i)
		    lats[i] = requestLattice(i + 1);
		ConstPosteriorCnRef cn_;
		if (connected(0))
		    cn_ = requestPosteriorCn(0);
		else {
		    std::pair<ConstLatticeRef, ConstFwdBwdRef> fbResult = (n_ == 1) ?
			fbBuilder_->build(lats.front()) : fbBuilder_->build(lats);
		    cn_ = buildFramePosteriorCn(fbResult.first, fbResult.second);
		}
		best_ = decoder_->decode(lats, cn_);

		/*
		  Core::Component::Message os(log());
		  os << Core::XmlOpen("best-time-frame-word-posterior-hypothesis");
		  for (ConstStateRef sr = best->getState(best->initialStateId()); sr->hasArcs();
		  sr = best->getState(sr->begin()->target()))
		  os << best->getInputAlphabet()->symbol(sr->begin()->input()) << Core::XmlBlank();
		  os << Core::XmlClose("best-time-frame-word-posterior-hypothesis");
		*/
	    }
	    return best_;
	}

	virtual void sync() {
	    best_.reset();
	}
    };
    NodeRef createMinimumFrameWerDecoderNode(const std::string &name, const Core::Configuration &config) {
	return NodeRef(new MinimumFrameWerDecoderNode(name, config));
    }
    // -------------------------------------------------------------------------

} // namesapce Flf

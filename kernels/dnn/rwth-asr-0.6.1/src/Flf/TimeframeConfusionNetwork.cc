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

#include "FlfCore/LatticeInternal.hh"
#include "FlfCore/Utility.hh"
#include "FwdBwd.hh"
#include "GammaCorrection.hh"
#include "TimeframeConfusionNetwork.hh"
#include "TimeframeConfusionNetworkBuilder.hh"
#include "RescoreInternal.hh"

#include "Union.hh"


namespace Flf {

    // -------------------------------------------------------------------------
    namespace {
	const class PosteriorCnConverter {
	private:
	    mutable ConstSemiringRef confSemiring_;

	private:
	    ConstSemiringRef confSemiring() const {
		if (!confSemiring_) {
		    confSemiring_ =
			Semiring::create(Fsa::SemiringTypeTropical, 1);
		    confSemiring_->setKey(0, "confidence");
		    confSemiring_->setScale(0, 0.0);
		}
		return confSemiring_;
	    }

	    Fsa::StateId addStateAndArc(
		Fsa::StateId sid, StaticLattice *s, StaticBoundaries *b,
		Probability p, Fsa::LabelId label) const {
		State *sp = s->newState(sid);
		s->setState(sp);
		b->set(sid, Boundary(Time(sid)));
		++sid;
		ScoresRef score = confSemiring()->create();
		score->set(0, p);
		sp->newArc(sid, score, label);
		return sid;
	    }

	public:
	    PosteriorCnConverter() {}

	    ConstLatticeRef convert(ConstPosteriorCnRef cn) const {
		if (!cn)
		    return ConstLatticeRef();
		StaticBoundaries *b = new StaticBoundaries;
		StaticLattice *s = new StaticLattice;
		s->setDescription("cn");
		s->setType(Fsa::TypeAcceptor);
		s->setProperties(Fsa::PropertyAcyclic, Fsa::PropertyAll);
		s->setInputAlphabet(cn->alphabet);
		s->setSemiring(confSemiring());
		s->setBoundaries(ConstBoundariesRef(b));
		for (Fsa::StateId sid = 0; sid < cn->size(); ++sid) {
		    State *sp = s->newState(sid);
		    s->setState(sp);
		    b->set(sid, Boundary(Time(sid)));
		    const PosteriorCn::Slot &pdf = (*cn)[sid];
		    for (PosteriorCn::Slot::const_iterator itPdf = pdf.begin(); itPdf != pdf.end(); ++itPdf) {
			ScoresRef score = confSemiring()->create();
			score->set(0, itPdf->score);
			sp->newArc(sid + 1, score, itPdf->label);
		    }
		}
		s->setStateFinal(s->newState(cn->size()), confSemiring()->clone(confSemiring()->one()));
		b->set(cn->size(), Boundary(Time(cn->size())));
		s->setInitialStateId(0);
		return ConstLatticeRef(s);
	    }

	    ConstLatticeRef decode(ConstPosteriorCnRef cn) const {
		if (!cn)
		    return ConstLatticeRef();
		StaticBoundaries *b = new StaticBoundaries;
		StaticLattice *s = new StaticLattice;
		s->setDescription("decode-cn");
		s->setType(Fsa::TypeAcceptor);
		s->setProperties(Fsa::PropertyAcyclic | Fsa::PropertyLinear, Fsa::PropertyAll);
		s->setInputAlphabet(cn->alphabet);
		s->setSemiring(confSemiring());
		s->setBoundaries(ConstBoundariesRef(b));
		Fsa::StateId sid = 0;
		Probability epsPosterior = 1.0;
		bool hasPendingEps = false;
		for (u32 i = 0; i < cn->size(); ++i) {
		    const PosteriorCn::Slot &pdf = (*cn)[i];
		    Probability maxPosterior = 0.0;
		    Fsa::LabelId label = Fsa::InvalidLabelId;
		    for (PosteriorCn::Slot::const_iterator itPdf = pdf.begin(); itPdf != pdf.end(); ++itPdf) {
			if (itPdf->score > maxPosterior) {
			    maxPosterior = itPdf->score;
			    label = itPdf->label;
			}
		    }
		    if (label == Fsa::Epsilon) {
			epsPosterior *= maxPosterior;
			hasPendingEps = true;
		    } else {
			if (hasPendingEps) {
			    sid = addStateAndArc(sid, s, b, epsPosterior, Fsa::Epsilon);
			    epsPosterior = 1.0;
			    hasPendingEps = false;
			}
			sid = addStateAndArc(sid, s, b, maxPosterior, label);
		    }
		}
		if (hasPendingEps)
		    sid = addStateAndArc(sid, s, b, epsPosterior, Fsa::Epsilon);
		s->setStateFinal(s->newState(sid), confSemiring()->clone(confSemiring()->one()));
		b->set(sid, Boundary(Time(sid)));
		s->setInitialStateId(0);
		return ConstLatticeRef(s);
	    }
	} convertPosteriorCn;
    } // namespace

    ConstLatticeRef posteriorCn2lattice(ConstPosteriorCnRef cn) {
	return convertPosteriorCn.convert(cn);
    }

    ConstLatticeRef decodePosteriorCn(ConstPosteriorCnRef cn) {
	return convertPosteriorCn.decode(cn);
    }
    // -------------------------------------------------------------------------


    // -------------------------------------------------------------------------
    void normalizePosteriorCn(PosteriorCn &cn) {
	for (u32 i = 0; i < cn.size(); ++i) {
	    PosteriorCn::Slot &pdf = cn[i];
	    Probability sum = 0.0;
	    for (PosteriorCn::Slot::const_iterator itPdf = pdf.begin(); itPdf != pdf.end(); ++itPdf)
		sum += itPdf->score;
	    verify(sum != 0.0);
	    if (sum < 0.0)
		Core::Application::us()->warning("In normalizePosteriorCn: Negative sum %f", sum);
	    for (PosteriorCn::Slot::iterator itPdf = pdf.begin(); itPdf != pdf.end(); ++itPdf)
		itPdf->score /= sum;
	}
    }
    // -------------------------------------------------------------------------


    // -------------------------------------------------------------------------
    bool isNormalizedPosteriorCn(ConstPosteriorCnRef cn) {
	for (u32 i = 0; i < cn->size(); ++i) {
	    const PosteriorCn::Slot &pdf = (*cn)[i];
	    Probability sum = 0.0;
	    for (PosteriorCn::Slot::const_iterator itPdf = pdf.begin(); itPdf != pdf.end(); ++itPdf) {
		if (itPdf->score < 0.0) return false;
		sum += itPdf->score;
	    }
	    if (Core::abs(sum - 1.0) >= 0.01)
		return false;
	}
	return true;
    }
    // -------------------------------------------------------------------------


    // -------------------------------------------------------------------------
    const Core::ParameterFloat FramePosteriorCnFeatures::paramAlpha(
	"alpha",
	"alpha",
	0.0);
    Score FramePosteriorCnFeatures::smooth(Score error, Score duration, Score alpha) {
	return error / (1.0 + alpha * (duration - 1.0));
    }

    class FramePosteriorCnFeatures::Internal {
    private:
	ConstPosteriorCnRef cn_;
	Fsa::LabelId label_;
	ProbabilityList framePosteriors_;
	ProbabilityList::iterator fBegin_, fEnd_;
    public:
	Internal(ConstPosteriorCnRef cn) : cn_(cn), label_(Fsa::InvalidLabelId) {}

	void update(Fsa::LabelId label, Time begin, Time duration) {
	    label_ = label;
	    if (framePosteriors_.size() < duration) {
		framePosteriors_.resize(duration);
		fBegin_ = framePosteriors_.begin();
	    }
	    fEnd_ = framePosteriors_.begin() + duration;
	    cn_->scores(fBegin_, fEnd_, begin, label);
	}

	Probability confidence() const {
	    switch (u32(fEnd_ - fBegin_)) {
	    case 0:
		return 1.0;
	    case 1:
		return *fBegin_;
	    default:
		Probability maxP = 0.0;
		for (ProbabilityList::iterator itP = fBegin_; itP != fEnd_; ++itP)
		    if (*itP > maxP) maxP = *itP;
		return maxP;
	    }
	}

	Score error(Score alpha) const {
	    switch (u32(fEnd_ - fBegin_)) {
	    case 0:
		return 0.0;
	    case 1:
		return 1.0 - *fBegin_;
	    default:
		Score sum = 0.0, duration = Score(fEnd_ - fBegin_);
		for (ProbabilityList::iterator itP = fBegin_; itP != fEnd_; ++itP)
		    sum += *itP;
		verify((0.0 <= sum) && (sum <= duration));
		return FramePosteriorCnFeatures::smooth(duration - sum, duration, alpha);
	    }
	}

	Score norm(Score alpha) const {
	    Score duration = Score(fEnd_ - fBegin_);
	    return FramePosteriorCnFeatures::smooth(duration, duration, alpha);
	}
    };

    FramePosteriorCnFeatures::FramePosteriorCnFeatures(Internal *internal) :
	internal_(internal) {}

    FramePosteriorCnFeatures::~FramePosteriorCnFeatures() {
	delete internal_;
    }

    void FramePosteriorCnFeatures::update(Fsa::LabelId label, Time begin, Time duration) {
	internal_->update(label, begin, duration);
    }

    Probability FramePosteriorCnFeatures::confidence() const {
	return internal_->confidence();
    }

    Score FramePosteriorCnFeatures::error(Score alpha) const {
	return internal_->error(alpha);
    }

    Score FramePosteriorCnFeatures::norm(Score alpha) const {
	return internal_->norm(alpha);
    }

    FramePosteriorCnFeaturesRef FramePosteriorCnFeatures::create(ConstPosteriorCnRef cn) {
	verify(cn);
	Internal *internal = new Internal(cn);
	return FramePosteriorCnFeaturesRef(new FramePosteriorCnFeatures(internal));
    }
    // -------------------------------------------------------------------------

    // -------------------------------------------------------------------------
    namespace {
	const Core::ParameterFloat paramGamma(
	    "gamma",
	    "gamma correction",
	    1.0);
    } // namespace
    // -------------------------------------------------------------------------

    // -------------------------------------------------------------------------
    class ExtendByFramePosteriorConfidenceLattice : public RescoreLattice {
    private:
	ScoreId id_;
	FramePosteriorCnFeaturesRef features_;

    public:
	ExtendByFramePosteriorConfidenceLattice(ConstLatticeRef l, ConstPosteriorCnRef cn, ScoreId id, RescoreMode rescoreMode) :
	    RescoreLattice(l, rescoreMode), id_(id) {
	    verify(cn);
	    features_ = FramePosteriorCnFeatures::create(cn);
	}
	virtual ~ExtendByFramePosteriorConfidenceLattice() {}

	virtual void rescore(State *sp) const {
	    const Boundaries &b = *fsa_->getBoundaries();
	    Time startTime = b.get(sp->id()).time();
	    for (State::iterator a = sp->begin(); a != sp->end(); ++a) {
		Time duration = b.get(a->target()).time() - startTime;
		features_->update(a->input(), startTime, duration);
		a->weight_->add(id_, features_->confidence());
	    }
	}
	virtual std::string describe() const {
	    return Core::form("extendByFCnConfidence(%s,dim=%d)",
			      fsa_->describe().c_str(), id_);
	}
    };

    ConstLatticeRef extendByFCnConfidence(ConstLatticeRef l, ConstPosteriorCnRef cn, ScoreId id, RescoreMode rescoreMode) {
	return ConstLatticeRef(new ExtendByFramePosteriorConfidenceLattice(l, cn, id, rescoreMode));
    }

    class ExtendByPosteriorCnConfidenceNode : public RescoreSingleDimensionNode {
	typedef RescoreSingleDimensionNode Precursor;
    private:
	f64 gamma_;
	FwdBwdBuilderRef fbBuilder_;
    protected:
	ConstLatticeRef rescore(ConstLatticeRef l, ScoreId id) {
	    ConstPosteriorCnRef cn;
	    if (connected(1)) {
		cn = requestPosteriorCn(1);
	    } else {
		std::pair<ConstLatticeRef, ConstFwdBwdRef> fbResult = fbBuilder_->build(l);
		cn = buildFramePosteriorCn(fbResult.first, fbResult.second);
	    }
	    gammaCorrection(cn, gamma_, true);
	    return extendByFCnConfidence(l, cn, id, rescoreMode);
	}

	void init(const std::vector<std::string> &arguments) {
	    Core::Component::Message msg(log());
	    if (connected(1)) {
		msg << "Read fCN from port 1.\n";
	    } else {
		msg << "Calculate fCN from lattice:\n";
		fbBuilder_ = FwdBwdBuilder::create(select("fb"));
	    gamma_ = paramGamma(config);
	    if (gamma_ != 1.0)
		msg << "Correct fCN with gamma=" << gamma_ << "\n";
	    }
	}
    public:
	ExtendByPosteriorCnConfidenceNode(const std::string &name, const Core::Configuration &config) :
	    Precursor(name, config) {}
	~ExtendByPosteriorCnConfidenceNode() {}
    };
    NodeRef createExtendByPosteriorCnConfidenceNode(const std::string &name, const Core::Configuration &config) {
	return NodeRef(new ExtendByPosteriorCnConfidenceNode(name, config));
    }
    // -------------------------------------------------------------------------


    // -------------------------------------------------------------------------
    class FramePosteriorCnFeatureLattice : public RescoreLattice {
	typedef RescoreLattice Precursor;
    public:
	struct FeatureIds {
	    ScoreId confidenceId;
	    ScoreId errorId;
	    Probability alpha;
	    FeatureIds() :
		confidenceId(Semiring::InvalidId),
		errorId(Semiring::InvalidId),
		alpha(0.05) {}
	};

    private:
	FeatureIds featureIds_;
	FramePosteriorCnFeaturesRef features_;

    public:
	FramePosteriorCnFeatureLattice(ConstLatticeRef l, ConstPosteriorCnRef cn, const FeatureIds &featureIds, RescoreMode rescoreMode) :
	    Precursor(l, rescoreMode), featureIds_(featureIds) {
	    verify(cn);
	    features_ = FramePosteriorCnFeatures::create(cn);
	}
	virtual ~FramePosteriorCnFeatureLattice() {}

	virtual void rescore(State *sp) const {
	    Time startTime = fsa_->boundary(sp->id()).time();
	    for (State::iterator a = sp->begin(); a != sp->end(); ++a) {
		Time duration = fsa_->boundary(a->target()).time() - startTime;
		features_->update(a->input(), startTime, duration);
		if (featureIds_.confidenceId != Semiring::InvalidId)
		    a->weight_->set(featureIds_.confidenceId, features_->confidence());
		if (featureIds_.errorId != Semiring::InvalidId)
		    a->weight_->set(featureIds_.errorId, features_->error(featureIds_.alpha));
	    }
	}

	virtual std::string describe() const {
	    return Core::form("addFramePosteriorCnFeatures(%s)", fsa_->describe().c_str());
	}
    };

    class FramePosteriorCnFeatureNode : public RescoreNode {
	typedef RescoreNode Precursor;
    public:
	static const Core::ParameterString paramConfidenceKey;
	static const Core::ParameterString paramErrorKey;

    private:
	f64 gamma_;
	std::string confidenceKey_;
	std::string errorKey_;
	FwdBwdBuilderRef fbBuilder_;

	mutable ConstSemiringRef lastSemiring_;
	mutable FramePosteriorCnFeatureLattice::FeatureIds lastIds_;

    protected:
	ConstLatticeRef rescore(ConstLatticeRef l) {
	    if (!lastSemiring_ || (lastSemiring_.get() != l->semiring().get())) {
		lastSemiring_ = l->semiring();
		lastIds_.confidenceId = lastSemiring_->id(confidenceKey_);
		lastIds_.errorId = lastSemiring_->id(errorKey_);
	    }
	    ConstPosteriorCnRef cn;
	    if (connected(1)) {
		cn = requestPosteriorCn(1);
	    } else {
		ConstLatticeRef lRef = connected(2) ? requestLattice(2) : l;
		std::pair<ConstLatticeRef, ConstFwdBwdRef> fbResult = fbBuilder_->build(lRef);
		cn = buildFramePosteriorCn(fbResult.first, fbResult.second);
	    }
	    gammaCorrection(cn, gamma_, true);
	    return ConstLatticeRef(new FramePosteriorCnFeatureLattice(l, cn, lastIds_, rescoreMode));
	}

    public:
	FramePosteriorCnFeatureNode(const std::string &name, const Core::Configuration &config) :
	    Precursor(name, config) {}
	~FramePosteriorCnFeatureNode() {}

	void init(const std::vector<std::string> &arguments) {
	    if (!connected(0))
		criticalError("FilterNode: Need a data source at port 0.");
	    Core::Component::Message msg(log());
	    if (connected(1)) {
		msg << "Read fCN from port 1.\n";
	    } else {
		if (connected(2))
		    msg << "Calculate fCN from lattice at port2:\n";
		else
		    msg << "Calculate fCN from lattice:\n";
		fbBuilder_ = FwdBwdBuilder::create(select("fb"));
	    }
	    gamma_ = paramGamma(config);
	    if (gamma_ != 1.0)
		msg << "Correct fCN with gamma=" << gamma_ << "\n";
	    msg << "Store the following fCN features:\n";
	    confidenceKey_ = paramConfidenceKey(config);
	    if (!confidenceKey_.empty())
		msg << "  - confidence to dimension \"" << confidenceKey_ << "\"\n";
	    errorKey_ = paramErrorKey(config);
	    if (!errorKey_.empty())
		msg << "  - expected error to dimension \"" << errorKey_ << "\"\n";
	    lastIds_.alpha = FramePosteriorCnFeatures::paramAlpha(select("error"), 0.05);
	    msg << "error smoothing alpha is " << lastIds_.alpha;
	}
    };
    const Core::ParameterString FramePosteriorCnFeatureNode::paramConfidenceKey(
	"confidence-key",
	"fCN confidence",
	"");
    const Core::ParameterString FramePosteriorCnFeatureNode::paramErrorKey(
	"error-key",
	"expected error derived from the fCN confidence; smoothed by alpha",
	"");

    NodeRef createFramePosteriorCnFeatureNode(const std::string &name, const Core::Configuration &config) {
	return NodeRef(new FramePosteriorCnFeatureNode(name, config));
    }
    // -------------------------------------------------------------------------

} // namespace Flf

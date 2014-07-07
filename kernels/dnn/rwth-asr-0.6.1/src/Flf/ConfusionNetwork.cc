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
#include "ConfusionNetwork.hh"
#include "Copy.hh"
#include "CenterFrameConfusionNetworkBuilder.hh"
#include "PivotArcConfusionNetworkBuilder.hh"
#include "StateClusterConfusionNetworkBuilder.hh"


namespace Flf {

    // -------------------------------------------------------------------------
	namespace {
		const Core::Choice choiceCnAlgorithm(
			"arc-clustering", ConfusionNetworkFactory::CnAlgorithmPivotArcClustering,
			"state-clustering",    ConfusionNetworkFactory::CnAlgorithmStateClustering,
			"center-frame",    ConfusionNetworkFactory::CnAlgorithmCenterFrame,
			// deprecated
			"pivot-clustering", ConfusionNetworkFactory::CnAlgorithmPivotArcClustering,
			"time-topology",    ConfusionNetworkFactory::CnAlgorithmStateClustering,
			"frame-example",    ConfusionNetworkFactory::CnAlgorithmCenterFrame,
			Core::Choice::endMark());

		const Core::ParameterString paramAlgorithm(
			"algorithm",
			"algorithm",
			"arc-clustering");
	} // namespace

	ConfusionNetworkFactory::~ConfusionNetworkFactory() {}

	void ConfusionNetworkFactory::dump(std::ostream &os) const {
		os << "n/a" << std::endl;
	}

	ConfusionNetworkFactoryRef ConfusionNetworkFactory::create(const Core::Configuration &config) {
		std::string cnAlgorithmName = paramAlgorithm(config);
		Core::Choice::Value cnAlgorithmChoice = choiceCnAlgorithm[cnAlgorithmName];
		if (cnAlgorithmChoice == Core::Choice::IllegalValue)
			Core::Application::us()->criticalError(
				"Unknwon CN algorithm \"%s\".",
				cnAlgorithmName.c_str());
		ConfusionNetworkFactory *cnFactory = 0;
		const Core::Configuration cnConfig(config, cnAlgorithmName);
		switch (CnAlgorithm(cnAlgorithmChoice)) {
		case CnAlgorithmStateClustering:
			Core::Application::us()->criticalError(
				"Algorithm not supported yet.");
			break;
		case CnAlgorithmPivotArcClustering:
			// parameterized version still missing
			// cnFactory = PivotArcCnFactory::create(cnConfig);
			cnFactory = new PivotArcCnFactory;
			break;
		case CnAlgorithmCenterFrame:
			cnFactory = new CenterFrameCnFactory(cnConfig);
			break;
		default:
			defect();
		}
		return ConfusionNetworkFactoryRef(cnFactory);
	}

	ConfusionNetworkFactoryRef ConfusionNetworkFactory::create(CnAlgorithmType cnAlgorithm) {
		ConfusionNetworkFactory *cnFactory = 0;
		switch (cnAlgorithm) {
		case CnAlgorithmStateClustering:
			Core::Application::us()->criticalError(
				"Algorithm not supported yet.");
			break;
		case CnAlgorithmPivotArcClustering:
			cnFactory = new PivotArcCnFactory;
			break;
		case CnAlgorithmCenterFrame:
			cnFactory = new CenterFrameCnFactory;
			break;
		default:
			defect();
		}
		return ConfusionNetworkFactoryRef(cnFactory);
	}
    // -------------------------------------------------------------------------


    // -------------------------------------------------------------------------
    ConstConfusionNetworkRef sausageLattice2cn(ConstLatticeRef l, bool keepEpsArcs) {
	if (!l || (l->initialStateId() == Fsa::InvalidStateId))
	    return ConstConfusionNetworkRef();
	ConstBoundariesRef b = l->getBoundaries();
	ConfusionNetwork *cn = new ConfusionNetwork;
	cn->alphabet = l->getInputAlphabet();
	cn->semiring = l->semiring();
	u32 stateClusterId = 0;
	Time begin = b->time(l->initialStateId());
	for (ConstStateRef sr = l->getState(l->initialStateId()), nextSr;  sr->hasArcs(); sr = nextSr) {
	    nextSr = l->getState(sr->begin()->target());
	    Time duration = b->time(nextSr->id()) - begin;
	    bool hasSlot = false;
	    for (State::const_iterator a = sr->begin(), end_a = sr->end(); a != end_a; ++a) {
		if (a->target() != nextSr->id())
		    Core::Application::us()->criticalError(
			"\"%s\" has not a \"sausage\" topology.", l->describe().c_str());
		if (keepEpsArcs || (a->input() != Fsa::Epsilon)) {
		    if (!hasSlot) {
			cn->push_back(ConfusionNetwork::Slot());
			++stateClusterId;
			hasSlot = true;
		    }
		    cn->back().push_back(
			ConfusionNetwork::Arc(
			    a->input(), a->weight(),
			    begin, duration,
			    stateClusterId - 1, stateClusterId));
		}
	    }
	    begin += duration;
	}
	return ConstConfusionNetworkRef(cn);
    }
    // -------------------------------------------------------------------------


    // -------------------------------------------------------------------------
    namespace {
	const class ConfusionNetworkConverter {
	private:
	    Score calcConfidence(ScoreList::const_iterator itP, ScoreList::const_iterator endP) const {
		switch (u32(endP - itP)) {
		case 0:
		    return 1.0;
		case 1:
		    return *itP;
		default:
		    Score maxP = 0.0;
		    for (; itP != endP; ++itP)
			if (*itP > maxP) maxP = *itP;
		    return maxP;
		}
	    }
	public:
	    ConfusionNetworkConverter() {}

	    ConstLatticeRef convert(ConstConfusionNetworkRef cn, bool approximateBoundaryTimes = false) const {
		if (!cn)
		    return ConstLatticeRef();
		StaticLattice *s = new StaticLattice;
		s->setDescription("cn");
		s->setType(Fsa::TypeAcceptor);
		s->setProperties(Fsa::PropertyAcyclic, Fsa::PropertyAll);
		s->setInputAlphabet(cn->alphabet);
		s->setSemiring(cn->semiring);
		State *sp = s->newState();
		s->setInitialStateId(sp->id());
		Flf::StaticBoundaries* bounds = approximateBoundaryTimes ? new StaticBoundaries : 0;
		for (ConfusionNetwork::const_iterator itSlot = cn->begin(), endSlot = cn->end();
		     itSlot != endSlot; ++itSlot) {
		    State *nextSp = s->newState();
		    Score bestArcScore = Core::Type<Score>::max;
		    for (ConfusionNetwork::Slot::const_iterator itArc = itSlot->begin(), endArc = itSlot->end();
			 itArc != endArc; ++itArc)
			{
			sp->newArc(nextSp->id(), itArc->scores, itArc->label);
			if(bounds)
			{
			    Score score = itArc->scores->project(cn->semiring->scales());
			    if(score && score < bestArcScore)
			    {
				bounds->set(sp->id(), Boundary(itArc->begin));
				bounds->set(nextSp->id(), Boundary(itArc->begin+itArc->duration));
				bestArcScore = score;
			    }
			}
			}
		    sp = nextSp;
		}
		ScoresRef scores = cn->semiring->one();
		if (cn->isNormalized()) {
		    scores = cn->semiring->clone(scores);
		    scores->set(cn->normalizedProperties->posteriorId, 1.0);
		}
		sp->setFinal(scores);
		s->setBoundaries(ConstBoundariesRef(bounds));
		return ConstLatticeRef(s);
	    }

	private:
	    class ArcPtrList : public std::vector<const ConfusionNetwork::Arc*> {
	    public:
		struct SortByLabel {
		    bool operator() (const ConfusionNetwork::Arc *arc1, const ConfusionNetwork::Arc *arc2) const
			{ return arc1->label < arc2->label; }
		};
	    };
	public:
	    ConstConfusionNetworkRef normalize(ConstConfusionNetworkRef cn, ScoreId posteriorId) const {
		if (!cn)
		    return ConstConfusionNetworkRef();
		if (posteriorId == Semiring::InvalidId)
		    Core::Application::us()->criticalError(
			"No dimension storing a posterior probability distribution is given; "
			"in order to normalize a CN a prob. dist. over all arcs in the slot is required, where lacking prob. mass is assigned to eps.");
		ConfusionNetwork *normCn = new ConfusionNetwork(cn->size());
		normCn->alphabet = cn->alphabet;
		normCn->semiring = cn->semiring;
		normCn->normalizedProperties = ConfusionNetwork::ConstNormalizedPropertiesRef(
		    new ConfusionNetwork::NormalizedProperties(posteriorId));
		ConstSemiringRef semiring = normCn->semiring;
		ArcPtrList arcPtrs;
		for (u32 i = 0; i < normCn->size(); ++i) {
		    // Squeeze words -> all arcs with same label are merged into one arc
		    const ConfusionNetwork::Slot &slot = (*cn)[i];
		    ConfusionNetwork::Slot &normSlot = (*normCn)[i];
		    Score sum = 0.0;
		    if (!slot.empty()) {
			if (arcPtrs.size() < slot.size())
			    arcPtrs.resize(slot.size(), 0);
			ArcPtrList::iterator itArcPtr = arcPtrs.begin();
			for (ConfusionNetwork::Slot::const_iterator itArc = slot.begin(), endArc = slot.end(); itArc != endArc; ++itArc, ++itArcPtr)
			    *itArcPtr = &*itArc;
			std::sort(arcPtrs.begin(), arcPtrs.begin() + slot.size(), ArcPtrList::SortByLabel());
			Score sumPosterior, maxPosterior;
			Score sumBegin, sumDuration;
			Fsa::LabelId lastLabel;
			{
			    const ConfusionNetwork::Arc &arc = *arcPtrs.front();
			    Score posterior = arc.scores->get(posteriorId);
			    normSlot.push_back(ConfusionNetwork::Arc(arc.label, arc.scores));
			    lastLabel = arc.label;
			    sumBegin = posterior * Score(arc.begin);
			    sumDuration = posterior * Score(arc.duration);
			    sumPosterior = maxPosterior = posterior;
			}
			for (ArcPtrList::const_iterator itArcPtr = arcPtrs.begin() + 1, endArcPtr = arcPtrs.begin() + slot.size();
			     itArcPtr != endArcPtr; ++itArcPtr) {
			    const ConfusionNetwork::Arc &arc = **itArcPtr;
			    Score posterior = arc.scores->get(posteriorId);
			    if (arc.label != lastLabel) {
				{
				    ConfusionNetwork::Arc &normArc = normSlot.back();
				    normArc.scores = semiring->clone(normArc.scores);
				    normArc.scores->set(posteriorId, sumPosterior);
				    normArc.begin = Time(Core::round(sumBegin / sumPosterior));
				    normArc.duration = Time(Core::round(sumDuration / sumPosterior));
				}
				sum += sumPosterior;
				normSlot.push_back(ConfusionNetwork::Arc(arc.label, arc.scores));
				lastLabel = arc.label;
				sumBegin = posterior * Score(arc.begin);
				sumDuration = posterior * Score(arc.duration);
				sumPosterior = maxPosterior = posterior;
			    } else {
				if (posterior > maxPosterior) {
				    ConfusionNetwork::Arc &normArc = normSlot.back();
				    normArc.scores   = arc.scores;
				    maxPosterior = posterior;
				}
				sumBegin += posterior * Score(arc.begin);
				sumDuration += posterior * Score(arc.duration);
				sumPosterior += posterior;
			    }
			}
			{
			    ConfusionNetwork::Arc &normArc = normSlot.back();
			    normArc.scores = semiring->clone(normArc.scores);
			    normArc.scores->set(posteriorId, sumPosterior);
			    normArc.begin = Time(Core::round(sumBegin / sumPosterior));
			    normArc.duration = Time(Core::round(sumDuration / sumPosterior));
			    sum += sumPosterior;
			}
			// Add missing proability mass to epsilon arc
			if (sum != 1.0) {
			    bool normalize = false;
			    Score dEpsPosterior = 1.0 - sum;
			    if (dEpsPosterior < 0.0) {
				// if (dEpsPosterior < -0.001)
				if (dEpsPosterior < -0.01)
				    Core::Application::us()->warning(
					"Normalize CN: Expected 1.0, got %f", (1.0 - dEpsPosterior));
				normalize = true;
			    } else {
				if (normSlot.front().label == Fsa::Epsilon) {
				    normSlot.front().scores->add(posteriorId, dEpsPosterior);
				} else if (dEpsPosterior >= 0.001) {
				    ScoresRef scores = semiring->clone(semiring->one());
				    scores->set(posteriorId, dEpsPosterior);
				    normSlot.insert(normSlot.begin(), ConfusionNetwork::Arc(
							Fsa::Epsilon,
							scores,
							(normSlot.front().begin + normSlot.front().duration) / 2, 0));
				} else
				    normalize = true;
			    }
			    if (normalize) {
				Score norm = 1.0 / sum;
				for (ConfusionNetwork::Slot::iterator itArc = normSlot.begin(), endArc = normSlot.end(); itArc != endArc; ++itArc)
				    itArc->scores->multiply(posteriorId, norm);
			    }
			}
		    } else {
			ScoresRef scores = semiring->clone(semiring->one());
			scores->set(posteriorId, 1.0);
			normSlot.push_back(ConfusionNetwork::Arc(
					       Fsa::Epsilon,
					       scores,
					       Speech::InvalidTimeframeIndex, 0));
		    }
		}
		if (cn->hasMap()) {
		    ConfusionNetwork::MapProperties *mapProperties = new ConfusionNetwork::MapProperties(*cn->mapProperties);
		    mapProperties->reduce(); // normalized CN contains only "lattice-state+arc -> slot" information
		    normCn->mapProperties = ConfusionNetwork::ConstMapPropertiesRef(mapProperties);
		}
		return ConstConfusionNetworkRef(normCn);
	    }

	    /*
	    class Decoder {
	    public:
		virtual Fsa::LabelId label(u32 iSlot) const = 0;
	    };

	    class MaximumPosteriorProbabilityDecoder : public Decocder {
	    public:
		MaximumPosteriorProbabilityDecoder(ConstConfusionNetworkRef cn, ScoreId posteriorId, const ScoreIdList systemPosteriorIds) :
		    cn_(cn), posteriorId_(posteriorId) {}
		virtual Fsa::LabelId label(u32 iSlot) const {
		}
	    };

	    class InversePosteriorProbabilityEntropyDecoder : public Decocder {
	    private:

	    public:
		InversePosteriorProbabilityEntropyDecoder(ConstConfusionNetworkRef cn, ScoreId posteriorId, const ScoreIdList systemPosteriorIds) :
		    cn_(cn), posteriorId_(posteriorId), systemPosteriorIds_(systemPosteriorIds),
		    weights_(systemPosteriorIds.size(), ScoreList(0.0, cn->size())) {

		}

		virtual Fsa::LabelId label(u32 iSlot) const {
		}
	    };
	    */

	    ConstLatticeRef decode(ConstConfusionNetworkRef cn, ScoreId posteriorId) const {
		if (!cn)
		    return ConstLatticeRef();
		if (posteriorId == Semiring::InvalidId) {
		    verify(cn->isNormalized());
		    posteriorId = cn->normalizedProperties->posteriorId;
		} else if (!cn->isNormalized()) {
		    cn = normalize(cn, posteriorId);
		}
		verify((posteriorId != Semiring::InvalidId)
		       && cn->isNormalized() && (cn->normalizedProperties->posteriorId == posteriorId));
		ConstSemiringRef semiring = cn->semiring;
		StaticBoundaries *b = new StaticBoundaries;
		StaticLattice *s = new StaticLattice;
		s->setDescription("decode-cn");
		s->setType(Fsa::TypeAcceptor);
		s->setProperties(Fsa::PropertyAcyclic | Fsa::PropertyLinear, Fsa::PropertyAll);
		s->setInputAlphabet(cn->alphabet);
		s->setSemiring(cn->semiring);
		s->setBoundaries(ConstBoundariesRef(b));
		s->setInitialStateId(0);
		Flf::State *sp = new Flf::State(0); s->setState(sp);
		Time minEndTime = 0, preferredEndTime = 0; // Attention: preferredEndTime >= minEndTime
		ScoreList pendingEpsPosteriors;
		for (u32 i = 0; i < cn->size(); ++i) {
		    const ConfusionNetwork::Slot &pdf = (*cn)[i];
		    Score maxPosterior = 0.0;
		    ConfusionNetwork::Slot::const_iterator itBestPdf = pdf.end();
		    for (ConfusionNetwork::Slot::const_iterator itPdf = pdf.begin(); itPdf != pdf.end(); ++itPdf) {
			Score posterior = itPdf->scores->get(posteriorId);
			if (posterior > maxPosterior) {
			    maxPosterior = posterior;
			    itBestPdf = itPdf;
			}
		    }
		    verify(itBestPdf != pdf.end());
		    if (itBestPdf->label != Fsa::Epsilon) {
			if (preferredEndTime < itBestPdf->begin) {
			    b->set(sp->id(), Boundary(preferredEndTime));
			    ScoresRef scores = semiring->clone(semiring->one());
			    scores->set(posteriorId, calcConfidence(pendingEpsPosteriors.begin(), pendingEpsPosteriors.end()));
			    pendingEpsPosteriors.clear();
			    sp->newArc(sp->id() + 1, scores, Fsa::Epsilon, Fsa::Epsilon);
			    sp = new Flf::State(sp->id() + 1); s->setState(sp);
			    preferredEndTime = itBestPdf->begin;
			} else
			    preferredEndTime = std::max(minEndTime ,(preferredEndTime + itBestPdf->begin) / 2);
			b->set(sp->id(), Boundary(preferredEndTime));
			sp->newArc(sp->id() + 1, itBestPdf->scores, itBestPdf->label, itBestPdf->label);
			sp = new Flf::State(sp->id() + 1); s->setState(sp);
			minEndTime = preferredEndTime + 1;
			preferredEndTime = std::max(minEndTime, itBestPdf->begin + itBestPdf->duration);
		    } else
			pendingEpsPosteriors.push_back(maxPosterior);
		}
		b->set(sp->id(), Boundary(preferredEndTime));
		ScoresRef scores = semiring->one();
		if (!pendingEpsPosteriors.empty()) {
		    scores = semiring->clone(scores);
		    scores->set(posteriorId, calcConfidence(pendingEpsPosteriors.begin(), pendingEpsPosteriors.end()));
		    pendingEpsPosteriors.clear();
		}
		sp->setFinal(scores);
		return ConstLatticeRef(s);
	    }
	} convertCn;
    } // namespace

    ConstLatticeRef cn2lattice(ConstConfusionNetworkRef cn, bool approximateBoundaryTimes) {
	return convertCn.convert(cn, approximateBoundaryTimes);
    }

    ConstConfusionNetworkRef normalizeCn(ConstConfusionNetworkRef cn, ScoreId posteriorId) {
	return convertCn.normalize(cn, posteriorId);
    }

    bool isNormalizeCn(ConstConfusionNetworkRef cn, ScoreId posteriorId) {
	return cn->isNormalized() && (cn->normalizedProperties->posteriorId == posteriorId);
    }

    ConstLatticeRef decodeCn(ConstConfusionNetworkRef cn, ScoreId posteriorId) {
	return convertCn.decode(cn, posteriorId);
    }
    // -------------------------------------------------------------------------


    // -------------------------------------------------------------------------
    class CnDecoderNode : public Node {
    public:
	static const Core::ParameterString paramPosteriorKey;
    private:
	Key posteriorKey_;
	ScoreId posteriorId_;

	ConstSemiringRef lastSemiring_;

	bool loadCn_;
	ConstConfusionNetworkRef cn_;
	ConstLatticeRef best_;
	ConstLatticeRef sausage_;

    private:
	ConstConfusionNetworkRef getCn() {
	    if (loadCn_) {
		cn_ = connected(0) ? requestCn(0) : sausageLattice2cn(requestLattice(1));
		loadCn_ = false;
	    }
	    return cn_;
	}

	ConstLatticeRef best() {
	    if (!best_) {
		ConstConfusionNetworkRef cn = getCn();
		if (cn) {
		    if (posteriorKey_.empty()) {
			if (!cn->isNormalized())
			    criticalError("Require a normalized CN.");
		    } else {
			if (!lastSemiring_ || (lastSemiring_.get() != cn->semiring.get())) {
			    lastSemiring_ = cn->semiring;
			    posteriorId_ = lastSemiring_->id(posteriorKey_);
			    if (posteriorId_ == Semiring::InvalidId)
				criticalError("Semiring \"%s\" has no dimension labeled \"%s\".",
					      lastSemiring_->name().c_str(), posteriorKey_.c_str());
			}
		    }
		    best_ = decodeCn(cn, posteriorId_);
		}
	    }
	    return best_;
	}

	ConstLatticeRef sausage() {
	    if (!sausage_)
		sausage_ = cn2lattice(getCn());
	    return sausage_;
	}

    public:
	CnDecoderNode(const std::string &name, const Core::Configuration &config) :
	    Node(name, config) {}
	virtual ~CnDecoderNode() {}

	virtual void init(const std::vector<std::string> &arguments) {
	    if (!connected(0) && !connected(1))
		criticalError("Require either CN at port 0 or sausage lattice at port 1.");
	    Core::Component::Message msg = log();
	    posteriorKey_ = paramPosteriorKey(config);
	    if (!posteriorKey_.empty())
		msg << "Posterior key is \"" << posteriorKey_ << "\".\n";
	    posteriorId_ = Semiring::InvalidId;
	    loadCn_ = true;
	}

	virtual void finalize() {}

	virtual ConstLatticeRef sendLattice(Port to) {
	    switch(to) {
	    case 0:
		return best();
	    case 1:
		return sausage();
	    default:
		defect();
		return ConstLatticeRef();
	    }
	}

	virtual void sync() {
	    loadCn_ = true;
	    cn_.reset();
	    best_.reset();
	    sausage_.reset();
	}
    };
    const Core::ParameterString CnDecoderNode::paramPosteriorKey(
	"posterior-key",
	"posterior key",
	"");
    NodeRef createCnDecoderNode(const std::string &name, const Core::Configuration &config) {
	return NodeRef(new CnDecoderNode(name, config));
    }
    // -------------------------------------------------------------------------


    // -------------------------------------------------------------------------
    CnFeatureConfiguration::CnFeatureConfiguration() :
	cn(),
	cnPosteriorId(Semiring::InvalidId),
	compose(false),
	duplicateOutput(false),
	confidenceId(Semiring::InvalidId),
	scoreId(Semiring::InvalidId),
	errId(Semiring::InvalidId),
	oracleOutput(false),
	slotEntropyId(Semiring::InvalidId),
	slotId(Semiring::InvalidId),
	nonEpsSlotId(Semiring::InvalidId),
	epsSlotThreshold(1.0) {}

    class CnFeatures;
    typedef Core::Ref<CnFeatures> CnFeaturesRef;
    class CnFeatures : public Core::ReferenceCounted {
    public:
	static const Score CorCost;
	static const Score SubCost;
	static const Score InsCost;
	static const Score DelCost;
    private:
	CnFeatureConfiguration cnConfig_;
	bool needsModification_;
	bool needsOracle_;
	bool needsPosterior_;
	ConstConfusionNetworkRef normalizedCn_;
	std::vector<Fsa::StateId> slotIdToNonEpsilonSlotIdMap_;

    public:
	CnFeatures(CnFeatureConfiguration cnConfig) : cnConfig_(cnConfig) {
	    if (!cnConfig_.cn || !cnConfig_.cn->hasMap())
		Core::Application::us()->criticalError("Confusion Network with lattice/CN map required.");
	    if (
		(cnConfig_.confidenceId != Semiring::InvalidId) ||
		(cnConfig_.scoreId != Semiring::InvalidId) ||
		(cnConfig_.errId != Semiring::InvalidId) ||
		(cnConfig_.oracleOutput) ||
		(cnConfig_.slotEntropyId != Semiring::InvalidId) ||
		(cnConfig_.slotId != Semiring::InvalidId) ||
		(cnConfig_.nonEpsSlotId != Semiring::InvalidId)) {
		needsModification_ = true;
		// oracle CN required
		if ((cnConfig_.errId != Semiring::InvalidId) || cnConfig_.oracleOutput) {
		    needsOracle_ = true;
		    if (!cnConfig_.cn->isOracleAlignment())
			Core::Application::us()->criticalError("Confusion Network with oracle alignment required.");
		}
		// normalized CN required
		if ((cnConfig_.confidenceId != Semiring::InvalidId)
		    || (cnConfig_.scoreId != Semiring::InvalidId)
		    || (cnConfig_.slotEntropyId != Semiring::InvalidId)
		    || (cnConfig_.nonEpsSlotId !=  Semiring::InvalidId)) {
		    if (!cnConfig_.cn->isNormalized()) {
			if (cnConfig_.cnPosteriorId == Semiring::InvalidId)
			    Core::Application::us()->criticalError("Posterior dimension for confusion network normalization required.");
			normalizedCn_ = normalizeCn(cnConfig_.cn, cnConfig_.cnPosteriorId);
		    } else {
			cnConfig_.cnPosteriorId = cnConfig_.cn->normalizedProperties->posteriorId;
			normalizedCn_ = cnConfig_.cn;
		    }
		    needsPosterior_ =
			(cnConfig_.confidenceId != Semiring::InvalidId) ||
			(cnConfig_.scoreId != Semiring::InvalidId);
		}
		// mapping to eps-slot free CN required
		if (cnConfig_.nonEpsSlotId  != Semiring::InvalidId) {
		    slotIdToNonEpsilonSlotIdMap_.resize(cnConfig_.cn->size(), Fsa::InvalidStateId);
		    Fsa::StateId nonEpsSlotId = 0;
		    std::vector<Fsa::StateId>::iterator itNonEpsilonSlotId = slotIdToNonEpsilonSlotIdMap_.begin();
		    for (ConfusionNetwork::const_iterator itSlot = cnConfig_.cn->begin(), endSlot = cnConfig_.cn->end(), itNormalizedSlot = normalizedCn_->begin();
			 itSlot != endSlot; ++itSlot, ++itNormalizedSlot, ++itNonEpsilonSlotId) {
			if ((itNormalizedSlot->front().label == Fsa::Epsilon)
			    && (itNormalizedSlot->front().scores->get(cnConfig_.cnPosteriorId) >= cnConfig_.epsSlotThreshold))
			    continue;
			ConfusionNetwork::Slot::const_iterator itArc = itSlot->begin(), endArc = itSlot->end();
			for (; (itArc != endArc) && (itArc->label == Fsa::Epsilon); ++itArc);
			if (itArc != endArc)
			    *itNonEpsilonSlotId = nonEpsSlotId++;
		    }
		}
	    } else
		needsModification_ = needsOracle_ = needsPosterior_ = false;
	}

	const CnFeatureConfiguration & cnConfig() const {
	    return cnConfig_;
	}

	const ConfusionNetwork & cn() const {
	    return *cnConfig_.cn;
	}

	const ConfusionNetwork::MapProperties & mapProperties() const {
	    verify_(cnConfig_.cn->mapProperties);
	    return *cnConfig_.cn->mapProperties;
	}

	const ConfusionNetwork::NormalizedProperties & normalizedProperties() const {
	    verify_(normalizedCn_);
	    return *normalizedCn_->normalizedProperties;
	}

	const ConfusionNetwork::OracleAlignmentProperties & oracleProperties() const {
	    verify_(cnConfig_.cn->oracleAlignmentProperties);
	    return *cnConfig_.cn->oracleAlignmentProperties;
	}

	bool needsModification() const {
	    return needsModification_;
	}

	void modify(Arc &arc, Fsa::StateId slotId) const {
	    return modify(arc, slotId, slotId, slotId);
	}

	Score cost(Fsa::LabelId hyp, Fsa::LabelId ref) const {
	    return (hyp == ref) ? CorCost :
		((ref == Fsa::Epsilon) ? InsCost :
		 ((hyp == Fsa::Epsilon) ? DelCost :
		  SubCost));
	}

	void modify(Arc &arc, Fsa::StateId slotId, Fsa::StateId fromSlotId, Fsa::StateId toSlotId) const {
	    if (!needsModification_)
		return;
	    arc.weight_ = cnConfig_.semiring->clone(arc.weight_);
	    if (slotId == Fsa::InvalidStateId) {
		if (cnConfig_.confidenceId != Semiring::InvalidId)
		    arc.weight()->set(cnConfig_.confidenceId, 1.0);
		if (cnConfig_.scoreId != Semiring::InvalidId)
		    arc.weight()->set(cnConfig_.scoreId, Semiring::One);
		if (cnConfig_.errId != Semiring::InvalidId)
		    arc.weight()->set(cnConfig_.errId, 0.0);
		if (cnConfig_.oracleOutput)
		    arc.output_ = Fsa::InvalidLabelId;
		if (cnConfig_.slotEntropyId != Semiring::InvalidId)
		    arc.weight()->set(cnConfig_.slotEntropyId, Semiring::Invalid);
		if (cnConfig_.slotId != Semiring::InvalidId)
		    arc.weight()->set(cnConfig_.slotId, Semiring::Invalid);
		if (cnConfig_.nonEpsSlotId != Semiring::InvalidId)
		    arc.weight()->set(cnConfig_.nonEpsSlotId, Semiring::Invalid);
		return;
	    }
	    verify((fromSlotId != Fsa::InvalidStateId) && (toSlotId != Fsa::InvalidStateId));
	    verify((fromSlotId <= slotId) && (slotId <= toSlotId) && (toSlotId < cnConfig_.cn->size()));
	    // oracle based features
	    if (needsOracle_) {
		const ConfusionNetwork::OracleAlignmentProperties &props = oracleProperties();
		// error over spanned slots
		if (cnConfig_.errId != Semiring::InvalidId) {
		    Score err = 0.0;
		    Fsa::StateId thisSlotId = fromSlotId;
		    for (; thisSlotId < slotId; ++thisSlotId)
			err += cost(Fsa::Epsilon, props.alignment[thisSlotId].label);
		    err += cost(arc.input(), props.alignment[thisSlotId].label);
		    for (++thisSlotId; thisSlotId <= toSlotId; ++thisSlotId)
			err += cost(Fsa::Epsilon, props.alignment[thisSlotId].label);
		    arc.weight()->set(cnConfig_.errId, err);
		}
		// oracle output
		if (cnConfig_.oracleOutput) {
		    arc.output_ = props.alignment[slotId].label;
		}
	    }
	    // slot entropy
	    if (cnConfig_.slotEntropyId != Semiring::InvalidId) {
		const ConfusionNetwork::Slot &slot = (*normalizedCn_)[slotId];
		Score e = 0.0;
		for (ConfusionNetwork::Slot::const_iterator itArc = slot.begin(), endArc = slot.end(); itArc != endArc; ++itArc) {
		    Score p = itArc->scores->get(cnConfig_.cnPosteriorId);
		    e += p * ::log(p);
		}
		e = -e;
		arc.weight()->set(cnConfig_.slotEntropyId, e);
	    }
	    // posterior based features
	    if (needsPosterior_) {
		const ConfusionNetwork::NormalizedProperties &props = normalizedProperties();
		// slot confidence
		if (cnConfig_.confidenceId != Semiring::InvalidId) {
		    Score posterior = props.posteriorScore((*normalizedCn_)[slotId], arc.input());
		    arc.weight()->set(cnConfig_.confidenceId, posterior);
		}
		// score over spanned slots
		if (cnConfig_.scoreId != Semiring::InvalidId) {
		    Score score = 0.0;
		    for (Fsa::StateId thisSlotId = fromSlotId; thisSlotId <= toSlotId; ++thisSlotId) {
			Score posterior = props.posteriorScore(
			    (*normalizedCn_)[thisSlotId],
			    ((thisSlotId == slotId) ? arc.input() : Fsa::Epsilon));
			if (posterior == 0.0)
			    { score = Semiring::Max; break; }
			else
			    score += -::log(posterior);
		    }
		    arc.weight()->set(cnConfig_.scoreId, score);
		}
	    }
	    // slot id
	    if (cnConfig_.slotId != Semiring::InvalidId) {
		arc.weight()->set(cnConfig_.slotId, Score(slotId));
	    }
	    // non-eps slot id
	    if (cnConfig_.nonEpsSlotId != Semiring::InvalidId) {
		if (arc.input() == Fsa::Epsilon) {
		    //verify_(((*cn_)[slotId].begin() + arcId)->label == Fsa::Epsilon);
		    arc.weight()->set(cnConfig_.nonEpsSlotId, Semiring::Invalid);
		} else {
		    //verify_(((*cn_)[slotId].begin() + arcId)->label != Fsa::Epsilon);
		    ScoreId nonEpsSlotId = slotIdToNonEpsilonSlotIdMap_[slotId];
		    if (nonEpsSlotId == Semiring::InvalidId) {
			arc.weight()->set(cnConfig_.nonEpsSlotId, Semiring::Invalid);
			arc.input_ = Fsa::Epsilon;
		    } else {
			arc.weight()->set(cnConfig_.nonEpsSlotId, Score(nonEpsSlotId));
		    }
		}
	    }
	}

	static CnFeaturesRef create(CnFeatureConfiguration cnConfig) {
	    return CnFeaturesRef(new CnFeatures(cnConfig));
	}
    };
    const Score CnFeatures::CorCost = 0.0;
    const Score CnFeatures::SubCost = 1.0;
    const Score CnFeatures::InsCost = 1.0;
    const Score CnFeatures::DelCost = 1.0;
    // -------------------------------------------------------------------------


    // -------------------------------------------------------------------------
    ConstLatticeRef composeAndAddCnFeatures(ConstLatticeRef l, const CnFeatures &cnFeatures) {
	if (!l)
	    return ConstLatticeRef();
	StaticLattice *s = new StaticLattice;
	StaticBoundaries *b = new StaticBoundaries;
	s->setBoundaries(ConstBoundariesRef(b));
	copy(l, s, b);
	if (cnFeatures.cnConfig().oracleOutput) {
	    s->setType(Fsa::TypeTransducer);
	    s->setOutputAlphabet(s->getInputAlphabet());
	}
	std::vector<Fsa::StateId> minOut(s->size(), Core::Type<Fsa::StateId>::max);
	for (Fsa::StateId sid = 0; sid < s->size(); ++sid) {
	    const State *sp = s->fastState(sid);
	    if (!sp)
		continue;
	    if (sp->hasArcs()) {
		ConfusionNetwork::MapProperties::Map::const_iterator itMap = cnFeatures.mapProperties().state(sp->id());
		for (State::const_iterator a = sp->begin(), end = sp->end(); a != end; ++a, ++itMap) {
		    Fsa::StateId slotId = itMap->sid;
		    if (slotId != Fsa::InvalidStateId)
			if (slotId < minOut[sid]) minOut[sid] = slotId;
		}
	    } else
		minOut[sid] = cnFeatures.cn().size();
	}
	const Semiring &semiring = *s->semiring();
	if (!cnFeatures.cnConfig().compose) {
	    s->setDescription(Core::form("addCnFeatures(%s)", s->describe().c_str()));
	    for (Fsa::StateId sid = 0; sid < s->size(); ++sid) {
		State *sp = s->fastState(sid);
		if (!sp)
		    continue;
		Fsa::StateId fromSlotId = minOut[sid];
		ConfusionNetwork::MapProperties::Map::const_iterator itMap = cnFeatures.mapProperties().state(sp->id());
		for (State::iterator a = sp->begin(), end = sp->end(); a != end; ++a, ++itMap) {
		    Fsa::StateId slotId = itMap->sid;
		    Fsa::StateId nextSlotId = minOut[a->target()];
		    cnFeatures.modify(
			*a, slotId,
			(fromSlotId == Core::Type<Fsa::StateId>::max) ? slotId : fromSlotId,
			(nextSlotId == Core::Type<Fsa::StateId>::max) ? slotId : nextSlotId - 1);
		}
	    }
	} else {
	    s->setDescription(Core::form("composeAndAddCnFeatures(%s)", s->describe().c_str()));
	    bool duplicateOutput = cnFeatures.cnConfig().duplicateOutput;
	    Fsa::StateId endSid = s->size(), nextSid = s->size();
	    for (Fsa::StateId sid = 0; sid < endSid; ++sid) {
		State *sp = s->fastState(sid);
		if (!sp)
		    continue;
		Fsa::StateId fromSlotId = minOut[sid];
		verify(fromSlotId != Core::Type<Fsa::StateId>::max); // can happen, if lattice has (many) arcs of length 0
		ConfusionNetwork::MapProperties::Map::const_iterator itMap = cnFeatures.mapProperties().state(sp->id());
		for (State::iterator a = sp->begin(), end = sp->end(); a != end; ++a, ++itMap) {
		    Fsa::StateId slotId = itMap->sid;
		    verify(slotId != Fsa::InvalidStateId);
		    Fsa::StateId toSlotId = minOut[a->target()];
		    verify(toSlotId != Core::Type<Fsa::StateId>::max); // can happen, if lattice has (many) arcs of length 0
		    --toSlotId;
		    cnFeatures.modify(*a, slotId);
		    verify((fromSlotId <= slotId) && (slotId <= toSlotId));
		    if ((fromSlotId != slotId) || (slotId != toSlotId)) {
			const Arc tmpArc = *a;
			Fsa::LabelId epsOutputLabel = (duplicateOutput && (l->type() != Fsa::TypeAcceptor)) ? tmpArc.output() : Fsa::Epsilon;
			State *thisSp = sp, *nextSp = 0;
			Arc *thisArc = &*a;
			Fsa::StateId thisSlotId = fromSlotId;
			if (thisSlotId < slotId) {
			    const Boundary fromBoundary = b->get(sid);
			    nextSp = new Flf::State(nextSid++); s->setState(nextSp);
			    b->set(nextSp->id(), fromBoundary);
			    thisArc->target_ = nextSp->id(); thisArc->weight_ = semiring.one(); thisArc->input_ = Fsa::Epsilon; a->output_ = epsOutputLabel;
			    cnFeatures.modify(*thisArc, thisSlotId);
			    thisSp = nextSp;
			    for(++thisSlotId; thisSlotId < slotId; ++thisSlotId) {
				nextSp = new Flf::State(nextSid++); s->setState(nextSp);
				b->set(nextSp->id(), fromBoundary);
				thisArc = thisSp->newArc(nextSp->id(), semiring.one(), Fsa::Epsilon, epsOutputLabel);
				cnFeatures.modify(*thisArc, thisSlotId);
				thisSp = nextSp;
			    }
			    *(thisArc = thisSp->newArc()) = tmpArc;
			}
			if (thisSlotId < toSlotId) {
			    const Boundary toBoundary = b->get(tmpArc.target());
			    for (++thisSlotId; thisSlotId <= toSlotId; ++thisSlotId) {
				nextSp = new Flf::State(nextSid++); s->setState(nextSp);
				b->set(nextSp->id(), toBoundary);
				thisArc->target_ = nextSp->id();
				thisArc = nextSp->newArc(Fsa::InvalidStateId, semiring.one(), Fsa::Epsilon, epsOutputLabel);
				cnFeatures.modify(*thisArc, thisSlotId);
			    }
			    thisArc->target_ = tmpArc.target();
			}
		    }
		}
	    }
	}
	return ConstLatticeRef(s);
    }

    ConstLatticeRef composeAndAddCnFeatures(ConstLatticeRef l, const CnFeatureConfiguration &cnConfig) {
	CnFeatures cnFeatures(cnConfig);
	return composeAndAddCnFeatures(l, cnFeatures);
    }
    // -------------------------------------------------------------------------


    // -------------------------------------------------------------------------
    class CnFeatureNode : public FilterNode {
	typedef FilterNode Precursor;
    public:
	static const Core::ParameterString paramPosteriorKey;

	static const Core::ParameterString paramKey;
	static const Core::ParameterBool paramOracleOutput;
	static const Core::ParameterFloat paramThreshold;

	static const Core::ParameterBool paramCompose;
	static const Core::ParameterBool paramDuplicateOutput;

    private:
	std::string cnPosteriorKey_;

	std::string confidenceKey_;
	std::string scoreKey_;
	std::string errKey_;
	std::string slotEntropyKey_;
	std::string slotKey_;
	std::string nonEpsSlotKey_;

	CnFeatureConfiguration cnConfig_;
	ConstSemiringRef lastCnSemiring_;

    protected:
	ConstLatticeRef filter(ConstLatticeRef l) {
	    cnConfig_.cn = requestCn(1);
	    if (!l)
		return ConstLatticeRef();
	    if (!cnConfig_.cn) {
		warning("No CN provided for lattice \"%s\"; skip lattice",
			l->describe().c_str());
		return ConstLatticeRef();
	    }
	    if (!lastCnSemiring_ || (lastCnSemiring_.get() != cnConfig_.cn->semiring.get())) {
		lastCnSemiring_ = cnConfig_.cn->semiring;
		cnConfig_.cnPosteriorId = lastCnSemiring_->id(cnPosteriorKey_);
	    }
	    if (!cnConfig_.semiring || (cnConfig_.semiring.get() != l->semiring().get())) {
		cnConfig_.semiring = l->semiring();
		cnConfig_.confidenceId = cnConfig_.semiring->id(confidenceKey_);
		cnConfig_.scoreId = cnConfig_.semiring->id(scoreKey_);
		cnConfig_.errId = cnConfig_.semiring->id(errKey_);
		cnConfig_.slotEntropyId = cnConfig_.semiring->id(slotEntropyKey_);
		cnConfig_.slotId = cnConfig_.semiring->id(slotKey_);
		cnConfig_.nonEpsSlotId = cnConfig_.semiring->id(nonEpsSlotKey_);
	    }
	    return composeAndAddCnFeatures(l, cnConfig_);
	}

    public:
	CnFeatureNode(const std::string &name, const Core::Configuration &config) :
	    Precursor(name, config) {}
	~CnFeatureNode() {}

	void init(const std::vector<std::string> &arguments) {
	    if (!connected(0))
		criticalError("Need a data source at port 0.");
	    if (!connected(1))
		criticalError("Need a CN at port 1.");
	    Core::Component::Message msg(log());
	    cnConfig_.compose = paramCompose(config);
	    if (cnConfig_.compose) {
		cnConfig_.duplicateOutput = paramDuplicateOutput(config);
		msg << "Extend all pathes in lattice to match CN size.\n";
		if (cnConfig_.duplicateOutput)
		    msg << "Preserve output-label (in some configurations, the output label identifies the system the arcs comes from)\n";
	    }
	    cnPosteriorKey_ = paramPosteriorKey(select("cn"));
	    if (cnPosteriorKey_.empty())
		msg << "CN posterior key: " << cnPosteriorKey_ << "\n";
	    //else criticalError("Definition of a key pointing at a posterior probability distribution in the CN is mandatory.");
	    msg << "Store the following CN features:\n";
	    confidenceKey_ = paramKey(select("confidence"));
	    if (!confidenceKey_.empty())
		msg << "  - confidence to dimension \"" << confidenceKey_ << "\"\n";
	    scoreKey_ = paramKey(select("score"));
	    if (!scoreKey_.empty())
		msg << "  - score to dimension \"" << scoreKey_ << "\"\n";
	    errKey_ = paramKey(select("cost"));
	    if (!errKey_.empty())
		msg << "  - cost to dimension \"" << errKey_ << "\"\n";
	    cnConfig_.oracleOutput = paramOracleOutput(config);
	    if (cnConfig_.oracleOutput)
		msg << "  - store oracle word as output label\n";
	    slotEntropyKey_ = paramKey(select("entropy"));
	    if (!slotEntropyKey_.empty())
		msg << "  - slot entropy to dimension \"" << slotEntropyKey_ << "\"\n";
	    slotKey_ = paramKey(select("slot"));
	    if (!slotKey_.empty())
		msg << "  - slot number to dimension \"" << slotKey_ << "\"\n";
	    nonEpsSlotKey_ = paramKey(select("non-eps-slot"));
	    if (!nonEpsSlotKey_.empty()) {
		cnConfig_.epsSlotThreshold = paramThreshold(select("non-eps-slot"), 1.0);
		msg << "  - non-epsilon-slot number to dimension \"" << slotKey_ << "\"\n";
		msg << "    epsilon-slot threshold is \"" << cnConfig_.epsSlotThreshold << "\"\n";
	    }
	    if (cnConfig_.oracleOutput && cnConfig_.duplicateOutput)
		criticalError("Oracle output and duplicate output are mutually exclusive.");
	}
    };
    const Core::ParameterString CnFeatureNode::paramPosteriorKey(
	"posterior-key",
	"posterior key",
	"");
    const Core::ParameterString CnFeatureNode::paramKey(
	"key",
	"key",
	"");
    const Core::ParameterBool CnFeatureNode::paramOracleOutput(
	"oracle-output",
	"store oracle as output label",
	false);
    const Core::ParameterFloat CnFeatureNode::paramThreshold(
	"threshold",
	"threshold");
    const Core::ParameterBool CnFeatureNode::paramCompose(
	"compose",
	"compose",
	false);
    const Core::ParameterBool CnFeatureNode::paramDuplicateOutput(
	"duplicate-output",
	"duplicate output labels",
	false);
    NodeRef createCnFeatureNode(const std::string &name, const Core::Configuration &config) {
	return NodeRef(new CnFeatureNode(name, config));
    }
    // -------------------------------------------------------------------------

} // namespace Flf

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

#include "FlfCore/Basic.hh"
#include "FlfCore/LatticeInternal.hh"
#include "FlfCore/Utility.hh"
#include "FlfCore/Traverse.hh"
#include "Best.hh"
#include "Copy.hh"
#include "FwdBwd.hh"
#include "Prune.hh"
#include "TimeframeConfusionNetwork.hh"
#include "Lexicon.hh"
#include "Info.hh"

namespace Flf {

    // -------------------------------------------------------------------------
    class FwdBwdPruningLattice : public SlaveLattice {
    private:
	ConstFwdBwdRef fb_;
	Score threshold_;
    public:
	FwdBwdPruningLattice(ConstLatticeRef l, ConstFwdBwdRef fb, Score threshold) :
	    SlaveLattice(l), fb_(fb), threshold_(threshold) {}
	virtual ~FwdBwdPruningLattice() {}

	virtual ConstStateRef getState(Fsa::StateId sid) const {
	    ConstStateRef sr = fsa_->getState(sid);
	    State *sp = new State(sid, sr->tags(), sr->weight());
	    FwdBwd::State::const_iterator itFbScore = fb_->state(sid).begin();
	    for (State::const_iterator a = sr->begin(); a != sr->end(); ++a, ++itFbScore)
		if (itFbScore->score() <= threshold_)
		    *sp->newArc() = *a;
	    return ConstStateRef(sp);
	}

	virtual std::string describe() const {
	    return Core::form("posterior-pruning(%s,%f)", fsa_->describe().c_str(), threshold_);
	}
    };

    ConstLatticeRef pruneByFwdBwdScores(ConstLatticeRef l, ConstFwdBwdRef fb, Score t, f32 minArcsPerSec, f32 maxArcsPerSec, s32 maxArcsPerSegment) {
	verify(fb);
	if (!l && (l->initialStateId() == Fsa::InvalidStateId))
	    return ConstLatticeRef();
	if (t == Core::Type<Score>::max && maxArcsPerSec == Core::Type<f32>::max)
	    return l;
	if (t <= 0.0)
	    return bestProjection(l).first;

	t = std::min( fb->max()-fb->min(), (f64)t );

	s32 minArcs = 0, maxArcs = Core::Type<s32>::max;

	if (minArcsPerSec || maxArcsPerSec != Core::Type<f32>::max) {
	    ConstStateMapRef chronological = sortChronologically(l);
	    if(!chronological || chronological->empty() || !l->getBoundaries() || !l->getBoundaries()->valid())
	    {
		Core::Application::us()->warning() << "can not prune by arcs per second, because boundaries are not available";
	    }else{
		f32 secs = (l->boundary(chronological->back()).time() - l->boundary(chronological->at(0)).time()) / f32(100);
		minArcs = minArcsPerSec * secs;
		if (maxArcsPerSec != Core::Type<f32>::max)
		    maxArcs = std::min((s32)(maxArcsPerSec * secs), maxArcsPerSegment);
		else
		    maxArcs = maxArcsPerSegment;
		if (maxArcs < 0)
		    maxArcs = maxArcsPerSegment; // Detect wrap
	    }
    }

	if (minArcsPerSec) {
	    LatticeCounts counts = count(l);
	    if (counts.nArcs_  <= minArcs)
		return l;
	}

	ConstLatticeRef p;
	p = ConstLatticeRef(new FwdBwdPruningLattice(l, fb, fb->min() + t));

	if(maxArcs != Core::Type<s32>::max)
	{
	    LatticeCounts counts = count(p);
	    while((counts.nArcs_ > maxArcs) && t > 0.1)
	    {
		t *= 0.9; /// @todo binary search instead of static steps
		Core::Application::us()->log() << "pruning because too many arcs: " << counts.nArcs_ << ", specified maximum: " << maxArcs << " new threshold: " << t;
		p = ConstLatticeRef(new FwdBwdPruningLattice(l, fb, t + fb->min()));
		counts = count(p);
	    }
	}
	if(minArcs)
	{
	    LatticeCounts counts = count(p);
	    while(counts.nArcs_ < minArcs && t < 100.0)
	    {
		t *= 1.2; /// @todo binary search instead of static steps
		Core::Application::us()->log() << "pruning because too few arcs per second: " << counts.nArcs_ << ", specified minimum: " << minArcs << " new threshold: " << t;
		p = ConstLatticeRef(new FwdBwdPruningLattice(l, fb, t + fb->min()));
		counts = count(p);
	    }
	}
	return p;
    }

    // -------------------------------------------------------------------------
    class PhoneCoveragePruningLattice : public SlaveLattice, public TraverseState {
    private:
	ConstFwdBwdRef fb_;
	u32 coverage_;
	Score threshold_;
	typedef std::pair< Fsa::StateId, s32 > ArcId;
	struct ArcIdHash {
	    u32 operator() (const ArcId& arc) const {
		return arc.first + arc.second * 318221 + (arc.first << 17);
	    }
	};
	typedef std::pair<f64, ArcId> ScoreAndArc;
	Lexicon::LemmaPronunciationAlphabetRef lpAlphabet_;
	std::vector< ScoreAndArc > bestArcs_;
	std::vector< ScoreAndArc > bestInArcForState_;
	std::set<Bliss::Phoneme::Id> nonWordPhones_;
	u32 nPhones_, maxTime_;
	Core::HashMap<ArcId, bool, ArcIdHash> coveredArcs_;

	ScoreAndArc* bestArcs(u32 time, u32 phone) const {
	    u32 index = (time * nPhones_ + phone) * coverage_;
	    verify(index+coverage_ <= bestArcs_.size());
	    return const_cast<ScoreAndArc*>(bestArcs_.data()) + index;
	}

	ScoreAndArc* bestArcs(u32 time, u32 phone)
	{
	    verify(phone < nPhones_);
	    u32 index = (time * nPhones_ + phone) * coverage_;
	    if(((time+1) * nPhones_) * coverage_ > bestArcs_.size())
		bestArcs_.resize(((time+1) * nPhones_) * coverage_,
				    ScoreAndArc(Core::Type<f64>::max,
						ArcId(Core::Type<Fsa::StateId>::max, Core::Type<s32>::max)));
	    return bestArcs_.data() + index;
	}

	void record(u32 time, u32 phoneId, ScoreAndArc scoredArc)
	{
	    if(time+1 > maxTime_)
		maxTime_ = time+1;
	    ScoreAndArc* best = bestArcs(time, phoneId);
	    ScoreAndArc* bestEnd = best + coverage_;
	    verify(bestEnd-bestArcs_.data() <= bestArcs_.size());

	    for(ScoreAndArc* current = bestEnd-1; current >= best; --current)
	    {
		if(scoredArc.first > current->first)
		{
		    if(current < bestEnd-1)
		    {
			// Insert behind
			verify((current-bestArcs_.data())+2+(coverage_ - 2 - (current - best)) <= bestArcs_.size());
			memmove(current+2, current+1, (coverage_ - 2 - (current - best))*sizeof(ScoreAndArc));
			*(current+1) = scoredArc;
		    }
		    return;
		}
	    }
	    memmove(best+1, best, (coverage_ - 1)*sizeof(ScoreAndArc));
	    *best = scoredArc;
	}

	void recordInArc(ArcId id, Fsa::StateId target, f64 score)
	{
	    if(target >= bestInArcForState_.size())
		bestInArcForState_.resize(target + 100,
					    ScoreAndArc(Core::Type<f64>::max,
						ArcId(Core::Type<Fsa::StateId>::max, Core::Type<s32>::max)));
	    if(bestInArcForState_[target].first > score)
		bestInArcForState_[target] = std::make_pair(score, id);
	}

    public:
	PhoneCoveragePruningLattice(ConstLatticeRef l, ConstFwdBwdRef fb, Score threshold, u32 coverage, std::set<Bliss::Phoneme::Id> nonWordPhones, Core::XmlChannel* statsChannel) :
	    SlaveLattice(l),
	    TraverseState(l),
	    fb_(fb),
	    coverage_(coverage),
	    threshold_(threshold),
	    nonWordPhones_(nonWordPhones),
	    nPhones_(Lexicon::us()->phonemeInventory()->nPhonemes()+1),
	    maxTime_(0) {
	    Lexicon::AlphabetId alphabetId = Lexicon::us()->alphabetId(l->getInputAlphabet());

	    verify(alphabetId == Lexicon::LemmaPronunciationAlphabetId);
	    lpAlphabet_ = Lexicon::us()->lemmaPronunciationAlphabet();

	    traverse();
	    for(u32 i = 0; i < bestArcs_.size(); ++i)
		if(bestArcs_[i].first != Core::Type<f64>::max)
		    coveredArcs_[bestArcs_[i].second] = true;
	    Core::HashMap<ArcId, bool, ArcIdHash> oldCoveredArcs = coveredArcs_;
	    for(Core::hash_map<ArcId, bool, ArcIdHash, std::equal_to<ArcId> >::iterator it = oldCoveredArcs.begin(); it != oldCoveredArcs.end(); ++it)
	    {
		connectPathForwards((l->getState(it->first.first)->begin() + it->first.second)->target());
		connectPathBackwards(it->first.first);
	    }
	    if(statsChannel)
		statistics(*statsChannel);
	}

	void statistics(Core::XmlChannel& channel) const {
	    Core::HistogramStatistics phoneCoverage("phone coverage");
	    Core::HistogramStatistics speechPhoneCoverage("phone coverage on speech");

	    for(u32 t = 0; t < maxTime_; ++t)
	    {
		f64 bestScore = Core::Type<f64>::max;
		u32 bestPhone = 0;
		for(u32 phone = 0; phone < nPhones_; ++phone)
		{
		    if(bestArcs(t, phone)->first < bestScore)
		    {
			bestScore = bestArcs(t, phone)->first;
			bestPhone = phone;
		    }
		}

		for(u32 phone = 0; phone < nPhones_; ++phone)
		{
		    if(nonWordPhones_.count(phone))
			continue;
		    ScoreAndArc* arcs = bestArcs(t, phone);
		    u32 achieved = 0;
		    for(ScoreAndArc* a = arcs; a < arcs + coverage_; ++a)
			if(a->first != Core::Type<f64>::max)
			    ++achieved;
		    phoneCoverage += achieved;
		    if(bestPhone != 0)
			speechPhoneCoverage += achieved;
		}
	    }

	    phoneCoverage.write(channel);
	    speechPhoneCoverage.write(channel);

	    u32 preservedArcs = 0, additionalArcs = 0;
	    for( Core::HashMap<ArcId, bool, ArcIdHash>::const_iterator it = coveredArcs_.begin(); it != coveredArcs_.end(); ++it )
	    {
		Flf::ConstStateRef state = l->getState(it->first.first);
		FwdBwd::Arc fbArc = fb_->arc(state, it->first.second + state->begin());
		if(fbArc.score() <= threshold_) {
		    preservedArcs += 1;
		}else{
		    additionalArcs += 1;
		}
	    }
	    Core::Application::us()->log() << "arcs preserved by phone coverage pruning: " << (preservedArcs+additionalArcs) << ", additional: " << additionalArcs;
	}

	// Marks the best successor path as covered, until another covered arc is reached
	void connectPathForwards(Fsa::StateId stateId)
	{
	    Flf::ConstStateRef state = l->getState(stateId);
	    f64 bestScore = Core::Type<f64>::max;
	    u32 bestArc = Core::Type<u32>::max;
	    for(Ftl::State<Flf::Arc>::const_iterator arcIt = state->begin(); arcIt != state->end(); ++arcIt)
	    {
		FwdBwd::Arc fbArc = fb_->arc(state, arcIt);
		if(fbArc.score() < bestScore)
		{
		    bestScore = fbArc.score();
		    bestArc = arcIt - state->begin();
		}
	    }
	    if(bestArc == Core::Type<u32>::max)
		return;
	    ArcId id(state->id(), bestArc);
	    if(coveredArcs_.count(id))
		return;
	    coveredArcs_[id] = true;
	    connectPathForwards((state->begin() + bestArc)->target());
	}

	void connectPathBackwards(Fsa::StateId stateId)
	{
	    verify(stateId < bestInArcForState_.size());
	    if(bestInArcForState_[stateId].first != Core::Type<f64>::max)
	    {
		if(coveredArcs_.count(bestInArcForState_[stateId].second))
		    return;
		coveredArcs_[bestInArcForState_[stateId].second] = true;
		connectPathBackwards(bestInArcForState_[stateId].second.first);
	    }
	}

	virtual void exploreState(Flf::ConstStateRef from) {
	    s32 startTime = l->getBoundaries()->get(from->id()).time();
	    for(Ftl::State<Flf::Arc>::const_iterator arcIt = from->begin(); arcIt != from->end(); ++arcIt)
	    {
		const Flf::Arc& arc(*arcIt);
		ArcId arcId(from->id(), arcIt - from->begin());
		FwdBwd::Arc fbArc = fb_->arc(from, arcIt);
		recordInArc(arcId, arc.target(), fbArc.score());
		s32 endTime = l->getBoundaries()->get(arc.target()).time();
		if(arc.input() == Fsa::Epsilon)
		    continue;
		const Bliss::LemmaPronunciation* lemmaPron = lpAlphabet_->lemmaPronunciation(arc.input());
		u32 phones = lemmaPron->pronunciation()->length();
		for(u32 p = 0; p < phones; ++p)
		{
		    s32 phoneStartTime = ((endTime - startTime) * p) / phones + startTime;
		    s32 phoneEndTime = ((endTime - startTime) * (p+1)) / phones + startTime;
		    verify(phoneStartTime >= startTime && phoneEndTime <= endTime);
		    verify(phoneStartTime >= 0 && phoneStartTime <= phoneEndTime);
		    ScoreAndArc scoredArc(fbArc.score(), arcId);
		    Bliss::Phoneme::Id phoneId = lemmaPron->pronunciation()->operator[](p);
		    if(nonWordPhones_.count(phoneId))
			phoneId = 0; // Count all nonword phones as one
		    for(s32 t = phoneStartTime; t != phoneEndTime; ++t)
			record(t, phoneId, scoredArc);
		}
	    }
	}

	virtual ConstStateRef getState(Fsa::StateId sid) const {
	    ConstStateRef sr = fsa_->getState(sid);
	    State *sp = new State(sid, sr->tags(), sr->weight());
	    FwdBwd::State::const_iterator itFbScore = fb_->state(sid).begin();
	    for (State::const_iterator a = sr->begin(); a != sr->end(); ++a, ++itFbScore)
		if(coveredArcs_.count(ArcId(sid, a-sr->begin())) || itFbScore->score() <= threshold_)
		    *sp->newArc() = *a;
	    return ConstStateRef(sp);
	}

	virtual std::string describe() const {
	    return Core::form("posterior-pruning-phone-coverage(%s,%f,%i)", fsa_->describe().c_str(), threshold_, coverage_);
	}
    };

    namespace {
	const Core::ParameterBool paramRelative(
	    "relative",
	    "threshold is interpreted relative to min. arc score",
	    true);
	const Core::ParameterBool paramAsProbability(
	    "as-probability",
	    "threshold given as probability (like the SRI lattice-tool)",
	    false);
	const Core::ParameterFloat paramThreshold(
	    "threshold",
	    "threshold",
	    Core::Type<Score>::max, 0.0, Core::Type<Score>::max);
	const Core::ParameterInt paramPhoneCoverage(
	    "min-phone-coverage",
	    "min-phone-coverage",
	    0, 0, 100);
	const Core::ParameterFloat paramMinArcsPerSecond(
	    "min-arcs-per-second",
	    "minimum number of arcs per second",
	    0, 0);
	const Core::ParameterFloat paramMaxArcsPerSecond(
	    "max-arcs-per-second",
	    "maximum number of arcs per second",
	    Core::Type<f32>::max, 1);
	const Core::ParameterInt paramMaxArcsPerSegment(
	    "max-arcs-per-segment",
	    "maximum total number of arcs per segment",
	    Core::Type<s32>::max, 1, Core::Type<s32>::max);
	const Core::ParameterStringVector paramNonWordPhones(
	    "nonword-phones", "Non-word (noise) phone symbols for phone coverage computation.\
			       Wildcards can be used at boundaries to select multiple phonemes.", ",");
    } // namespace

    class FwdBwdPruner::Internal : public Core::Component {
    public:
	Core::XmlChannel configurationChannel;
	Core::XmlChannel statisticsChannel;
	FwdBwdBuilderRef fbBuilder;
	bool isRelative;
	bool asProbability;
	Score t;
	u32 coverage;
	f32 minArcsPerSec;
	f32 maxArcsPerSec;
	s32 maxArcsPerSegment;
	std::set<Bliss::Phoneme::Id> nonWordPhones;
    public:
	Internal(const Core::Configuration &config, FwdBwdBuilderRef fbBuilder) :
	    Core::Component(config),
	    configurationChannel(config, "configuration"),
	    statisticsChannel(config, "statistics") {
	    this->fbBuilder = (fbBuilder) ? fbBuilder : FwdBwdBuilder::create(select("fb"));
	    isRelative = paramRelative(config);
	    asProbability = paramAsProbability(config);
	    t = paramThreshold(config);
	    coverage = paramPhoneCoverage(config);
	    minArcsPerSec = paramMinArcsPerSecond(config);
	    maxArcsPerSec = paramMaxArcsPerSecond(config);
	    maxArcsPerSegment = paramMaxArcsPerSegment(config);
	    std::vector<std::string> nonWordPhoneSelection = paramNonWordPhones(config);
	    for(std::vector<std::string>::iterator phoneIt = nonWordPhoneSelection.begin();
		    phoneIt != nonWordPhoneSelection.end(); ++phoneIt)
	    {
		std::set<Bliss::Phoneme::Id> selection = Lexicon::us()->phonemeInventory()->parseSelection(*phoneIt);
		nonWordPhones.insert(selection.begin(), selection.end());
	    }
	    if(nonWordPhones.size())
	     log("using %i non-word phones", (s32)nonWordPhones.size());
	    if (t < 0.0)
		warning("Fwd/Bwd pruning threshold is negative; set to 0.");
	    if (t != Core::Type<Score>::max) {
		if (asProbability) {
		    if (t > 1.0)
			criticalError("Probability threshold %f is not in [0.0,1.0].", t);
		    t = (t == 0.0) ? Core::Type<Score>::max : -::log(t);
		}
	    }
	    if (configurationChannel) {
		configurationChannel << Core::XmlOpen("configuration")
		    + Core::XmlAttribute("component", this->name());
		{
		    std::ostream &os(configurationChannel);
		    if (t == 0.0)
			os << "threshold = 0.0; single best" << std::endl;
		    else if (t == Core::Type<Score>::max)
			os << "threshold = inf(p=1.0); no pruning" << std::endl;
		    else
			os << "threshold = " << t << " (p = " << ::exp(-t) << ")" << std::endl;
		    os << "thresholds is " << (isRelative ? "relative to min. fwd/bwd. score" : "absolute") << "." << std::endl;
		}
		configurationChannel << Core::XmlClose("configuration");
	    }
	}
	virtual ~Internal() {}

	Score threshold(ConstFwdBwdRef fb) {
	    Score threshold = (t == Core::Type<Score>::max) ?
		Core::Type<Score>::max :
		(isRelative) ? fb->min() + t : t;
	    if (statisticsChannel) {
		statisticsChannel << Core::XmlOpen("statistics")
		    + Core::XmlAttribute("component", this->name());
		statisticsChannel << Core::XmlFull("threshold", threshold);
		statisticsChannel << Core::XmlClose("statistics");
	    }
	    return threshold;
	}
    };

    FwdBwdPruner::FwdBwdPruner() :internal_(0)  {}

    FwdBwdPruner::~FwdBwdPruner() {
	delete internal_;
    }

    ConstLatticeRef FwdBwdPruner::prune(ConstLatticeRef l, bool trim) {
	std::pair<ConstLatticeRef, ConstFwdBwdRef> fbResult = internal_->fbBuilder->build(l);
	return prune(fbResult.first, fbResult.second, trim);
    }

    ConstLatticeRef FwdBwdPruner::prune(ConstLatticeRef l, ConstFwdBwdRef fb, bool trim) {
	ConstLatticeRef p = pruneByFwdBwdScores(l, fb, internal_->threshold(fb)-fb->min(), internal_->minArcsPerSec, internal_->maxArcsPerSec, internal_->maxArcsPerSegment);
	if (trim) {
	    StaticLatticeRef s = StaticLatticeRef(new StaticLattice);
	    persistent(p, s.get(), 0);
	    trimInPlace(s);
	    if (s && (s->initialStateId() != Fsa::InvalidStateId)) {
		s->setBoundaries(p->getBoundaries());
		p = ConstLatticeRef(s);
	    } else
		p = bestProjection(l).first;
	}
	return p;
    }

    FwdBwdPrunerRef FwdBwdPruner::create(const Core::Configuration &config, FwdBwdBuilderRef fbBuilder) {
	FwdBwdPruner *fwdBwdPruner = new FwdBwdPruner;
	fwdBwdPruner->internal_ = new FwdBwdPruner::Internal(config, fbBuilder);
	return FwdBwdPrunerRef(fwdBwdPruner);
    }


    class FwdBwdPruningNode : public FilterNode {
	typedef Node Precursor;
    public:
	static const Core::ParameterBool paramTrim;
    private:
	FwdBwdPrunerRef fbPruner_;
	bool trim_;
    protected:
	virtual ConstLatticeRef filter(ConstLatticeRef l) {
	    if (!l)
		return ConstLatticeRef();
	    return fbPruner_->prune(l, trim_);
	}
    public:
	FwdBwdPruningNode(const std::string &name, const Core::Configuration &config) :
	    FilterNode(name, config) {}
	virtual ~FwdBwdPruningNode() {}
	virtual void init(const std::vector<std::string> &arguments) {
	    fbPruner_ = FwdBwdPruner::create(config);
	    Core::Component::Message msg = log();
	    trim_ = paramTrim(config);
	    if (trim_)
		msg << "Trim pruned lattice(s).\n";
	}
    };
    const Core::ParameterBool FwdBwdPruningNode::paramTrim(
	"trim",
	"trim after applying thresholding",
	true);
    NodeRef createFwdBwdPruningNode(const std::string &name, const Core::Configuration &config) {
	return NodeRef(new FwdBwdPruningNode(name, config));
    }
    // -------------------------------------------------------------------------

    // -------------------------------------------------------------------------
    namespace {
	struct CnProbabilityWeakOrder {
	    ScoreId posteriorId;
	    CnProbabilityWeakOrder(ScoreId posteriorId) : posteriorId(posteriorId) {}
	    bool operator() (const ConfusionNetwork::Arc &a1, const ConfusionNetwork::Arc &a2) const {
		return a1.scores->get(posteriorId) > a2.scores->get(posteriorId);
	    }
	};
    } //namespace
    void prune(ConstConfusionNetworkRef cnRef, Score threshold, u32 maxSlotSize, bool normalize) {
	if (!cnRef)
	    return;
	if (!cnRef->isNormalized())
	    Core::Application::us()->criticalError("Confusion network pruning does only work for normalized CNs.");
	ConfusionNetwork &cn = const_cast<ConfusionNetwork&>(*cnRef);
	ScoreId posteriorId = cn.normalizedProperties->posteriorId;
	verify(posteriorId != Semiring::InvalidId);
	for (ConfusionNetwork::iterator itSlot = cn.begin(), endSlot = cn.end(); itSlot != endSlot; ++itSlot) {
	    ConfusionNetwork::Slot &slot = *itSlot;
	    std::sort(slot.begin(), slot.end(), CnProbabilityWeakOrder(posteriorId));
	    ConfusionNetwork::Slot::iterator itArc = slot.begin(), endArc = slot.end();
	    Score sum = 0.0;
	    for (u32 i = 0, max = std::min(maxSlotSize, u32(slot.size()));
		 (i < max) && (sum < threshold); ++i, ++itArc)
		sum += itArc->scores->get(posteriorId);
	    if (itArc != endArc) {
		slot.erase(itArc, endArc);
		verify(!slot.empty());
		if (normalize) {
		    Score norm = 1.0 / sum;
		    for (itArc = slot.begin(), endArc = slot.end(); itArc != endArc; ++itArc)
			itArc->scores->multiply(posteriorId, norm);
		}
	    }
	    std::sort(slot.begin(), slot.end());
	}
    }
    // -------------------------------------------------------------------------


    // -------------------------------------------------------------------------
    namespace {
	struct PosteriorCnProbabilityWeakOrder {
	    bool operator() (const PosteriorCn::Arc &a1, const PosteriorCn::Arc &a2) const {
		return a1.score > a2.score;
	    }
	};
    } //namespace
    void prune(ConstPosteriorCnRef cnRef, Score threshold, u32 maxSlotSize, bool normalize) {
	if (!cnRef)
	    return;
	PosteriorCn &cn = const_cast<PosteriorCn&>(*cnRef);
	for (PosteriorCn::iterator itSlot = cn.begin(), endSlot = cn.end(); itSlot != endSlot; ++itSlot) {
	    PosteriorCn::Slot &slot = *itSlot;
	    std::sort(slot.begin(), slot.end(), PosteriorCnProbabilityWeakOrder());
	    PosteriorCn::Slot::iterator itArc = slot.begin(), endArc = slot.end();
	    Score sum = 0.0;
	    for (u32 i = 0, max = std::min(maxSlotSize, u32(slot.size()));
		 (i < max) && (sum < threshold); ++i, ++itArc)
		sum += itArc->score;
	    if (itArc != endArc) {
		slot.erase(itArc, endArc);
		verify(!slot.empty());
		if (normalize) {
		    Score norm = 1.0 / sum;
		    for (itArc = slot.begin(), endArc = slot.end(); itArc != endArc; ++itArc)
			itArc->score *= norm;
		}
	    }
	    std::sort(slot.begin(), slot.end());
	}
    }
    // -------------------------------------------------------------------------


    // -------------------------------------------------------------------------
    void removeEpsSlots(ConstConfusionNetworkRef cnRef, Score threshold) {
	if (!cnRef)
	    return;
	ConfusionNetwork &cn = const_cast<ConfusionNetwork&>(*cnRef);
	ConfusionNetwork::iterator itFrom = cn.begin(), itTo = cn.begin(), endSlot = cn.end();
	std::vector<Fsa::StateId> slotIdMapping(cn.size(), Fsa::InvalidStateId);
	std::vector<u32>::iterator itTargetSlotId = slotIdMapping.begin();
	u32 targetSlotId = 0;
	if (!cnRef->isNormalized()) {
	    if (threshold < 1.0)
		Core::Application::us()->warning("Epsilon slot removal for non-normalized CNs does not support thresholding.");
	    for (; itFrom != endSlot; ++itFrom, ++itTargetSlotId) {
		const ConfusionNetwork::Slot &from = *itFrom;
		for (ConfusionNetwork::Slot::const_iterator itArc = from.begin(), endArc = from.end(); itArc != endArc; ++itArc)
		    if (itArc->label != Fsa::Epsilon) {
			if (itTo != itFrom)
			    *itTo = from;
			++itTo;
			*itTargetSlotId = targetSlotId++;
			break;
		    }
	    }
	} else {
	    ScoreId posteriorId = (threshold != Core::Type<Score>::max) ?
		cnRef->normalizedProperties->posteriorId : Semiring::InvalidId;
	    for (; itFrom != endSlot; ++itFrom, ++itTargetSlotId) {
		const ConfusionNetwork::Slot &from = *itFrom;
		if ((from.front().label != Fsa::Epsilon)
		    || ((from.size() > 1)
			&& ((posteriorId == Semiring::InvalidId) || (from.front().scores->get(posteriorId) < threshold)))) {
		    if (itTo != itFrom)
			*itTo = from;
		    ++itTo;
		    *itTargetSlotId = targetSlotId++;
		}
	    }
	}
	cn.erase(itTo, cn.end());
	verify(cn.size() == targetSlotId);
	if (cn.hasMap()) {
	    ConfusionNetwork::MapProperties &map = const_cast<ConfusionNetwork::MapProperties&>(*cn.mapProperties);
	    std::vector<u32>::const_iterator itTargetSlotId = slotIdMapping.begin();
	    for (Core::Vector<Fsa::StateId>::iterator itSlotIndex = map.slotIndex.begin(), endSlotIndex = map.slotIndex.end();
		 itSlotIndex != endSlotIndex; ++itSlotIndex, ++itTargetSlotId)
		if (*itTargetSlotId != Fsa::InvalidStateId)
		    map.slotIndex[*itTargetSlotId] = *itSlotIndex;
	    map.slotIndex.erase(map.slotIndex.begin() + cn.size(), map.slotIndex.end());
	    for (ConfusionNetwork::MapProperties::Map::iterator itLat2Cn = map.lat2cn.begin(), endLat2Cn = map.lat2cn.end();
		 itLat2Cn != endLat2Cn; ++itLat2Cn) if (itLat2Cn->sid != Fsa::InvalidStateId) {
		    Fsa::StateId cnSid = slotIdMapping[itLat2Cn->sid];
		    if (cnSid == Fsa::InvalidStateId)
			itLat2Cn->sid = itLat2Cn->aid = Fsa::InvalidStateId;
		    else
			itLat2Cn->sid = cnSid;
		}
	}
    }
    // -------------------------------------------------------------------------


    // -------------------------------------------------------------------------
    void removeEpsSlots(ConstPosteriorCnRef cnRef, Score threshold) {
	if (!cnRef)
	    return;
	PosteriorCn &cn = const_cast<PosteriorCn&>(*cnRef);
	PosteriorCn::iterator itTo = cn.begin();
	for (PosteriorCn::iterator itFrom = cn.begin(), endSlot = cn.end(); itFrom != endSlot; ++itFrom) {
	    const PosteriorCn::Slot &from = *itFrom;
	    if ((from.front().label != Fsa::Epsilon)
		|| ((from.size() > 1)
		    && (from.front().score < threshold))) {
		if (itTo != itFrom)
		    *itTo = from;
		++itTo;
	    }
	}
	cn.erase(itTo, cn.end());
    }
    // -------------------------------------------------------------------------


    // -------------------------------------------------------------------------
    class CnPruningNode : public Node {
	typedef Node Precursor;
    public:
	static const Core::ParameterFloat paramThreshold;
	static const Core::ParameterInt paramMaxSlotSize;
	static const Core::ParameterBool paramNormalize;
	static const Core::ParameterBool paramRemoveEpsSlots;
    protected:
	bool prune_;
	Score threshold_;
	u32 maxSlotSize_;
	bool normalize_;
	bool rmEpsSlots_;
	Score epsSlotThreshold_;
    public:
	CnPruningNode(const std::string &name, const Core::Configuration &config) :
	    Node(name, config) {}
	virtual ~CnPruningNode() {}
	virtual void init(const std::vector<std::string> &arguments) {
	    threshold_ = paramThreshold(config);
	    verify(threshold_ > 0.0);
	    maxSlotSize_ = paramMaxSlotSize(config);
	    verify(maxSlotSize_ > 0);
	    normalize_ = paramNormalize(config);
	    prune_ = (threshold_ != Core::Type<Score>::max) || (maxSlotSize_ != Core::Type<u32>::max);
	    rmEpsSlots_ = paramRemoveEpsSlots(config);
	    epsSlotThreshold_ = paramThreshold(select("eps-slot-removal"));
	    Core::Component::Message msg = log();
	    if (prune_) {
		msg << "Prune";
		if (threshold_ != Core::Type<Score>::max)
		    msg << ", threshold = " << threshold_;
		if (maxSlotSize_ != Core::Type<u32>::max)
		    msg << ", max. slot size = " << maxSlotSize_;
		msg << "\n";
		if (normalize_)
		    msg << "Re-normalize slot-wise posterior prob. dist. after pruning.\n";
	    }
	    if (rmEpsSlots_) {
		msg << "Remove epsilon slots";
		if (epsSlotThreshold_ != Core::Type<Score>::max)
		    msg << ", threshold = " << epsSlotThreshold_;
		msg << "\n";
	    }
	}
    };
    const Core::ParameterFloat CnPruningNode::paramThreshold(
	"threshold",
	"probability threshold",
	Core::Type<Score>::max);
    const Core::ParameterInt CnPruningNode::paramMaxSlotSize(
	"max-slot-size",
	"max. slot size",
	Core::Type<u32>::max);
    const Core::ParameterBool CnPruningNode::paramNormalize(
	"normalize",
	"normalize",
	true);
    const Core::ParameterBool CnPruningNode::paramRemoveEpsSlots(
	"remove-eps-slots",
	"remove eps slots",
	false);

    class NormalizedCnPruningNode : public CnPruningNode {
	typedef CnPruningNode Precursor;
    public:
	NormalizedCnPruningNode(const std::string &name, const Core::Configuration &config) :
	    Precursor(name, config) {}
	virtual ~NormalizedCnPruningNode() {}

	virtual ConstConfusionNetworkRef sendCn(Port to) {
	    verify(connected(to));
	    ConstConfusionNetworkRef cn = requestCn(to);
	    if (cn) {
		if (prune_)
		    prune(cn, threshold_, maxSlotSize_, normalize_);
		if (rmEpsSlots_)
		    removeEpsSlots(cn, epsSlotThreshold_);
	    }
	    return cn;
	}
    };
    NodeRef createNormalizedCnPruningNode(const std::string &name, const Core::Configuration &config) {
	return NodeRef(new NormalizedCnPruningNode(name, config));
    }

    class PosteriorCnPruningNode : public CnPruningNode {
	typedef CnPruningNode Precursor;
    public:
	PosteriorCnPruningNode(const std::string &name, const Core::Configuration &config) :
	    Precursor(name, config) {}
	virtual ~PosteriorCnPruningNode() {}

	virtual ConstPosteriorCnRef sendPosteriorCn(Port to) {
	    verify(connected(to));
	    ConstPosteriorCnRef cn = requestPosteriorCn(to);
	    if (cn) {
		if (prune_)
		    prune(cn, threshold_, maxSlotSize_, normalize_);
		if (rmEpsSlots_)
		    removeEpsSlots(cn, epsSlotThreshold_);
	    }
	    return cn;
	}
    };
    NodeRef createPosteriorCnPruningNode(const std::string &name, const Core::Configuration &config) {
	return NodeRef(new PosteriorCnPruningNode(name, config));
    }
    // -------------------------------------------------------------------------

} // namespace Flf

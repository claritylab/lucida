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
#include <Fsa/Arithmetic.hh>
#include <Fsa/Best.hh>
#include <Fsa/Basic.hh>
#include <Fsa/Compose.hh>
#include <Fsa/Output.hh>
#include <Fsa/Prune.hh>
#include <Fsa/Rational.hh>
#include <Fsa/RemoveEpsilons.hh>
#include <Lattice/Utilities.hh>
#include <Lattice/Basic.hh>
#include <queue>
#include <Am/ClassicAcousticModel.hh>
#include <Speech/AllophoneStateGraphBuilder.hh>
#include <Search/Types.hh>
#include "Aligner.hh"

using namespace Search;
using Speech::Alignment;
using Speech::AlignmentItem;

class Aligner::SearchSpace : public Core::Component {
private:
    typedef Fsa::StaticAutomaton Model;
    Core::Ref<const Model> model_;
    bool viterbi_;
public:
    typedef Fsa::StateId StateId;
    typedef s32 StateHypothesisIndex;

private:
    typedef StateHypothesisIndex Trace;
    static const Trace invalidTrace = -1;
    struct StateHypothesis {
	StateId state;
	Score score;
	Trace trace;
	Am::AllophoneStateIndex emission;
	s32 nRefs; /**< number of references by traceback arcs in the following timeframe */
	StateHypothesis(StateId si, Score sc, Trace tr, Am::AllophoneStateIndex em) :
	    state(si), score(sc), trace(tr), emission(em), nRefs(0) {}
    };

    TimeframeIndex time_;
    typedef std::vector<StateHypothesis> StateHypothesisList;
    mutable StateHypothesisList stateHypotheses_;
    mutable StateHypothesisIndex firstHypCurrentFrame_;
    std::vector<StateHypothesisIndex> stateHypothesisForState_;
    mutable Fsa::StaticAutomaton *traceback_;
    Core::Ref<Fsa::StaticAutomaton> tracebackRef_;

    void activateOrUpdateStateHypothesisViterbi(StateId, Score, Trace, Am::AllophoneStateIndex);
    void activateOrUpdateStateHypothesisBaumWelch(StateId, Score, Trace, Am::AllophoneStateIndex, Fsa::Weight);
    void deleteStateHypothesisViterbi(StateHypothesisIndex shi);
    void deleteStateHypothesisBaumWelch(StateHypothesisIndex shi);
    Fsa::ConstAutomatonRef getAlignmentFsaViterbi() const;
    Fsa::ConstAutomatonRef getAlignmentFsaBaumWelch() const;

public:
    SearchSpace(const Core::Configuration&, bool viterbi);
    void setModel(Fsa::ConstAutomatonRef);
    void clear();
    void addStartupHypothesis();
    TimeframeIndex time() const { return time_; }
    u32 nStateHypotheses() const { return stateHypotheses_.size() - firstHypCurrentFrame_; }
    void expand(Core::Ref<const Am::AcousticModel> acousticModel,
		Mm::FeatureScorer::Scorer &emissionScores);
    Score minimumScore() const;
    Score score(StateHypothesisIndex shi) const { return stateHypotheses_[shi].score; }
    void prune(Score threshold);
    void collectGarbage() const;
    Score finalStatePotential() const;
    Fsa::ConstAutomatonRef getAlignmentFsa() const;
};

Aligner::SearchSpace::SearchSpace(const Core::Configuration &c, bool viterbi) :
    Core::Component(c), viterbi_(viterbi), time_(0),
    traceback_(new Fsa::StaticAutomaton), tracebackRef_(Core::ref(traceback_))
{}

void Aligner::SearchSpace::setModel(Fsa::ConstAutomatonRef m)
{
    require(m);
    model_ = Core::ref((const Model*)(Fsa::staticCopy(m).get()));
    stateHypothesisForState_.resize(model_->size());
    clear();
}

void Aligner::SearchSpace::clear() {
    stateHypotheses_.clear();
    firstHypCurrentFrame_ = 0;
    time_ = 0;
    if (!viterbi_) {
	traceback_->clear();
	traceback_->setInitialStateId(Fsa::InvalidStateId);
    }
}

inline void Aligner::SearchSpace::activateOrUpdateStateHypothesisViterbi(
    StateId state,
    Score score,
    Trace trace,
    Am::AllophoneStateIndex emission)
{
    require(state < stateHypothesisForState_.size());
    StateHypothesisIndex shi = stateHypothesisForState_[state];
    if (shi >= StateHypothesisIndex(stateHypotheses_.size()) ||
	shi < firstHypCurrentFrame_ ||
	stateHypotheses_[shi].state != state)
	{
	    stateHypothesisForState_[state] = stateHypotheses_.size();
	    stateHypotheses_.push_back(StateHypothesis(state, score, trace, emission));
	} else {
	require(shi < (StateHypothesisIndex)stateHypotheses_.size());
	StateHypothesis &sh(stateHypotheses_[shi]);
	if (sh.score >= score) {
	    sh.score = score;
	    require(sh.trace < (StateHypothesisIndex)stateHypotheses_.size());
	    require(trace < (StateHypothesisIndex)stateHypotheses_.size());
	    sh.trace = trace;
	    sh.emission = emission;
	}
    }
}

inline void Aligner::SearchSpace::activateOrUpdateStateHypothesisBaumWelch(
    StateId toState,
    Score toStatePotential,
    StateHypothesisIndex fromShi,
    Am::AllophoneStateIndex emission,
    Fsa::Weight arcScore)
{
    StateHypothesisIndex toShi = stateHypothesisForState_[toState];
    Fsa::State *fromShs = traceback_->fastState(fromShi);
    Fsa::State *toShs;
    StateHypothesisIndex nHypotheses = stateHypotheses_.size();
    if (toShi >= nHypotheses ||
	toShi < firstHypCurrentFrame_ ||
	stateHypotheses_[toShi].state != toState)
	{
	    toShs = new Fsa::State(nHypotheses);
	    traceback_->setState(toShs);
	    stateHypothesisForState_[toState] = nHypotheses;
	    stateHypotheses_.push_back(StateHypothesis(toState, toStatePotential, fromShi, emission)); // trace and emission unused
	} else {
	StateHypothesis &sh(stateHypotheses_[toShi]);
	sh.score = (Score)traceback_->semiring()->collect((Fsa::Weight)sh.score, (Fsa::Weight)toStatePotential);
	sh.trace = fromShi; // unused in baum-welch training
	sh.emission = emission; // also unused
	toShs = traceback_->fastState(toShi);
    }
    ++stateHypotheses_[fromShi].nRefs;
    toShs->newArc(fromShs->id(), arcScore, emission);
}

void Aligner::SearchSpace::addStartupHypothesis() {
    require_(model_);
    stateHypotheses_.push_back(StateHypothesis(model_->initialStateId(), 0.0, invalidTrace, invalidTrace));
    stateHypotheses_[0].nRefs = 1; // never delete the entry state hypothesis!
    stateHypothesisForState_[model_->initialStateId()] = 0;
    firstHypCurrentFrame_ = 0;
    if (!viterbi_) {
	traceback_->setType(Fsa::TypeAcceptor);
	traceback_->setSemiring(Fsa::LogSemiring);
	traceback_->setInputAlphabet(model_->getInputAlphabet());
	traceback_->addProperties(Fsa::PropertyAcyclic);
	Fsa::State *final = traceback_->newFinalState(traceback_->semiring()->one());
	verify(final->id() == 0);
    }
}

void Aligner::SearchSpace::expand(
    Core::Ref<const Am::AcousticModel> acousticModel,
    Mm::FeatureScorer::Scorer &emissionScores)
{
    require_(model_);
    StateHypothesisIndex firstHypPreviousFrame = firstHypCurrentFrame_;
    firstHypCurrentFrame_ = stateHypotheses_.size();
    for (StateHypothesisIndex shi = firstHypPreviousFrame; shi < firstHypCurrentFrame_; ++shi) {
	const StateHypothesis &sh(stateHypotheses_[shi]);
	const Fsa::State *s = model_->fastState(sh.state);
	const Score score = sh.score;
	for (Fsa::State::const_iterator a = s->begin(); a != s->end(); ++a) {
	    Score emissionScore = emissionScores->score(acousticModel->emissionIndex(a->input_));
	    Score transitionScore = Score(a->weight_);
	    if (viterbi_) {
		Score sco = score + transitionScore + emissionScore;
		activateOrUpdateStateHypothesisViterbi(a->target_, sco, shi, a->input_);
	    } else { // baum-welch
		Score arcScore = transitionScore + emissionScore;
		activateOrUpdateStateHypothesisBaumWelch(a->target_, score + arcScore, shi, a->input_, (Fsa::Weight)arcScore);
	    }
	}
    }
    if (viterbi_) {
	for (StateHypothesisIndex shi = firstHypCurrentFrame_; shi < (StateHypothesisIndex)stateHypotheses_.size(); ++shi)
	    ++stateHypotheses_[stateHypotheses_[shi].trace].nRefs;
    }
    ++time_;
}

Score Aligner::SearchSpace::minimumScore() const {
    Score result = Core::Type<Score>::max;
    for (StateHypothesisList::const_iterator sh = stateHypotheses_.begin() + + firstHypCurrentFrame_;
	 sh != stateHypotheses_.end(); ++sh)
	if (result > sh->score)
	    result = sh->score;
    return result;
}

void Aligner::SearchSpace::prune(Score threshold) {
    StateHypothesisList::iterator in, out, in_end;
    StateHypothesisIndex shi;
    in = out = stateHypotheses_.begin() + firstHypCurrentFrame_;
    in_end = stateHypotheses_.end();
    shi = firstHypCurrentFrame_;

    if (viterbi_) {
	for (; in != in_end; ++in, ++shi) {
	    if (in->score <= threshold) {
		*(out++) = *in;
	    } else {
		deleteStateHypothesisViterbi(shi);
	    }
	}
    } else { // baum-welch
	for (; in != in_end; ++in, ++shi) {
	    if (in->score <= threshold) {
		Fsa::StateId out_shi = out - stateHypotheses_.begin();
		Fsa::State *s = traceback_->fastState(shi);
		s->setId(out_shi);
		traceback_->setState(s);
		*(out++) = *in;
	    } else {
		deleteStateHypothesisBaumWelch(shi);
	    }
	}
    }
    stateHypotheses_.erase(out, stateHypotheses_.end());
}

inline void Aligner::SearchSpace::deleteStateHypothesisViterbi(StateHypothesisIndex shi) {
    while (--stateHypotheses_[shi].nRefs <= 0) {
	shi = stateHypotheses_[shi].trace;
    }
}

inline void Aligner::SearchSpace::deleteStateHypothesisBaumWelch(StateHypothesisIndex shi) {
    std::queue<StateHypothesisIndex> hyps;
    hyps.push(shi);
    while (!hyps.empty()) {
	shi = hyps.front(); hyps.pop();
	if (--(stateHypotheses_[shi].nRefs) <= 0) {
	    Fsa::State *s = traceback_->fastState(shi);
	    for (Fsa::State::const_iterator a = s->begin(); a != s->end(); ++a) {
		hyps.push(a->target());
	    }
	}
    }
}

void Aligner::SearchSpace::collectGarbage() const {
    StateHypothesisList::iterator in, out, in_end;
    StateHypothesisIndex shi_in, shi_out;
    in = out = stateHypotheses_.begin();
    in_end = stateHypotheses_.end();
    shi_in = shi_out = 0;
    if (viterbi_) {
	StateHypothesisIndex *map = new StateHypothesisIndex[stateHypotheses_.size()];
	for (; in != in_end; ++in, ++shi_in) {
	    if (in->nRefs > 0 || shi_in >= firstHypCurrentFrame_) {
		*(out++) = *in;
		map[shi_in] = shi_out;
		++shi_out;
	    }
	}
	firstHypCurrentFrame_ = (out - stateHypotheses_.begin()) - nStateHypotheses();
	stateHypotheses_.erase(out, stateHypotheses_.end());
	in = stateHypotheses_.begin()+1;
	in_end = stateHypotheses_.end();
	for (; in != in_end; ++in)
	    in->trace = map[in->trace];
	delete[] map;
    } else { // baum-welch
	for (StateHypothesisIndex shi = (StateHypothesisIndex)stateHypotheses_.size();
	     shi <= (StateHypothesisIndex)traceback_->maxStateId(); ++shi)
	    {
		traceback_->deleteState(shi);
	    }

	for (; in != in_end; ++in, ++shi_in) {
	    if (in->nRefs > 0 || shi_in >= firstHypCurrentFrame_) {
		*(out++) = *in;
	    } else {
		traceback_->deleteState(shi_in);
	    }
	}
	firstHypCurrentFrame_ = (out - stateHypotheses_.begin()) - nStateHypotheses();
	stateHypotheses_.erase(out, stateHypotheses_.end());
	traceback_->normalize();
	ensure(traceback_->maxStateId() == stateHypotheses_.size()-1);
    }
}

Score Aligner::SearchSpace::finalStatePotential() const {
    Score result = Core::Type<Score>::max;
    for (StateHypothesisIndex shi = firstHypCurrentFrame_; shi < (StateHypothesisIndex)stateHypotheses_.size(); ++shi) {
	const Fsa::State *s = model_->fastState(stateHypotheses_[shi].state);
	if (s->isFinal()) {
	    const Score score = (Score)s->weight_ + stateHypotheses_[shi].score;
	    if (viterbi_) {
		result = std::min(result, score);
	    } else { // baum-welch
		if (result == Core::Type<Score>::max) {
		    result = score;
		} else {
		    result = (Score)traceback_->semiring()->collect((Fsa::Weight)result, (Fsa::Weight)score);
		}
	    }
	}
    }
    return result;
}

Fsa::ConstAutomatonRef Aligner::SearchSpace::getAlignmentFsaViterbi() const {
    StateHypothesisIndex bestHyp = invalidTrace;
    Score bestScore = Core::Type<Score>::max;
    for (StateHypothesisIndex shi = firstHypCurrentFrame_; shi < (StateHypothesisIndex)stateHypotheses_.size(); ++shi) {
	const Fsa::State *s = model_->fastState(stateHypotheses_[shi].state);
	if (s->isFinal())
	    {
		const Score score = (Score)s->weight_ + stateHypotheses_[shi].score;
		if (score < bestScore) {
		    bestScore = score;
		    bestHyp = shi;
		}
	    }
    }
    Fsa::StaticAutomaton *result = new Fsa::StaticAutomaton();
    result->setType(Fsa::TypeAcceptor);
    result->setSemiring(Fsa::TropicalSemiring);
    result->setInputAlphabet(model_->getInputAlphabet());
    result->addProperties(Fsa::PropertyLinear | Fsa::PropertyAcyclic | Fsa::PropertySausages);
    if (bestScore == Core::Type<Score>::max) {
	//return empty automaton if alignment does not reach final state
	return Core::ref(result);
    }

    Fsa::State *s = new Fsa::State(time_);
    s->setFinal(result->semiring()->one());
    result->setState(s);
    StateHypothesisIndex shi = bestHyp;
    StateHypothesisIndex trace = stateHypotheses_[bestHyp].trace;
    for (TimeframeIndex time = time_; time > 0; --time) {
	ensure(trace != invalidTrace);
	s = new Fsa::State(time-1);
	s->newArc(time, Fsa::Weight(stateHypotheses_[shi].score - stateHypotheses_[trace].score), stateHypotheses_[shi].emission);
	result->setState(s);
	shi = trace;
	trace = stateHypotheses_[shi].trace;
    }
    result->setInitialStateId(0);
    return Fsa::ConstAutomatonRef(result);
}

Fsa::ConstAutomatonRef Aligner::SearchSpace::getAlignmentFsaBaumWelch() const {
    Fsa::State *final = traceback_->newState();
    traceback_->setInitialStateId(final->id());
    for (StateHypothesisIndex shi = firstHypCurrentFrame_; shi < (StateHypothesisIndex)stateHypotheses_.size(); ++shi) {
	const Fsa::State *s = model_->fastState(stateHypotheses_[shi].state);
	if (s->isFinal()) {
	    final->newArc(shi, s->weight_, Fsa::Epsilon);
	}
    }
    Fsa::ConstAutomatonRef result = Fsa::transpose(tracebackRef_);
    hope(result->hasProperty(Fsa::PropertyStorage | Fsa::PropertyAcyclic));
    // otherwise wrap "staticCopy" around transpose or use a non-lazy implementation of transpose
    traceback_->deleteState(final->id()); // undo changes in the traceback fsa to make this method 'const'
    return result;
}

Fsa::ConstAutomatonRef Aligner::SearchSpace::getAlignmentFsa() const {
    collectGarbage();
    if (viterbi_)
	return getAlignmentFsaViterbi();
    else
	return getAlignmentFsaBaumWelch();
}


// ================================================================================


const Core::ParameterFloat Aligner::paramMinAcousticPruningThreshold(
    "min-acoustic-pruning",
    "minimal threshold for pruning of state hypotheses",
    50, Core::Type<f32>::delta);

const Core::ParameterFloat Aligner::paramMaxAcousticPruningThreshold(
    "max-acoustic-pruning",
    "maximal threshold for pruning of state hypotheses",
    Core::Type<f32>::max, 0.0);

const Core::ParameterFloat Aligner::paramAcousticPruningThresholdIncrementFactor(
    "acoustic-pruning-increment-factor",
    "maximal threshold for pruning of state hypotheses",
    2, 1);

const Core::ParameterFloat Aligner::paramMinAverageNumberOfStateHypotheses(
    "min-average-number-of-states",
    "alignments with less average number of states per time-frame are repeated with a higher pruning",
    0, 0);

const Core::ParameterBool Aligner::paramIncreasePruningUntilNoScoreDifference(
    "increase-pruning-until-no-score-difference",
    "alignments are repeated with increasing pruning threshold until final score does not change",
    true);

const Core::ParameterBool Aligner::paramLogIterations(
    "log-iterations",
    "log number of iterations required for determining alignment",
    false);

const Core::Choice Aligner::choiceMode(
    "viterbi", modeViterbi,
    "baum-welch", modeBaumWelch,
    Core::Choice::endMark());

const Core::ParameterChoice Aligner::paramMode(
    "mode", &choiceMode, "describes how alignments are created and observations are weighted", modeViterbi);

const Core::ParameterFloat Aligner::paramMinWeight(
    "min-weight",
    "alignment items with a weight (i.e. state posterior probability) below this threshold are pruned",
    0.0);

const Core::ParameterBool Aligner::paramUsePartialSums(
    "use-partial-sums",
    "weigths of alignment items are not normalized over a timeframe",
    false);

Aligner::Aligner(const Core::Configuration &c) :
    Core::Component(c),
    ss_(0),
    nStateHypotheses_("active states after pruning"),
    statePosteriorStats_("state-posteriors"),
    statisticsChannel_(config, "statistics"),
    alignmentChannel_(config, "dump-alignment")
{
    minAcousticPruningThreshold_ = paramMinAcousticPruningThreshold(config);
    maxAcousticPruningThreshold_ = paramMaxAcousticPruningThreshold(config);
    acousticPruningThresholdIncrementFactor_ = paramAcousticPruningThresholdIncrementFactor(config);
    if ((minAcousticPruningThreshold_ > maxAcousticPruningThreshold_) ||
	(acousticPruningThresholdIncrementFactor_ <= 1 &&
	 minAcousticPruningThreshold_ != maxAcousticPruningThreshold_)) {
	criticalError() << "Inconsistent acoustic pruning settings:" << " min=" << minAcousticPruningThreshold_
			<< " max=" << maxAcousticPruningThreshold_
			<< " increment-factor=" << acousticPruningThresholdIncrementFactor_;
    }
    minAverageNumberOfStateHypotheses_ = paramMinAverageNumberOfStateHypotheses(config);
    increasePruningUntilNoScoreDifference_ = paramIncreasePruningUntilNoScoreDifference(config);
    logIterations_ = paramLogIterations(config);
    selectMode();
    minWeight_ = paramMinWeight(config);
    usePartialSums_ = paramUsePartialSums(config);
    ss_ = new SearchSpace(select("search"), viterbi_);
}

Aligner::~Aligner() {
    if (statisticsChannel_.isOpen())
	statePosteriorStats_.writeXml(statisticsChannel_);
    delete ss_;
}

Mm::Weight Aligner::getLogWeightThreshold() const {
    if (minWeight_ > Core::Type<Mm::Weight>::delta) {
	return -std::log(minWeight_);
    } else {
	return Core::Type<Mm::Weight>::max;
    }
}

void Aligner::setModel(
    Fsa::ConstAutomatonRef model,
    Core::Ref<const Am::AcousticModel> acousticModel)
{
    require(model);
    require(model->initialStateId() != Fsa::InvalidStateId);
    model_ = model;
    acousticModel_ = acousticModel;
    ss_->setModel(model_);
    ss_->addStartupHypothesis();
    nStateHypotheses_.clear();
    acousticPruningThreshold_ = maxAcousticPruningThreshold_;
}

void Aligner::restart() {
    ss_->clear();
    if (model_) ss_->addStartupHypothesis();
    nStateHypotheses_.clear();
}

void Aligner::feed(Mm::FeatureScorer::Scorer emissionScores) {
    require(model_);
    ss_->expand(acousticModel_, emissionScores);
    ss_->prune(ss_->minimumScore() + acousticPruningThreshold_);
    if (ss_->time() % 500 == 0)
	ss_->collectGarbage();
    nStateHypotheses_ += ss_->nStateHypotheses();
}

void Aligner::feed(const std::vector<Mm::FeatureScorer::Scorer> &emissionScorers) {
    Score previousScore = Core::Type<Score>::max;
    // make a copy of the feature scorers HERE to reuse the cache over all pruning iterations
    std::vector<Mm::FeatureScorer::Scorer> scorers = emissionScorers;
    nIterations_ = 0;
    for(Score t = minAcousticPruningThreshold_;
	t <= maxAcousticPruningThreshold_;
	t *= acousticPruningThresholdIncrementFactor_) {
	nIterations_++;
	restart();
	acousticPruningThreshold_ = t;
	std::vector<Mm::FeatureScorer::Scorer>::iterator s;
	for(s = scorers.begin(); s != scorers.end(); ++ s) feed(*s);
	if (acousticPruningThresholdIncrementFactor_ <= 1) break;
	Score score = alignmentScore();
	if (reachedFinalState() and nStateHypotheses_.average() >= minAverageNumberOfStateHypotheses_) {
	    if (!increasePruningUntilNoScoreDifference_) break;
	    if (t > minAcousticPruningThreshold_ and Core::isAlmostEqual(score, previousScore)) break;
	}
	previousScore = score;
    }
    if (logIterations_)
	log("required number of alignment iterations: ") << nIterations_;
}

Score Aligner::alignmentScore() const {
    return ss_->finalStatePotential();
}

void Aligner::selectMode(Mode mode)
{
    log("mode is set to '") << choiceMode[mode] << "'";
    viterbi_ = (mode == modeViterbi);
}

void Aligner::selectMode()
{
    selectMode((Mode)paramMode(config));
}

class AlignmentExtractor : public Fsa::DfsState
{
protected:
    TimeframeIndex time_;
    Alignment *alignment_;
public:
    AlignmentExtractor(Fsa::ConstAutomatonRef f) : Fsa::DfsState(f) {}
    virtual ~AlignmentExtractor() {}

    void exploreArc(Fsa::ConstStateRef from, const Fsa::Arc &a) {
	if (a.input() != Fsa::Epsilon) {
	    alignment_->push_back(AlignmentItem(time_, a.input(), a.weight()));
	    ++time_;
	}
    }
    virtual void exploreTreeArc(Fsa::ConstStateRef from, const Fsa::Arc &a) {
	exploreArc(from, a);
    }
    virtual void exploreNonTreeArc(Fsa::ConstStateRef from, const Fsa::Arc &a) {
	verify(color(a.target()) == Black); // verify, that the fsa contains no loops
	exploreArc(from, a);
    }
    virtual void finishArc(Fsa::ConstStateRef from, const Fsa::Arc &a) {
	if (a.input() != Fsa::Epsilon) --time_;
    }
    void extract(Alignment &alignment) {
	alignment.clear();
	fsa_ = Fsa::normalize(fsa_);
	time_ = 0;
	alignment_ = &alignment;
	recursiveDfs();
    }
};

void Aligner::getAlignment(
    Alignment &result,
    std::pair<Fsa::ConstAutomatonRef, Fsa::Weight> alignmentPosteriorFsa) const
{
    AlignmentExtractor ae(alignmentPosteriorFsa.first);
    ae.extract(result);
    if (!viterbi_) {
	result.combineItems(Fsa::LogSemiring);
	result.sortItems(false);
	result.clipWeights(0, Core::Type<Mm::Weight>::max);
	result.filterWeights(0, getLogWeightThreshold());
	if (!usePartialSums_) {
	    result.expm();
	    result.normalizeWeights();
	} else {
	    result.addWeight(-f32(alignmentPosteriorFsa.second));
	}
    } else {
	result.addWeight(1);
    }
    for (Alignment::const_iterator it = result.begin(); it != result.end(); ++it)
	statePosteriorStats_ += it->weight;
    result.setScore(alignmentScore());
    result.setAlphabet(acousticModel_->allophoneStateAlphabet());
}

void Aligner::getAlignment(Alignment &result) const {
    getAlignment(result, getAlignmentPosteriorFsa(getAlignmentFsa()));
}

Fsa::ConstAutomatonRef Aligner::getAlignmentFsa() const {
    Fsa::ConstAutomatonRef result = ss_->getAlignmentFsa();
    if (result->initialStateId() == Fsa::InvalidStateId)
	warning("Alignment did not reach any final state.");
    if (statisticsChannel_.isOpen()) {
	statisticsChannel_
	    << Core::XmlOpen("alignment-statistics")
	    << Core::XmlFull("frames", ss_->time())
	    << Core::XmlFull("acoustic-pruning-threshold", acousticPruningThreshold_)
	    << Core::XmlOpen("score")
	    << Core::XmlFull("avg", ss_->finalStatePotential() / f32(ss_->time()))
	    << Core::XmlFull("total", ss_->finalStatePotential())
	    << Core::XmlClose("score")
	    << nStateHypotheses_
	    << Core::XmlClose("alignment-statistics");
    }
    if (alignmentChannel_.isOpen()) {
	Fsa::info(result, alignmentChannel_);
	Fsa::drawDot(result, "/tmp/alignment.dot");
	Fsa::write(result, "bin:/tmp/alignment.binfsa.gz");
    }

    return result;
}

std::pair<Fsa::ConstAutomatonRef, Fsa::Weight> Aligner::getAlignmentPosteriorFsa(
    Fsa::ConstAutomatonRef alignmentFsa) const
{
    if (alignmentFsa->initialStateId() == Fsa::InvalidStateId) {
	return std::make_pair(alignmentFsa, alignmentFsa->semiring()->one());
    }
    Fsa::ConstAutomatonRef result;
    Fsa::Weight totalInv = alignmentFsa->semiring()->one();
    if (viterbi_) {
	//	result = Fsa::extend(Fsa::multiply(alignmentFsa, (Fsa::Weight)0.0), (Fsa::Weight)1.0);
	result = Fsa::multiply(alignmentFsa, (Fsa::Weight)0.0);
    } else { // baum-welch
	result = Fsa::posterior64(
	    Fsa::prunePosterior(
		alignmentFsa,
		Fsa::Weight(getLogWeightThreshold()),
		false),
	    totalInv);
	result = Fsa::trim(result);
	result = Fsa::normalize(result);
    }
    if (alignmentChannel_.isOpen()) {
	Fsa::info(result, alignmentChannel_);
	Fsa::drawDot(result, "/tmp/alignment-posterior.dot");
	Fsa::write(result, "bin:/tmp/alignment-posterior.binfsa.gz");
    }
    return std::make_pair(result, totalInv);
}

// ================================================================================

class Aligner::WordLatticeBuilder::AddWordBoundaryDisambiguatorsAutomaton : public Fsa::SlaveAutomaton
{
    typedef Fsa::SlaveAutomaton Precursor;
protected:
    mutable ConstStateRef lastState_;
    Fsa::LabelId disambiguator_;
    Fsa::Weight one_;
    Fsa::StateId mapStateId(Fsa::StateId id) const {
	if (id == Fsa::InvalidStateId) return id;
	return ((id & Fsa::StateIdMask) * 2) | (id & ~Fsa::StateIdMask);
    }
    Fsa::StateId mapStateIdInverse(Fsa::StateId id) const {
	if (id == Fsa::InvalidStateId) return id;
	return ((id & Fsa::StateIdMask) / 2) | (id & ~Fsa::StateIdMask);
    }
    bool isDisambiguatorState(Fsa::StateId id) const {
	return (id % 2 == 1);
    }

public:
    AddWordBoundaryDisambiguatorsAutomaton(Fsa::ConstAutomatonRef f, Fsa::LabelId disambiguator) :
	Precursor(f), disambiguator_(disambiguator), one_(f->semiring()->one()) {};

    virtual Fsa::StateId initialStateId() const {
	return mapStateId(fsa_->initialStateId());
    }

    virtual ConstStateRef getState(Fsa::StateId s) const {
	Fsa::State *sp;
	if (lastState_->refCount() == 1) {
	    sp = const_cast<State*>(lastState_.get());
	    *sp = *Precursor::fsa_->getState(mapStateIdInverse(s));
	} else {
	    sp = new State(*Precursor::fsa_->getState(mapStateIdInverse(s)));
	    lastState_ = ConstStateRef(sp);
	}
	sp->setId(s);
	for (Fsa::State::iterator a = sp->begin(); a != sp->end(); ++a) {
	    a->target_ = mapStateId(a->target());
	}
	if (!isDisambiguatorState(s)) {
	    sp->newArc(s+1, one_, disambiguator_);
	}
	return lastState_;
    }

    virtual std::string describe() const {
	return std::string("add-word-boundary-disambiguators(") + fsa_->describe() + ")";
    }
};


class Aligner::WordLatticeBuilder::Converter : public Fsa::DfsState
{
    typedef Fsa::DfsState Precursor;
private:
    Fsa::StaticAutomaton *t_;                      /**< lattice acoustic fsa to build */
    Lattice::WordBoundaries *wordBoundaries_;      /**< word boundaries to build */
    Am::ConstAllophoneStateAlphabetRef allophoneStateAlphabet_;

    struct StateInfo {
	TimeframeIndex time_;			   /**< current timeframe */
	Fsa::LabelId   wordLabelId_;		   /**< label of the current word */
	Fsa::LabelId   lastAllophone_;		   /**< last allophone found on the path */

	void reset(TimeframeIndex time) {
	    time_ = time;
	    wordLabelId_ = Fsa::InvalidLabelId;
	    lastAllophone_ = Fsa::InvalidLabelId;
	}
    };
    typedef Core::Vector<StateInfo> StateInfos;
    StateInfos stateInfos_;

protected:
    Fsa::State* getState(Fsa::StateId id) {
	require(id >= 0);
	Fsa::StateRef sp = t_->state(id);
	if (sp) {
	    return sp.get();
	} else {
	    Fsa::State *s = new Fsa::State(id);
	    t_->setState(s);
	    Fsa::ConstStateRef orig = fsa_->getState(id);
	    if (orig->isFinal()) {
		s->setFinal(orig->weight());
	    }
	    return s;
	}
    }

    void setCoarticulatedWordBoundary(Fsa::StateId s, TimeframeIndex time, Fsa::LabelId lastAllophone) {
	Lattice::WordBoundary::Transit transit;
	if (lastAllophone != Fsa::InvalidLabelId) {
	    Am::Phonology::SemiContext future =
		allophoneStateAlphabet_->allophoneState(lastAllophone).allophone()->future();
	    if (future.size()) {
		transit.initial = allophoneStateAlphabet_->allophoneState(lastAllophone).allophone()->central();
		transit.final = future[0];
	    }
	}
	wordBoundaries_->set(s, Lattice::WordBoundary(time, transit));
    }

    void exploreArc(Fsa::ConstStateRef fromState, const Fsa::Arc &a) {
	Fsa::StateId from = fromState->id();
	Fsa::StateId to = a.target();
	stateInfos_.grow(to);
	StateInfo &fromInfo = stateInfos_[from];
	StateInfo &toInfo = stateInfos_[to];
	Fsa::State *s = getState(from); getState(to);

	if (a.output_ != Fsa::Epsilon) {
	    toInfo.wordLabelId_ = a.output_;
	} else {
	    toInfo.wordLabelId_ = fromInfo.wordLabelId_;
	}

	if (a.input() == Fsa::Epsilon) {
	    toInfo.lastAllophone_ = fromInfo.lastAllophone_;
	    toInfo.time_ = fromInfo.time_;
	    s->newArc(to, a.weight(), Fsa::Epsilon);

	} else if (allophoneStateAlphabet_->isDisambiguator(a.input())) {
	    s->newArc(to, a.weight(), toInfo.wordLabelId_);
	    setCoarticulatedWordBoundary(a.target(), fromInfo.time_, fromInfo.lastAllophone_);
	    toInfo.reset(fromInfo.time_);

	} else {
	    toInfo.lastAllophone_ = a.input_;
	    toInfo.time_ = fromInfo.time_ + 1;
	    s->newArc(to, a.weight(), Fsa::Epsilon);
	}
    }

    virtual void exploreTreeArc(Fsa::ConstStateRef from, const Fsa::Arc &a) {
	exploreArc(from, a);
    }

    virtual void exploreNonTreeArc(Fsa::ConstStateRef from, const Fsa::Arc &a) {
	verify(color(a.target()) == Black); // no loops allowed
	exploreArc(from, a);
    }

public:
    Converter(Fsa::ConstAutomatonRef allophoneToLemmaPronunciationTransducer,
	      Am::ConstAllophoneStateAlphabetRef allophoneStateAlphabet) :
	Precursor(allophoneToLemmaPronunciationTransducer),
	t_(new Fsa::StaticAutomaton()),
	wordBoundaries_(new Lattice::WordBoundaries()),
	allophoneStateAlphabet_(allophoneStateAlphabet)
	{};

    Lattice::ConstWordLatticeRef build() {
	t_->setType(Fsa::TypeAcceptor);
	t_->addProperties(Fsa::PropertyAcyclic);
	t_->setSemiring(fsa_->semiring());
	t_->setInputAlphabet(fsa_->getOutputAlphabet());

	Fsa::StateId initial = fsa_->initialStateId();
	require(initial >= 0);

	t_->setInitialStateId(initial);
	StateInfo si; si.reset(0);
	stateInfos_.grow(initial);
	stateInfos_[initial] = si;
	setCoarticulatedWordBoundary(initial, si.time_, si.lastAllophone_);

	dfs();

	Fsa::ConstAutomatonRef t = Fsa::removeEpsilons(Core::ref(t_));
	Lattice::WordLattice *wordLattice = new Lattice::WordLattice;
	wordLattice->setWordBoundaries(Core::ref(wordBoundaries_));
	wordLattice->setFsa(Fsa::ConstAutomatonRef(t), Lattice::WordLattice::acousticFsa);

	Lattice::ConstWordLatticeRef result =
	    Lattice::normalize(Lattice::ConstWordLatticeRef(wordLattice));
	/** the following 2 lines are a workaround for the current Lattice::normalize procedure
	 *  which produces correct new word boundaries only if all fsa states are accessed afterwards
	 *  with 'Fsa::getState'
	 */
	result->part(Lattice::WordLattice::acousticFsa) =
	    Fsa::ConstAutomatonRef(Fsa::staticCopy(result->part(Lattice::WordLattice::acousticFsa)));
	return result;
    }
};


Aligner::WordLatticeBuilder::WordLatticeBuilder(
    const Core::Configuration &configuration,
    Bliss::LexiconRef lexicon,
    Core::Ref<const Am::AcousticModel> acousticModel) :
    Core::Component(configuration),
    lexicon_(lexicon),
    acousticModel_(acousticModel),
    dumpAutomaton_(configuration, "dump-automaton")
{
    lemmaPronunciationAlphabet_ = lexicon_->lemmaPronunciationAlphabet();
    allophoneStateAlphabet_ = acousticModel_->allophoneStateAlphabet();
}

void Aligner::WordLatticeBuilder::setModelTransducer(Fsa::ConstAutomatonRef modelTransducer)
{
    require(modelTransducer);
    allophoneToLemmaPronunciationTransducer_ = modelTransducer;
}

Fsa::ConstAutomatonRef Aligner::WordLatticeBuilder::addWordBoundaryDisambiguators(Fsa::ConstAutomatonRef f) const
{
    return Fsa::ConstAutomatonRef(new AddWordBoundaryDisambiguatorsAutomaton(f, allophoneStateAlphabet_->disambiguator(0)));
}


Fsa::ConstAutomatonRef Aligner::WordLatticeBuilder::buildAlignmentToLemmaPronunciationTransducer(
    Fsa::ConstAutomatonRef alignmentFsaWithDisambiguators) const
{
    return
	Fsa::trim(
	    Fsa::composeMatching(alignmentFsaWithDisambiguators, Fsa::multiply(allophoneToLemmaPronunciationTransducer_, Fsa::Weight(0.0)))
	    );
}


Lattice::ConstWordLatticeRef Aligner::WordLatticeBuilder::convertToWordLattice(
    Fsa::ConstAutomatonRef alignmentToLemmaPronunciationTransducer) const
{

    Converter transducerToLattice(alignmentToLemmaPronunciationTransducer, allophoneStateAlphabet_);
    return transducerToLattice.build();
}


Lattice::ConstWordLatticeRef Aligner::WordLatticeBuilder::build(Fsa::ConstAutomatonRef alignmentFsa)
{
    if (alignmentFsa->initialStateId() == Fsa::InvalidStateId) {
	error("Cannot generate word lattice because alignment did not reach a final state.");
	return Lattice::ConstWordLatticeRef();
    }

    Fsa::ConstAutomatonRef f = Fsa::staticCopy(
	Fsa::changeSemiring(addWordBoundaryDisambiguators(alignmentFsa), Fsa::TropicalSemiring));
    require(f);
    if (dumpAutomaton_.isOpen()) {
	Fsa::info(f, dumpAutomaton_);
	Fsa::drawDot(f, "/tmp/alignment-with-disambiguators.dot");
	Fsa::write(f, "bin:/tmp/alignment-with-disambiguators.binfsa.gz", Fsa::storeStates);
    }

    f = buildAlignmentToLemmaPronunciationTransducer(f);
    require(f);
    if (dumpAutomaton_.isOpen()) {
	Fsa::info(f, dumpAutomaton_);
	Fsa::drawDot(f, "/tmp/alignment-to-lemma-pronunciation-transducer.dot");
	Fsa::write(f, "bin:/tmp/alignment-to-lemma-pronunciation-transducer.binfsa.gz", Fsa::storeStates);
    }

    f = Fsa::best(f); /* <-- you may remove this line to create non-linear lattices from baum-welch
		       * alignments. The drawback is that even in viterbi mode the resulting lattice may
		       * become very large due to redundant parallel silence arcs. A silence segment of length n
		       * induces n(n+1)/2 silence word arcs! Possible solution: disallow consecutive
		       * silence words in the lemmaPronunciation acceptors.
		       */

    require(f);
    if (dumpAutomaton_.isOpen()) {
	Fsa::info(f, dumpAutomaton_);
	Fsa::drawDot(f, "/tmp/best-alignment-to-lemma-pronunciation-path.dot");
	Fsa::write(f, "bin:/tmp/best-alignment-to-lemma-pronunciation-path.binfsa.gz", Fsa::storeStates);
    }

    Lattice::ConstWordLatticeRef result = convertToWordLattice(f);
    require(result);
    if (dumpAutomaton_.isOpen()) {
	Fsa::info(result->part(Lattice::WordLattice::acousticFsa), dumpAutomaton_);
	Fsa::drawDot(result->part(Lattice::WordLattice::acousticFsa), "/tmp/lattice.dot");
	Lattice::dumpWordBoundaries(result->wordBoundaries(), "/tmp/lattice-word-boundaries.xml");
    }

    return result;
}


// ================================================================================


WordSequenceAligner::WordSequenceAligner(
    const Core::Configuration &c,
    Core::Ref<const Bliss::Lexicon> lexicon,
    Core::Ref<const Am::AcousticModel> acousticModel)
    :
    Core::Component(c),
    acousticModel_(acousticModel),
    lexicon_(lexicon),
    modelBuilder_(0),
    aligner_(c),
    alignmentChannel_(config, "alignment-fsa")
{
    modelBuilder_ = new Speech::AllophoneStateGraphBuilder(config, lexicon, acousticModel_);
}

WordSequenceAligner::~WordSequenceAligner() {
    delete modelBuilder_;
}

Alignment WordSequenceAligner::align(
    const Mm::FeatureScorer& featureScorer,
    const std::vector<Core::Ref<const Mm::Feature> >& featureSequence)
{
    std::vector<Core::Ref<const Mm::Feature> >::const_iterator p;
    std::vector<Mm::FeatureScorer::Scorer> scorers;
    for (p = featureSequence.begin(); p != featureSequence.end(); ++p)
	scorers.push_back(featureScorer.getScorer(*p));
    aligner_.feed(scorers);
    aligner_.getAlignment(alignment_);
    if (alignmentChannel_.isOpen())
	alignment_.writeXml(alignmentChannel_);
    return alignment_;
}

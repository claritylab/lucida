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
#include "RescoreLm.hh"
#include <Core/Application.hh>
#include <Core/Choice.hh>
#include <Core/Parameter.hh>

#include "FlfCore/Basic.hh"
#include "Convert.hh"
#include "Copy.hh"
#include <Lm/ScaledLanguageModel.hh>
#include <Lm/Module.hh>

namespace Flf
{
struct WordEndHypothesis {
    Lm::History h;
    Score score;

    Fsa::StateId preState;
    Flf::Arc arc;
};

template <class A, class B>
struct CompareSecond {
    bool operator()(const std::pair<A, B> &a, const std::pair<A, B> &b) const {
	return (a.second < b.second);
    }
};

template <class Key>
struct StandardHash {
    inline u32 operator()(Key a) const {
	a = (a ^ 0xc761c23c) ^ (a >> 19);
	a = (a + 0xfd7046c5) + (a << 3);
	return a;
    }
};

struct HistoryLemmaPairHash {
    std::size_t operator()(const std::pair<Lm::HistoryHandle, Bliss::Lemma::Id>& item) const {
	return reinterpret_cast<size_t>(item.first) + StandardHash<std::size_t>()(item.second + reinterpret_cast<size_t>(item.first));
    }
};

// Expands the incoming lattice. The ideal structure for the incoming lattice is a mesh.
// If no LM is given, then only the transits are expanded.
// The word end beam is relative to the LM scale.
ConstLatticeRef decodeRescoreLm(ConstLatticeRef lat, Core::Ref<Lm::LanguageModel> lm, float wordEndBeam, u32 wordEndLimit, const std::vector<const Bliss::Lemma*>& prefix, const std::vector<const Bliss::Lemma*>& suffix) {
    verify(lat->getBoundaries()->valid());
    lat = sortByTopologicalOrder(lat);

    LexiconRef lexicon = Lexicon::us();

    Core::Ref<const Bliss::LemmaAlphabet> lAlphabet;
    Core::Ref<const Bliss::LemmaPronunciationAlphabet> lpAlphabet;

    switch (lexicon->alphabetId(lat->getInputAlphabet())) {
	case Lexicon::LemmaAlphabetId:
	    lAlphabet = Lexicon::us()->lemmaAlphabet();
	    break;
	case Lexicon::LemmaPronunciationAlphabetId:
	    lpAlphabet = Lexicon::us()->lemmaPronunciationAlphabet();
	    break;
	default:
	    defect();
    }

    if (dynamic_cast<Lm::ScaledLanguageModel*>(lm.get()))
	lm = Core::Ref<Lm::LanguageModel>( const_cast<Lm::LanguageModel*>( dynamic_cast<Lm::ScaledLanguageModel*>(lm.get())->unscaled().get() ) );

    ConstSemiringRef semiring = lat->semiring();

    StaticBoundaries *b = new StaticBoundaries;
    StaticLattice *s = new StaticLattice(Fsa::TypeAcceptor);
    s->setProperties(lat->knownProperties(), lat->properties());
    s->setInputAlphabet(lat->getInputAlphabet());
    s->setSemiring(semiring);
    s->setBoundaries(ConstBoundariesRef(b));

    ScoreId lmScoreId = semiring->id("lm");

    float lmScale = semiring->scale(lmScoreId);

    std::cout << "decoding lattice with LM scale " << lmScale << " prefix " << prefix.size() << " suffix " << suffix.size() << std::endl;

    wordEndBeam *= lmScale;

    const u32 maxCacheSize = 10000;

    std::vector<std::pair<Fsa::StateId, Lm::History> > appendFinalState;

    typedef Core::hash_map<std::pair<Lm::HistoryHandle, Bliss::Lemma::Id>, std::pair<Score, Lm::History>, HistoryLemmaPairHash> LmCache;
    LmCache lmCache;

    typedef std::multimap<std::pair<Speech::TimeframeIndex, Fsa::StateId>, WordEndHypothesis> Hypotheses;
    Hypotheses hypotheses;

    {
	WordEndHypothesis initialHyp;
	initialHyp.h = lm->startHistory();
	initialHyp.score = 0;
	initialHyp.arc = Arc(Fsa::InvalidStateId, ScoresRef(), Fsa::Epsilon, Fsa::Epsilon);
	for (u32 i = 0; i < prefix.size(); ++i)
	    Lm::addLemmaScore(lm, lmScale, prefix[i], lmScale, initialHyp.h, initialHyp.score);
	initialHyp.preState = Fsa::InvalidStateId;
	hypotheses.insert(std::make_pair(std::make_pair(lat->boundary(lat->initialStateId()).time(), lat->initialStateId()), initialHyp));
    }

    Speech::TimeframeIndex lastPrunedTimeframe = -1;

    while (!hypotheses.empty())
    {
	Speech::TimeframeIndex time = hypotheses.begin()->first.first;

	if (time != lastPrunedTimeframe)
	{
	    // Step 1: Prune hypotheses for timeframe
	    Hypotheses::iterator frameBegin = hypotheses.begin();
	    Hypotheses::iterator frameEnd = hypotheses.upper_bound(std::make_pair(time, Core::Type<Fsa::StateId>::max));
	    Score best = Core::Type<Score>::max;
	    for (Hypotheses::iterator hypIt = frameBegin; hypIt != frameEnd; ++hypIt)
	    {
		if (hypIt->second.score < best)
		    best = hypIt->second.score;
	    }
	    u32 count = 0, pruned = 0;
	    for (Hypotheses::iterator hypIt = frameBegin; hypIt != frameEnd; )
	    {
		if (hypIt->second.score > best + wordEndBeam) {
		    Hypotheses::iterator removeIt = hypIt;
		    ++hypIt;
		    ++pruned;
		    hypotheses.erase(removeIt);
		}else{
		    ++hypIt;
		    ++count;
		}
	    }
//              std::cout << "pruned " << pruned << " remaining " << count << std::endl;
	    // The first element may have been deleted
	    frameBegin = hypotheses.begin();
	    if (count > wordEndLimit)
	    {
		std::vector<std::pair<Hypotheses::iterator, Score> > scores;
		for (Hypotheses::iterator hypIt = frameBegin; hypIt != frameEnd; ++hypIt)
		    scores.push_back(std::make_pair(hypIt, hypIt->second.score));

		std::nth_element(scores.begin(), scores.begin() + wordEndLimit, scores.end(), CompareSecond<Hypotheses::iterator, Score>());
		for (std::vector<std::pair<Hypotheses::iterator, Score> >::const_iterator deleteIt = scores.begin() + wordEndLimit; deleteIt != scores.end(); ++deleteIt)
		    hypotheses.erase(deleteIt->first);
	    }
	    frameBegin = hypotheses.begin();
	    lastPrunedTimeframe = time;
	}

	verify(!hypotheses.empty());

	Fsa::StateId stateId = hypotheses.begin()->first.second;
	ConstStateRef state = lat->getState(stateId);

	Hypotheses::iterator stateEnd = hypotheses.upper_bound(std::make_pair(time, stateId));

	// Step 2: Recombine and build (create a state for each each distinct history, and build the corresponding incoming arcs)
	std::multimap<Lm::History, WordEndHypothesis> historyHyps;
	for (Hypotheses::iterator hypIt = hypotheses.begin(); hypIt != stateEnd; ++hypIt)
	    historyHyps.insert(std::make_pair(hypIt->second.h, hypIt->second));

	while (!historyHyps.empty())
	{
	    Lm::History history = historyHyps.begin()->first;
	    std::multimap<Lm::History, WordEndHypothesis>::iterator endIt = historyHyps.upper_bound(historyHyps.begin()->first);

	    State* newState = s->newState(state->tags());     //, state->weight());
	    b->set(newState->id(), lat->boundary(stateId));
	    Score best = Core::Type<Score>::max;

	    if (newState->isFinal())      //&& newState->weight().get())
	    {
		bool hadSentenceEnd = false;
		if (historyHyps.begin()->second.preState != Fsa::InvalidStateId)
		{
		    Fsa::LabelId labelId = historyHyps.begin()->second.arc.input();

		    if (Fsa::FirstLabelId <= labelId && labelId <= Fsa::LastLabelId)
		    {
			const Bliss::Lemma *lemma = (lAlphabet) ?
						    lAlphabet->lemma(labelId) :
						    lpAlphabet->lemmaPronunciation(labelId)->lemma();
			verify(lemma);
			if (lemma->hasSyntacticTokenSequence() && lemma->syntacticTokenSequence().size() && lemma->syntacticTokenSequence().operator[](lemma->syntacticTokenSequence().size() - 1) == lm->sentenceEndToken())
			{
			    hadSentenceEnd = true;
			}
		    }
		}

		if (!hadSentenceEnd)
		{
		    appendFinalState.push_back(std::make_pair(newState->id(), history));
		    newState->setTags(newState->tags() & ~Fsa::StateTagFinal);
		}
//                  newState->weight() = semiring->clone(newState->weight());
//                  newState->weight()->set(lmScoreId, lm->sentenceEndScore(history));
	    }

	    for (std::multimap<Lm::History, WordEndHypothesis>::const_iterator hypIt = historyHyps.begin();
		 hypIt != endIt; ++hypIt)
	    {
		if (hypIt->second.score < best)
		    best = hypIt->second.score;

		if (hypIt->second.preState != Fsa::InvalidStateId)
		    const_cast<State&>(*s->getState(hypIt->second.preState)).newArc(newState->id(), hypIt->second.arc.weight(), hypIt->second.arc.input(), hypIt->second.arc.output());
	    }

	    // Step 3: Create successor hypotheses
	    for (u32 arcI = 0; arcI < state->nArcs(); ++arcI)
	    {
		const Arc* arc = state->getArc(arcI);
		WordEndHypothesis nextHyp;
		nextHyp.h = history;
		nextHyp.score = best;
		nextHyp.preState = newState->id();
		nextHyp.arc = *arc;
		nextHyp.arc.setWeight(semiring->clone(nextHyp.arc.weight()));

		Fsa::LabelId labelId = arc->input();

		if (Fsa::FirstLabelId <= labelId && labelId <= Fsa::LastLabelId)
		{
		    const Bliss::Lemma *lemma = (lAlphabet) ?
						lAlphabet->lemma(labelId) :
						lpAlphabet->lemmaPronunciation(labelId)->lemma();
		    verify(lemma);

		    Bliss::Lemma::Id id = lemma->id();
		    LmCache::const_iterator lmCacheIt = lmCache.find(std::make_pair(history.handle(), id));

		    if (lmCacheIt != lmCache.end())
		    {
			nextHyp.h = lmCacheIt->second.second;
			nextHyp.arc.setScore(lmScoreId, lmCacheIt->second.first);
		    }else{
			Score rawScore = 0;
			verify(nextHyp.h.isValid());
			Lm::addLemmaScore(lm, 1.0, lemma, 1.0, nextHyp.h, rawScore);
			nextHyp.arc.setScore(lmScoreId, rawScore);
			lmCache.insert(std::make_pair(std::make_pair(history.handle(), id), std::make_pair(rawScore, nextHyp.h)));
		    }
		}else{
		    nextHyp.arc.setScore(lmScoreId, 0);
		}
		nextHyp.score += semiring->project(nextHyp.arc.weight());
		hypotheses.insert(std::make_pair(std::make_pair(lat->boundary(arc->target()).time(), arc->target()), nextHyp));
	    }

	    historyHyps.erase(historyHyps.begin(), endIt);
	}

	hypotheses.erase(hypotheses.begin(), hypotheses.upper_bound(std::make_pair(time, stateId)));

	if (lmCache.size() > maxCacheSize)
	    lmCache.clear();
    }

    s->setInitialStateId(lat->initialStateId());

    if (appendFinalState.size())
    {
	State* finalState = s->newState(Fsa::StateTagFinal);
	Flf::Boundary finalB = s->boundary(appendFinalState.front().first);
	finalB.setTime(finalB.time() + 1);   // Be consistent with the recognizer: It also appends a length-1 epsilon arc
	std::cout << "decoding creating final state for " << appendFinalState.size() << " predecessor states, time " << finalB.time() << std::endl;
	b->set(finalState->id(), finalB);
	for (std::vector<std::pair<Fsa::StateId, Lm::History> >::const_iterator it = appendFinalState.begin();
	     it != appendFinalState.end(); ++it)
	{
	    Flf::Arc* finalArc = const_cast<State&>(*s->getState(it->first)).newArc();
	    finalArc->setTarget(finalState->id());
	    finalArc->setInput(Fsa::Epsilon);
	    finalArc->setOutput(Fsa::Epsilon);
	    finalArc->setWeight(semiring->clone(semiring->defaultWeight()));

	    Lm::History h = it->second;
	    Flf::Score s = 0;

	    for (u32 i = 0; i < suffix.size(); ++i)
		Lm::addLemmaScore(lm, 1.0, suffix[i], 1.0, h, s);

	    s += lm->sentenceEndScore(h);

	    finalArc->setScore(lmScoreId, s);
	}
    }

    s->setDescription("decode_rescore(" + lat->describe() + ")");

    std::cout << "ready" << std::endl;

    // Last step: Trim the resulting lattice, removing dead-end paths
    StaticLatticeRef createdLattice(s);
    trimInPlace(createdLattice);
    return copyBoundaries(createdLattice);
}

// -------------------------------------------------------------------------
class DecodeRescoreLmNode : public FilterNode
{
public:
    static const Core::ParameterFloat paramWordEndBeam;
    static const Core::ParameterInt paramWordEndLimit;
private:
    ConstLatticeRef latL_;
    f32 wordEndBeam_;
    u32 wordEndLimit_;
    Core::Ref<Lm::LanguageModel> lm_;
protected:
    ConstLatticeRef filter(ConstLatticeRef l) {
	if (!l)
	    return ConstLatticeRef();
	if (!latL_)
	    latL_ = decodeRescoreLm(l, lm_, wordEndBeam_, wordEndLimit_);

	return latL_;
    }
public:
    DecodeRescoreLmNode(const std::string &name, const Core::Configuration &config) :
	FilterNode(name, config) {}
    virtual ~DecodeRescoreLmNode() {}

    virtual void init(const std::vector<std::string> &arguments) {
	wordEndBeam_ = paramWordEndBeam(config);
	wordEndLimit_ = paramWordEndLimit(config);
	log() << "using word end beam " << wordEndBeam_;

	lm_ = Lm::Module::instance().createLanguageModel(select("lm"), Lexicon::us());
	if (!lm_)
	    criticalError("DecodeRescoreLmNode: failed to load language model");
    }

    virtual void sync() {
	latL_.reset();
    }
};
const Core::ParameterFloat DecodeRescoreLmNode::paramWordEndBeam(
    "word-end-beam",
    "word end beam, relative to LM scale (the default is huge)",
    20.0,
    0.0);

const Core::ParameterInt DecodeRescoreLmNode::paramWordEndLimit(
    "word-end-limit",
    "maximum number of allowed word end hypotheses per timeframe",
    50000,
    1);

NodeRef createDecodeRescoreLmNode(const std::string &name, const Core::Configuration &config) {
    return NodeRef(new DecodeRescoreLmNode(name, config));
}
// -------------------------------------------------------------------------
}

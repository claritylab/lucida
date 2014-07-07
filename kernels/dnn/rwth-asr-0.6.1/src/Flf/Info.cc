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
#include <Core/Utility.hh>

#include "FlfCore/Basic.hh"
#include "FlfCore/Ftl.hh"
#include "Copy.hh"
#include "FwdBwd.hh"
#include "Info.hh"
#include "Lexicon.hh"
#include "Map.hh"


namespace Flf {

    // -------------------------------------------------------------------------
    bool isEmpty(ConstLatticeRef l) {
	return FtlWrapper::isEmpty(l);
    }

    LatticeCounts count(ConstLatticeRef l, bool progress)
    { return FtlWrapper::count(l, progress); }

    size_t countInput(ConstLatticeRef l, Fsa::LabelId label, bool progress)
    { return FtlWrapper::countInput(l, progress); }

    size_t countOutput(ConstLatticeRef l, Fsa::LabelId label, bool progress)
    { return FtlWrapper::countOutput(l, progress); }
    // -------------------------------------------------------------------------


    // -------------------------------------------------------------------------
    namespace {
	void basicInfo(ConstLatticeRef l, Core::XmlWriter &o) {
	    o << Core::XmlFull("description", l->describe());
	    o << Core::XmlFull("fsa-type", Fsa::TypeChoice[l->type()]);
	    std::string tmp;
	    for (Core::Choice::const_iterator i = Fsa::PropertyChoice.begin(); i != Fsa::PropertyChoice.end(); ++i) {
		if (l->knowsProperty(i->value())) {
		    if (!tmp.empty()) tmp += " ";
		    if (!l->hasProperty(i->value())) tmp += "!";
		    tmp += i->ident();
		}
	    }
	    o << Core::XmlFull("fsa-properties", tmp);
	    o << Core::XmlFull("semiring", l->semiring()->name());
	    o << Core::XmlFull(
		"input-alphabet", Lexicon::us()->alphabetName(Lexicon::us()->alphabetId(l->getInputAlphabet())));
	    if (l->type() == Fsa::TypeTransducer)
		o << Core::XmlFull(
		    "output-alphabet", Lexicon::us()->alphabetName(Lexicon::us()->alphabetId(l->getOutputAlphabet())));
	    if (l->initialStateId() != Fsa::InvalidStateId)
		o << Core::XmlFull("initial-state-id", l->initialStateId());
	    else
		o << Core::XmlEmpty("no-states");
	}
    } // namespace


    void cheapInfo(ConstLatticeRef l, Core::XmlWriter &o) {
	o << Core::XmlOpen("lattice-info") + Core::XmlAttribute("type", "cheap");
	basicInfo(l, o);
	o << Core::XmlClose("lattice-info");
    }


    void normalInfo(ConstLatticeRef l, Core::XmlWriter &o) {
	o << Core::XmlOpen("lattice-info") + Core::XmlAttribute("type", "normal");
	basicInfo(l, o);
	if (l->initialStateId() != Fsa::InvalidStateId) {
	    Fsa::AutomatonCounts counts = FtlWrapper::count(l, false);
	    o << Core::XmlOpen("lattice-statistics");
	    o << Core::XmlFull("states", counts.nStates_);
	    o << Core::XmlFull("final-states", counts.nFinals_);
	    o << Core::XmlFull("max-state-id", counts.maxStateId_);
	    o << Core::XmlFull("arcs", counts.nArcs_);
	    o << Core::XmlFull("input-epsilon-arcs", counts.nIEps_);
	    if (l->type() != Fsa::TypeAcceptor) {
		o << Core::XmlFull("output-epsilon-arcs", counts.nOEps_);
		o << Core::XmlFull("io-epsilon-arcs", counts.nIoEps_);
	    }
	    o << Core::XmlClose("lattice-statistics");
	}
	o << Core::XmlClose("lattice-info");
    }


    namespace {
	struct LatticeStatistics {
	    u32 nInputEpsilonArcs;
	    u32 nInputNonWordArcs;
	    u32 nInputWordArcs;
	    u32 nOutputEpsilonArcs;
	    u32 nOutputNonWordArcs;
	    u32 nOutputWordArcs;
	    u32 nFinalStates;
	    Time minTime, maxTime;
	    LatticeStatistics() :
		nInputEpsilonArcs(0), nInputNonWordArcs(0), nInputWordArcs(0),
		nOutputEpsilonArcs(0), nOutputNonWordArcs(0), nOutputWordArcs(0),
		nFinalStates(0),
		minTime(Core::Type<Time>::max), maxTime(Core::Type<Time>::min) {}
	};
	struct BestStatistics : public LatticeStatistics {
	    ScoresRef scores;
	    Score score;
	    Score posteriorScore;
	    u32 nNonEps;
	    f64 densityAllNonEps;
	    f64 densityNonEpsNonEps;
	    u32 nWords;
	    f64 densityAllWord;
	    f64 densityWordWord;
	    BestStatistics() :
		LatticeStatistics(),
		nNonEps(0), densityAllNonEps(0.0), densityNonEpsNonEps(0.0),
		nWords(0), densityAllWord(0.0), densityWordWord(0.0) {}
	};
	struct TraceElement {
	    Score score;
	    Fsa::StateId bptr;
	    Arc arc;
	    TraceElement() : score(Semiring::Max), bptr(Fsa::InvalidStateId) {}
	};
	typedef std::vector<TraceElement> Traceback;
    } // namespace
    void extendedInfo(ConstLatticeRef l, Core::XmlWriter &o, LatticeStatistics &stats, BestStatistics &bestStats) {
	o << Core::XmlOpen("lattice-info") + Core::XmlAttribute("type", "extended");
	basicInfo(l, o);
	if (l->initialStateId() != Fsa::InvalidStateId) {
	    // init
	    l = persistent(l);
	    bool isAcceptor = (l->type() == Fsa::TypeAcceptor);
	    ConstSemiringRef semiring = l->semiring();
	    ConstSemiringRef posteriorSemiring = toLogSemiring(semiring);
	    Lexicon::AlphabetId inputAlphabetId = Lexicon::us()->alphabetId(l->getInputAlphabet());
	    LabelMapRef inputNonWordMap;
	    if ((inputAlphabetId == Lexicon::LemmaAlphabetId)
		|| (inputAlphabetId == Lexicon::LemmaPronunciationAlphabetId))
		inputNonWordMap = LabelMap::createNonWordToEpsilonMap(inputAlphabetId);
	    Lexicon::AlphabetId outputAlphabetId = inputAlphabetId;
	    LabelMapRef outputNonWordMap;
	    if (!isAcceptor) {
		outputAlphabetId = Lexicon::us()->alphabetId(l->getOutputAlphabet());
		if ((outputAlphabetId == Lexicon::LemmaAlphabetId)
		    || (outputAlphabetId == Lexicon::LemmaPronunciationAlphabetId))
		    outputNonWordMap = LabelMap::createNonWordToEpsilonMap(outputAlphabetId);
	    }

	    // fwd/bwd-scores, topological sort
	    std::pair<ConstLatticeRef, ConstFwdBwdRef> result = FwdBwd::build(l, posteriorSemiring);
	    l = result.first;
	    ConstFwdBwdRef fb = result.second;
	    ConstStateMapRef topologicalSort = sortTopologically(l);
	    Fsa::StateId initialSid = topologicalSort->front();

	    // lattice statistics
	    Traceback traceback(topologicalSort->maxSid + 1);
	    traceback[initialSid].score = 0.0;
	    TraceElement bestTrace;
	    for (u32 i = 0; i < topologicalSort->size(); ++i) {
		Fsa::StateId sid = (*topologicalSort)[i];
		ConstStateRef sr = l->getState(sid);
		Time t = l->boundary(sid).time();
		if (t < stats.minTime) stats.minTime = t;
		if (t > stats.maxTime) stats.maxTime = t;
		const TraceElement &currentTrace = traceback[sid];
		if (sr->isFinal()) {
		    Score score = currentTrace.score + semiring->project(sr->weight());
		    if (score < bestTrace.score) {
			bestTrace.score = score;
			bestTrace.bptr = sid;
		    }
		    ++stats.nFinalStates;
		}
		for (State::const_iterator a = sr->begin(); a != sr->end(); ++a) {
		    Score score = currentTrace.score + semiring->project(a->weight());
		    TraceElement &trace = traceback[a->target()];
		    if (score < trace.score) {
			trace.score = score;
			trace.bptr = sid;
			trace.arc = *a;
		    }
		    if (a->input() == Fsa::Epsilon)
			++stats.nInputEpsilonArcs;
		    else if (inputNonWordMap && !(*inputNonWordMap)[a->input()].empty())
			++stats.nInputNonWordArcs;
		    else
			++stats.nInputWordArcs;
		    if (!isAcceptor) {
			if (a->output() == Fsa::Epsilon)
			    ++stats.nOutputEpsilonArcs;
			else if (outputNonWordMap && !(*outputNonWordMap)[a->output()].empty())
			    ++stats.nOutputNonWordArcs;
			else
			    ++stats.nOutputWordArcs;
		    }
		}
	    }
	    verify(bestTrace.bptr != Fsa::InvalidStateId);

	    // best statistics
	    bestStats.minTime = l->boundary(initialSid).time();
	    bestStats.maxTime = l->boundary(bestTrace.bptr).time();
	    bestStats.nFinalStates = 1;
	    bestStats.scores = l->getState(bestTrace.bptr)->weight();
	    Fsa::StateId bestSid = bestTrace.bptr;
	    while (bestSid != initialSid) {
		const TraceElement &trace = traceback[bestSid];
		if (trace.arc.input() == Fsa::Epsilon)
		    ++bestStats.nInputEpsilonArcs;
		else if (inputNonWordMap && !(*inputNonWordMap)[trace.arc.input()].empty())
		    ++bestStats.nInputNonWordArcs;
		else
		    ++bestStats.nInputWordArcs;
		if (!isAcceptor) {
		    if (trace.arc.output() == Fsa::Epsilon)
			++bestStats.nOutputEpsilonArcs;
		    else if (outputNonWordMap && !(*outputNonWordMap)[trace.arc.output()].empty())
			++bestStats.nOutputNonWordArcs;
		    else
			++bestStats.nOutputWordArcs;
		}
		bestStats.scores = semiring->extend(bestStats.scores, trace.arc.weight());
		bestSid = trace.bptr;
	    }
	    bestStats.score = semiring->project(bestStats.scores);
	    bestStats.posteriorScore = posteriorSemiring->project(bestStats.scores) - fb->sum();
	    // verify(Core::isAlmostEqualUlp(bestTrace.score, bestStats.score, 100));

	    // dump lattice statistics
	    o << Core::XmlOpen("lattice-statistics");
	    if (l->getBoundaries()->valid()) {
		o << Core::XmlFull("min-timeframe", stats.minTime);
		o << Core::XmlFull("max-timeframe", stats.maxTime);
		if(stats.maxTime != stats.minTime) {
		    o << Core::XmlFull("arcs-per-second",
			(f64(stats.nInputEpsilonArcs + stats.nInputNonWordArcs + stats.nInputWordArcs) * 100)
			    / (stats.maxTime - stats.minTime));
		    o << Core::XmlFull("word-arcs-per-second", (f64(stats.nInputWordArcs) * 100) / (stats.maxTime - stats.minTime));
		}
	    }
	    o << Core::XmlFull("states", topologicalSort->size());
	    o << Core::XmlFull("final-states", stats.nFinalStates);
	    o << Core::XmlFull("max-state-id", topologicalSort->maxSid);
	    u32 nArcs = stats.nInputEpsilonArcs + stats.nInputNonWordArcs + stats.nInputWordArcs;
	    o << Core::XmlFull("arcs", nArcs);
	    o << Core::XmlFull("input-epsilon-arcs", stats.nInputEpsilonArcs);
	    if (inputNonWordMap) {
		o << Core::XmlFull("input-non-word-arcs", stats.nInputNonWordArcs);
		o << Core::XmlFull("input-word-arcs", stats.nInputWordArcs);
	    }
	    if (!isAcceptor) {
		o << Core::XmlFull("output-epsilon-arcs", stats.nOutputEpsilonArcs);
		if (outputNonWordMap) {
		    o << Core::XmlFull("output-non-word-arcs", stats.nOutputNonWordArcs);
		    o << Core::XmlFull("output-word-arcs", stats.nOutputWordArcs);
		}
	    }
	    bestStats.nNonEps = bestStats.nInputNonWordArcs + bestStats.nInputWordArcs;
	    if (bestStats.nNonEps > 0) {
		f64 nNonEps = f64(bestStats.nNonEps);
		bestStats.densityAllNonEps = f64(stats.nInputEpsilonArcs + stats.nInputNonWordArcs + stats.nInputWordArcs) / nNonEps;
		bestStats.densityNonEpsNonEps = f64(stats.nInputNonWordArcs + stats.nInputWordArcs) / nNonEps;
	    }
	    o << Core::XmlFull("density", bestStats.densityAllNonEps)
		+ Core::XmlAttribute("numerator", "all arcs")
		+ Core::XmlAttribute("denominator", "non-epsilon arcs");
	    o << Core::XmlFull("density", bestStats.densityNonEpsNonEps)
		+ Core::XmlAttribute("numerator", "non-epsilon arcs")
		+ Core::XmlAttribute("denominator", "non-epsilon arcs");
	    if (inputNonWordMap) {
		bestStats.nWords = bestStats.nInputWordArcs;
		if (bestStats.nWords > 0) {
		    f64 nWords = f64(bestStats.nWords);
		    bestStats.densityAllWord = f64(stats.nInputEpsilonArcs + stats.nInputNonWordArcs + stats.nInputWordArcs) / nWords;
		    bestStats.densityWordWord = f64(stats.nInputWordArcs) / nWords;
		}
		o << Core::XmlFull("density", bestStats.densityAllWord)
		    + Core::XmlAttribute("numerator", "all arcs")
		    + Core::XmlAttribute("denominator", "word arcs");
		o << Core::XmlFull("density", bestStats.densityWordWord)
		    + Core::XmlAttribute("numerator", "word arcs")
		    + Core::XmlAttribute("denominator", "word arcs");
	    }
	    o << Core::XmlClose("lattice-statistics");

	    // dump best statistics
	    o << Core::XmlOpen("best-statistics");
	    o << Core::XmlFull("scores", semiring->describe(bestStats.scores, Fsa::HintShowDetails | HintUnscaled));
	    o << Core::XmlFull("score", bestStats.score);
	    o << Core::XmlFull("posterior-score", Core::form("%.15f", bestStats.posteriorScore));
	    o << Core::XmlFull("posterior-probability", Core::form("%.15f", ::exp(-bestStats.posteriorScore)));
	    if (l->getBoundaries()->valid()) {
		o << Core::XmlFull("start-timeframe", bestStats.minTime);
		o << Core::XmlFull("end-timeframe", bestStats.maxTime);
	    }
	    u32 nBestArcs = bestStats.nInputEpsilonArcs + bestStats.nInputNonWordArcs + bestStats.nInputWordArcs;
	    o << Core::XmlFull("arcs", nBestArcs);
	    o << Core::XmlFull("input-epsilon-arcs", bestStats.nInputEpsilonArcs);
	    if (inputNonWordMap) {
		o << Core::XmlFull("input-non-word-arcs", bestStats.nInputNonWordArcs);
		o << Core::XmlFull("input-word-arcs", bestStats.nInputWordArcs);
	    }
	    if (!isAcceptor) {
		o << Core::XmlFull("output-epsilon-arcs", bestStats.nOutputEpsilonArcs);
		if (outputNonWordMap) {
		    o << Core::XmlFull("output-non-word-arcs", bestStats.nOutputNonWordArcs);
		    o << Core::XmlFull("output-word-arcs", bestStats.nOutputWordArcs);
		}
	    }
	    o << Core::XmlClose("best-statistics");
	}
	o << Core::XmlClose("lattice-info");
    }
    void extendedInfo(ConstLatticeRef l, Core::XmlWriter &o) {
	LatticeStatistics stats;
	BestStatistics bestStats;
	extendedInfo(l, o, stats, bestStats);
    }


    void memoryInfo(ConstLatticeRef l, Core::XmlWriter &o) {
	o << Core::XmlOpen("lattice-info") + Core::XmlAttribute("type", "memory");
	basicInfo(l, o);
	l->dumpMemoryUsage(o);
	o << Core::XmlFull("total", l->getMemoryUsed());
	o << Core::XmlClose("lattice-info");
    }


    void info(ConstLatticeRef l, Core::XmlWriter &o, InfoType infoType) {
	if (l){
	    switch (infoType) {
	    case InfoTypeCheap:
		cheapInfo(l, o);
		break;
	    case InfoTypeNormal:
		normalInfo(l, o);
		break;
	    case InfoTypeExtended:
		extendedInfo(l, o);
		break;
	    case InfoTypeMemory:
		memoryInfo(l, o);
		break;
	    }
	} else
	    o << Core::XmlOpen("lattice-info") << Core::XmlEmpty("empty-lattice") << Core::XmlClose("lattice-info");
    }
    // -------------------------------------------------------------------------


    // -------------------------------------------------------------------------
    class InfoNode : public FilterNode {
	friend class Network;
    public:
	static const Core::ParameterString paramInfoType;
    private:
	InfoType infoType_;
	// extended
	u32 nLattices_;
	u32 nBestNonEpsLattices_;
	u32 nBestNonEpsTokens_;
	f64 sumAllNonEps_;
	f64 sumNonEpsNonEps_;
	u32 nBestWordLattices_;
	u32 nBestWordTokens_;
	f64 sumAllWord_;
	f64 sumTime_;
	f64 sumWordWord_;
    protected:
	void overallInfo(Core::XmlWriter &o) const {
	    o << Core::XmlOpen("overall-lattice-statistics");
	    o << Core::XmlFull("lattices", nLattices_);
	    o << Core::XmlOpen("best");
	    o << Core::XmlFull("lattices", nBestNonEpsLattices_)
		+ Core::XmlAttribute("type", "has-non-eps-tokens");
	    o << Core::XmlFull("tokens", nBestNonEpsTokens_)
		+ Core::XmlAttribute("type", "non-eps");
	    o << Core::XmlFull("lattices", nBestWordLattices_)
		+ Core::XmlAttribute("type", "has-word-tokens");
	    o << Core::XmlFull("tokens", nBestWordTokens_)
		+ Core::XmlAttribute("type", "word");
	    if(sumTime_)
	    {
		o << Core::XmlFull("arcs-per-second", sumAllWord_ / sumTime_);
		o << Core::XmlFull("word-arcs-per-second", sumWordWord_ / sumTime_);
	    }
	    o << Core::XmlClose("best");
	    f64 densityAllNonEps = 0.0;
	    f64 densityNonEpsNonEps = 0.0;
	    if (nBestNonEpsTokens_ > 0) {
		f64 nNonEps = f64(nBestNonEpsTokens_);
		densityAllNonEps = sumAllNonEps_ / nNonEps;
		densityNonEpsNonEps = sumNonEpsNonEps_ / nNonEps;
	    }
	    o << Core::XmlFull("density", densityAllNonEps)
		+ Core::XmlAttribute("numerator", "all arcs")
		+ Core::XmlAttribute("denominator", "non-epsilon arcs");
	    o << Core::XmlFull("density", densityNonEpsNonEps)
		+ Core::XmlAttribute("numerator", "non-epsilon arcs")
		+ Core::XmlAttribute("denominator", "non-epsilon arcs");
	    f64 densityAllWord = 0.0;
	    f64 densityWordWord = 0.0;
	    if (nBestWordTokens_ > 0) {
		f64 nWords = f64(nBestWordTokens_);
		densityAllWord = sumAllNonEps_ / nWords;
		densityWordWord = sumWordWord_ / nWords;
	    }
	    o << Core::XmlFull("density", densityAllWord)
		+ Core::XmlAttribute("numerator", "all arcs")
		+ Core::XmlAttribute("denominator", "word arcs");
	    o << Core::XmlFull("density", densityWordWord)
		+ Core::XmlAttribute("numerator", "word arcs")
		+ Core::XmlAttribute("denominator", "word arcs");
	    o << Core::XmlClose("overall-lattice-statistics");
	}

	virtual ConstLatticeRef filter(ConstLatticeRef l) {
	    if (l) {
		if (infoType_ == InfoTypeExtended) {
		    LatticeStatistics stats;
		    BestStatistics bestStats;
		    extendedInfo(l, log(), stats, bestStats);
		    ++nLattices_;
		    if (bestStats.nNonEps > 0) {
			++nBestNonEpsLattices_;
			nBestNonEpsTokens_ += bestStats.nNonEps;
			sumAllNonEps_ += f64(bestStats.nNonEps) * bestStats.densityAllNonEps;
			sumNonEpsNonEps_ += f64(bestStats.nNonEps) * bestStats.densityNonEpsNonEps;
		    }
		    if (bestStats.nWords > 0) {
			++nBestWordLattices_;
			nBestWordTokens_ += bestStats.nWords;
			sumAllWord_ += f64(bestStats.nWords) * bestStats.densityAllWord;
			sumWordWord_ += f64(bestStats.nWords) * bestStats.densityWordWord;
		    }
		    sumTime_ += bestStats.maxTime - bestStats.minTime;
		} else
		    info(l, log(), infoType_);
	    } else
		log() << Core::XmlOpen("lattice-info")
		      << Core::XmlEmpty("empty-lattice")
		      << Core::XmlClose("lattice-info");
	    return l;
	}
    public:
	InfoNode(const std::string &name, const Core::Configuration &config) :
	    FilterNode(name, config) {}
	virtual ~InfoNode() {}
	virtual void init(const std::vector<std::string> &arguments) {
	    infoType_ = getInfoType(paramInfoType(config));
	    nLattices_ = 0;
	    nBestNonEpsLattices_ = 0;
	    nBestNonEpsTokens_ = 0;
	    sumAllNonEps_ = 0.0;
	    sumNonEpsNonEps_ = 0.0;
	    nBestWordLattices_ = 0;
	    nBestWordTokens_ = 0;
	    sumAllWord_ = 0.0;
	    sumTime_ = 0.0;
	    sumWordWord_ = 0.0;
	}
	virtual void finalize() {
	    overallInfo(log());
	}
    };
    const Core::ParameterString InfoNode::paramInfoType(
	"info-type",
	"info type",
	"normal");
    NodeRef createInfoNode(const std::string &name, const Core::Configuration &config) {
	return NodeRef(new InfoNode(name, config));
    }
    // -------------------------------------------------------------------------

} // namespace Flf

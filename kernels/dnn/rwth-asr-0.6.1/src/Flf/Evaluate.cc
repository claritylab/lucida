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
#include <Bliss/Evaluation.hh>
#include <Core/Application.hh>
#include <Core/Component.hh>
#include <Core/Parameter.hh>
#include <Core/StringUtilities.hh>
#include <Core/XmlStream.hh>
#include <Fsa/Automaton.hh>
#include <Fsa/Static.hh>

#include "FlfCore/Basic.hh"
#include "Best.hh"
#include "Evaluate.hh"
#include "Formattings.hh"
#include "Lexicon.hh"
#include "Map.hh"


namespace Flf {

// -------------------------------------------------------------------------
class Evaluator : public LayeredComponent {
public:
    static const Core::ParameterBool paramSingleBest;
    static const Core::ParameterBool paramBestInLattice;
    static const Core::Choice choiceSsspAlgorithm;
    static const Core::ParameterChoice paramSsspAlgorithm;


private:
    Bliss::Evaluator *blissEvaluator_;
    u32 lastLemmaUpdates_;
    bool logSingleBest_;
    bool logBestInLattice_;
    Fsa::ConstAutomatonRef emptyFsa_;

protected:
    SingleSourceShortestPathAlgorithm ssspAlgorithm_;

public:
    Evaluator(const Core::Configuration &config, const std::string &name);
    virtual ~Evaluator();

    Bliss::Evaluator * blissEvaluator();

    /**
     * Set reference and evaluate lattice against
     * single best and best in lattice (as specified in config),
     * lattice output can be any alphabet specified by the current lexicon
     *
     * evaluate is null reference save
     *
     **/
    void setReference(const std::string &ref);
    void setNormalizedReference(const std::string &ref);
    void evaluate(ConstLatticeRef l);

    /**
     * Log the evaluation of the given lattice against the current segment's orth;
     * lattice output has to be lemma, lemma-pronunciation, or eval alphabet
     *
     * logEvaluation is null reference save
     *
     **/
    void logEvaluation(ConstLatticeRef l, const std::string &name = "");

    /**
     * Log the preferred orthographic representation of a linear lattice;
     * lattice output has to be lemma alphabet
     *
     * logOrthography is null reference save
     *
     **/
    void logOrthography(ConstLatticeRef l, const std::string &name = "");
};
// -------------------------------------------------------------------------


// -------------------------------------------------------------------------
const Core::ParameterBool Evaluator::paramSingleBest(
	"single-best",
	"decode lattice and evaluate result",
	true);
const Core::ParameterBool Evaluator::paramBestInLattice(
	"best-in-lattice",
	"evaluate lattice",
	true);
const Core::Choice Evaluator::choiceSsspAlgorithm(
	"dijkstra",                Dijkstra,
	"bellman-ford",            BellmanFord,
	"projecting-bellman-ford", ProjectingBellmanFord,
	Core::Choice::endMark());

const Core::ParameterChoice Evaluator::paramSsspAlgorithm(
	"algorithm",
	&choiceSsspAlgorithm,
	"single source shortest path algorithm",
	Dijkstra);



Evaluator::Evaluator(const Core::Configuration &config, const std::string &name) :
    Core::Component(config),
    LayeredComponent(config, name),
    blissEvaluator_(0),
    lastLemmaUpdates_(0),
    ssspAlgorithm_(static_cast<SingleSourceShortestPathAlgorithm>(paramSsspAlgorithm(config)))
    {
    logSingleBest_ = paramSingleBest(config);
    logBestInLattice_ = paramBestInLattice(config);
    Fsa::ConstSemiringRef sr = Fsa::getSemiring(Fsa::SemiringTypeTropical);
    Fsa::StaticAutomaton * sf = new Fsa::StaticAutomaton(Fsa::TypeAcceptor);
    sf->setSemiring(sr);
    sf->setInputAlphabet(Lexicon::us()->lemmaAlphabet());
    sf->setState(new Fsa::State(0, Fsa::StateTagFinal, sr->one()));
    sf->setInitialStateId(0);
    emptyFsa_ = Fsa::ConstAutomatonRef(sf);
    }

Evaluator::~Evaluator() {
    delete blissEvaluator_;
}

Bliss::Evaluator * Evaluator::blissEvaluator() {
    if (!blissEvaluator_ || (lastLemmaUpdates_ != Lexicon::us()->nLemmaUpdates())) {
	if (blissEvaluator_) {
	    delete blissEvaluator_;
	    log("%d lemmas were updated; create new evaluator.",
		    (Lexicon::us()->nLemmaUpdates() - lastLemmaUpdates_));
	}
	blissEvaluator_ = new Bliss::Evaluator(config, Lexicon::us());
	lastLemmaUpdates_ = Lexicon::us()->nLemmaUpdates();
    }
    return blissEvaluator_;
}

void Evaluator::logEvaluation(ConstLatticeRef l, const std::string &name) {
    openLayer(name);
    Fsa::ConstAutomatonRef f;
    if (l) {
	log("evaluate lattice \"%s\"", l->describe().c_str());
	f = toFsa(l);
    } else {
	log("evaluate empty lattice");
	f = emptyFsa_;
    }
    blissEvaluator()->evaluateWords(f, name);
    closeLayer();
}

void Evaluator::setReference(const std::string &ref) {
    std::string normalizedRef(ref);
    Core::normalizeWhitespace(normalizedRef);
    Core::enforceTrailingBlank(normalizedRef);
    setNormalizedReference(normalizedRef);
}

void Evaluator::setNormalizedReference(const std::string &ref) {
    blissEvaluator()->setReferenceTranscription(ref);
    Core::XmlWriter &os(clog());
    if (ref.empty())
	os << Core::XmlEmpty("orth") + Core::XmlAttribute("source", "reference");
    else
	os << Core::XmlOpen("orth") + Core::XmlAttribute("source", "reference")
    << ref << Core::XmlClose("orth");
}

void Evaluator::evaluate(ConstLatticeRef l) {
    if (l)
	l = projectInput(mapInput(l, MapToLemma));
    if (logSingleBest_) {
	ConstLatticeRef eval = (l && !l->hasProperty(Fsa::PropertyLinear)) ? best(l, ssspAlgorithm_) : l;
	logEvaluation(eval, "single best");
    }
    if (logBestInLattice_) {
	ConstLatticeRef eval = l;
	logEvaluation(eval, "best-in-lattice");
    }
}
// -------------------------------------------------------------------------


// -------------------------------------------------------------------------
void evaluate(ConstLatticeRef l, const std::string &ref) {
    Core::Configuration _config(
	    Core::Application::us()->getConfiguration(), "evaluator");
    Evaluator evaluator(_config, "evaluated");
    evaluator.setReference(ref);
    evaluator.evaluate(l);
}
// -------------------------------------------------------------------------


// -------------------------------------------------------------------------
class EvaluatorNode : public FilterNode {
    typedef FilterNode Precursor;
private:
    Evaluator *evaluator_;
    ConstSemiringRef evalSemiring_;
    bool validReference_;
    Lexicon::SymbolMap lemmaSymbolMap_;
    std::vector<Fsa::LabelId> refLabels_;
protected:
    void setReference() {
	if (validReference_)
	    return;
	if (connected(1)) {
	    ConstSegmentRef segment = requestSegment(1);
	    if (segment->hasOrthography()) {
		lemmaSymbolMap_.indices(segment->orthography(), refLabels_);
		evaluator_->setReference(segment->orthography());
	    } else {
		error("EvaluatorNode: Missing reference.");
		evaluator_->setReference("");
	    }
	} else {
	    std::string refStr = requestString(2);
	    lemmaSymbolMap_.indices(refStr, refLabels_);
	    evaluator_->setReference(refStr);
	}
	refLabels_.clear();
	validReference_ = true;
    }
    virtual ConstLatticeRef filter(ConstLatticeRef l) {
	setReference();
	ConstLatticeRef eval = l;
	if (evalSemiring_)
	    eval = changeSemiring(eval, evalSemiring_);
	evaluator_->evaluate(eval);
	return l;
    }
public:
    EvaluatorNode(const std::string &name, const Core::Configuration &config) :
	Precursor(name, config), evaluator_(0) { validReference_ = false; }
    virtual ~EvaluatorNode() {
	delete evaluator_;
    }
    virtual void init(const std::vector<std::string> &arguments) {
	if (!(connected(1) || connected(2)))
	    criticalError("EvaluatorNode: Need a data source either at port 1 or port 2");
	evaluator_ = new Evaluator(config, name);
	evalSemiring_ = Semiring::create(select("semiring"));
	if (evalSemiring_)
	    log("Use semiring %s for evaluation.", evalSemiring_->name().c_str());
	lemmaSymbolMap_ = Lexicon::us()->symbolMap(Lexicon::LemmaAlphabetId);
    }
    void sync() {
	validReference_ = false;
    }
};

NodeRef createEvaluatorNode(const std::string &name, const Core::Configuration &config) {
    return NodeRef(new EvaluatorNode(name, config));
}
// -------------------------------------------------------------------------

} // namespace Flf

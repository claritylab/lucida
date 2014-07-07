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
#include "AbstractSegmentwiseTrainer.hh"
#include <Modules.hh>
#include <Bliss/Orthography.hh>
#include <Lattice/Merge.hh>
#include <Fsa/Cache.hh>
#include <Fsa/Compose.hh>
#include <Fsa/Project.hh>
#include "AcousticSegmentwiseTrainer.hh"

using namespace Speech;

/**
 *  AbstractSegmentwiseTrainer
 */
const Core::ParameterFloat AbstractSegmentwiseTrainer::paramWeightThreshold(
    "weight-threshold",
    "discard all observations with absolute weight smaller or equal to this threshold",
    Core::Type<f32>::epsilon, Core::Type<f64>::min);

const Core::ParameterInt AbstractSegmentwiseTrainer::paramPosteriorTolerance(
    "posterior-tolerance",
    "tolerance in posterior computation, i.e., error of forward and backward flows w.r.t. least significant bits",
    100,
    0,
    Core::Type<s32>::max);

Core::ParameterString AbstractSegmentwiseTrainer::paramLatticeName(
    "lattice-name",
    "name of lattice with total scores",
    Lattice::WordLattice::totalFsa);

AbstractSegmentwiseTrainer::AbstractSegmentwiseTrainer(
    const Core::Configuration &c) :
    Core::Component(c),
    Precursor(c),
    weightThreshold_(paramWeightThreshold(config)),
    posteriorTolerance_(paramPosteriorTolerance(config)),
    orthToLemma_(0),
    part_(paramLatticeName(c))
{}

AbstractSegmentwiseTrainer::~AbstractSegmentwiseTrainer()
{
    delete orthToLemma_;
}

Lattice::ConstWordLatticeRef AbstractSegmentwiseTrainer::extractNumerator(
    const std::string &orth,
    Lattice::ConstWordLatticeRef denominator) const
{
    return Lattice::extractNumerator(
	orth, denominator, orthToLemma_,
	lemmaPronToLemma_, lemmaToLemmaConfusion_);
}


Lattice::ConstWordLatticeRef AbstractSegmentwiseTrainer::turnOffCompetingHypotheses(
    const std::string &orth,
    Lattice::ConstWordLatticeRef denominator)
{
#if 1
    criticalError("turnOffCompetingHypotheses requires MODULE_SPEECH_ADVANCED");
    return Lattice::ConstWordLatticeRef();
#endif
}


void AbstractSegmentwiseTrainer::initialize(Bliss::LexiconRef lexicon)
{
    Precursor::initialize(lexicon);
    verify(!orthToLemma_);
    orthToLemma_ = new Bliss::OrthographicParser(select("orthographic-parser"), lexicon);
    lemmaPronToLemma_ = lexicon->createLemmaPronunciationToLemmaTransducer();

    Fsa::ConstAutomatonRef lemmaToEval = lexicon->createLemmaToEvaluationTokenTransducer();
    lemmaToLemmaConfusion_ = Fsa::composeMatching(lemmaToEval, Fsa::invert(lemmaToEval));
    lemmaToLemmaConfusion_ = Fsa::cache(lemmaToLemmaConfusion_);
}

/**
 * factory
 */
Core::Choice AbstractSegmentwiseTrainer::choiceCriterion(
    "MMI", maximumMutualInformation,
    "weighted-MMI", weightedMaximumMutualInformation,
    "MCE", minimumClassificationError,
    "ME", minimumError,
    "gis-ME", gisMinimumError,
    "log-ME", logMinimumError,
    "context-prior", contextPrior,
    "context-accuracy", contextAccuracy,
    "weighted-ME", weightedMinimumError,
    "legacy-ME-with-i-smoothing", legacyMinimumErrorWithISmoothing,
    "ME-with-i-smoothing", minimumErrorWithISmoothing,
    "log-ME-with-i-smoothing", logMinimumErrorWithISmoothing,
    "weighted-ME-with-i-smoothing", weightedMinimumErrorWithISmoothing,
    "CORRECTIVE", corrective,
    "minimum-least-squared-error", minimumLeastSquaredError,
    "plain", plain,
    Core::Choice::endMark());

Core::ParameterChoice AbstractSegmentwiseTrainer::paramCriterion(
    "criterion",
    &choiceCriterion,
    "criterion of discriminative acoustic model trainer",
    maximumMutualInformation);

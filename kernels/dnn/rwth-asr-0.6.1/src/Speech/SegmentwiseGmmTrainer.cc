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
#include "SegmentwiseGmmTrainer.hh"
#include <Fsa/Basic.hh>
#include <Fsa/Cache.hh>
#include <Fsa/Sssp.hh>
#include <Lattice/Arithmetic.hh>
#include <Core/Archive.hh>
#include <Core/BinaryStream.hh>
#include <Flow/Data.hh>
#include <Flow/DataAdaptor.hh>
#include <Flow/Datatype.hh>
#include "AlignmentNode.hh"
#include "Module.hh"

/*! @todo: remove Modules.hh dependency (see below) */
#include <Modules.hh>

using namespace Speech;

/**
 *  SegmentwiseGmmTrainer: base class
 */
SegmentwiseGmmTrainer::SegmentwiseGmmTrainer(
    const Core::Configuration &c) :
    Core::Component(c),
    Precursor(c),
    featureDescription_(*this),
    initialized_(false),
    mixtureSetTrainer_(0)
{
    initializeMixtureSetTrainer();
}

SegmentwiseGmmTrainer::~SegmentwiseGmmTrainer()
{
    delete mixtureSetTrainer_;
}

void SegmentwiseGmmTrainer::setFeatureDescription(const Mm::FeatureDescription &description)
{
    Core::Ref<const Mm::FeatureScorerScaling> featureScorer(
	required_cast(
	    const Mm::FeatureScorerScaling*,
	    acousticModel()->featureScorer().get()));
    if (!initialized_) {
	featureDescription_ = description;
	size_t dimension;
	featureDescription_.mainStream().getValue(Mm::FeatureDescription::nameDimension, dimension);
	mixtureSetTrainer_->initializeAccumulation(
	    acousticModel()->nEmissions(),
	    dimension,
	    featureScorer->assigningFeatureScorer());
	initialized_ = true;
    } else {
	if (featureDescription_ != description) {
	    criticalError("change of features is not allowed");
	}
	mixtureSetTrainer_->setAssigningFeatureScorer(featureScorer->assigningFeatureScorer());
    }
}

namespace InternalGmm {

    class NumeratorAccumulator : public GmmAccumulator
    {
    protected:
	virtual void accumulate(Core::Ref<const Feature::Vector> f, Mm::MixtureIndex m, Mm::Weight w) {
	    trainer_->accumulate(f, m, w);
	}
    public:
	NumeratorAccumulator(const GmmAccumulator &accumulator) :
	    GmmAccumulator(accumulator) {}
    };

    class DenominatorAccumulator : public GmmAccumulator
    {
    protected:
	virtual void accumulate(Core::Ref<const Feature::Vector> f, Mm::MixtureIndex m, Mm::Weight w) {
	    trainer_->accumulateDenominator(f, m, w);
	}
    public:
	DenominatorAccumulator(const GmmAccumulator &accumulator) :
	    GmmAccumulator(accumulator) {}
    };


} // namespace InternalGmm

using namespace InternalGmm;

GmmAccumulator* SegmentwiseGmmTrainer::createAcc()
{
    return new GmmAccumulator(
	segmentwiseFeatureExtractor()->features(portId()),
	alignmentGenerator(),
	mixtureSetTrainer_,
	weightThreshold(),
	acousticModel());
}

GmmAccumulator* SegmentwiseGmmTrainer::createNumAcc()
{
    return new NumeratorAccumulator(*acc());
}

GmmAccumulator* SegmentwiseGmmTrainer::createDenAcc()
{
    return new DenominatorAccumulator(*acc());
}


GmmAccumulator* SegmentwiseGmmTrainer::createMleAcc()
{
#if 1
    defect();
    return 0;
#endif
}


void SegmentwiseGmmTrainer::accumulateObjectiveFunction(f32 obj)
{
    log("objective-function: ") << obj;
    mixtureSetTrainer_->accumulateObjectiveFunction(obj);
}

void SegmentwiseGmmTrainer::initializeMixtureSetTrainer()
{
    verify(!mixtureSetTrainer_);
    mixtureSetTrainer_ =
	Speech::Module::instance().createDiscriminativeMixtureSetTrainer(
	    select("mixture-set-trainer"));
}

void SegmentwiseGmmTrainer::leaveCorpus(Bliss::Corpus *corpus)
{
    if (corpus->level() == 0) {
	write();
    }
    Precursor::leaveCorpus(corpus);
}


/*
 *  SegmentwiseGmmTrainer: risk based
 */
MinimumErrorSegmentwiseGmmTrainer::MinimumErrorSegmentwiseGmmTrainer(
    const Core::Configuration &c) :
    Core::Component(c),
    Precursor(c)
{}

void MinimumErrorSegmentwiseGmmTrainer::processWordLattice(
    Lattice::ConstWordLatticeRef lattice, Bliss::SpeechSegment *s)
{
    Precursor::processWordLattice(lattice, s);
    if (!lattice) {
	return;
    }
    if (!lattice->hasPart(part_)) {
	criticalError("lattice has no \" ") << part_ << "\" part";
    }
    if (!lattice->hasPart(Lattice::WordLattice::accuracyFsa)) {
	criticalError("lattice has no \" ") << Lattice::WordLattice::accuracyFsa << "\" part";
    }

    // calculate the posterior automaton with expectation semiring of Q(Z), with Z = (P,P*A)
    // A = accuracies, P = scaled joint probabilities
    Lattice::ConstWordLatticeRef denominator = Lattice::getPart(lattice, part_); // denominator = P-lattice
    Fsa::Weight expectation; // expected accuracy (objective function of current sample)

    Fsa::ConstAutomatonRef fsa =
	    Fsa::posteriorE(Fsa::changeSemiring(denominator->mainPart(), Fsa::LogSemiring),
	    lattice->part(Lattice::WordLattice::accuracyFsa),
	    expectation, true, posteriorTolerance());

    /**
     * accumulation of the covariance Cov(A, gradient( log(P) ) )
     */
    // pass over all arcs with negative arc weights (negative accuracy = denominator)
    accumulateDenominator(
	Fsa::multiply(fsa, Fsa::Weight(f32(-1))),
	denominator->wordBoundaries());
    // pass over all arcs with positive arc weights (positive accuracy = numerator)
    accumulateNumerator(fsa, denominator->wordBoundaries());
    accumulateObjectiveFunction(expectation);
    resetAcc();
}


/**
 * factory
 */
SegmentwiseGmmTrainer*
SegmentwiseGmmTrainer::createSegmentwiseGmmTrainer(
    const Core::Configuration &config)
{
    /*! @todo: remove */
    return Speech::Module::instance().createSegmentwiseGmmTrainer(config);
}

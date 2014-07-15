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
#include "FeatureScorer.hh"
#include "LinearAndActivationLayer.hh"

#include <Math/Vector.hh>
#include <Math/Module.hh>

using namespace Nn;

BaseFeatureScorer::BaseFeatureScorer(const Core::Configuration &c) :
    Core::Component(c),
    Precursor(c),
    prior_(c),
    labelWrapper_(0),
    nClasses_(0),
    inputDimension_(0),
    network_(c)
{ }

BaseFeatureScorer::~BaseFeatureScorer(){
    if (labelWrapper_)
	delete labelWrapper_;
}


void BaseFeatureScorer::init(Core::Ref<const Mm::MixtureSet> mixtureSet){
    require(mixtureSet);
    nClasses_ = mixtureSet->nMixtures();
    if (labelWrapper_)
	delete labelWrapper_;
    labelWrapper_ = new ClassLabelWrapper(select("class-labels"), nClasses_);
    if (!labelWrapper_->isOneToOneMapping())
	error("no one-to-one correspondence between network outputs and classes!");

    network_.initializeNetwork(1);
    require_eq(network_.getTopLayer().getOutputDimension(), labelWrapper_->nClassesToAccumulate());
    LinearAndSoftmaxLayer<f32> *topLayer = dynamic_cast<LinearAndSoftmaxLayer<f32>* >(&network_.getTopLayer());
    if (!topLayer)
	error("output layer must be of type 'linear+softmax'");

    if (network_.getLayer(0).nInputActivations() != 1)
	Core::Component::criticalError("Multiple input streams not implemented in Nn/FeatureScorer.");

    inputDimension_ = network_.getLayer(0).getInputDimension(0);

    // remove prior from bias
    // read
    if (prior_.fileName() != "")
	prior_.read();
    else
	prior_.setFromMixtureSet(mixtureSet, *labelWrapper_);
    // subtract
    network_.finishComputation();
    topLayer->removeLogPriorFromBias(prior_);
    network_.initComputation();
    log("l1 norm of all network weights is: ") << network_.l1norm();
}

//
// OnDemandFeatureScorer

OnDemandFeatureScorer::Context::Context(const Mm::FeatureVector &featureVector,
    const OnDemandFeatureScorer *featureScorer, size_t cacheSize,bool check) :
    Precursor::CachedAssigningContextScorer(featureScorer, cacheSize),
    featureVector_(featureVector)
{
    if (check) {
	if (featureVector.size() != featureScorer->dimension()) {
	    Core::Application::us()->error("dimension mismatch: feature-vector-size: ") << featureVector.size() << " vs feature-scorer-dimension: " << featureScorer->dimension();
	}
    }
    featureScorer->forwardHiddenLayers(featureVector, activationOfLastHiddenLayer_);
}

OnDemandFeatureScorer::OnDemandFeatureScorer(const Core::Configuration &c, Core::Ref<const Mm::MixtureSet> mixture) :
    Core::Component(c),
    Precursor(c),
    topLayerOutputDimension_(0),
    outputLayer_(0)
{
    init(mixture);
    log("using nn-on-demand-hybrid feature scorer");
}

OnDemandFeatureScorer::~OnDemandFeatureScorer() {
    if (outputLayer_)
	delete outputLayer_;
}

void OnDemandFeatureScorer::init(Core::Ref<const Mm::MixtureSet> mixtureSet){
    Precursor::init(mixtureSet);
    outputLayer_ = dynamic_cast<LinearAndSoftmaxLayer<f32>* >(network_.popLayer());
    require(outputLayer_);
    if (network_.nLayers() > 0) {
	topLayerOutputDimension_ = network_.getTopLayer().getOutputDimension();
    } else {
	if (outputLayer_->nInputActivations() != 1) {
	    Core::Application::us()->criticalError("Nn/OnDemandFeatureScorer is only implemented for a single input stream.");
	}
	topLayerOutputDimension_ = outputLayer_->getInputDimension(0);
    }
    // must be in main memory for getScore()
    outputLayer_->getBias()->finishComputation();
}

void OnDemandFeatureScorer::forwardHiddenLayers(const Mm::FeatureVector &in, NnMatrix &out) const{
    out.resize(topLayerOutputDimension_, 1);

    out.initComputation(false);

    if (network_.nLayers() > 0){
	network_.forward(in);
	out.swap(network_.getTopLayerOutput());

    }
    else {
	out.copy(&(in.at(0)));
    }

    out.finishComputation();

}

Mm::AssigningFeatureScorer::ScoreAndBestDensity OnDemandFeatureScorer::calculateScoreAndDensity(
    const CachedAssigningContextScorer* cs, Mm::MixtureIndex mixtureIndex) const
{

    const Context *_cs = required_cast(const Context*, cs);
    const NnMatrix &activation = _cs->activationOfLastHiddenLayer_;
    ScoreAndBestDensity result;
    Mm::Score score = 0.0;
    if (labelWrapper_->isClassToAccumulate(mixtureIndex))
	score = outputLayer_->getScore(activation, labelWrapper_->getOutputIndexFromClassIndex(mixtureIndex));
    else
	score = Core::Type<Mm::Score>::max;
    result.score = score;
    result.bestDensity = 0;

    return result;
}

Mm::Score OnDemandFeatureScorer::calculateScore(const CachedAssigningContextScorer* cs, Mm::MixtureIndex mixtureIndex, Mm::DensityIndex dnsInMix) const
{
    Mm::Score score = calculateScore(required_cast(const Context*, cs)->activationOfLastHiddenLayer_, mixtureIndex);
    return score;
}

Mm::Score OnDemandFeatureScorer::calculateScore(const NnMatrix &activation, Mm::MixtureIndex mixtureIndex) const {
    Mm::Score score = 0.0;
    if (labelWrapper_->isClassToAccumulate(mixtureIndex))
	score = outputLayer_->getScore(activation, labelWrapper_->getOutputIndexFromClassIndex(mixtureIndex));
    else
	score = Core::Type<Mm::Score>::max;

    return score;
}

/*
 *
 *
 * FullFeatureScorer
 *
 *
 *
 */



FullFeatureScorer::Context::Context(const Mm::FeatureVector &featureVector,
    const FullFeatureScorer *featureScorer, size_t cacheSize,bool check) :
    Precursor::CachedAssigningContextScorer(featureScorer, cacheSize),
    featureVector_(featureVector)
{
    if (check) {
	if (featureVector.size() != featureScorer->dimension()) {
	    Core::Application::us()->error("dimension mismatch: feature-vector-size: ") << featureVector.size() << " vs feature-scorer-dimension: " << featureScorer->dimension();
	}
    }
    featureScorer->computeScores(featureVector, scores_);
}

FullFeatureScorer::FullFeatureScorer(const Core::Configuration &c, Core::Ref<const Mm::MixtureSet> mixture) :
    Core::Component(c),
    Precursor(c),
    topLayerOutputDimension_(0)
{
    init(mixture);
    log("using nn-full-hybrid feature scorer");
}

FullFeatureScorer::~FullFeatureScorer() {}

void FullFeatureScorer::init(Core::Ref<const Mm::MixtureSet> mixtureSet){
    Precursor::init(mixtureSet);
    LinearAndSoftmaxLayer<f32> *outputlayer = dynamic_cast<LinearAndSoftmaxLayer<f32>* >( &(network_.getTopLayer()));
    require(outputlayer);
    outputlayer->setEvaluateSoftmax(false);

    topLayerOutputDimension_ = outputlayer->getOutputDimension();
}

void FullFeatureScorer::computeScores(const Mm::FeatureVector &in, NnMatrix &out) const{

    network_.forward(in);
    out.resize(topLayerOutputDimension_, 1);
    out.initComputation(false);
    out.swap(network_.getTopLayerOutput());
    out.finishComputation();
}

Mm::AssigningFeatureScorer::ScoreAndBestDensity FullFeatureScorer::calculateScoreAndDensity(
    const CachedAssigningContextScorer* cs, Mm::MixtureIndex mixtureIndex) const
{
    const Context *_cs = required_cast(const Context*, cs);
    ScoreAndBestDensity result;
    Mm::Score score = 0.0;
    if (labelWrapper_->isClassToAccumulate(mixtureIndex))
	score = -_cs->scores_.at(labelWrapper_->getOutputIndexFromClassIndex(mixtureIndex), 0);
    else
	score = Core::Type<Mm::Score>::max;
    result.score = score;
    result.bestDensity = 0;

    return result;
}

Mm::Score FullFeatureScorer::calculateScore(const CachedAssigningContextScorer* cs, Mm::MixtureIndex mixtureIndex, Mm::DensityIndex dnsInMix) const
{
    return calculateScore(required_cast(const Context*, cs)->scores_, mixtureIndex);
}

Mm::Score FullFeatureScorer::calculateScore(const NnMatrix &scores, Mm::MixtureIndex mixtureIndex) const {
    Mm::Score score = 0.0;
    if (labelWrapper_->isClassToAccumulate(mixtureIndex))
	score = -scores.at(labelWrapper_->getOutputIndexFromClassIndex(mixtureIndex), 0);
    else
	score = Core::Type<Mm::Score>::max;
    return score;
}

/*
 *
 *
 * PrecomputedFeatureScorer
 *
 *
 *
 */



PrecomputedFeatureScorer::Context::Context(const Mm::FeatureVector &featureVector,
    const PrecomputedFeatureScorer *featureScorer, size_t cacheSize,bool check) :
    Precursor::CachedAssigningContextScorer(featureScorer, cacheSize),
    featureVector_(featureVector)
{
    if (check) {
	if (featureVector.size() != featureScorer->dimension()) {
	    Core::Application::us()->error("dimension mismatch: feature-vector-size: ") << featureVector.size() << " vs feature-scorer-dimension: " << featureScorer->dimension();
	}
    }
}

PrecomputedFeatureScorer::PrecomputedFeatureScorer(const Core::Configuration &c, Core::Ref<const Mm::MixtureSet> mixture) :
    Core::Component(c),
    Precursor(c)
{
    init(mixture);
}

PrecomputedFeatureScorer::~PrecomputedFeatureScorer() {}

void PrecomputedFeatureScorer::init(Core::Ref<const Mm::MixtureSet> mixtureSet) {
    require(mixtureSet);
    if (labelWrapper_)
	delete labelWrapper_;
    labelWrapper_ = new ClassLabelWrapper(select("class-labels"), mixtureSet->nMixtures());
    nClasses_ = labelWrapper_->nClasses();

    inputDimension_ = labelWrapper_->nClassesToAccumulate();

    if (prior_.fileName() != "")
	prior_.read();
    else
	prior_.setFromMixtureSet(mixtureSet, *labelWrapper_);
    log("using nn-precomputed-hybrid feature scorer");
    log("feature scorer applies prior scale ") << prior_.scale();
}

Mm::AssigningFeatureScorer::ScoreAndBestDensity PrecomputedFeatureScorer::calculateScoreAndDensity(
    const CachedAssigningContextScorer* cs, Mm::MixtureIndex mixtureIndex) const
{
    const Context *_cs = required_cast(const Context*, cs);
    ScoreAndBestDensity result;
    Mm::Score score = 0.0;
    if (labelWrapper_->isClassToAccumulate(mixtureIndex)){
	u32 outputIndex = labelWrapper_->getOutputIndexFromClassIndex(mixtureIndex);
	score = -_cs->featureVector_.at(outputIndex);
	score += prior_.scale() * prior_.at(outputIndex);
    }
    else{
	score = Core::Type<Mm::Score>::max;
    }
    result.score = score;
    result.bestDensity = 0;
    return result;
}

Mm::Score PrecomputedFeatureScorer::calculateScore(const CachedAssigningContextScorer* cs, Mm::MixtureIndex mixtureIndex, Mm::DensityIndex dnsInMix) const
{
    return calculateScore(required_cast(const Context*, cs)->featureVector_, mixtureIndex);
}

Mm::Score PrecomputedFeatureScorer::calculateScore(const Mm::FeatureVector &featureVector , Mm::MixtureIndex mixtureIndex) const {
    Mm::Score score = 0.0;
    if (labelWrapper_->isClassToAccumulate(mixtureIndex)){
	u32 outputIndex = labelWrapper_->getOutputIndexFromClassIndex(mixtureIndex);
	score = -featureVector.at(outputIndex);
	score += prior_.scale() * prior_.at(outputIndex);
    }
    else
	score = Core::Type<Mm::Score>::max;
    return score;
}

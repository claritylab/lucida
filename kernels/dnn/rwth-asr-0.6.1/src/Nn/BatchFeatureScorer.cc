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
#include "BatchFeatureScorer.hh"

#include <Math/Module.hh>

using namespace Nn;

const Core::ParameterInt BatchFeatureScorer::paramBufferSize(
	"buffer-size", "buffer size (and also batch size) for the feature scorer", 8);

BatchFeatureScorer::BatchFeatureScorer(const Core::Configuration& config, Core::Ref<const Mm::MixtureSet> mixtureSet) :
	Core::Component(config),
	Mm::FeatureScorer(config),
	bufferSize_(paramBufferSize(config)),
	prior_(config),
	nBufferedFeatures_(0),
	currentFeature_(0),
	scoreComputed_(bufferSize_, false),
	nClasses_(0),
	inputDimension_(0),
	labelWrapper_(0),
	network_(config)
{
    init(mixtureSet);
}

BatchFeatureScorer::~BatchFeatureScorer(){
    if (labelWrapper_)
	delete labelWrapper_;
}

void BatchFeatureScorer::init(Core::Ref<const Mm::MixtureSet> mixtureSet) {

    log("initialize nn-batch-feature-scorer with buffer size ") << bufferSize_;
    nClasses_ = mixtureSet->nMixtures();

    // (a) load class label wrapper
    if (labelWrapper_)
	delete labelWrapper_;
    labelWrapper_ = new ClassLabelWrapper(select("class-labels"), nClasses_);
    if (!labelWrapper_->isOneToOneMapping())
	error("no one-to-one correspondence between network outputs and classes!");

    // (b) initialize network and do some checks
    network_.initializeNetwork(bufferSize_);
    require_eq(network_.getTopLayer().getOutputDimension(), labelWrapper_->nClassesToAccumulate());
    LinearAndSoftmaxLayer<f32> *topLayer = dynamic_cast<LinearAndSoftmaxLayer<f32>* >(&network_.getTopLayer());
    if (!topLayer)
	error("output layer must be of type 'linear+softmax'");

    // compute only the scores
    topLayer->setEvaluateSoftmax(false);
    // ensure that there is only a single input stream
    if (network_.getLayer(0).nInputActivations() != 1)
	Core::Component::criticalError("Multiple input streams not implemented in BatchFeatureScorer.");

    inputDimension_ = network_.getLayer(0).getInputDimension(0);

    // (c) remove prior from bias
    // read
    if (prior_.fileName() != "")
	prior_.read();
    else
	prior_.setFromMixtureSet(mixtureSet, *labelWrapper_);

    // subtract
    network_.finishComputation();
    topLayer->removeLogPriorFromBias(prior_);
    network_.initComputation();

    // (d) set everything in the network to computation mode
    log("l1 norm of all network weights is: ") << network_.l1norm();
    log("batch feature scorer uses buffer size: ") << bufferSize_;

}

void BatchFeatureScorer::setFeature(u32 position, const Mm::FeatureVector& f) const {
    require_lt(position, bufferSize_);
    if (buffer_.nRows() != f.size())
	buffer_.resize(f.size(), bufferSize_);
    for (u32 i = 0; i < f.size(); i++)
	buffer_.at(i, position) = f.at(i);
}

void BatchFeatureScorer::addFeature(const Mm::FeatureVector &f) const {
    require(!bufferFilled());
    setFeature(nBufferedFeatures_, f);
    scoreComputed_[nBufferedFeatures_] = false;
    nBufferedFeatures_++;
}

void BatchFeatureScorer::reset() const {
    scoreComputed_.assign(bufferSize_, false);
    nBufferedFeatures_ = 0;
    currentFeature_ = 0;
}

Mm::FeatureScorer::Scorer BatchFeatureScorer::getScorer(const Mm::FeatureVector& f) const {
    require(bufferFilled());
    // store feature f
    u32 position = currentFeature_ ? (currentFeature_ - 1) % bufferSize_ : bufferSize_ - 1;
    setFeature(position, f);
    scoreComputed_[position] = false;
    // create scorer for feature at index currentFeature_
    Scorer scorer(new ContextScorer(this, currentFeature_));
    // update pointer to current feature (treat buffer as ring buffer)
    currentFeature_ = (currentFeature_ + 1) % bufferSize_;
    return scorer;
}

Mm::FeatureScorer::Scorer BatchFeatureScorer::flush() const {
    require(!bufferEmpty());
    // create scorer for feature at index currentFeature_
    Scorer scorer(new ContextScorer(this, currentFeature_));
    // update pointer to current feature
    currentFeature_ = (currentFeature_ + 1) % bufferSize_;
    nBufferedFeatures_--;
    if (bufferEmpty()) reset();
    return scorer;
}

Mm::Score BatchFeatureScorer::getScore(Mm::EmissionIndex e, u32 position) const {
    require_lt(position, bufferSize_);
    // process buffer, if score for position is not up-to-date
    if (!scoreComputed_[position]) {
	// set everything to computing state
	network_.initComputation();
	buffer_.initComputation();
	network_.forward(buffer_);
	// scores and buffer must be readable/writable
	network_.getTopLayerOutput().finishComputation();
	buffer_.finishComputation(false);
	// mark all scores in buffer as computed
	scoreComputed_.assign(bufferSize_, true);
    }
    // return score
    Mm::Score score = 0.0;
    if (labelWrapper_->isClassToAccumulate(e)) {
	score = -network_.getTopLayerOutput().at(labelWrapper_->getOutputIndexFromClassIndex(e), position);
    }
    else{
	score = Core::Type<Mm::Score>::max;
    }
    return score;
}

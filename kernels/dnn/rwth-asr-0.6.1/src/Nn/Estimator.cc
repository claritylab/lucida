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
#include "Estimator.hh"
#include "MeanNormalizedSgdEstimator.hh"
#include "RpropEstimator.hh"
#include "LinearLayer.hh"

using namespace Nn;

template<class T>
const Core::Choice Estimator<T>::choiceEstimatorType(
	"dummy", dummy,
	"steepest-descent", steepestDescentEstimator,
	"steepest-descent-l1-clipping", steepestDescentL1Clipping,
	"mean-normalized-steepest-descent", meanNormalizedSgd,
	"mean-normalized-steepest-descent-l1-clipping", meanNormalizedSgdL1Clipping,
	"rprop", rprop,
	"prior-estimator", priorEstimator,
	Core::Choice::endMark());

template<class T>
const Core::ParameterChoice Estimator<T>::paramEstimatorType(
	"estimator", &choiceEstimatorType,
	"estimator for weights estimation in training", dummy);

template<typename T>
const Core::ParameterBool Estimator<T>::paramBatchMode(
	"batch-mode",
	"use batch estimator, i.e. do not update after each mini-batch, but accumulate statistics", false);


template<typename T>
const Core::ParameterFloat Estimator<T>::paramLearningRate("learning-rate", "(initial) learning-rate", 1.0);

template<typename T>
const Core::ParameterFloat Estimator<T>::paramBiasLearningRate(
	"bias-learning-rate", "bias is optimized with bias-learning-rate * learning-rate", 1.0);

template<typename T>
const Core::ParameterBool Estimator<T>::paramLogStepSize(
	"log-step-size", "log the step size, if true", false);

template<typename T>
Estimator<T>::Estimator(const Core::Configuration& config) :
	Precursor(config),
	statisticsChannel_(config, "statistics"),
	batchMode_(paramBatchMode(config)),
	initialLearningRate_(paramLearningRate(config)),
	biasLearningRate_(paramBiasLearningRate(config)),
	logStepSize_(paramLogStepSize(config))
{
    this->log("initial learning rate: ") << initialLearningRate_;
    if (batchMode_)
	this->log("using batch estimator");
    if (biasLearningRate_ != 1.0)
	this->log("bias learning rate: ") << biasLearningRate_;
    if (logStepSize_) {
	this->log("logging step size norm");
    }
}

template<class T>
Estimator<T>* Estimator<T>::createEstimator(const Core::Configuration& config) {
    Estimator<T>* estimator;

    switch ( (EstimatorType) paramEstimatorType(config) ) {
    case steepestDescentEstimator:
	estimator = new SteepestDescentEstimator<T>(config);
	Core::Application::us()->log("Create Estimator: steepest-descent");
	break;
    case steepestDescentL1Clipping:
	estimator = new SteepestDescentL1ClippingEstimator<T>(config);
	Core::Application::us()->log("Create Estimator: steepest-descent-l1-clipping");
	break;
    case meanNormalizedSgd:
	estimator = new MeanNormalizedSgd<T>(config);
	Core::Application::us()->log("Create Estimator: mean-normalized-steepest-descent");
	break;
    case meanNormalizedSgdL1Clipping:
	estimator = new MeanNormalizedSgdL1Clipping<T>(config);
	Core::Application::us()->log("Create Estimator: mean-normalized-steepest-descent-l1-clipping-estimator");
	break;
    case rprop:
	estimator = new RpropEstimator<T>(config);
	Core::Application::us()->log("Create Estimator: Rprop");
	break;
    case priorEstimator:
	estimator = new PriorEstimator<T>(config);
	Core::Application::us()->log("Create Estimator: Prior estimator");
	break;
    default:
	estimator = new Estimator<T>(config);
	Core::Application::us()->log("Create Estimator: dummy");
	break;
    };

    return estimator;
}

//=============================================================================

template<typename T>
const Core::ParameterBool SteepestDescentEstimator<T>::paramUsePredefinedLearningRateDecay(
	"use-predefined-learning-rate-decay", "use learning-rate * tau / (tau + numberOfUpdates) as learning-rate", false);

template<typename T>
const Core::ParameterFloat SteepestDescentEstimator<T>::paramLearningRateTau("learning-rate-tau", "", 1000.0);

template<typename T>
const Core::ParameterInt SteepestDescentEstimator<T>::paramNumberOfUpdates(
	"number-of-updates", "number of updates done so far", 0);

template<typename T>
const Core::ParameterFloat SteepestDescentEstimator<T>::paramMomentumFactor(
	"momentum-factor", "momentum factor, suggested value: 0.9", 0.0);

template<typename T>
SteepestDescentEstimator<T>::SteepestDescentEstimator(const Core::Configuration& config) :
Core::Component(config),
Precursor(config),
usePredefinedLearningRateDecay_(paramUsePredefinedLearningRateDecay(config)),
tau_(paramLearningRateTau(config)),
nUpdates_(paramNumberOfUpdates(config)),
momentumFactor_(paramMomentumFactor(config)),
momentum_(momentumFactor_ > 0),
oldDeltas_(0)
{
    if (usePredefinedLearningRateDecay_){
	this->log("using predefined learning rate decay with parameter tau: ") << tau_;
	this->log("number of updates so far is ") << nUpdates_;
    }
    if (momentum_)
	this->log("using momentum with momentum factor: ") << momentumFactor_;
}

template<typename T>
SteepestDescentEstimator<T>::~SteepestDescentEstimator()
{
    if (momentum_) {
	delete oldDeltas_;
    }
}

template<typename T>
void SteepestDescentEstimator<T>::estimate(NeuralNetwork<T>& network, Statistics<T>& statistics) {

    T learningRate = initialLearningRate_;
    if (usePredefinedLearningRateDecay_) {
	learningRate = initialLearningRate_ * tau_ / (tau_ + nUpdates_);
    }

    /* estimation of parameters */
    require(statistics.hasGradient());

    // if momentum is used and oldDeltas_ not yet initialized, just copy the statistics
    if (momentum_ && (oldDeltas_ == 0)) {
	oldDeltas_ = new Statistics<T>(statistics);
    }

    if (usePredefinedLearningRateDecay_ && statisticsChannel_.isOpen())
	statisticsChannel_ << "learningRate: " << learningRate;

    T stepSize = 0;
    for (u32 layer = 0; layer < network.nLayers(); layer++) {
	if (network.getLayer(layer).isTrainable()) {

	    /* estimation of weights */
	    for (u32 stream = 0; stream < network.getLayer(layer).nInputActivations(); stream++) {
		NnMatrix *weights = network.getLayer(layer).getWeights(stream);
		require(weights);
		/* regular update, if no momentum used */
		if (!momentum_) {
		    weights->add(statistics.gradientWeights(layer)[stream],
			    (T) -learningRate * network.getLayer(layer).learningRate());
		}
		/* momentum update */
		else {
		    // update old deltas (include new statistics)
		    oldDeltas_->gradientWeights(layer)[stream].scale(momentumFactor_);
		    oldDeltas_->gradientWeights(layer)[stream].add(statistics.gradientWeights(layer)[stream], (T) (1 - momentumFactor_));
		    // update weights
		    weights->add(oldDeltas_->gradientWeights(layer)[stream],
			    (T) -learningRate * network.getLayer(layer).learningRate());
		}
		/* log step size */
		if (logStepSize_) {
		    if (!momentum_) {
			stepSize += statistics.gradientWeights(layer)[stream].l1norm()
					* learningRate * network.getLayer(layer).learningRate();
		    }
		    else {
			stepSize += oldDeltas_->gradientWeights(layer)[stream].l1norm()
					* learningRate * network.getLayer(layer).learningRate();
		    }
		}
	    }

	    /* estimation of bias */
	    NnVector *bias = network.getLayer(layer).getBias();
	    require(bias);
	    /* regular update, if no momentum used */
	    if (!momentum_) {
		bias->add(statistics.gradientBias(layer),
			(T) -learningRate * biasLearningRate_ * network.getLayer(layer).learningRate());
	    }
	    /* momentum update */
	    else {
		// update old deltas (include new statistics)
		oldDeltas_->gradientBias(layer).scale(momentumFactor_);
		oldDeltas_->gradientBias(layer).add(statistics.gradientBias(layer), (T) (1 - momentumFactor_));
		// update bias
		bias->add(oldDeltas_->gradientBias(layer),
			(T) -learningRate * biasLearningRate_ * network.getLayer(layer).learningRate());
	    }
	    /* log step size */
	    if (logStepSize_) {
		if (!momentum_) {
		    stepSize += statistics.gradientBias(layer).l1norm()
				    * learningRate * biasLearningRate_ * network.getLayer(layer).learningRate();
		}
		else {
		    stepSize += oldDeltas_->gradientBias(layer).l1norm()
				    * learningRate * biasLearningRate_ * network.getLayer(layer).learningRate();
		}
	    }
	}
    }

    if (logStepSize_ && statisticsChannel_.isOpen()) {
	statisticsChannel_ << "step-size: " << stepSize;
    }

    nUpdates_++;
}

//=============================================================================


template<typename T>
SteepestDescentL1ClippingEstimator<T>::SteepestDescentL1ClippingEstimator(const Core::Configuration& config) :
Core::Component(config),
Precursor(config)
{}

template<typename T>
void SteepestDescentL1ClippingEstimator<T>::estimate(NeuralNetwork<T>& network, Statistics<T>& statistics) {

    Precursor::estimate(network, statistics);

    T learningRate = this->initialLearningRate_;
    if (this->usePredefinedLearningRateDecay_) {
	learningRate = this->initialLearningRate_ * this->tau_ / (this->tau_ + this->nUpdates_ - 1);
    }

    for (u32 layer = 0; layer < network.nLayers(); layer++) {
	if (network.getLayer(layer).isTrainable()) {
	    /* estimation of weights */
	    for (u32 stream = 0; stream < network.getLayer(layer).nInputActivations(); stream++) {
		NnMatrix *weights = network.getLayer(layer).getWeights(stream);
		require(weights);
		weights->l1clipping(network.getLayer(layer).regularizationConstant() * learningRate * network.getLayer(layer).learningRate());
	    }
	    /* estimation of bias */
	    NnVector *bias = network.getLayer(layer).getBias();
	    require(bias);
	    bias->l1clipping(network.getLayer(layer).regularizationConstant() * learningRate * network.getLayer(layer).learningRate());
	}
    }

    if (logStepSize_ && statisticsChannel_.isOpen()) {
	statisticsChannel_ << "step size does not include l1-regularization";
    }
}

//=============================================================================

template<typename T>
PriorEstimator<T>::PriorEstimator(const Core::Configuration& config) :
Core::Component(config),
Precursor(config)
{
    if (!this->batchMode_){
	this->batchMode_ = true;
	this->log("using batch mode, because prior estimation only possible in batch mode");
    }
}


// explicit template instantiation
namespace Nn {

template class Estimator<f32>;
template class Estimator<f64>;

template class SteepestDescentEstimator<f32>;
template class SteepestDescentEstimator<f64>;

template class SteepestDescentL1ClippingEstimator<f32>;
template class SteepestDescentL1ClippingEstimator<f64>;

template class PriorEstimator<f32>;
template class PriorEstimator<f64>;

}

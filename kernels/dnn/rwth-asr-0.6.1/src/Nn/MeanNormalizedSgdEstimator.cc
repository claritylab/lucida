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
#include "MeanNormalizedSgdEstimator.hh"
#include <algorithm>

using namespace Nn;

template<typename T>
MeanNormalizedSgd<T>::MeanNormalizedSgd(const Core::Configuration& config) :
Core::Component(config),
Precursor(config),
firstEstimation_(true)
{}

/**
 * for all trainable layers:
 * run through all preceeding layers and ensure they collect activation statistics
 */
template<typename T>
void MeanNormalizedSgd<T>::checkForStatistics(NeuralNetwork<T>& network) {
    for (u32 layer = 0; layer < network.nLayers(); layer++) {
	if (network.getLayer(layer).isTrainable()) {
	    for (u32 stream = 0; stream < network.getLayer(layer).nPredecessors(); stream++) {
		u32 predecessor = network.getLayer(layer).getPredecessor(stream);
		if (!network.getLayer(predecessor).hasActivationStatistics()) {
		    Core::Component::log()
		    << network.getLayer(predecessor).getName()
		    << " is a predecessor of " << network.getLayer(layer).getName()
		    << ", but has no activation statistics. Assume zero mean for this input stream.";
		}
	    }
	}
    }
}

/**
 * estimation with mean-normalized SGD (see Master Thesis of Alexander Richard)
 */
template<typename T>
void MeanNormalizedSgd<T>::estimate(NeuralNetwork<T>& network, Statistics<T>& statistics) {

    if (firstEstimation_) {
	checkForStatistics(network);
	firstEstimation_ = false;
    }

    T learningRate = initialLearningRate_;

    /* estimation of parameters */
    require(statistics.hasGradient());

    T stepSize = 0;
    for (u32 layer = 0; layer < network.nLayers(); layer++) {
	if (network.getLayer(layer).isTrainable()) {

	    /* modify weights gradient and update weights */
	    for (u32 stream = 0; stream < network.getLayer(layer).nPredecessors(); stream++) {
		// if layer has predecessors with activation statistics...
		u32 predecessor = network.getLayer(layer).getPredecessor(stream);
		if (network.getLayer(predecessor).hasActivationStatistics()) {
		    // ... modify gradient of weights for the current input stream
		    statistics.gradientWeights(layer)[stream].addOuterProduct(
			    network.getLayer(predecessor).getActivationMean(),
			    statistics.gradientBias(layer), -1.0);
		}
		// update weights
		NnMatrix *weights = network.getLayer(layer).getWeights(stream);
		require(weights);
		weights->add(statistics.gradientWeights(layer)[stream],
			(T) -learningRate * network.getLayer(layer).learningRate());
		// log step size
		if (logStepSize_) {
		    stepSize += statistics.gradientWeights(layer)[stream].l1norm() * learningRate
			    * network.getLayer(layer).learningRate();
		}
	    }

	    /* modify bias gradient and update bias */
	    for (u32 stream = 0; stream < network.getLayer(layer).nPredecessors(); stream++) {
		// if layer has predecessors with activation statistics...
		u32 predecessor = network.getLayer(layer).getPredecessor(stream);
		if (network.getLayer(predecessor).hasActivationStatistics()) {
		    // ... modify gradient of bias
		    statistics.gradientWeights(layer)[stream].multiply(network.getLayer(predecessor).getActivationMean(),
			    statistics.gradientBias(layer), true, -1.0, 1.0);
		}
	    }
	    // update bias
	    NnVector *bias = network.getLayer(layer).getBias();
	    require(bias);
	    bias->add(statistics.gradientBias(layer),
		    (T) -learningRate * biasLearningRate_ * network.getLayer(layer).learningRate());
	    /* log step size */
	    if (logStepSize_) {
		stepSize += statistics.gradientBias(layer).l1norm() * learningRate
			* biasLearningRate_ * network.getLayer(layer).learningRate();
	    }
	}
    }
    if (logStepSize_ && statisticsChannel_.isOpen()) {
	statisticsChannel_ << "step-size: " << stepSize;
    }
}

//=============================================================================

template<typename T>
MeanNormalizedSgdL1Clipping<T>::MeanNormalizedSgdL1Clipping(const Core::Configuration& config) :
Core::Component(config),
Precursor(config)
{}

template<typename T>
void MeanNormalizedSgdL1Clipping<T>::estimate(NeuralNetwork<T>& network, Statistics<T>& statistics) {

    Precursor::estimate(network, statistics);

    T learningRate = this->initialLearningRate_;

    for (u32 layer = 0; layer < network.nLayers(); layer++) {
	if (network.getLayer(layer).isTrainable()) {
	    if (network.getLayer(layer).nInputActivations() != 1) {
		Core::Application::us()->criticalError("Estimation for multiple streams not yet implemented.");
	    }
	    NnMatrix *weights = network.getLayer(layer).getWeights(0);
	    NnVector *bias = network.getLayer(layer).getBias();
	    require(weights);
	    require(bias);
	    weights->l1clipping(network.getLayer(layer).regularizationConstant() * learningRate * network.getLayer(layer).learningRate());
	    bias->l1clipping(network.getLayer(layer).regularizationConstant() * learningRate * network.getLayer(layer).learningRate());
	}
    }

    if (logStepSize_ && statisticsChannel_.isOpen()) {
	statisticsChannel_ << "step size does not include l1-regularization";
    }
}

//=============================================================================

// explicit template instantiation
namespace Nn {

template class MeanNormalizedSgd<f32>;
template class MeanNormalizedSgd<f64>;

template class MeanNormalizedSgdL1Clipping<f32>;
template class MeanNormalizedSgdL1Clipping<f64>;

}

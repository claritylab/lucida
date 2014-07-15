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
#include "RpropEstimator.hh"

#include "Statistics.hh"

using namespace Nn;

template<typename T>
const Core::ParameterString RpropEstimator<T>::paramStepSizesOld(
	"old-step-sizes", "old step sizes file", "");

template<typename T>
const Core::ParameterString RpropEstimator<T>::paramStepSizesNew(
	"new-step-sizes", "new step sizes file", "");

template<typename T>
const Core::ParameterString RpropEstimator<T>::paramPreviousStatistics(
	"previous-statistics", "previous statistics filename", "");

template<typename T>
const Core::ParameterFloat RpropEstimator<T>::paramIncreasingFactor(
	"increasing-factor", "factor to increase step size", 1.2);

template<typename T>
const Core::ParameterFloat RpropEstimator<T>::paramDecreasingFactor(
	"decreasing-factor", "factor to decrease step size", 0.5);

template<typename T>
const Core::ParameterFloat RpropEstimator<T>::paramRelativeInitialStepSize(
	"relative-initial-step-size", "choose initial step size proportional to average weight of parameters", 0.0);

template<typename T>
const Core::Choice RpropEstimator<T>::choiceInitializationType(
	"constant", constant,
	"block-wise-average", blockwiseAverage,
	"min-block-average", minBlockAverage,
	Core::Choice::endMark());

template<typename T>
const Core::ParameterChoice RpropEstimator<T>::paramInitializationType(
	"step-size-initialization-type", &choiceInitializationType,
	"type of step size initialization", constant);


template<typename T>
RpropEstimator<T>::RpropEstimator(const Core::Configuration& c) :
    Core::Component(c),
    Precursor(c),
    increasingFactor_(paramIncreasingFactor(c)),
    decreasingFactor_(paramDecreasingFactor(c)),
    relativeInitialStepSize_(paramRelativeInitialStepSize(c)),
    initializationType_((initializationType)paramInitializationType(c)),
    stepSizes_(0),
    prevStatistics_(0),
    needInit_(true)
{
    logProperties();
}

template<typename T>
RpropEstimator<T>::~RpropEstimator() {
    if (stepSizes_)
	delete stepSizes_;
    if (prevStatistics_)
	delete prevStatistics_;
}

template<typename T>
void RpropEstimator<T>::estimate(NeuralNetwork<T>& network, Statistics<T>& statistics) {
    require(statistics.hasGradient());

    if (needInit_)
	initialize(network);

    verify(stepSizes_);

    // do everything on CPU (currently Rprop is only used in batch mode)
    network.finishComputation();

    statistics.finishComputation();
    stepSizes_->finishComputation();
    if (prevStatistics_)
	prevStatistics_->finishComputation();

    // update step sizes (except of first iteration)
    if (prevStatistics_){
	for (u32 layer = 0; layer < network.nLayers(); layer++) {
	    if (network.getLayer(layer).isTrainable()) {
		for (u32 stream = 0; stream < network.getLayer(layer).nInputActivations(); stream++) {
		    NnMatrix &gradient = statistics.gradientWeights(layer).at(stream);
		    NnMatrix &prevGradient = prevStatistics_->gradientWeights(layer).at(stream);
		    NnMatrix &stepSizes = stepSizes_->gradientWeights(layer).at(stream);
		    for (u32 i = 0; i < gradient.nRows(); i++){
			for (u32 j = 0; j < gradient.nColumns(); j++){
			    updateStepSize(prevGradient.at(i,j), gradient.at(i,j), stepSizes.at(i,j));
			}
		    }
		}
		NnVector &gradient = statistics.gradientBias(layer);
		NnVector &prevGradient = prevStatistics_->gradientBias(layer);
		NnVector &stepSizes = stepSizes_->gradientBias(layer);
		for (u32 i = 0; i < gradient.nRows(); i++)
		    updateStepSize(prevGradient.at(i), gradient.at(i), stepSizes.at(i));
	    }
	}
    }
    // update parameters
    for (u32 layer = 0; layer < network.nLayers(); layer++) {
	if (network.getLayer(layer).isTrainable()) {
	    for (u32 stream = 0; stream < network.getLayer(layer).nInputActivations(); stream++) {
		NnMatrix *weights = network.getLayer(layer).getWeights(stream);
		require(weights);
		NnMatrix &gradient = statistics.gradientWeights(layer).at(stream);
		NnMatrix &stepSizes = stepSizes_->gradientWeights(layer).at(stream);
		// update weights
		for (u32 i = 0; i < gradient.nRows(); i++){
		    for (u32 j = 0; j < gradient.nColumns(); j++)
			updateParameter(stepSizes.at(i,j), gradient.at(i,j), weights->at(i,j));
		}
	    }
	    NnVector *bias = network.getLayer(layer).getBias();
	    require(bias);
	    NnVector &gradient = statistics.gradientBias(layer);
	    NnVector &stepSizes = stepSizes_->gradientBias(layer);

	    // update bias
	    for (u32 i = 0; i < gradient.nRows(); i++)
		updateParameter(stepSizes.at(i), gradient.at(i), bias->at(i));
	}
    }
    if (this->logStepSize_ && this->statisticsChannel_.isOpen()) {
	T normStepSize = 0;
	for (u32 layer = 0; layer < network.nLayers(); layer++) {
	    if (network.getLayer(layer).isTrainable()) {
		for (u32 stream = 0; stream < network.getLayer(layer).nInputActivations(); stream++) {
		    NnMatrix &stepSizes = stepSizes_->gradientWeights(layer).at(stream);
		    stepSizes.initComputation();
		    normStepSize += stepSizes.l1norm();
		    stepSizes.finishComputation(false);
		}
		NnVector &stepSizes = stepSizes_->gradientBias(layer);
		stepSizes.initComputation();
		normStepSize += stepSizes.l1norm();
		stepSizes.finishComputation(false);
	    }
	}
	this->statisticsChannel_ << "step-size: " << normStepSize;
    }

    if (!this->batchMode_){
	if (prevStatistics_)
	    delete prevStatistics_;
	prevStatistics_ = new Statistics<T>(statistics);
    }
    else {
	if (!stepSizes_->write(paramStepSizesNew(this->config)))
	    this->error("failed to write step sizes to ") << paramStepSizesNew(this->config);
    }
}

template<typename T>
void RpropEstimator<T>::initialize(const NeuralNetwork<T>& network){
    if (needInit_){
	verify(!stepSizes_);
	verify(!prevStatistics_);

	if (!network.isComputing())
	    network.initComputation();
	T parameterNorm = network.l1norm(true);

	// determine minimum of block averages
	T blockMin = Core::Type<T>::max;
	for (u32 layer = 0; layer < network.nLayers(); layer++) {
	    if (network.getLayer(layer).isTrainable()) {
		for (u32 stream = 0; stream < network.getLayer(layer).nInputActivations(); stream++) {
		    const NnMatrix *weights = network.getLayer(layer).getWeights(stream);
		    T avgBlockNorm = weights->l1norm() / weights->size();
		    blockMin = std::min(blockMin, avgBlockNorm);
		}
		const NnVector *bias = network.getLayer(layer).getBias();
		T avgBlockNorm = bias->l1norm() / bias->size();
		blockMin = std::min(blockMin, avgBlockNorm);
	    }
	}

	u32 nParameters = network.nFreeParameters();
	T avgParameterNorm = parameterNorm / nParameters;
	this->log("average norm of trainable parameters: ") << avgParameterNorm;

	// step sizes are stored in gradient-field of statistics, which has exactly the same structure
	stepSizes_ = new Statistics<T>(network.nLayers(), Statistics<T>::GRADIENT);
	stepSizes_->initialize(network);


	std::string filename = paramStepSizesOld(this->config);
	if (filename != ""){
	    if (!stepSizes_->read(paramStepSizesOld(this->config)))
		this->error("failed to read step size file ") << filename;

	    require(paramPreviousStatistics(this->config) != ""); // need previous gradient (except of the initial step)

	    u32 statisticsTypeFile = 0;
	    bool dummy;
	    if (!Nn::Statistics<T>::getTypeFromFile(paramPreviousStatistics(this->config), statisticsTypeFile, dummy))
		this->error("could not read header from file: ") << paramPreviousStatistics(this->config);
	    u32 requiredStats = requiredStatistics();
	    u32 statisticsType = statisticsTypeFile | requiredStats; // union
	    prevStatistics_ = new Statistics<T>(network.nLayers(), statisticsType);
	    prevStatistics_->initialize(network);
	    if (!prevStatistics_->read(paramPreviousStatistics(this->config)))
		this->error("failed to read previous statistics from file ") << paramPreviousStatistics(this->config);
	}
	else {
	    stepSizes_->initComputation(false);
	    for (u32 layer = 0; layer < network.nLayers(); layer++) {
		if (network.getLayer(layer).isTrainable()) {
		    for (u32 stream = 0; stream < network.getLayer(layer).nInputActivations(); stream++) {
			T initialStepSize = 0;
			const NnMatrix *weights = network.getLayer(layer).getWeights(stream);
			if (initializationType_ == constant)
			    initialStepSize = relativeInitialStepSize_ != 0 ? relativeInitialStepSize_ * avgParameterNorm : this->initialLearningRate_;
			else if (initializationType_ == minBlockAverage)
			    initialStepSize = blockMin * relativeInitialStepSize_;
			else if (initializationType_ == blockwiseAverage)
			    initialStepSize = weights->l1norm() / weights->size() * relativeInitialStepSize_;
			stepSizes_->gradientWeights(layer).at(stream).fill(initialStepSize);
		    }
		    T initialStepSize = 0;
		    const NnVector *bias = network.getLayer(layer).getBias();
		    if (initializationType_ == constant)
			initialStepSize = relativeInitialStepSize_ != 0 ? relativeInitialStepSize_ * avgParameterNorm : this->initialLearningRate_;
		    else if (initializationType_ == minBlockAverage)
			initialStepSize = blockMin * relativeInitialStepSize_;
		    else if (initializationType_ == blockwiseAverage)
			initialStepSize = bias->l1norm() / bias->size() * relativeInitialStepSize_;
		    stepSizes_->gradientBias(layer).fill(initialStepSize);
		}
	    }
	    stepSizes_->finishComputation();
	}
	stepSizes_->initComputation();
	this->log("average norm of step sizes: ") << (stepSizes_->gradientL1Norm() / nParameters);
	stepSizes_->finishComputation(false);
	network.finishComputation(false);

	needInit_ = false;

    }
}

template<typename T>
inline void RpropEstimator<T>::updateStepSize(const T &prevGradient, const T& gradient, T& stepSize) const {
    if (stepSize < 0)
	stepSize *= -1.0;			// un-set hold
    else if (prevGradient * gradient >= 0) 	// no sign change in gradient
	stepSize *= increasingFactor_;
    else					// sign change, negative sign marks hold, i.e. no step is done
	stepSize *= -decreasingFactor_;
}

template<typename T>
inline void RpropEstimator<T>::updateParameter(const T &stepSize, const T& gradient, T& parameter) const {
    if (stepSize > 0)
	parameter -= stepSize * signum(gradient); // minimization problem formulation -> go into direction of negative gradient
}

template<typename T>
void RpropEstimator<T>::logProperties() const {
    if (relativeInitialStepSize_ != 0){
	this->log("using initial step size relative to parameter norm, factor: ") << relativeInitialStepSize_;
	if (initializationType_ == blockwiseAverage)
	    this->log("averaging initial step size block-wise");
	else if (initializationType_ == minBlockAverage)
	    this->log("setting initial step size to minimum of block-wise averages times ") << relativeInitialStepSize_;
    }
}


namespace Nn {
template class RpropEstimator<f32>;
template class RpropEstimator<f64>;
}

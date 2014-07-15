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
#include "LinearLayer.hh"

#include <Math/Module.hh>
#include <Math/Vector.hh>
#include <Math/Matrix.hh>

using namespace Nn;


template<typename T>
const Core::Choice LinearLayer<T>::choiceInitializationType(
	"zero", zero,
	"random", random,
	"identity", identity,
	"file", file,
	Core::Choice::endMark());

template<typename T>
const Core::ParameterChoice LinearLayer<T>::paramInitializationType(
	"initialization-type", &choiceInitializationType,
	"initialization method for the parameters", random);

template<typename T>
const Core::ParameterFloat LinearLayer<T>::paramBiasInitializationRangeMin(
	"bias-initialization-range-min", "minimal value for random initialization of bias", -0.1);

template<typename T>
const Core::ParameterFloat LinearLayer<T>::paramBiasInitializationRangeMax(
	"bias-initialization-range-max", "maximal value for random initialization of bias", 0.1);

template<typename T>
const Core::ParameterFloat LinearLayer<T>::paramWeightInitializationRangeMin(
	"weight-initialization-range-min", "minimal value for random initialization of weights", -0.1);

template<typename T>
const Core::ParameterFloat LinearLayer<T>::paramWeightInitializationRangeMax(
	"weight-initialization-range-max", "maximal value for random initialization of weights", 0.1);

template<typename T>
const Core::ParameterFloat LinearLayer<T>::paramWeightInitializationScalingFactor(
	"weight-scaling-factor", "factor to scale diagonal in identity initialization", 1.0);

template<typename T>
const Core::ParameterBool LinearLayer<T>::paramIgnoreParameterFile(
	"ignore-parameter-file", "do not read parameters of this layer from file", false);

template<typename T>
const Core::ParameterBool LinearLayer<T>::paramHasBias(
	"has-bias", "has bias", true);

template<typename T>
const Core::ParameterString LinearLayer<T>::paramParameterFile(
	"parameter-file", "read parameters of this layer from file", "");

template<typename T>
const Core::ParameterBool LinearLayer<T>::paramTrainable(
	"trainable", "Can the parameters of this layer be trained?", true);


template<typename T>
LinearLayer<T>::LinearLayer(const Core::Configuration &config) :
    Core::Component(config),
    NeuralNetworkLayer<T>(config),
    initializationType_((InitializationType)paramInitializationType(config)),
    biasInitializationRangeMin_(paramBiasInitializationRangeMin(config)),
    biasInitializationRangeMax_(paramBiasInitializationRangeMax(config)),
    weightInitializationRangeMin_(paramWeightInitializationRangeMin(config)),
    weightInitializationRangeMax_(paramWeightInitializationRangeMax(config)),
    weightInitializationIdentityScalingFactor_(paramWeightInitializationScalingFactor(config)),
    ignoreParameterFile_(paramIgnoreParameterFile(config)),
    hasBias_(paramHasBias(config)),
    bias_(0),
    weights_(0),
    parameterFile_(paramParameterFile(config)),
    trainable_(paramTrainable(config)),
    timeForwardLinear_(0),
    timeForwardBias_(0),
    timeBackward_(0)
{}

template<typename T>
LinearLayer<T>::~LinearLayer() {}

template<typename T>
void LinearLayer<T>::setInputDimension(u32 stream, u32 size) {
    Precursor::setInputDimension(stream, size);
    if ((weights_.size() <= stream) || (weights_[stream].nRows() != size)) {
	Precursor::needInit_ = true;
    }
}

template<typename T>
void LinearLayer<T>::setOutputDimension(u32 size) {
    Precursor::outputDimension_ = size;
    bool sizeFits = (bias_.nRows() == size);
    for (u32 stream = 0; stream < weights_.size(); stream++) {
	if (weights_[stream].nRows() != size) sizeFits = false;
    }
    if (!sizeFits) {
	Precursor::needInit_ = true;
    }
}

template<typename T>
void LinearLayer<T>::initializeNetworkParameters() {
    for (u32 stream = 0; stream < weights_.size(); stream++){
	require(!weights_[stream].isComputing());
    }
    require(!bias_.isComputing());
    switch (initializationType_) {
    case random:
	Core::Component::log("Initialize parameters of ") << Precursor::getName() << " randomly";
	initializeParametersRandomly();
	break;
    case identity:
	Core::Component::log("Initialize parameters of ") << Precursor::getName() << " with the identity matrix";
	initializeParametersWithIdentityMatrix();
	break;
    case file:
	Core::Component::log("Initialize parameters of ") << Precursor::getName() << " from file";
	loadNetworkParameters(parameterFile_);
	break;
    case zero:
    default:
	Core::Component::log("Initialize parameters of ") << Precursor::getName() << " with zero";
	initializeParametersWithZero();
	break;
    };

    // Initialization done
    Precursor::needInit_ = false;
}

/**	Initialize the weights with random values */
template<typename T>
void LinearLayer<T>::initializeParametersRandomly() {
    // initialize the bias and weights (resize)
    bias_.resize(Precursor::getOutputDimension(), 0, true);
    weights_.resize(Precursor::nInputActivations());
    for (u32 stream = 0; stream < weights_.size(); stream++)
	weights_.at(stream).resize(Precursor::getInputDimension(stream), Precursor::getOutputDimension());

    require(weightInitializationRangeMin_ < weightInitializationRangeMax_);
    require(biasInitializationRangeMin_ < biasInitializationRangeMax_);

    // initialization of random number generator in Tools/NnTrainer

    // ... randomize the weights ...
    for (u32 stream = 0; stream < Precursor::nInputActivations(); stream++) {
	for (u32 row = 0; row < weights_.at(stream).nRows(); row++) {
	    for (u32 column = 0; column < weights_.at(stream).nColumns(); ++column) {
		weights_.at(stream).at(row, column) = ((weightInitializationRangeMax_
			- weightInitializationRangeMin_) * (T) rand() / RAND_MAX)
			+ weightInitializationRangeMin_;
	    }
	}
    }

    // ... and randomize the bias
    for (u32 row = 0; row < bias_.nRows(); row++)
	bias_.at(row) = hasBias_ ? ((biasInitializationRangeMax_ - biasInitializationRangeMin_) * (T) rand() / RAND_MAX)
		+ biasInitializationRangeMin_ : 0;
    // Initialization done
    Precursor::needInit_ = false;
}

/**	Initialize the weights with zero */
template<typename T>
void LinearLayer<T>::initializeParametersWithZero() {
    // initialize the bias and weights (resize)
    bias_.resize(Precursor::getOutputDimension(), 0, true);
    weights_.resize(Precursor::nInputActivations());
    for (u32 stream = 0; stream < weights_.size(); stream++) {
	weights_.at(stream).resize(Precursor::getInputDimension(stream), Precursor::getOutputDimension());

	for (u32 row = 0; row < weights_[stream].nRows(); row++) {
	    for (u32 column = 0; column < weights_[stream].nColumns(); ++column) {
		weights_.at(stream).at(row, column) = 0;
	    }
	}
    }
    for (u32 row = 0; row < bias_.nRows(); row++) {
	bias_.at(row) = 0;
    }
    // Initialization done
    Precursor::needInit_ = false;
}

/**	Initialize the weights with zero */
template<typename T>
void LinearLayer<T>::initializeParametersWithIdentityMatrix() {
    initializeParametersWithZero();

    for (u32 stream = 0; stream < weights_.size(); stream++) {
	u32 n = std::min(weights_.at(stream).nRows(), weights_.at(stream).nColumns());
	for (u32 i = 0; i < n; i++) {
	    weights_.at(stream).at(i,i) = weightInitializationIdentityScalingFactor_;
	}
    }

    // Initialization done
    Precursor::needInit_ = false;
}

/**	Initialize the weights from file */
template<typename T>
void LinearLayer<T>::loadNetworkParameters(const std::string &filename) {
    for (u32 stream = 0; stream < weights_.size(); stream++){
	require(!weights_[stream].isComputing());
    }
    require(!bias_.isComputing());
    if (ignoreParameterFile_) {
	Core::Component::log("Ignore parameter file for ") << Precursor::getName();
	initializeNetworkParameters();
    }
    else {
	Core::Component::log("reading parameter file ") << filename << " for layer " << Precursor::getName();
	Math::Matrix<T> parameters;
	Math::Module::instance().formats().read(filename, parameters);
	setParameters(parameters);
    }

    // Initialization done
    Precursor::needInit_ = false;
}

/**	Save weights to file */
template<typename T>
inline void LinearLayer<T>::saveNetworkParameters(const std::string &filename) const {
    // synchronization
    bool weightsComputing = weights_.at(0).isComputing();
    bool biasComputing = bias_.isComputing();
    if (weightsComputing) {
	for (u32 stream = 0; stream < weights_.size(); stream++)
	    weights_.at(stream).finishComputation();
    }
    if (biasComputing)
	bias_.finishComputation();

    // save bias and weights in one file
    // get the size of the current layer ...
    u32 inputSize = 0;
    for (u32 stream = 0; stream < Precursor::nInputActivations(); stream ++) {
	inputSize += Precursor::getInputDimension(stream);
    }
    if (hasBias_)
	inputSize++;

    u32 outputSize = Precursor::getOutputDimension();

    Math::Matrix<T> parameters(outputSize, inputSize);

    // Convert Math::FastVector + Math::FastMatrix -> Math::Matrix
    // bias and all weight matrices are concatenated in output file
    for (u32 i = 0; i < outputSize; ++i) {
	if (hasBias_)
	    parameters[i][0] = bias_.at(i);

	u32 k = hasBias_ ? 1 : 0; // start at 1 because of the bias
	for (u32 stream = 0; stream < Precursor::nInputActivations(); stream++) {
	    for (u32 j = 0; j < weights_.at(stream).nRows(); ++j) {
		parameters[i][k] = weights_.at(stream).at(j, i);
		k++;
	    }
	}
    }

    // write matrix to file
    Math::Module::instance().formats().write(filename, parameters, 20);

    // synchronization

    if (weightsComputing) {
	for (u32 stream = 0; stream < weights_.size(); stream++)
	    weights_.at(stream).initComputation(false);
    }
    if (biasComputing)
	bias_.initComputation(false);
}

/**	Forward the input */
template<typename T>
void LinearLayer<T>::_forward(const std::vector<NnMatrix*>& input, NnMatrix& output, bool reset) {
    // boundary check
    require_eq(bias_.size(), output.nRows());
    require_eq(weights_.size(), input.size());
    timeval start, end;

    // first: (input * weight) for each weight matrix (note: first stream handled separately due to reset flag)
    gettimeofday(&start, NULL);
    output.addMatrixProduct(weights_[0], *(input.at(0)), (reset ? T(0) : T(1)), T(1), true, false);
    for (u32 stream = 1; stream < weights_.size(); stream++) {
	output.addMatrixProduct(weights_.at(stream), *(input.at(stream)), T(1), T(1), true, false);
    }

    Math::Cuda::deviceSync(Precursor::measureTime_ && Math::CudaDataStructure::hasGpu());
    gettimeofday(&end, NULL);
    timeForwardLinear_  += Core::timeDiff(start, end);

    // second: add bias
    gettimeofday(&start, NULL);
    if (hasBias_)
	output.addToAllColumns(bias_);
    Math::Cuda::deviceSync(Precursor::measureTime_ && Math::CudaDataStructure::hasGpu());
    gettimeofday(&end, NULL);
    timeForwardBias_ += Core::timeDiff(start, end);
}

template<typename T>
void LinearLayer<T>::_backpropagateWeights(const NnMatrix& errorSignalIn, std::vector<NnMatrix*>& errorSignalOut) {
    require_eq(bias_.size(), errorSignalIn.nRows());

    timeval start, end;
    // errorSignalOut = weights * errorSignalIn for all streams

    gettimeofday(&start, NULL);
    for (u32 stream = 0; stream < weights_.size(); stream++)
	errorSignalOut.at(stream)->addMatrixProduct(weights_.at(stream), errorSignalIn, T(1));

    Math::Cuda::deviceSync(Precursor::measureTime_ && Math::CudaDataStructure::hasGpu());
    gettimeofday(&end, NULL);
    timeBackward_ += Core::timeDiff(start, end);
}

template<typename T>
void LinearLayer<T>::forward(const std::vector<NnMatrix*>& input, NnMatrix& output, bool reset) {
    require(!Precursor::needInit_);
    _forward(input, output, reset);
}

template<typename T>
void LinearLayer<T>::backpropagateWeights(const NnMatrix& errorSignalIn, std::vector<NnMatrix*>& errorSignalOut) {
    require(!Precursor::needInit_);
    _backpropagateWeights(errorSignalIn, errorSignalOut);
}

template<typename T>
void LinearLayer<T>::addToWeightsGradient(const NnMatrix& layerInput,
	const NnMatrix& errorSignalIn, u32 stream, NnMatrix& gradientWeights) {
    gradientWeights.addMatrixProduct(layerInput, errorSignalIn, T(1), T(1), false, true);
}

template<typename T>
void LinearLayer<T>::addToBiasGradient(const NnMatrix& layerInput,
	const NnMatrix& errorSignalIn, u32 stream, NnVector& gradientBias) {
    // calculate bias gradient only once for all input streams
    if (stream == 0 && hasBias_)
	gradientBias.addSummedColumns(errorSignalIn);
}

template<typename T>
inline void LinearLayer<T>::forward(const std::vector<NnMatrix*>& input, NnMatrix& output) {
    forward(input, output, true);
}

/**	Set the bias and the weight parameters.
 *
 * 	Set the bias and weight parameters from a single matrix.
 * 	The first column of the matrix is the bias and the rest are the weight
 * 	parameters
 *
 * 	@param	parameters	The matrix containing the bias and the weights
 */
template<typename T>
void LinearLayer<T>::setParameters(const Math::Matrix<T>& parameters) {
    for (u32 stream = 0; stream < weights_.size(); stream++)
	require(!weights_[stream].isComputing());
    require(!bias_.isComputing());

    // resize bias/ weights
    require_eq(parameters.nRows(), this->getOutputDimension());
    u32 totalInputSize = 0;
    for (u32 stream = 0; stream < this->nInputActivations(); stream++) {
	totalInputSize += this->getInputDimension(stream);
    }
    u32 inputSizeFromFile = hasBias_ ? parameters.nColumns() - 1 : parameters.nColumns();
    if (inputSizeFromFile != totalInputSize){
	this->error("dimension mismatch: (parameter file vs. layer-dimension) ")
		    << inputSizeFromFile << " vs. " << totalInputSize;
    }
    bias_.resize(parameters.nRows());
    weights_.resize(this->nInputActivations());
    for (u32 stream = 0; stream < this->nInputActivations(); stream++) {
	weights_.at(stream).resize(this->getInputDimension(stream), parameters.nRows());
    }

    // Convert Flow::Matrix -> Flow::FastMatrix
    for (u32 row = 0; row < parameters.nRows(); row++) {
	// first: bias (first column)
	if (hasBias_)
	    bias_.at(row) = parameters[row][0];

	// second: weights (all other elements, interchange row <-> column)
	u32 column = hasBias_ ? 1 : 0;
	for (u32 stream = 0; stream < weights_.size(); stream++) {
	    for (u32 r = 0; r < weights_.at(stream).nRows(); r++) {
		weights_.at(stream).at(r,row) = parameters[row][column];
		column++;
	    }
	}
    }

    Precursor::needInit_ = false;
}

template<typename T>
void LinearLayer<T>::initComputation(bool sync) const {
    if (!isComputing_) {
	for (u32 stream = 0; stream < weights_.size(); stream++) {
	    weights_.at(stream).initComputation(sync);
	}
	bias_.initComputation(sync);
    }
    isComputing_ = true;
}

template<typename T>
void LinearLayer<T>::finishComputation(bool sync) const {
    if (isComputing_) {
	for (u32 stream = 0; stream < weights_.size(); stream++) {
	    weights_.at(stream).finishComputation(sync);
	}
	bias_.finishComputation(sync);
    }
    isComputing_ = false;
}

template<typename T>
void LinearLayer<T>::finalize() {
    if (this->measureTime_){
	this->log("Linear layer: Time for linear part of forward pass: ") << timeForwardLinear_;
	this->log("Linear layer: Time for bias part of forward pass: ") << timeForwardBias_;
	this->log("Linear layer: Time for backward pass: ") << timeBackward_;
    }
    Precursor::finalize();
}

template<typename T>
u32 LinearLayer<T>::getNumberOfFreeParameters() const {
    u32 params = 0;
    if (trainable_) {
	for (u32 stream = 0; stream < weights_.size(); stream++)
	    params += weights_[stream].size();
	params += bias_.size();
    }
    return params;
}

/*===========================================================================*/
// explicit template instantiation
namespace Nn {
template class LinearLayer<f32>;
template class LinearLayer<f64>;
}

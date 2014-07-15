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
/* Implementation of neural networks */

#include <Math/Module.hh> // XML I/O stuff
// all the neural network layer
#include "NeuralNetworkLayer.hh"
#include "ActivationLayer.hh"
#include "LinearLayer.hh"
#include "LinearAndActivationLayer.hh"
#include "PreprocessingLayer.hh"


using namespace Nn;

/*===========================================================================*/
template<typename T>
const Core::Choice NeuralNetworkLayer<T>::choiceNetworkLayerType(
	"source", featureSource,
	"identity", identityLayer,
	// activation
	"sigmoid", sigmoidLayer,
	"softmax", softmaxLayer,
	"tanh", tanhLayer,
	"rectified", rectifiedLayer,
	// linear + activation
	"linear", linearLayer,
	"linear+sigmoid", linearAndSigmoidLayer,
	"linear+softmax", linearAndSoftmaxLayer,
	"linear+tanh", linearAndTanhLayer,
	"linear+rectified", linearAndRectifiedLayer,
	// preprocessing
	"logarithm", logarithmPreprocessingLayer,
	"mean-and-variance-normalization", meanAndVarianceNormalizationPreprocessingLayer,
	"polynomial", polynomialPreprocessingLayer,
	"gaussian-noise", gaussianNoisePreprocessingLayer,
	// convolutional (need MODULE_NN_CONVOLUTIONAL)
	"convolutional", convolutionalLayer,
	"convolutional+sigmoid", convolutionalAndSigmoidLayer,
	"pooling", poolingLayer,
	// recurrent (need MODULE_NN_RECURRENT)
	"recurrent-linear", recurrentLinearLayer,
	Core::Choice::endMark());

template<typename T>
const Core::ParameterChoice NeuralNetworkLayer<T>::paramNetworkLayerType(
	"layer-type", &choiceNetworkLayerType, "neural network layer type", identityLayer);

template<typename T>
const Core::ParameterString NeuralNetworkLayer<T>::paramNetworkLayerName(
	"layer-name", "Name of the neural network layer", "");

// specification of the input dimension is only needed for the layers connected to
// a feature stream, all other dimensions are passed through
template<typename T>
const Core::ParameterInt NeuralNetworkLayer<T>::paramDimensionIn(
	"dimension-input", "Dimension of the input of the layer", 0);

template<typename T>
const Core::ParameterInt NeuralNetworkLayer<T>::paramDimensionOut(
	"dimension-output", "Dimension of the output of the layer", 0);

template<typename T>
const Core::ParameterInt NeuralNetworkLayer<T>::paramNetworkLayerWindow(
	"window-size", "Size of the window (symmetric => +- win/2)", 1);

template<typename T>
const Core::ParameterInt NeuralNetworkLayer<T>::paramNetworkLayerDelay(
	"delay-size", "Time shift of the input", 0);

template<typename T>
const Core::Choice NeuralNetworkLayer<T>::choiceActivationStatisticsSmoothingMethod(
	"no-statistics", noStatistics,
	"none", none,
	"exponential-trace", exponentialTrace,
	Core::Choice::endMark());

template<typename T>
const Core::ParameterChoice NeuralNetworkLayer<T>::paramActivationStatisticsSmoothingMethod(
	"statistics-smoothing-method", &choiceActivationStatisticsSmoothingMethod,
	"smoothing of mean and standard deviation of activations", noStatistics);

template<typename T>
const Core::ParameterString NeuralNetworkLayer<T>::paramOldActivationMeanFile(
	"old-activations-mean-file", "mean file of activation statistics", "");

template<typename T>
const Core::ParameterString NeuralNetworkLayer<T>::paramOldActivationStandardDeviationFile(
	"old-activations-standard-deviation-file", "standard deviation file of activation statistics", "");

template<typename T>
const Core::ParameterString NeuralNetworkLayer<T>::paramNewActivationMeanFile(
	"new-activations-mean-file", "mean file of activation statistics", "");

template<typename T>
const Core::ParameterString NeuralNetworkLayer<T>::paramNewActivationStandardDeviationFile(
	"new-activations-standard-deviation-file", "standard deviation file of activation statistics", "");

template<typename T>
const Core::ParameterFloat NeuralNetworkLayer<T>::paramExponentialTraceInterpolationFactor(
	"exponential-trace-interpolation-factor", "avgValue = factor*avgValue + (1-factor)*newValue", 0.5);

template<typename T>
const Core::ParameterFloat NeuralNetworkLayer<T>::paramActivationVarianceInterpolation(
	"activation-variance-interpolation", "interpolation: alpha * activation variances + (1-alpha) * unity", 1.0);

template<typename T>
const Core::ParameterFloat NeuralNetworkLayer<T>::paramDropoutProbability(
	"dropout-probability", "probability that an activation is set to zero", 0.0);

template<typename T>
const Core::ParameterFloat NeuralNetworkLayer<T>::paramGaussianNoiseRatio(
	"gaussian-noise-ratio", "ratio * avg layer std-dev is used as std-dev when adding gaussian noise to the activations", 0.0);

template<typename T>
const Core::ParameterFloat NeuralNetworkLayer<T>::paramLearningRate(
	"layer-learning-rate", "layer specific learning rate factor", 1.0);

template<typename T>
const Core::ParameterFloat NeuralNetworkLayer<T>::paramRegularizationConstant("regularization-constant",
	"regularization constant, can be set separately for every layer", 0.0);


template<typename T>
NeuralNetworkLayer<T>::NeuralNetworkLayer(const Core::Configuration &config) :
    Core::Component(config),
    layerType_((LayerType) paramNetworkLayerType(config)),
    layerName_(config.getName()),
    oldMeanFile_(paramOldActivationMeanFile(config)),
    oldStandardDeviationFile_(paramOldActivationStandardDeviationFile(config)),
    newMeanFile_(paramNewActivationMeanFile(config)),
    newStandardDeviationFile_(paramNewActivationStandardDeviationFile(config)),
    statisticsSmoothing_((ActivationStatisticsSmoothingMethod) paramActivationStatisticsSmoothingMethod(config)),
    exponentialTraceInterpolationFactor_(paramExponentialTraceInterpolationFactor(config)),
    activationVarianceInterpolation_(paramActivationVarianceInterpolation(config)),
    learningRate_(paramLearningRate(config)),
    regularizationConstant_(paramRegularizationConstant(config)),
    dropoutProbability_(paramDropoutProbability(config)),
    gaussianNoiseRatio_(paramGaussianNoiseRatio(config)),
    inputActivationIndices_(0),
    outputActivationIndex_(0),
    predecessorLayers_(0),
    inputDimensions_(0),
    outputDimension_(paramDimensionOut(config)),
    needInit_(true),
    isFinalized_(false),
    isComputing_(false),
    measureTime_(false),
    nObservations_(0),
    activationStatisticsNeedInit_(true),
    refreshMean_(true),
    refreshVariance_(true)
{
    if (dropoutProbability_ > 0)
	Core::Component::log("using dropout probability ") << dropoutProbability_ << " for " << layerName_;
    if (gaussianNoiseRatio_ > 0)
	Core::Component::log("using gaussian noise ratio ") << gaussianNoiseRatio_ << " for " << layerName_;
    // this is necessary if initializeNetwork(u32 batchSize) is called instead of initializeNetwork(u32 batchSize, std::vector<u32>& streamSizes)
    inputDimensions_.push_back(paramDimensionIn(config));
}

template<typename T>
void NeuralNetworkLayer<T>::setInputActivationIndex(u32 activationIndex, u32 i) {
    if (! (i < inputActivationIndices_.size()) )
	inputActivationIndices_.resize(i +1);
    inputActivationIndices_[i] = activationIndex;
}

template<typename T>
void NeuralNetworkLayer<T>::addPredecessor(u32 topologyIndex, u32 i) {
    if (! (i < predecessorLayers_.size()) )
	predecessorLayers_.resize(i +1);
    predecessorLayers_[i] = topologyIndex;

}

template<typename T>
void NeuralNetworkLayer<T>::setInputDimension(u32 stream, u32 dim) {
    if (stream >= inputDimensions_.size())
	inputDimensions_.resize(stream + 1);
    inputDimensions_[stream] = dim;
}

template<typename T>
void NeuralNetworkLayer<T>::initializeActivationStatistics(const NnMatrix& output) {
    if (activationStatisticsNeedInit_) {

	// initialize statistics containers
	activationSum_.resize(output.nRows());
	activationSumOfSquares_.resize(output.nRows());
	activationMean_.initComputation();
	activationVariance_.initComputation();

	// initialize activationSum_
	Math::Vector<T> mean;
	if (!oldMeanFile_.empty()) {
	    Core::Component::log("reading mean activation file ") << oldMeanFile_ << " for layer " << getName();
	    Math::Module::instance().formats().read(oldMeanFile_, mean);
	    nObservations_ = (statisticsSmoothing_ == exponentialTrace) ? 1 : output.nColumns();
	    for (u32 i = 0; i < activationSum_.nRows(); i++)
		activationSum_.at(i) = mean[i] * nObservations_;
	    activationSum_.initComputation();
	} else {
	    activationSum_.initComputation();
	    activationSum_.setToZero();
	}

	// initialize activationSumOfSquares_
	Math::Vector<T> standardDeviation;
	if (!oldStandardDeviationFile_.empty()) {
	    Core::Component::log("reading standard deviation activation file ") << oldStandardDeviationFile_ << " for layer " << getName();
	    Math::Module::instance().formats().read(oldStandardDeviationFile_, standardDeviation);
	    nObservations_ = (statisticsSmoothing_ == exponentialTrace) ? 1 : output.nColumns();
	    for (u32 i = 0; i < activationSumOfSquares_.nRows(); i++)
		activationSumOfSquares_.at(i) = (standardDeviation[i] * standardDeviation[i] + mean[i] * mean[i]) * nObservations_;
	    activationSumOfSquares_.initComputation();
	} else {
	    activationSumOfSquares_.initComputation();
	    activationSumOfSquares_.setToZero();
	}

	// some logging
	switch ( statisticsSmoothing_ ) {
	case none:
	    Core::Component::log() << layerName_ << ": Accumulate activation statistics without smoothing.";
	    break;
	case exponentialTrace:
	    Core::Component::log() << layerName_ << ": Accumulate activation statistics with exponential trace.";
	    break;
	default:
	    break;
	}
    }
    activationStatisticsNeedInit_ = false;
}

template<typename T>
void NeuralNetworkLayer<T>::nonSmoothedStatisticsUpdate(const NnMatrix& output) {
    if (activationStatisticsNeedInit_) {
	initializeActivationStatistics(output);
    }
    // update the statistics
    activationSum_.addSummedColumns(output);
    activationSumOfSquares_.addSquaredSummedColumns(output);
    nObservations_ += output.nColumns();
}

template<typename T>
void NeuralNetworkLayer<T>::exponentialTraceStatisticsUpdate(const NnMatrix& output) {
    if (activationStatisticsNeedInit_) {
	initializeActivationStatistics(output);
	if (oldMeanFile_.empty())
	    activationSum_.addSummedColumns(output, T(1.0) / output.nColumns());
	if (oldStandardDeviationFile_.empty())
	    activationSumOfSquares_.addSquaredSummedColumns(output, 1.0 / output.nColumns());
    }

    // update the statistics
    activationSum_.scale(exponentialTraceInterpolationFactor_);
    activationSum_.addSummedColumns(output,
	    (T(1.0) - exponentialTraceInterpolationFactor_) / output.nColumns());
    activationSumOfSquares_.scale(exponentialTraceInterpolationFactor_);
    activationSumOfSquares_.addSquaredSummedColumns(output,
	    (T(1.0) - exponentialTraceInterpolationFactor_) / output.nColumns());
}

template<typename T>
void NeuralNetworkLayer<T>::updateStatistics(const NnMatrix& output) {
    switch ( statisticsSmoothing_ ) {
    case none:
	nonSmoothedStatisticsUpdate(output);
	break;
    case exponentialTrace:
	exponentialTraceStatisticsUpdate(output);
	break;
    default:
	break;
    }
    refreshMean_ = true;
    refreshVariance_ = true;
}

template<typename T>
void NeuralNetworkLayer<T>::applyDropout(NnMatrix& output) {
    output.dropout(dropoutProbability_);
    // scale result to keep accumulated activations in same order of magnitude
    output.scale(1.0 / (1.0 - dropoutProbability_));
}

template<typename T>
void NeuralNetworkLayer<T>::addGaussianNoise(NnMatrix& output) {
    require(statisticsSmoothing_ != noStatistics);
    T avgStdDev = std::sqrt(getActivationVariance().asum() / getActivationVariance().nRows());
    output.addGaussianNoise(avgStdDev * gaussianNoiseRatio_);
}

template<typename T>
void NeuralNetworkLayer<T>::finalizeForwarding(NnMatrix& output) {
    if (dropoutProbability_ > 0) {
	applyDropout(output);
    }
    updateStatistics(output);
    if (gaussianNoiseRatio_ > 0) {
	addGaussianNoise(output);
    }
}

template<typename T>
typename NeuralNetworkLayer<T>::NnVector& NeuralNetworkLayer<T>::getActivationMean() {
    require(statisticsSmoothing_ != noStatistics);
    require(!activationStatisticsNeedInit_);

    if (activationMean_.nRows() != activationSum_.nRows()) {
	activationMean_.resize(activationSum_.nRows());
	refreshMean_ = true;
    }

    if (refreshMean_) {
	require(activationSum_.isComputing());
	require(activationMean_.isComputing());
	u32 nObservations = (statisticsSmoothing_ == exponentialTrace) ? 1 : nObservations_;
	activationMean_.setToZero();
	activationMean_.add(activationSum_, T (1.0 / nObservations));
	refreshMean_ = false;
    }

    return activationMean_;
}

template<typename T>
typename NeuralNetworkLayer<T>::NnVector& NeuralNetworkLayer<T>::getActivationVariance() {
    require(statisticsSmoothing_ != noStatistics);
    require(!activationStatisticsNeedInit_);

    if (activationVariance_.nRows() != activationSum_.nRows()) {
	activationVariance_.resize(activationSum_.nRows());
	refreshVariance_ = true;
    }

    if (refreshVariance_) {
	require(activationSum_.isComputing());
	require(activationSumOfSquares_.isComputing());
	require(activationMean_.isComputing());
	require(activationVariance_.isComputing());
	u32 nObservations = (statisticsSmoothing_ == exponentialTrace) ? 1 : nObservations_;
	activationVariance_.setToZero();
	activationVariance_.add(getActivationMean());
	activationVariance_.elementwiseMultiplication(activationVariance_);
	activationVariance_.scale(-1.0);
	activationVariance_.add(activationSumOfSquares_, T (1.0 / nObservations));
	activationVariance_.scale(activationVarianceInterpolation_);
	activationVariance_.addConstantElementwise(1.0 - activationVarianceInterpolation_);
	refreshVariance_ = false;
    }

    return activationVariance_;
}

/* dummy for backpropagateWeights: just copy the error signal (required for layers without weights) */
template<typename T>
void NeuralNetworkLayer<T>::backpropagateWeights(const NnMatrix& errorSignalIn, std::vector<NnMatrix*>& errorSignalOut) {
    require_eq(errorSignalOut.size(), 1);
    require_eq(errorSignalIn.nRows(), errorSignalOut[0]->nRows());
    require_eq(errorSignalIn.nColumns(), errorSignalOut[0]->nColumns());
    /* forward error signal */
    errorSignalOut[0]->copy(errorSignalIn);
}

template<typename T>
void NeuralNetworkLayer<T>::finalize() {
    if (!isFinalized_) {
	// store activation statistics, if desired
	if (statisticsSmoothing_ != noStatistics) {
	    // save standard deviation file
	    if (!newStandardDeviationFile_.empty()) {
		getActivationVariance().finishComputation();
		// sqrt for standard deviation
		for (u32 i = 0; i < activationSumOfSquares_.nRows(); i++) {
		    getActivationVariance().at(i) = std::sqrt(getActivationVariance().at(i));
		}
		Core::Component::log("Save activations standard deviation to ") << newStandardDeviationFile_;
		saveVector(getActivationVariance(), newStandardDeviationFile_);
	    }
	    // save mean file
	    if (!newMeanFile_.empty()) {
		getActivationMean().finishComputation();
		Core::Component::log("Save activations mean to ") << newMeanFile_;
		saveVector(getActivationMean(), newMeanFile_);
	    }
	}
	isFinalized_ = true;
    }
}


template<typename T>
void NeuralNetworkLayer<T>::saveVector(const NnVector& vector, const std::string &filename) {
    require(!filename.empty());
    // determine file suffix
    std::string suffix;
    if ((filename.length() >= 4) && (filename.substr(0,4) == "bin:")) {
	suffix = ".bin";
    } else {
	suffix = ".xml";
    }
    // save the vector
    std::ostringstream type;
    if (typeid(T) == typeid(f32)) {
	type << "f32";
    } else if (typeid(T) == typeid(f64)) {
	type << "f64";
    }
    std::string newFilename = filename + "-" + type.str() + suffix;

    // convert NnVector to Math::Vector
    Math::Vector<T> tmp;
    tmp.resize(vector.nRows());
    for (u32 i = 0; i < vector.nRows(); i++) {
	tmp.at(i) = vector.at(i);
    }

    Math::Module::instance().formats().write(newFilename, tmp, 20);
}

template<typename T>
NeuralNetworkLayer<T>* NeuralNetworkLayer<T>::createNeuralNetworkLayer(const Core::Configuration &config) {
    NeuralNetworkLayer<T>* layer = 0;

    // get the type of the neural network layer ...
    LayerType layerType = (LayerType) paramNetworkLayerType(config);

    // ... and create the correct neural network layer
    switch ( layerType ) {
    case NeuralNetworkLayer<T>::featureSource : {
	layer = new IdentityLayer<T>(config);
	Core::Application::us()->log("creating new feature source layer");
	break;
    }
    case NeuralNetworkLayer<T>::identityLayer : {
	layer = new IdentityLayer<T>(config);
	Core::Application::us()->log("creating new identity layer");
	break;
    }
    case NeuralNetworkLayer<T>::tanhLayer : {
	layer = new TanhLayer<T>(config);
	Core::Application::us()->log("creating new tanh layer");
	break;
    }
    case NeuralNetworkLayer<T>::rectifiedLayer : {
	layer = new RectifiedLayer<T>(config);
	Core::Application::us()->log("creating new rectified layer");
	break;
    }
    case NeuralNetworkLayer<T>::sigmoidLayer : {
	layer = new SigmoidLayer<T>(config);
	Core::Application::us()->log("creating new sigmoid layer");
	break;
    }
    case NeuralNetworkLayer<T>::softmaxLayer : {
	layer = new SoftmaxLayer<T>(config);
	Core::Application::us()->log("creating new softmax layer");
	break;
    }
    case NeuralNetworkLayer<T>::linearLayer : {
	layer = new LinearLayer<T>(config);
	Core::Application::us()->log("creating new linear layer");
	break;
    }
    case NeuralNetworkLayer<T>::linearAndSigmoidLayer : {
	layer = new LinearAndSigmoidLayer<T>(config);
	Core::Application::us()->log("creating new linear+sigmoid layer");
	break;
    }
    case NeuralNetworkLayer<T>::linearAndSoftmaxLayer : {
	layer = new LinearAndSoftmaxLayer<T>(config);
	Core::Application::us()->log("creating new linear+softmax layer");
	break;
    }
    case NeuralNetworkLayer<T>::linearAndTanhLayer : {
	layer = new LinearAndTanhLayer<T>(config);
	Core::Application::us()->log("creating new linear+tanh layer");
	break;
    }
    case NeuralNetworkLayer<T>::linearAndRectifiedLayer : {
	layer = new LinearAndRectifiedLayer<T>(config);
	Core::Application::us()->log("creating new linear+rectified layer");
	break;
    }
#if 1
    case NeuralNetworkLayer<T>::convolutionalLayer :
    case NeuralNetworkLayer<T>::convolutionalAndSigmoidLayer :
    case NeuralNetworkLayer<T>::poolingLayer :
	Core::Application::us()->criticalError("Need MODULE_NN_CONVOLUTIONAL for enabling convolutional layers");
	break;
#endif
#if 1
    case NeuralNetworkLayer<T>::recurrentLinearLayer :
	Core::Application::us()->criticalError("Need MODULE_NN_RECURRENT for enabling recurrent layers");
	break;
#endif
    case NeuralNetworkLayer<T>::logarithmPreprocessingLayer : {
	layer =  new LogarithmPreprocessingLayer<T>(config);
	Core::Application::us()->log("creating new logarithm-preprocessing layer");
	break;
    }
    case NeuralNetworkLayer<T>::meanAndVarianceNormalizationPreprocessingLayer: {
	layer = new MeanAndVarianceNormalizationPreprocessingLayer<T>(config);
	Core::Application::us()->log("creating new mean-and-variance-normalization-preprocessing layer");
	break;
    }
    case NeuralNetworkLayer<T>::polynomialPreprocessingLayer: {
	layer = new PolynomialPreprocessingLayer<T>(config);
	Core::Application::us()->log("creating new polynomial-preprocessing layer");
	break;
    }
    case NeuralNetworkLayer<T>::gaussianNoisePreprocessingLayer: {
	layer = new GaussianNoisePreprocessingLayer<T>(config);
	Core::Application::us()->log("creating new Gaussian noise preprocessing layer");
	break;
    }
    default:
	std::cerr << "The neural network layer type is unknown or not implemented!";
	break;
    };
    Core::Application::us()->log("output dimension of new layer: ") << layer->getOutputDimension();
    // return the new neural network layer
    return layer;
}

/*===========================================================================*/
// explicit template instantiation
namespace Nn {
template class NeuralNetworkLayer<f32>;
template class NeuralNetworkLayer<f64>;
}

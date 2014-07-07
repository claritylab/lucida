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
#ifndef _NN_NEURAL_NETWORK_LAYER_HH
#define _NN_NEURAL_NETWORK_LAYER_HH

#include <cmath>
#include <numeric>
#include <Core/Component.hh>
#include <Core/Parameter.hh>

#include <Math/Vector.hh>
#include <Math/Matrix.hh>
#include <Math/CudaMatrix.hh>
#include <Math/CudaVector.hh>

#include <Speech/Feature.hh>
#include <utility>

#include "Types.hh"

namespace Nn {

//=============================================================================
/**	Abstract base class for all neural network layers
 *
 *	To create a new neural network layer, derive your new layer from this
 *	base class, add a new layer type, and extend the "createNeuralNetworkLayer"
 *	method.
 *
 *	Current neural network layer implementations
 *	- activation functions
 *	  - identity
 *	  - sigmoid
 *	  - softmax
 *	  - tanh
 *	  - rectified
 *	  - linear (feed-forward)
 *	- combined: parameter + activation
 *	  - linear+sigmoid
 *	  - linear+softmax
 *	  - linear+rlu
 *	  - linear+tanh
 *	- pre-/post-processing steps
 *	  - logarithm
 *	  - PCA (mean + variance)
 *	  - polynomial
 *	  - GaussianNoise
 *
 *	The layer can have multiple inputs from other layers.
 *	Inputs are connected at "ports", which are just indices of the connections specified in the configuration.
 *	The layer has only one output, which also has a "port" index.
 *	When building up the network, the predecessor layers need to be set, because they are required in backpropagation.
 *	Plans: Simplify design of neural network layer, remove redundancy
 *
 */
template <typename T>
class NeuralNetworkLayer : virtual public Core::Component {
    typedef Core::Component Precursor;
public:
    typedef typename Types<T>::NnVector NnVector;
    typedef typename Types<T>::NnMatrix NnMatrix;
public:
    enum LayerType {
	featureSource,
	identityLayer,
	tanhLayer,
	rectifiedLayer,
	sigmoidLayer,
	softmaxLayer,
	linearLayer,
	linearAndSigmoidLayer,
	linearAndSoftmaxLayer,
	linearAndTanhLayer,
	linearAndRectifiedLayer,
	convolutionalLayer,
	convolutionalAndSigmoidLayer,
	poolingLayer,
	recurrentLinearLayer,
	logarithmPreprocessingLayer,
	meanAndVarianceNormalizationPreprocessingLayer,
	polynomialPreprocessingLayer,
	gaussianNoisePreprocessingLayer,
    };
protected:
    static const Core::Choice choiceNetworkLayerType;
    static const Core::ParameterChoice paramNetworkLayerType;
    static const Core::ParameterString paramNetworkLayerName;
    static const Core::ParameterInt paramDimensionIn;
    static const Core::ParameterInt paramDimensionOut;
    static const Core::ParameterInt paramNetworkLayerWindow;
    static const Core::ParameterInt paramNetworkLayerDelay;

    /* parameters for collecting activation statistics */
    enum ActivationStatisticsSmoothingMethod {
	noStatistics,
	none,
	exponentialTrace,
    };
    static const Core::Choice choiceActivationStatisticsSmoothingMethod;
    static const Core::ParameterChoice paramActivationStatisticsSmoothingMethod;
    static const Core::ParameterString paramOldActivationMeanFile;
    static const Core::ParameterString paramOldActivationStandardDeviationFile;
    static const Core::ParameterString paramNewActivationMeanFile;
    static const Core::ParameterString paramNewActivationStandardDeviationFile;
    static const Core::ParameterFloat paramExponentialTraceInterpolationFactor;
    static const Core::ParameterFloat paramActivationVarianceInterpolation;
    static const Core::ParameterFloat paramDropoutProbability;
    static const Core::ParameterFloat paramGaussianNoiseRatio;
    static const Core::ParameterFloat paramLearningRate;
    static const Core::ParameterFloat paramRegularizationConstant;
protected:
    // type of the layer
    const LayerType layerType_;
    // name of the layer specified in configuration
    const std::string layerName_;
    // indices of the input connections
    // activation statistics parameters
    const std::string oldMeanFile_;
    const std::string oldStandardDeviationFile_;
    const std::string newMeanFile_;
    const std::string newStandardDeviationFile_;
    const ActivationStatisticsSmoothingMethod statisticsSmoothing_;
    const T exponentialTraceInterpolationFactor_;
    const T activationVarianceInterpolation_;
    // layer-specific learning rate (multiplied with general learning rate in Estimator)
    const T learningRate_;		// TODO change to vector corresponding to the connections
    // regularization constant
    const T regularizationConstant_;  // TODO change to vector corresponding to the connections
    // dropout probability
    const T dropoutProbability_;
    // ratio for adding gaussian noise to activations
    const T gaussianNoiseRatio_;
protected:
    // indices of inputs & outputs
    std::vector<u32> inputActivationIndices_;
    u32 outputActivationIndex_;
    // indices of predecessor layers
    std::vector<u32> predecessorLayers_;
    // input and output dimensions
    std::vector<u32> inputDimensions_;
    u32 outputDimension_;
    // needs initialization
    bool needInit_;
    // needs finalization
    bool isFinalized_;
    // is in computing state (GPU)
    mutable bool isComputing_;
    // measure runtime
    bool measureTime_;

    // activation statistics
    u32 nObservations_;
    NnVector activationSum_;
    NnVector activationSumOfSquares_;
    NnVector activationMean_;
    NnVector activationVariance_;
    bool activationStatisticsNeedInit_;
    bool refreshMean_;
    bool refreshVariance_;

public:
    NeuralNetworkLayer(const Core::Configuration &config);
    virtual ~NeuralNetworkLayer() {}
public:
    // name of the layer specified in configuration
    virtual std::string getName() const { return layerName_; }
    // type of the layer
    virtual LayerType getLayerType() const { return layerType_; }

    // number of input activations
    virtual u32 nInputActivations() const { return inputActivationIndices_.size(); }
    // set i'th input activation index
    virtual void setInputActivationIndex(u32 activationIndex, u32 i);
    // get port number with index 'index'
    virtual u32 getInputActivationIndex(u32 i) const { require_lt(i, inputActivationIndices_.size()); return inputActivationIndices_.at(i); }
    // set the output activation index
    virtual void setOutputActivationIndex(u32 activationIndex) { outputActivationIndex_ = activationIndex; }
    // get output activation index
    virtual u32 getOutputActivationIndex() { return outputActivationIndex_; }
    // set input dimension of stream 'stream'
    virtual void setInputDimension(u32 stream, u32 dim);
    // set output dimension
    virtual void setOutputDimension(u32 dim) { outputDimension_ = dim; }
    // get input dimension of stream 'stream'
    virtual u32 getInputDimension(u32 stream) const { require_lt(stream, inputDimensions_.size()); return inputDimensions_.at(stream); }
    // get output dimension
    virtual u32 getOutputDimension() const { return outputDimension_; }

    // manage predecessors (need to be known for backpropagation)
    // add a predecessor
    virtual void addPredecessor(u32 topologyIndex, u32 i);
    // get a predecessor topology index
    virtual u32 getPredecessor(u32 i) { require_lt(i, predecessorLayers_.size()); return predecessorLayers_.at(i); }
    // number of layers connecting to this layer (TODO should be the same as nInputActivations)
    virtual u32 nPredecessors() { return predecessorLayers_.size(); }

    // get layer-specific learning rate
    virtual T learningRate() const { return learningRate_; }
    // get regularization constant
    virtual T regularizationConstant() const { return regularizationConstant_; }
    // initialize activation statistics
    virtual void initializeActivationStatistics(const NnMatrix& output);
    // parameters to be trained?
    virtual bool isTrainable() const { return false; };
    // composed layer: composition of different layers, e.g. linear+softmax
    virtual bool isComposedLayer() const { return false; };
    // forward current input
    virtual void forward(const std::vector<NnMatrix*>& input, NnMatrix& output) = 0;
    // forward recurrent connection
    virtual void forwardRecurrent(const std::vector<NnMatrix*>& input, NnMatrix& output) {}
    // apply postprocessing steps after forwarding (e.g. collecting statistics about activations)
    virtual void finalizeForwarding(NnMatrix& output);
    // backpropagation: activation part
    virtual void backpropagateActivations(const NnMatrix& errorSignalIn, NnMatrix& errorSignalOut,
	    const NnMatrix& activations) {}
    // backpropagation: linear part (default: just forward the error signal)
    virtual void backpropagateWeights(const NnMatrix& errorSignalIn, std::vector<NnMatrix*>& errorSignalOut);

    // calculate weight gradients from layer input and error and store result in gradient{Weights,Bias};
    // this may vary among different layer types, this is why the trainer needs to ask
    // the layer to calculate its own gradients
    virtual void addToWeightsGradient(const NnMatrix& layerInput, const NnMatrix& errorSignalIn, u32 stream, NnMatrix& gradientWeights) {};
    virtual void addToBiasGradient(const NnMatrix& layerInput, const NnMatrix& errorSignalIn, u32 stream, NnVector& gradientBias) {};

    // return pointer to weight matrix and bias vector (if any)
    virtual NnMatrix* getWeights(u32 stream){ return 0; }
    virtual NnVector* getBias(){ return 0; }
    virtual const NnMatrix* getWeights(u32 stream) const { return 0; }
    virtual const NnVector* getBias() const { return 0; }

    // get activation statistics
    virtual NnVector& getActivationSum() { require(statisticsSmoothing_ != noStatistics); return activationSum_; }
    virtual NnVector& getActivationSumOfSquares() { require(statisticsSmoothing_ != noStatistics); return activationSumOfSquares_; }
    virtual NnVector& getActivationMean();
    virtual NnVector& getActivationVariance();
    virtual bool hasActivationStatistics() const { return statisticsSmoothing_ != noStatistics; }

    // Initialize the parameters
    virtual void initializeNetworkParameters() {};
    // Save the parameters to disk
    virtual void saveNetworkParameters(const std::string &filename) const {};
    // Load the parameters from disk
    virtual void loadNetworkParameters(const std::string &filename) {};

    virtual void initComputation(bool sync = true) const {}
    virtual void finishComputation(bool sync = true) const {}
    bool isComputing() const { return isComputing_; }

    virtual void finalize();
    // let the layer resize the gradients according to its trainable parameters
    // TODO this implementation should be moved to LinearLayer !
    virtual void resizeWeightsGradient(Types<f32>::NnMatrix &gradient, u32 stream) const { gradient.resize(getInputDimension(stream), getOutputDimension()); }
    virtual void resizeBiasGradient(Types<f32>::NnVector &gradient) const { gradient.resize(getOutputDimension()); }
    virtual void resizeWeightsGradient(Types<f64>::NnMatrix &gradient, u32 stream) const { gradient.resize(getInputDimension(stream), getOutputDimension()); }
    virtual void resizeBiasGradient(Types<f64>::NnVector &gradient) const { gradient.resize(getOutputDimension()); }

    // returns the number of free parameters ( = parameters that are optimized)
    virtual u32 getNumberOfFreeParameters() const { return 0; };
    // get and set measureTime
    bool measuresTime() const { return measureTime_; }
    void setMeasureTime(bool val) { measureTime_ = val; }
protected:
    virtual void applyDropout(NnMatrix& output);
    virtual void addGaussianNoise(NnMatrix& output);
private:
    void nonSmoothedStatisticsUpdate(const NnMatrix& output);
    void exponentialTraceStatisticsUpdate(const NnMatrix& output);
    void updateStatistics(const NnMatrix& output);

    static void saveVector(const NnVector& vector, const std::string &filename);
public:
    static NeuralNetworkLayer<T>* createNeuralNetworkLayer(const Core::Configuration &config);
};


} // namespace Nn

#endif // _NN_NEURAL_NETWORK_LAYER_HH

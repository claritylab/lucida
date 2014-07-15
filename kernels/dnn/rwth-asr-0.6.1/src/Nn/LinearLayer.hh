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
#ifndef _NN_NEURAL_NETWORK_LAYER_WEIGHTS_HH
#define _NN_NEURAL_NETWORK_LAYER_WEIGHTS_HH

// Neural Network Layer implementation

#include <string>
#include <vector>

#include <Core/Types.hh>
#include <Math/Matrix.hh>
#include "NeuralNetworkLayer.hh"
#include "Types.hh"



namespace Nn {

/*
 * Linear layer
 *
 * Combined linear + non-linear activation layers derive from this class
 *
 */
template<typename T>
class LinearLayer : public virtual NeuralNetworkLayer<T> {
    typedef NeuralNetworkLayer<T> Precursor;
protected:
    typedef typename Types<T>::NnVector NnVector;
    typedef typename Types<T>::NnMatrix NnMatrix;
public:
    enum InitializationType {
	zero,
	random,
	identity,
	file,
    };
protected:
    using Precursor::isComputing_;
protected:
    static const Core::Choice choiceInitializationType;
    static const Core::ParameterChoice paramInitializationType;
    static const Core::ParameterFloat paramBiasInitializationRangeMin;
    static const Core::ParameterFloat paramBiasInitializationRangeMax;
    static const Core::ParameterFloat paramWeightInitializationRangeMin;
    static const Core::ParameterFloat paramWeightInitializationRangeMax;
    static const Core::ParameterFloat paramWeightInitializationScalingFactor;
    static const Core::ParameterBool paramIgnoreParameterFile;
    static const Core::ParameterString paramParameterFile;
    static const Core::ParameterBool paramHasBias;
    static const Core::ParameterBool paramTrainable;
protected:
    const InitializationType initializationType_;	// method to initialize the parameters
    const T biasInitializationRangeMin_;
    const T biasInitializationRangeMax_;
    const T weightInitializationRangeMin_;
    const T weightInitializationRangeMax_;
    const T weightInitializationIdentityScalingFactor_;
    const bool ignoreParameterFile_;			// do not read parameters from file
    const bool hasBias_;
    NnVector bias_;					// bias vector
    std::vector<NnMatrix> weights_;			// multiple weight matrices (for multiple input streams)
    std::string parameterFile_;
    bool trainable_;					// optimize parameters of this layer
private:
    double timeForwardLinear_, timeForwardBias_, timeBackward_;
public:
    LinearLayer(const Core::Configuration &config);
    virtual ~LinearLayer();
public:
    // trainer needs to access weights and bias
    virtual NnMatrix* getWeights(u32 stream) { return &(weights_.at(stream)); }
    virtual NnVector* getBias() { return &bias_; }
    virtual const NnMatrix* getWeights(u32 stream) const { return &(weights_.at(stream)); }
    virtual const NnVector* getBias() const { return &bias_; }

    // IO
    virtual void initializeNetworkParameters();
    virtual void loadNetworkParameters(const std::string &filename);
    virtual void saveNetworkParameters(const std::string &filename) const ;

    // initialization methods
    virtual void setInputDimension(u32 stream, u32 size);
    virtual void setOutputDimension(u32 size);

    // getter methods
    virtual bool isTrainable() const { return trainable_; }

    // forward
    void forward(const std::vector<NnMatrix*>& input, NnMatrix& output, bool reset);
    virtual void forward(const std::vector<NnMatrix*>& input, NnMatrix& output);

    // backward
    virtual void backpropagateWeights(const NnMatrix& errorSignalIn, std::vector<NnMatrix*>& errorSignalOut);
    virtual void addToWeightsGradient(const NnMatrix& layerInput, const NnMatrix& errorSignalIn, u32 stream, NnMatrix& gradientWeights);
    virtual void addToBiasGradient(const NnMatrix& layerInput, const NnMatrix& errorSignalIn, u32 stream, NnVector& gradientBias);

    virtual void finalize();
    virtual u32 getNumberOfFreeParameters() const;

    virtual void initComputation(bool sync = true) const;
    virtual void finishComputation(bool sync = true) const;

protected:
    virtual void _forward(const std::vector<NnMatrix*>& input, NnMatrix& output, bool reset);
    virtual void _backpropagateWeights(const NnMatrix& errorSignalIn, std::vector<NnMatrix*>& errorSignalOut);

    virtual void initializeParametersRandomly();
    virtual void initializeParametersWithZero();
    virtual void initializeParametersWithIdentityMatrix();
    void setParameters(const Math::Matrix<T>& parameters);
};

} // namespace Nn

#endif // _NN_NEURAL_NETWORK_LAYER_WEIGHTS_HH

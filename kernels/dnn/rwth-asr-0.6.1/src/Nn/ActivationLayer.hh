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
#ifndef _NN_NEURAL_NETWORK_LAYER_ACTIVATION_FUNCTION_HH
#define _NN_NEURAL_NETWORK_LAYER_ACTIVATION_FUNCTION_HH

// Neural Network Layer implementation
#include <cmath>
#include <numeric>

#include <Math/CudaMatrix.hh>
#include <Math/CudaVector.hh>
#include "NeuralNetworkLayer.hh"

namespace Nn {

//=============================================================================
/**	Apply identity activation to the input */
template<typename T>
class IdentityLayer : public virtual NeuralNetworkLayer<T> {
protected:
    typedef typename Types<T>::NnVector NnVector;
    typedef typename Types<T>::NnMatrix NnMatrix;
public:
    IdentityLayer(const Core::Configuration &config);
    virtual ~IdentityLayer();
public:
    virtual void forward(const std::vector<NnMatrix*>& input, NnMatrix& output);
    virtual void backpropagateActivations(const NnMatrix& errorSignalIn, NnMatrix& errorSignalOut,
	    const NnMatrix& activations);
};

//=============================================================================
/**	Apply tanh activation to the input */
template<typename T>
class TanhLayer : public virtual NeuralNetworkLayer<T> {
protected:
    typedef typename Types<T>::NnVector NnVector;
    typedef typename Types<T>::NnMatrix NnMatrix;
public:
    TanhLayer(const Core::Configuration &config);
    virtual ~TanhLayer();
protected:
    /**	Apply the tanh function to the input features */
    void _forward(const NnMatrix& input, NnMatrix& output);
    void _backpropagateActivations(const NnMatrix& errorSignalIn, NnMatrix& errorSignalOut, const NnMatrix& activations);
public:
    /**	Apply the tanh function to the input features */
    virtual void forward(const std::vector<NnMatrix*>& input, NnMatrix& output);
    virtual void backpropagateActivations(const NnMatrix& errorSignalIn, NnMatrix& errorSignalOut,
	    const NnMatrix& activations);
};

//=============================================================================
/**	Apply sigmoid activation to the input */
template<typename T>
class SigmoidLayer : public virtual NeuralNetworkLayer<T> {
protected:
    typedef typename Types<T>::NnVector NnVector;
    typedef typename Types<T>::NnMatrix NnMatrix;
private:
    const T gamma_;
    mutable double timeForwardSigmoid_, timeBackwardSigmoid_;
protected:
    static const Core::ParameterFloat paramScaleGamma;
    T getGamma() const { return gamma_; }
public:
    SigmoidLayer(const Core::Configuration &config);
    virtual ~SigmoidLayer();
protected:
    void _forward(const NnMatrix& input, NnMatrix& output);
    void _backpropagateActivations(const NnMatrix& errorSignalIn, NnMatrix& errorSignalOut, const NnMatrix& activations);
public:
    virtual void forward(const std::vector<NnMatrix*>& input, NnMatrix& output);
    virtual void backpropagateActivations(const NnMatrix& errorSignalIn, NnMatrix& errorSignalOut,
	    const NnMatrix& activations);
    // log runtime statistics
    virtual void finalize();
};

//=============================================================================
/**	Apply softmax activation to the input */
template<typename T>
class SoftmaxLayer : public virtual NeuralNetworkLayer<T> {
    typedef NeuralNetworkLayer<T> Precursor;
protected:
    typedef typename Types<T>::NnVector NnVector;
    typedef typename Types<T>::NnMatrix NnMatrix;
private:
    mutable double timeForwardSoftmax_, timeBackwardSoftmax_;
    mutable NnVector tmpVector_;
    mutable NnMatrix tmpMatrix_;
public:
    SoftmaxLayer(const Core::Configuration &config);
    virtual ~SoftmaxLayer();
protected:
    void _forward(const NnMatrix& input, NnMatrix& output);
    void _backpropagateActivations(const NnMatrix& errorSignalIn, NnMatrix& errorSignalOut, const NnMatrix& activations);
public:
    virtual void forward(const std::vector<NnMatrix*>& input, NnMatrix& output);
    virtual void backpropagateActivations(const NnMatrix& errorSignalIn, NnMatrix& errorSignalOut,
	    const NnMatrix& activations);
    // log runtime statistics
    virtual void finalize();
};

//=============================================================================
/**	Apply linear rectified activation to the input */
template<typename T>
class RectifiedLayer : public virtual NeuralNetworkLayer<T> {
protected:
    typedef typename Types<T>::NnVector NnVector;
    typedef typename Types<T>::NnMatrix NnMatrix;
private:
    mutable double timeForwardRectified_, timeBackwardRectified_;
public:
    RectifiedLayer(const Core::Configuration &config);
    virtual ~RectifiedLayer();
protected:
    void _forward(const NnMatrix& input, NnMatrix& output);
    void _backpropagateActivations(const NnMatrix& errorSignalIn, NnMatrix& errorSignalOut, const NnMatrix& activations);
public:
    virtual void forward(const std::vector<NnMatrix*>& input, NnMatrix& output);
    virtual void backpropagateActivations(const NnMatrix& errorSignalIn, NnMatrix& errorSignalOut,
	    const NnMatrix& activations);
    // log runtime statistics
    virtual void finalize();
};
} // namespace Nn

#endif // _NN_NEURAL_NETWORK_LAYER_ACTIVATION_FUNCTION_HH

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
/* Implementation of different activation functions for neural networks */

#include "ActivationLayer.hh"
#include <Core/Assertions.hh>

using namespace Nn;

/*===========================================================================*/
template<typename T>
IdentityLayer<T>::IdentityLayer(const Core::Configuration &config) :
Core::Component(config), NeuralNetworkLayer<T>(config) {}

template<typename T>
IdentityLayer<T>::~IdentityLayer() {}

/**	Identity activation does nothing => copy input to output */
template<typename T>
void IdentityLayer<T>::forward(const std::vector<NnMatrix*>& input, NnMatrix& output) {
    // one stream, same matrix size
    require_eq(input.size(), 1u);
    require_eq(input[0]->nRows(), output.nRows());
    require_eq(input[0]->nColumns(), output.nColumns());

    // no transformation to be performed here, so just copy the data
    output.copy(*input[0]);
}

template<typename T>
void IdentityLayer<T>::backpropagateActivations(const NnMatrix& errorSignalIn,
	NnMatrix& errorSignalOut, const NnMatrix& activations) {
    require_eq(errorSignalIn.nRows(), errorSignalOut.nRows());
    require_eq(errorSignalIn.nColumns(), errorSignalOut.nColumns());

    // no change in error signal
    errorSignalOut.copy(errorSignalIn);
}

/*===========================================================================*/
template<typename T>
TanhLayer<T>::TanhLayer(const Core::Configuration &config) :
Core::Component(config), NeuralNetworkLayer<T>(config) {}

template<typename T>
TanhLayer<T>::~TanhLayer() {}

template<typename T>
void TanhLayer<T>::_forward(const NnMatrix& input, NnMatrix& output) {
    if (&input != &output) {
	output.copy(input);
    }
    output.tanh();
}

template<typename T>
void TanhLayer<T>::_backpropagateActivations(const NnMatrix& errorSignalIn, NnMatrix& errorSignalOut,
	const NnMatrix& activations) {
    if (&errorSignalIn != &errorSignalOut) {
	errorSignalOut.copy(errorSignalIn);
    }
    // errorSignalOut = errorSignalIn .* (1 - activations .* activations)
    errorSignalOut.elementwiseMultiplicationWithTanhDerivative(activations);
}

template<typename T>
void TanhLayer<T>::forward(const std::vector<NnMatrix*>& input, NnMatrix& output) {
    // one stream, same matrix size
    require_eq(input.size(), 1u);
    require_eq(input[0]->nRows(), output.nRows());
    require_eq(input[0]->nColumns(), output.nColumns());

    _forward(*(input[0]), output);
}

template<typename T>
void TanhLayer<T>::backpropagateActivations(const NnMatrix& errorSignalIn, NnMatrix& errorSignalOut,
	const NnMatrix& activations) {
    require_eq(errorSignalIn.nRows(), errorSignalOut.nRows());
    require_eq(errorSignalIn.nColumns(), errorSignalOut.nColumns());

    _backpropagateActivations(errorSignalIn, errorSignalOut, activations);
}

/*===========================================================================*/
template<typename T>
const Core::ParameterFloat SigmoidLayer<T>::paramScaleGamma(
	"gamma", "exponential scaling factor", 1.0);

template<typename T>
SigmoidLayer<T>::SigmoidLayer(const Core::Configuration &config) :
Core::Component(config),
NeuralNetworkLayer<T>(config),
gamma_(paramScaleGamma(config)),
timeForwardSigmoid_(0),
timeBackwardSigmoid_ (0)
{};

template<typename T>
SigmoidLayer<T>::~SigmoidLayer() {};

/**	Apply the sigmoid function to the input features */
template<typename T>
void SigmoidLayer<T>::_forward(const NnMatrix& input, NnMatrix& output) {
    timeval start, end;
    if (&input != &output) {
	output.copy(input);
    }
    gettimeofday(&start, NULL);
    output.sigmoid(gamma_);
    Math::Cuda::deviceSync(this->measureTime_ && Math::CudaDataStructure::hasGpu());
    gettimeofday(&end, NULL);
    timeForwardSigmoid_  += Core::timeDiff(start, end);
}

template<typename T>
void SigmoidLayer<T>::_backpropagateActivations(const NnMatrix& errorSignalIn, NnMatrix& errorSignalOut,
	const NnMatrix& activations) {

    timeval start, end;
    if (&errorSignalIn != &errorSignalOut) {
	errorSignalOut.copy(errorSignalIn);
    }
    // errorSignalOut = errorSignalIn .* (activations .* (1 - activations))

    gettimeofday(&start, NULL);
    errorSignalOut.elementwiseMultiplicationWithSigmoidDerivative(activations);
    Math::Cuda::deviceSync(this->measureTime_&& Math::CudaDataStructure::hasGpu());
    gettimeofday(&end, NULL);
    timeBackwardSigmoid_ += Core::timeDiff(start, end);
}

template<typename T>
void SigmoidLayer<T>::forward(const std::vector<NnMatrix*>& input, NnMatrix& output) {
    // one stream, same matrix size
    require_eq(input.size(), 1u);
    require_eq(input[0]->nRows(), output.nRows());
    require_eq(input[0]->nColumns(), output.nColumns());

    // forward pass
    _forward(*(input[0]), output);
}

template<typename T>
void SigmoidLayer<T>::backpropagateActivations(const NnMatrix& errorSignalIn,
	NnMatrix& errorSignalOut, const NnMatrix& activations) {
    require_eq(errorSignalIn.nRows(), errorSignalOut.nRows());
    require_eq(errorSignalIn.nColumns(), errorSignalOut.nColumns());

    _backpropagateActivations(errorSignalIn, errorSignalOut, activations);
}

template<typename T>
void SigmoidLayer<T>::finalize(){
    if (this->measureTime_){
	this->log("Sigmoid layer: Time for forward pass: ") << timeForwardSigmoid_;
	this->log("Sigmoid layer: Time for backward pass: ") << timeBackwardSigmoid_;
    }
    NeuralNetworkLayer<T>::finalize();
}

/*===========================================================================*/

template<typename T>
SoftmaxLayer<T>::SoftmaxLayer(const Core::Configuration &config) :
Core::Component(config),
NeuralNetworkLayer<T>(config),
timeForwardSoftmax_(0),
timeBackwardSoftmax_ (0)
{}

template<typename T>
SoftmaxLayer<T>::~SoftmaxLayer() {}

/**	Apply the softmax function to a bunch of features
 */
template<typename T>
void SoftmaxLayer<T>::_forward(const NnMatrix& input, NnMatrix& output) {
    timeval start, end;
    if (&input != &output) {
	output.copy(input);
    }
    gettimeofday(&start, NULL);
    output.softmax(tmpVector_, tmpMatrix_);
    Math::Cuda::deviceSync(this->measureTime_&& Math::CudaDataStructure::hasGpu());
    gettimeofday(&end, NULL);
    timeForwardSoftmax_  += Core::timeDiff(start, end);
}

template<typename T>
void SoftmaxLayer<T>::_backpropagateActivations(const NnMatrix& errorSignalIn, NnMatrix& errorSignalOut,
	const NnMatrix& activations) {
    timeval start, end;
    if (&errorSignalIn != &errorSignalOut) {
	errorSignalOut.copy(errorSignalIn);
    }
    gettimeofday(&start, NULL);
    errorSignalOut.multiplicationWithSoftmaxDerivative(activations);
    Math::Cuda::deviceSync(this->measureTime_&& Math::CudaDataStructure::hasGpu());
    gettimeofday(&end, NULL);
    timeBackwardSoftmax_ += Core::timeDiff(start, end);
}

template<typename T>
void SoftmaxLayer<T>::forward(const std::vector<NnMatrix*>& input, NnMatrix& output) {
    // one stream, same matrix size
    require_eq(input.size(), 1u);
    require_eq(input[0]->nRows(), output.nRows());
    require_eq(input[0]->nColumns(), output.nColumns());

    // forward the network
    _forward(*(input[0]), output);
}

template<typename T>
void SoftmaxLayer<T>::backpropagateActivations(const NnMatrix& errorSignalIn,
	NnMatrix& errorSignalOut, const NnMatrix& activations) {
    require_eq(errorSignalIn.nRows(), errorSignalOut.nRows());
    require_eq(errorSignalIn.nColumns(), errorSignalOut.nColumns());

    _backpropagateActivations(errorSignalIn, errorSignalOut, activations);
}

template<typename T>
void SoftmaxLayer<T>::finalize(){
    if (this->measureTime_){
	this->log("Softmax layer: Time for forward pass: ") << timeForwardSoftmax_;
	this->log("Softmax layer: Time for backward pass: ") << timeBackwardSoftmax_;
    }
    NeuralNetworkLayer<T>::finalize();
}

/*===========================================================================*/
template<typename T>
RectifiedLayer<T>::RectifiedLayer(const Core::Configuration &config) :
Core::Component(config),
NeuralNetworkLayer<T>(config),
timeForwardRectified_(0),
timeBackwardRectified_ (0)
{};

template<typename T>
RectifiedLayer<T>::~RectifiedLayer() {};

/**	Apply the rectified linear function to the input features */
template<typename T>
void RectifiedLayer<T>::_forward(const NnMatrix& input, NnMatrix& output) {
    timeval start, end;
    if (&input != &output) {
	output.copy(input);
    }
    gettimeofday(&start, NULL);
    output.ensureMinimalValue(0);
    Math::Cuda::deviceSync(this->measureTime_&& Math::CudaDataStructure::hasGpu());
    gettimeofday(&end, NULL);
    timeForwardRectified_  += Core::timeDiff(start, end);
}

template<typename T>
void RectifiedLayer<T>::_backpropagateActivations(const NnMatrix& errorSignalIn, NnMatrix& errorSignalOut,
	const NnMatrix& activations) {

    timeval start, end;
    if (&errorSignalIn != &errorSignalOut) {
	errorSignalOut.copy(errorSignalIn);
    }
    // errorSignalOut = errorSignalIn .* sign(activations)

    gettimeofday(&start, NULL);
    errorSignalOut.elementwiseMultiplicationWithRectifiedDerivative(activations);
    Math::Cuda::deviceSync(this->measureTime_&& Math::CudaDataStructure::hasGpu());
    gettimeofday(&end, NULL);
    timeBackwardRectified_ += Core::timeDiff(start, end);
}

template<typename T>
void RectifiedLayer<T>::forward(const std::vector<NnMatrix*>& input, NnMatrix& output) {
    // one stream, same matrix size
    require_eq(input.size(), 1u);
    require_eq(input[0]->nRows(), output.nRows());
    require_eq(input[0]->nColumns(), output.nColumns());

    // forward pass
    _forward(*(input[0]), output);
}

template<typename T>
void RectifiedLayer<T>::backpropagateActivations(const NnMatrix& errorSignalIn,
	NnMatrix& errorSignalOut, const NnMatrix& activations) {
    require_eq(errorSignalIn.nRows(), errorSignalOut.nRows());
    require_eq(errorSignalIn.nColumns(), errorSignalOut.nColumns());

    _backpropagateActivations(errorSignalIn, errorSignalOut, activations);
}

template<typename T>
void RectifiedLayer<T>::finalize(){
    if (this->measureTime_){
	this->log("Rlu layer: Time for forward pass: ") << timeForwardRectified_;
	this->log("Rlu layer: Time for backward pass: ") << timeBackwardRectified_;
    }
    NeuralNetworkLayer<T>::finalize();
}


/*===========================================================================*/
// explicit template instantiation
namespace Nn {

template class IdentityLayer<f32>;
template class TanhLayer<f32>;
template class SigmoidLayer<f32>;
template class SoftmaxLayer<f32>;
template class RectifiedLayer<f32>;

template class IdentityLayer<f64>;
template class TanhLayer<f64>;
template class SigmoidLayer<f64>;
template class SoftmaxLayer<f64>;
template class RectifiedLayer<f64>;

}

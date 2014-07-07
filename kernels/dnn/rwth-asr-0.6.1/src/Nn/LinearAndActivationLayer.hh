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
#ifndef _NN_NNLAYER_WEIHTS_AND_ACTIVATION_HH_
#define _NN_NNLAYER_WEIHTS_AND_ACTIVATION_HH_

#include <Math/CudaVector.hh>
#include <Math/CudaMatrix.hh>

#include "ActivationLayer.hh"
#include "LinearLayer.hh"
#include "Prior.hh"
#include "Types.hh"

namespace Nn {

/*
 *  Layers combining linear and non-linear (actication) part
 *
 *  The layers derive from LinearLayer and an actication layer and just call their
 *  predecessors. The advantage is that they only need a single activation and error
 *  signal, which saves memory.
 *
 *  documentation of layer methods in NeuralNetworkLayer.hh
 */

/*
 * Linear and sigmoid layer
 *
 */
template<typename T>
class LinearAndSigmoidLayer : public LinearLayer<T>, public SigmoidLayer<T> {
    typedef LinearLayer<T> PrecursorLinear;
    typedef SigmoidLayer<T> PrecursorSigmoid;
protected:
    typedef typename Types<T>::NnVector NnVector;
    typedef typename Types<T>::NnMatrix NnMatrix;
public:
    LinearAndSigmoidLayer(const Core::Configuration &config);
    virtual ~LinearAndSigmoidLayer();
    // is a combined layer
    virtual bool isComposedLayer() const { return true; }
    virtual void forward(const std::vector<NnMatrix*>& input, NnMatrix& output);
    virtual void backpropagateActivations(const NnMatrix& errorSignalIn, NnMatrix& errorSignalOut,
	    const NnMatrix& activations);
    virtual void backpropagateWeights(const NnMatrix& errorSignalIn, std::vector<NnMatrix*>& errorSignalOut);
    virtual void addToWeightsGradient(const NnMatrix& layerInput, const NnMatrix& errorSignalIn, u32 stream, NnMatrix& gradientWeights);
    virtual void addToBiasGradient(const NnMatrix& layerInput, const NnMatrix& errorSignalIn, u32 stream, NnVector& gradientBias);
    virtual void initComputation(bool sync = true) const;
    virtual void finalize();
    virtual u32 getNumberOfFreeParameters() const;

protected:
    virtual void _forward(const std::vector<NnMatrix*>& input, NnMatrix& output, bool reset);
};

/*
 * Linear and tanh layer
 *
 */
template<typename T>
class LinearAndTanhLayer : public LinearLayer<T>, public TanhLayer<T> {
    typedef LinearLayer<T> PrecursorLinear;
    typedef TanhLayer<T> PrecursorTanh;
protected:
    typedef typename Types<T>::NnVector NnVector;
    typedef typename Types<T>::NnMatrix NnMatrix;
public:
    LinearAndTanhLayer(const Core::Configuration &config);
    virtual ~LinearAndTanhLayer() {};
    virtual bool isComposedLayer() const { return true; }
    virtual void forward(const std::vector<NnMatrix*>& input, NnMatrix& output);
    virtual void backpropagateActivations(const NnMatrix& errorSignalIn, NnMatrix& errorSignalOut,
	    const NnMatrix& activations);
    virtual void backpropagateWeights(const NnMatrix& errorSignalIn, std::vector<NnMatrix*>& errorSignalOut);
    virtual void addToWeightsGradient(const NnMatrix& layerInput, const NnMatrix& errorSignalIn, u32 stream, NnMatrix& gradientWeights);
    virtual void addToBiasGradient(const NnMatrix& layerInput, const NnMatrix& errorSignalIn, u32 stream, NnVector& gradientBias);

    virtual void initComputation(bool sync = true) const;
    virtual u32 getNumberOfFreeParameters() const;

protected:
    virtual void _forward(const std::vector<NnMatrix*>& input, NnMatrix& output, bool reset);
};

/*
 * 	Linear and rectified linear layer
 */
template<typename T>
class LinearAndRectifiedLayer : public LinearLayer<T>, public RectifiedLayer<T> {
    typedef LinearLayer<T> PrecursorLinear;
    typedef RectifiedLayer<T> PrecursorRectified;
protected:
    typedef typename Types<T>::NnVector NnVector;
    typedef typename Types<T>::NnMatrix NnMatrix;
public:
    LinearAndRectifiedLayer(const Core::Configuration &config);
    virtual ~LinearAndRectifiedLayer() {};
    virtual bool isComposedLayer() const { return true; }
    virtual void forward(const std::vector<NnMatrix*>& input, NnMatrix& output);
    virtual void backpropagateActivations(const NnMatrix& errorSignalIn, NnMatrix& errorSignalOut,
	    const NnMatrix& activations);
    virtual void backpropagateWeights(const NnMatrix& errorSignalIn, std::vector<NnMatrix*>& errorSignalOut);
    virtual void addToWeightsGradient(const NnMatrix& layerInput, const NnMatrix& errorSignalIn, u32 stream, NnMatrix& gradientWeights);
    virtual void addToBiasGradient(const NnMatrix& layerInput, const NnMatrix& errorSignalIn, u32 stream, NnVector& gradientBias);

    virtual void initComputation(bool sync = true) const;
    virtual void finalize();
    virtual u32 getNumberOfFreeParameters() const;

protected:
    virtual void _forward(const std::vector<NnMatrix*>& input, NnMatrix& output, bool reset);
};

/*
 *  Linear and softmax layer
 *
 *  This layer has some additional methods, which are useful, because it is typically the output layer
 *
 */
template<typename T>
class LinearAndSoftmaxLayer : public LinearLayer<T>, public SoftmaxLayer<T> {
    typedef LinearLayer<T> PrecursorLinear;
    typedef SoftmaxLayer<T> PrecursorSoftmax;
protected:
    typedef typename Types<T>::NnVector NnVector;
    typedef typename Types<T>::NnMatrix NnMatrix;
protected:
    static const Core::ParameterBool paramEvaluateSoftmax;
protected:
    bool evaluateSoftmax_;			// apply softmax or just linear part
    bool logPriorIsRemovedFromBias_;		// has bias been modified in initialization ?
public:
    LinearAndSoftmaxLayer(const Core::Configuration &config);
    virtual ~LinearAndSoftmaxLayer() {};

    virtual bool isComposedLayer() const { return true; }
    virtual void forward(const std::vector<NnMatrix*>& input, NnMatrix& output);
    virtual void backpropagateActivations(const NnMatrix& errorSignalIn, NnMatrix& errorSignalOut,
	    const NnMatrix& activations);
    virtual void backpropagateWeights(const NnMatrix& errorSignalIn, std::vector<NnMatrix*>& errorSignalOut);
    virtual void addToWeightsGradient(const NnMatrix& layerInput, const NnMatrix& errorSignalIn, u32 stream, NnMatrix& gradientWeights);
    virtual void addToBiasGradient(const NnMatrix& layerInput, const NnMatrix& errorSignalIn, u32 stream, NnVector& gradientBias);


    virtual void initComputation(bool sync = true) const;
    virtual void finalize();
    virtual u32 getNumberOfFreeParameters() const;

    // remove scaled log-prior from bias parameters
    template<typename S>
    void removeLogPriorFromBias(const Prior<S> &logPrior);

    // apply softmax to current activation
    virtual void applySoftmax(NnMatrix &activations);
    // computes scores (negative inner products) of activations in for class columnIndex
    T getScore(const NnMatrix &in, u32 columnIndex);

    void setEvaluateSoftmax(bool val){
	if (!val)
	    this->log("setting evaluateSoftmax to false");
	if (val)
	    this->log("setting evaluateSoftmax to true");
	evaluateSoftmax_ = val;
    }

    bool evaluatesSoftmax() const { return evaluateSoftmax_; }
    bool logPriorIsRemovedFromBias() const { return logPriorIsRemovedFromBias_; }

protected:
    virtual void _forward(const std::vector<NnMatrix*>& input, NnMatrix& output, bool reset);
};

template<typename T>
template<typename S>
inline void LinearAndSoftmaxLayer<T>::removeLogPriorFromBias(const Prior<S> &priors){
    require(this->getBias());
    require_eq(priors.size(), this->getBias()->size());
    T prioriScale = priors.scale();
    if (prioriScale != S(0.0)){
	this->log("removing scaled log-prior from bias parameters (scale: ") << prioriScale << ")";
	// subtract
	if (this->getBias()->isComputing()){
	    typename Types<S>::NnVector priorVector;
	    priors.getVector(priorVector);
	    this->getBias()->add(priorVector, -S(prioriScale));
	}
	else {
	    for (u32 c = 0; c < this->getBias()->size(); c++)
		this->getBias()->at(c) -= prioriScale * priors.at(c);
	}

	logPriorIsRemovedFromBias_ = true;
    }
}


} // namespace

#endif /* _NN_NNLAYER_WEIHTS_AND_ACTIVATION_HH_ */

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
#ifndef _NN_NEURAL_NETWORK_LAYER_PROCESSING_FUNCTION_HH
#define _NN_NEURAL_NETWORK_LAYER_PROCESSING_FUNCTION_HH

#include "NeuralNetworkLayer.hh"


/*
 *	Implementation of several feature pre-processing steps as neural network layers
 *
 * 	Advantage over usage of Flow:
 * 	-> GPU acceleration
 * 	-> works on windowed features which are computed in BufferedFeatureExtractor
 *
 */

namespace Nn {

/**
 * 	Apply logarithm to the input
 */
template<typename T>
class LogarithmPreprocessingLayer : public virtual NeuralNetworkLayer<T> {
    typedef typename Types<T>::NnVector NnVector;
    typedef typename Types<T>::NnMatrix NnMatrix;
public:
    LogarithmPreprocessingLayer(const Core::Configuration &config);
    virtual ~LogarithmPreprocessingLayer();
private:
    void _forward(const NnMatrix& input, NnMatrix& output);
    void _backpropagateActivations(const NnMatrix& errorSignalIn, NnMatrix& errorSignalOut, const NnMatrix& activations);
public:
    virtual void forward(const std::vector<NnMatrix*>& input, NnMatrix& output);
    virtual void backpropagateActivations(const NnMatrix& errorSignalIn, NnMatrix& errorSignalOut,
	    const NnMatrix& activations);
};


/*
 * 	Apply mean and variance normalization to input
 */
template<typename T>
class MeanAndVarianceNormalizationPreprocessingLayer : public virtual NeuralNetworkLayer<T> {
    typedef NeuralNetworkLayer<T> Precursor;
    typedef typename Types<T>::NnVector NnVector;
    typedef typename Types<T>::NnMatrix NnMatrix;
protected:
    static const Core::ParameterString paramFilenameMean;
    static const Core::ParameterString paramFilenameStandardDeviation;
    const std::string filenameMean_;
    const std::string filenameStandardDeviation_;
protected:
    bool needInit_;
    NnVector mean_;
    NnVector standardDeviation_;
public:
    MeanAndVarianceNormalizationPreprocessingLayer(const Core::Configuration &config);
    virtual ~MeanAndVarianceNormalizationPreprocessingLayer(){};
private:
    void loadNetworkParameterMean(const std::string &filename);
    void loadNetworkParameterVariance(const std::string &filename);
    void _forward(const NnMatrix& input, NnMatrix& output);
public:
    virtual void loadNetworkParameters(const std::string &filename);
    virtual void initializeNetworkParameters() { loadNetworkParameters(""); }
public:
    virtual void forward(const std::vector<NnMatrix*>& input, NnMatrix& output);
};


/**
 *  Nth-order polynomial preprocessing layer (useful for log-linear models)
 */

template<typename T>
class PolynomialPreprocessingLayer : public virtual NeuralNetworkLayer<T> {
    typedef NeuralNetworkLayer<T> Precursor;
    typedef typename Types<T>::NnVector NnVector;
    typedef typename Types<T>::NnMatrix NnMatrix;
    static const Core::ParameterInt paramOrder;
protected:
    const u32 order_;
public:
    PolynomialPreprocessingLayer(const Core::Configuration &config);
    virtual ~PolynomialPreprocessingLayer() {}
private:
    void _forward(const NnMatrix& input, NnMatrix& output);
public:
    virtual void forward(const std::vector<NnMatrix*>& input, NnMatrix& output);
};

//====================================================================================

/** Gaussian noise preprocessing layer */

template<typename T>
class GaussianNoisePreprocessingLayer : public virtual NeuralNetworkLayer<T> {
    typedef NeuralNetworkLayer<T> Precursor;
    typedef typename Types<T>::NnVector NnVector;
    typedef typename Types<T>::NnMatrix NnMatrix;
    static const Core::ParameterFloat paramStandardDeviation;
protected:
    const T standardDeviation_;
public:
    GaussianNoisePreprocessingLayer(const Core::Configuration &config);
    virtual ~GaussianNoisePreprocessingLayer() {}
private:
    void _forward(const NnMatrix& input, NnMatrix& output);
public:
    virtual void forward(const std::vector<NnMatrix*>& input, NnMatrix& output);
};


}
#endif // _NN_NEURAL_NETWORK_LAYER_PROCESSING_FUNCTION_HH

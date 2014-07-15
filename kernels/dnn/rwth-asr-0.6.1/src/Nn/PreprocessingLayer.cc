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

#include <cmath>
#include <numeric>

#include "PreprocessingLayer.hh"

// for reading/writing vectors/matrices
#include <Math/Module.hh>
#include <Math/Vector.hh>
#include <Core/VectorParser.hh>
#include <Math/Matrix.hh>
#include <Core/MatrixParser.hh>


using namespace Nn;

/*===========================================================================*/
template<typename T>
LogarithmPreprocessingLayer<T>::LogarithmPreprocessingLayer(const Core::Configuration &config) :
Core::Component(config),
NeuralNetworkLayer<T> (config)
{}

template<typename T>
LogarithmPreprocessingLayer<T>::~LogarithmPreprocessingLayer() {}

/**	Transform the features by logarithm
 *
 * 	@param	source	Input features
 * 	@param	target	Transformed (output) features
 * 	@param	nFrames	Number of frames to be processed
 *
 */
template<typename T>
void LogarithmPreprocessingLayer<T>::_forward(const NnMatrix& input, NnMatrix& output) {
    if (&input != &output) {
	output.copy(input);
    }

    output.log();
}

template<typename T>
inline void LogarithmPreprocessingLayer<T>::_backpropagateActivations(const NnMatrix& errorSignalIn,
	NnMatrix& errorSignalOut, const NnMatrix& activations) {
    if (&errorSignalIn != &errorSignalOut) {
	errorSignalOut.copy(errorSignalIn);
    }
    // errorSignalOut = activations .^ (-1)
    errorSignalOut.elementwiseDivision(activations);
}

template<typename T>
void LogarithmPreprocessingLayer<T>::forward(const std::vector<NnMatrix*>& input, NnMatrix& output) {
    // one stream, same matrix size
    require_eq(input.size(), 1u);
    require_eq(input[0]->nRows(), output.nRows());
    require_eq(input[0]->nColumns(), output.nColumns());

    _forward(*(input[0]), output);
}

template<typename T>
inline void LogarithmPreprocessingLayer<T>::backpropagateActivations(const NnMatrix& errorSignalIn,
	NnMatrix& errorSignalOut, const NnMatrix& activations) {
    require_eq(errorSignalIn.nRows(), errorSignalOut.nRows());
    require_eq(errorSignalIn.nColumns(), errorSignalOut.nColumns());

    _backpropagateActivations(errorSignalIn, errorSignalOut, activations);
}

/*===========================================================================*/
/**
 *	normalize the features by mean and variance
 *
 * 	@param	source	Input features
 * 	@param	target	Transformed (output) features
 * 	@param	nFrames	Number of frames to be processed
 *
 */
template<typename T>
const Core::ParameterString MeanAndVarianceNormalizationPreprocessingLayer<T>::paramFilenameMean(
	"mean-file", "Filename of the mean vector", "");

template<typename T>
const Core::ParameterString MeanAndVarianceNormalizationPreprocessingLayer<T>::paramFilenameStandardDeviation(
	"standard-deviation-file", "Filename of the standard deviation vector", "");

template<typename T>
MeanAndVarianceNormalizationPreprocessingLayer<T>::MeanAndVarianceNormalizationPreprocessingLayer(const Core::Configuration &config) :
    Core::Component(config),
    NeuralNetworkLayer<T> (config),
    filenameMean_(paramFilenameMean(config)),
    filenameStandardDeviation_(paramFilenameStandardDeviation(config)),
    needInit_(true),
    mean_(),
    standardDeviation_()
{
    this->log("mean file: ") << filenameMean_;
    this->log("standard deviation file: ") << filenameStandardDeviation_;
}

template<typename T>
void MeanAndVarianceNormalizationPreprocessingLayer<T>::_forward(const NnMatrix& input, NnMatrix& output) {
    require(!needInit_);

    if (&input != &output) {
	output.copy(input);
    }
    output.addToAllColumns(mean_, -1.0);
    output.divideRowsByScalars(standardDeviation_);
}

template<typename T>
void MeanAndVarianceNormalizationPreprocessingLayer<T>::forward(const std::vector<NnMatrix*>& input, NnMatrix& output) {
    // one stream, same matrix size
    require_eq(input.size(), 1u);
    require_eq(input[0]->nRows(), output.nRows());
    require_eq(input[0]->nColumns(), output.nColumns());

    _forward(*(input[0]), output);
}

template<typename T>
void MeanAndVarianceNormalizationPreprocessingLayer<T>::loadNetworkParameterMean(const std::string &filename) {
    // parse the xml file
    Math::Vector<T> parameters;
    Core::XmlVectorDocument<T> parser(Core::Component::getConfiguration(), parameters);
    parser.parseFile(filename.c_str());

    // Convert Math::Vector -> Math::FastVector
    mean_.resize(parameters.size());
    for (u32 index = 0; index < parameters.size(); ++index) {
	mean_.at(index) = parameters[index];
    }
}

template<typename T>
void MeanAndVarianceNormalizationPreprocessingLayer<T>::loadNetworkParameterVariance(const std::string &filename) {
    // parse the xml file
    Math::Vector<T> parameters;
    Core::XmlVectorDocument<T> parser(Core::Component::getConfiguration(), parameters);
    parser.parseFile(filename.c_str());

    // Convert Math::Vector -> Math::FastVector
    standardDeviation_.resize(parameters.size());
    for (u32 index = 0; index < parameters.size(); ++index) {
	standardDeviation_.at(index) = parameters[index];
    }
}

template<typename T>
void MeanAndVarianceNormalizationPreprocessingLayer<T>::loadNetworkParameters(const std::string &filename) {
    // load the mean vector
    loadNetworkParameterMean(filenameMean_);

    // load the variance matrix
    loadNetworkParameterVariance(filenameStandardDeviation_);

    mean_.initComputation();
    standardDeviation_.initComputation();

    // initialization done
    needInit_ = false;
}

/*===========================================================================*/
/**
 *
 *  compute polynomial features
 *
 */
template<typename T>
const Core::ParameterInt PolynomialPreprocessingLayer<T>::paramOrder(
	"order", "polynomial order", 1);


template<typename T>
PolynomialPreprocessingLayer<T>::PolynomialPreprocessingLayer(const Core::Configuration &c) :
    Core::Component(c),
    NeuralNetworkLayer<T>(c),
    order_(paramOrder(c))
{
    this->log("creating polynomial feature extraction layer with order: ") << order_;
    u32 outputDimension = Precursor::inputDimensions_[0];
    if (order_ > 1)
	outputDimension += Precursor::inputDimensions_[0] * (Precursor::inputDimensions_[0] + 1) / 2;
    if (order_ > 2)
	outputDimension += Precursor::inputDimensions_[0] * (Precursor::inputDimensions_[0] + 1) * (Precursor::inputDimensions_[0] + 2)/ 6;
    if (order_ > 3)
	this->error("only order up to three implemented yet!");

    if (Precursor::outputDimension_ != outputDimension){
	this->log("resizing output of polynomial layer to ") << outputDimension;
	Precursor::outputDimension_ = outputDimension;
    }
}

template<typename T>
void PolynomialPreprocessingLayer<T>::_forward(const NnMatrix& input, NnMatrix &output){
    if (order_ == 1)
	output.copy(input);
    if (order_ == 2)
	output.setToSecondOrderFeatures(input);
    if (order_ == 3)
	output.setToThirdOrderFeatures(input);
}

template<typename T>
void PolynomialPreprocessingLayer<T>::forward(const std::vector<NnMatrix*>& input, NnMatrix& output) {
    // one stream only
    require_eq(input.size(), 1u);
    require_eq(input[0]->nColumns(), output.nColumns());

    _forward(*(input[0]), output);
}

/*===========================================================================*/
/**
 *
 *  add gaussian noise to the input
 *
 */
template<typename T>
const Core::ParameterFloat GaussianNoisePreprocessingLayer<T>::paramStandardDeviation(
	"standard-deviation", "standard deviation", 1.0);


template<typename T>
GaussianNoisePreprocessingLayer<T>::GaussianNoisePreprocessingLayer(const Core::Configuration &c) :
    Core::Component(c),
    NeuralNetworkLayer<T>(c),
    standardDeviation_(paramStandardDeviation(c))
{
    this->log("creating gaussian noise layer with standard deviation: ") << standardDeviation_;
}

template<typename T>
void GaussianNoisePreprocessingLayer<T>::_forward(const NnMatrix& input, NnMatrix &output) {
    output.addGaussianNoise(standardDeviation_);
}

template<typename T>
void GaussianNoisePreprocessingLayer<T>::forward(const std::vector<NnMatrix*>& input, NnMatrix& output) {
    // one stream only
    require_eq(input.size(), 1u);
    require_eq(input[0]->nColumns(), output.nColumns());
    require_eq(input[0]->nRows(), output.nRows());

    _forward(*(input[0]), output);
}


/*===========================================================================*/
namespace Nn {

// (explicit) template instantiation for type f32
template class LogarithmPreprocessingLayer<f32>;
template class MeanAndVarianceNormalizationPreprocessingLayer<f32>;
template class PolynomialPreprocessingLayer<f32>;
template class GaussianNoisePreprocessingLayer<f32>;

// (explicit) template instantiation for type f64
template class LogarithmPreprocessingLayer<f64>;
template class MeanAndVarianceNormalizationPreprocessingLayer<f64>;
template class PolynomialPreprocessingLayer<f64>;
template class GaussianNoisePreprocessingLayer<f64>;

}

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
#include "Regularizer.hh"
#include "LinearLayer.hh"

using namespace Nn;

template<class T>
const Core::Choice Regularizer<T>::choiceRegularizerType(
	"none", none,
	"l2-regularizer", l2Regularizer,
	Core::Choice::endMark());

template<class T>
const Core::ParameterChoice Regularizer<T>::paramRegularizerType(
	"regularizer", &choiceRegularizerType,
	"regularizer (adds regularization term to objective function)", none);

template<typename T>
Regularizer<T>::Regularizer(const Core::Configuration& config) :
Core::Component(config)
{}

template<class T>
Regularizer<T>* Regularizer<T>::createRegularizer(const Core::Configuration& config) {
    Regularizer<T>* regularizer;

    switch ( (RegularizerType) paramRegularizerType(config) ) {
    case l2Regularizer:
	regularizer = new L2Regularizer<T>(config);
	Core::Application::us()->log("Create regularizer: l2-regularizer");
	break;
    default:
	regularizer = new Regularizer<T>(config);
	Core::Application::us()->log("Create regularizer: none");
	break;
    };

    return regularizer;
}

//=============================================================================

template<typename T>
L2Regularizer<T>::L2Regularizer(const Core::Configuration& config):
Core::Component(config),
Precursor(config)
{}

template<typename T>
T L2Regularizer<T>::objectiveFunction(NeuralNetwork<T>& network, u32 batchSize) {
    T objectiveFunction = 0;
    for (u32 layer = 0; layer < network.nLayers(); layer++) {
	if ((network.getLayer(layer).isTrainable()) &&
		(network.getLayer(layer).regularizationConstant() > 0)) {
	    T tmpObjectiveFunction = 0;

	    NnVector *bias = network.getLayer(layer).getBias();
	    require(bias);
	    tmpObjectiveFunction = bias->sumOfSquares();
	    for (u32 stream = 0; stream < network.getLayer(layer).nInputActivations(); stream++) {
		NnMatrix *matrix = network.getLayer(layer).getWeights(stream);
		require(matrix);
		tmpObjectiveFunction += matrix->sumOfSquares();
	    }
	    tmpObjectiveFunction *= network.getLayer(layer).regularizationConstant() / 2.0;
	    objectiveFunction += tmpObjectiveFunction;
	}
    }
    return batchSize * objectiveFunction;
}

template<typename T>
void L2Regularizer<T>::addGradient(NeuralNetwork<T>& network, Statistics<T>& statistics) {
    for (u32 layer = 0; layer < network.nLayers(); layer++) {
	if ((network.getLayer(layer).isTrainable()) &&
		(network.getLayer(layer).regularizationConstant() > 0)) {
	    for (u32 stream = 0; stream < network.getLayer(layer).nInputActivations(); stream++) {
		NnMatrix *weights = network.getLayer(layer).getWeights(stream);
		require(weights);
		statistics.gradientWeights(layer)[stream].add(*weights, network.getLayer(layer).regularizationConstant() * statistics.nObservations());
	    }
	    NnVector *bias = network.getLayer(layer).getBias();
	    require(bias);
	    statistics.gradientBias(layer).add(*bias, network.getLayer(layer).regularizationConstant() * statistics.nObservations());
	}
    }
}

//=============================================================================

// explicit template instantiation
namespace Nn {

template class Regularizer<f32>;
template class Regularizer<f64>;

template class L2Regularizer<f32>;
template class L2Regularizer<f64>;

}

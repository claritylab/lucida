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
#include "BatchEstimator.hh"

#include "Statistics.hh"
#include "NeuralNetwork.hh"
#include "Estimator.hh"
#include "Regularizer.hh"

using namespace Nn;

template<typename T>
const Core::ParameterStringVector BatchEstimator<T>::paramStatisticsFiles(
	"statistics-files", "filenames to read statistics from", ",");

template<typename T>
BatchEstimator<T>::BatchEstimator(const Core::Configuration& config) :
	Precursor(config),
	statisticsFiles_(paramStatisticsFiles(config)),
	statistics_(0),
	network_(0),
	estimator_(0),
	regularizer_(0)
{}

template<typename T>
BatchEstimator<T>::~BatchEstimator() {
    if (statistics_)
	delete statistics_;
    if (network_)
	delete network_;
    if (estimator_)
	delete estimator_;
    if (regularizer_)
	delete regularizer_;
}

template<typename T>
void BatchEstimator<T>::initialize() {
    // create network
    network_ = new NeuralNetwork<T>(config);
    network_->initializeNetwork(1);
    network_->initComputation();

    // create estimator
    estimator_ = Estimator<T>::createEstimator(config);
    estimator_->setBatchMode(true);

    if (statisticsFiles_.size() == 0)
	this->error("need at least one statistics file");

    // create a statistics object which holds the statistics contained in the file and those required by the estimator
    bool dummy;
    u32 statisticsTypeFile = 0;
    if (!Nn::Statistics<T>::getTypeFromFile(statisticsFiles_.at(0), statisticsTypeFile, dummy))
	this->error("could not read header from file: ") << statisticsFiles_.at(0);
    u32 requiredStatistics = estimator_->requiredStatistics();
    u32 statisticsType = statisticsTypeFile | requiredStatistics; // union
    // create statistics
    statistics_ = new Statistics<T>(network_->nLayers(), statisticsType);
    statistics_->initialize(*network_);

    if (!statistics_->combine(statisticsFiles_))
	this->error("failed to combine statistics files: ") << Core::vector2str(statisticsFiles_, ",");
    else
	this->log("combined statistics files: ") << Core::vector2str(statisticsFiles_, ",");

    statistics_->initComputation();

    for (u32 layer = 0; layer < network_->nLayers(); layer++)
	network_->getLayer(layer).initializeActivationStatistics(network_->getLayerOutput(layer));

    // create regularizer and apply regularization
    regularizer_ = Regularizer<T>::createRegularizer(config);
    statistics_->addToObjectiveFunction(regularizer_->objectiveFunction(*network_ ,statistics_->nObservations()));
    regularizer_->addGradient(*network_, *statistics_);

    statistics_->finalize();

    this->log("total-number-of-observations: ") << statistics_->nObservations();
    this->log("total-frame-classification-error: ") << (T) statistics_->classificationError();
    this->log("total-objective-function: ") << statistics_->objectiveFunction();
    this->log("l1-norm of trainable network weights: ") << network_->l1norm(true);
    this->log("l1-norm of gradient: ") << statistics_->gradientL1Norm();
}

template<typename T>
void BatchEstimator<T>::estimate() {
    estimator_->estimate(*network_, *statistics_);
}

template<typename T>
void BatchEstimator<T>::finalize() {
    network_->saveNetworkParameters();
}

//=============================================================================

// explicit template instantiation
namespace Nn {
template class BatchEstimator<f32>;
template class BatchEstimator<f64>;
}

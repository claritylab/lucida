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
#include "NeuralNetworkTrainer.hh"
#include "FeedForwardTrainer.hh"
#include "MeanNormalizedSgdEstimator.hh"
#include <Math/Blas.hh>
#include <limits>

#include <Math/Module.hh> // XML I/O stuff for writing parameters

#include <Modules.hh>

using namespace Nn;

template<typename T>
const Core::Choice NeuralNetworkTrainer<T>::choiceNetworkTrainer(
	"dummy", dummy,
	"feed-forward-trainer", feedForwardTrainer,
	"frame-classification-error", frameClassificationErrorAccumulator,
	"mean-and-variance-accumulator", meanAndVarianceAccumulator,
	Core::Choice::endMark());

template<typename T>
const Core::ParameterChoice NeuralNetworkTrainer<T>::paramNetworkTrainer(
	"trainer", &choiceNetworkTrainer,
	"trainer for the neural network", dummy);

template<typename T>
const Core::Choice NeuralNetworkTrainer<T>::choiceTrainingCriterion(
	"none", none,
	"cross-entropy", crossEntropy,
	"squared-error", squaredError,
	"binary-divergence", binaryDivergence,
	Core::Choice::endMark());

template<typename T>
const Core::ParameterChoice NeuralNetworkTrainer<T>::paramTrainingCriterion(
	"training-criterion", &choiceTrainingCriterion,
	"training criterion for the neural network", none);

template<typename T>
const Core::ParameterInt NeuralNetworkTrainer<T>::paramEpoch("epoch", "current epoch", 1);

template<typename T>
const Core::ParameterBool NeuralNetworkTrainer<T>::paramWeightedAccumulation(
	"weighted-accumulation", "use weights in training if possible and available", false);

template<typename T>
const Core::ParameterBool NeuralNetworkTrainer<T>::paramMeasureTime(
	"measure-time", "Measures time for executing methods in FeedForwardTrainer", false);

template<typename T>
NeuralNetworkTrainer<T>::NeuralNetworkTrainer(const Core::Configuration &config) :
Core::Component(config),
criterion_( (Criterion) paramTrainingCriterion(config)),
weightedAccumulation_(paramWeightedAccumulation(config)),
classWeights_(0),
measureTime_(paramMeasureTime(config)),
needsNetwork_(true),
statisticsChannel_(config, "statistics"),
needInit_(true),
network_(0)
{
    estimator_ = Estimator<T>::createEstimator(config);
    regularizer_ = Regularizer<T>::createRegularizer(config);
    logProperties();
}

template<typename T>
NeuralNetworkTrainer<T>::~NeuralNetworkTrainer(){
    if (network_){
	delete network_;
    }
}

template<typename T>
void NeuralNetworkTrainer<T>::initializeTrainer(u32 batchSize) {
    std::vector<u32> streamSizes;
    initializeTrainer(batchSize, streamSizes);
}

template<typename T>
void NeuralNetworkTrainer<T>::setBatchSize(u32 batchSize) {
    if (network_)
	network_->resizeActivations(batchSize);
}

template<typename T>
void NeuralNetworkTrainer<T>::initializeTrainer(u32 batchSize, std::vector<u32>& streamSizes) {
    if (needInit_) {
	if (estimator().type() == "prior-estimator")
	    this->needsNetwork_ = false;
	if (needsNetwork_){
	    network_ = new NeuralNetwork<T>(config),
		    // initialize the network with each layer and initialize (gpu) computation for the matrices
		    network_->initializeNetwork(batchSize, streamSizes);
	}
    }
    needInit_ = false;
}

template<typename T>
void NeuralNetworkTrainer<T>::setClassWeights(const Math::Vector<T> *classWeights){
    classWeights_ = classWeights;
}

template<typename T>
void NeuralNetworkTrainer<T>::finalize() {
    if (network_){
	network_->finalize();
	// save only when network has been changed
	if (estimator_ && (!estimator_->batchMode()))
	    network_->saveNetworkParameters();
    }
}

template<typename T>
void NeuralNetworkTrainer<T>::resetHistory() {
    if (network_)
	network_->resetPreviousActivations();
}

template<typename T>
void NeuralNetworkTrainer<T>::logProperties() const {
    if (weightedAccumulation_){
	this->log("using weighted accumulation");
    }
    if (measureTime_){
	this->log("measuring computation time");
    }
}

// create the specific type of supervised neural network trainer
template<typename T>
NeuralNetworkTrainer<T>* NeuralNetworkTrainer<T>::createSupervisedTrainer(const Core::Configuration& config) {
    NeuralNetworkTrainer<T>* trainer = NULL;

    Criterion criterion = (Criterion) paramTrainingCriterion(config);

    // get the type of the trainer
    switch ( (TrainerType) paramNetworkTrainer(config) ) {
    case feedForwardTrainer:
	switch (criterion) {
	case crossEntropy:
	    trainer = new FeedForwardCrossEntropyTrainer<T>(config);
	    Core::Application::us()->log("Create trainer: feed-forward-trainer with cross entropy criterion");
	    break;
	case squaredError:
	    trainer = new FeedForwardSquaredErrorTrainer<T>(config);
	    Core::Application::us()->log("Create trainer: feed-forward-trainer with squared error criterion");
	    break;
	case binaryDivergence:
	    trainer = new FeedForwardBinaryDivergenceTrainer<T>(config);
	    Core::Application::us()->log("Create trainer: feed-forward-trainer with binary divergence criterion");
	    break;
	default:
	    Core::Application::us()->log("Create trainer: default feed-forward trainer");
	    trainer = new FeedForwardDefaultTrainer<T>(config);
	    break;
	};
	break;
    case frameClassificationErrorAccumulator:
	trainer = new FrameErrorEvaluator<T>(config);
	Core::Application::us()->log("Create trainer: frame-classification-error");
	break;
    case meanAndVarianceAccumulator:
	trainer = new MeanAndVarianceTrainer<T>(config);
	Core::Application::us()->log("Create trainer: mean-and-variance-estimation");
	break;
    default: // dummy trainer
	Core::Application::us()->warning("The given trainer is not a valid supervised trainer type. Create dummy trainer.");
	trainer = new NeuralNetworkTrainer<T>(config);
	Core::Application::us()->log("Create trainer: dummy");
	break;
    };

    return trainer;
}

// create the specific type of unsupervised neural network trainer
template<typename T>
NeuralNetworkTrainer<T>* NeuralNetworkTrainer<T>::createUnsupervisedTrainer(const Core::Configuration& config) {
    NeuralNetworkTrainer<T>* trainer = NULL;

    // get the type of the trainer
    switch ( (TrainerType) paramNetworkTrainer(config) ) {
    case meanAndVarianceAccumulator:
	trainer = new MeanAndVarianceTrainer<T>(config);
	Core::Application::us()->log("Create trainer: mean-and-variance-estimation");
	break;
    default: // dummy trainer
	Core::Application::us()->warning("The given trainer is not a valid unsupervised trainer type. Create dummy trainer.");
	trainer = new NeuralNetworkTrainer<T>(config);
	Core::Application::us()->log("Create trainer: dummy");
	break;
    };

    return trainer;
}

//=============================================================================

template<typename T>
FrameErrorEvaluator<T>::FrameErrorEvaluator(const Core::Configuration &config) :
Core::Component(config),
Precursor(config),
nObservations_(0),
nFrameClassificationErrors_(0),
objectiveFunction_(0)
{}

template<typename T>
void FrameErrorEvaluator<T>::initializeTrainer(u32 batchSize, std::vector<u32>& streamSizes) {
    if (Precursor::needInit_) {
	std::stringstream s;
	switch (Precursor::criterion_) {
	case Precursor::crossEntropy:
	    s << "cross entropy";
	    break;
	case Precursor::squaredError:
	    s << "squared error";
	    break;
	case Precursor::binaryDivergence:
	    s << "binary divergence";
	    break;
	default:
	    s << "none";
	    break;
	};
	Core::Component::log("use criterion: ") << s.str();
	Precursor::initializeTrainer(batchSize, streamSizes);
    }
}

template<typename T>
void FrameErrorEvaluator<T>::finalize() {
    Core::Component::log("total-frame-classification-error: ") << (T) nFrameClassificationErrors_ / nObservations_;
    Core::Component::log("total-objective-function: ") << objectiveFunction_ / nObservations_;
    network().finalize();
}

template<typename T>
void FrameErrorEvaluator<T>::processBatch(std::vector<NnMatrix>& features,
	Math::CudaVector<u32> &alignment, NnVector& weights) {

    alignment.initComputation();
    if (Precursor::weightedAccumulation_)
	weights.initComputation();

    network().forward(features);

    u32 batchFrameClassificationErrors = network().getTopLayerOutput().nClassificationErrors(alignment);
    T batchObjectiveFunction = 0;
    if (Precursor::weightedAccumulation_) {
	switch (Precursor::criterion_) {
	case Precursor::crossEntropy:
	    batchObjectiveFunction = network().getTopLayerOutput().weightedCrossEntropyObjectiveFunction(alignment, weights);
	    break;
	case Precursor::squaredError:
	    batchObjectiveFunction = network().getTopLayerOutput().weightedSquaredErrorObjectiveFunction(alignment, weights);
	    break;
	case Precursor::binaryDivergence:
	    batchObjectiveFunction = network().getTopLayerOutput().weightedBinaryDivergenceObjectiveFunction(alignment, weights);
	    break;
	default:
	    break;
	};
    } else {
	switch (Precursor::criterion_) {
	case Precursor::crossEntropy:
	    batchObjectiveFunction = network().getTopLayerOutput().crossEntropyObjectiveFunction(alignment);
	    break;
	case Precursor::squaredError:
	    batchObjectiveFunction = network().getTopLayerOutput().squaredErrorObjectiveFunction(alignment);
	    break;
	case Precursor::binaryDivergence:
	    batchObjectiveFunction = network().getTopLayerOutput().binaryDivergenceObjectiveFunction(alignment);
	    break;
	default:
	    break;
	};
    }

    if (statisticsChannel_.isOpen()){
	statisticsChannel_ << Core::XmlOpen("batch-statistics")
	<< Core::XmlFull("frame-classification-error-rate-on-batch", (T) batchFrameClassificationErrors / features[0].nColumns())
	<< Core::XmlFull("objective-function-on-batch", batchObjectiveFunction / features[0].nColumns());
	statisticsChannel_ << Core::XmlClose("batch-statistics");
    }
    nFrameClassificationErrors_ += batchFrameClassificationErrors;
    nObservations_ += features[0].nColumns();
    objectiveFunction_ += batchObjectiveFunction;
}

//=============================================================================

template<typename T>
const Core::ParameterString MeanAndVarianceTrainer<T>::paramMeanFile(
	"mean-file", "", "");

template<typename T>
const Core::ParameterString MeanAndVarianceTrainer<T>::paramStandardDeviationFile(
	"standard-deviation-file", "", "");

template<typename T>
const Core::ParameterString MeanAndVarianceTrainer<T>::paramStatisticsFile(
	"statistics-filename", "filename to write statistics to", "");

template<typename T>
MeanAndVarianceTrainer<T>::MeanAndVarianceTrainer(const Core::Configuration &config) :
Core::Component(config),
Precursor(config),
statistics_(0),
meanFile_(paramMeanFile(config)),
standardDeviationFile_(paramStandardDeviationFile(config)),
statisticsFile_(paramStatisticsFile(config))
{
    this->needsNetwork_ = false;
}

template<typename T>
MeanAndVarianceTrainer<T>::~MeanAndVarianceTrainer(){
    if (statistics_)
	delete statistics_;
}


template<typename T>
void MeanAndVarianceTrainer<T>::saveVector(std::string& filename, Math::Vector<T>& vector) {
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

    Math::Module::instance().formats().write(newFilename, vector, 20);
}

template<typename T>
void MeanAndVarianceTrainer<T>::initializeTrainer(u32 batchSize, std::vector<u32>& streamSizes) {
    Precursor::initializeTrainer(batchSize, streamSizes);
    if (streamSizes.size() != 1)
	Core::Component::criticalError("MeanAndVarianceTrainer only implemented for single input streams");

    statistics_ = new Statistics<T>(0, Statistics<T>::MEAN_AND_VARIANCE);
    statistics_->featureSum().resize(streamSizes[0]);
    statistics_->featureSum().setToZero();
    statistics_->squaredFeatureSum().resize(streamSizes[0]);
    statistics_->squaredFeatureSum().setToZero();
    statistics_->initComputation();
    tmp_.resize(streamSizes[0], batchSize);
    tmp_.initComputation();
    tmp_.setToZero();
}

template<typename T>
void MeanAndVarianceTrainer<T>::finalize() {
    statistics_->finishComputation();
    if (statisticsFile_ != ""){
	statistics_->write(statisticsFile_);
    }
}

template<typename T>
void MeanAndVarianceTrainer<T>::writeMeanAndStandardDeviation(Statistics<T> &statistics) {
    statistics.finalize(true);
    statistics.finishComputation();
    u32 dim = statistics.featureSum().size();
    mean_.resize(dim);
    standardDeviation_.resize(dim);
    for (u32 i = 0; i < dim; i++){
	mean_.at(i) = statistics.featureSum().at(i);
	standardDeviation_.at(i) = std::sqrt(statistics.squaredFeatureSum().at(i));
    }
    this->log("estimating mean and variance from ") << statistics.nObservations() << " observations";
    this->log("write mean vector to file: ") << meanFile_;
    saveVector(meanFile_, mean_);
    this->log("write standard deviation vector to file: ") << standardDeviationFile_;
    saveVector(standardDeviationFile_, standardDeviation_);
}

template<typename T>
void MeanAndVarianceTrainer<T>::processBatch(std::vector<NnMatrix>& features) {
    // in unsupervised case: no weighted accumulation, no alignment
    Math::CudaVector<u32> dummyAlignment;
    NnVector dummyWeights;
    Precursor::weightedAccumulation_ = false;
    processBatch(features, dummyAlignment, dummyWeights);
}

template<typename T>
void MeanAndVarianceTrainer<T>::processBatch(std::vector<NnMatrix>& features,
	Math::CudaVector<u32> &alignment, NnVector& weights) {
    if (Precursor::weightedAccumulation_)
	weights.initComputation();
    features[0].initComputation();
    if (features[0].nColumns() != tmp_.nColumns())
	tmp_.resize(tmp_.nRows(), features[0].nColumns());

    tmp_.copy(features[0]);

    // weight features
    if (Precursor::weightedAccumulation_)
	features[0].multiplyColumnsByScalars(weights);

    // accumulate sum
    statistics_->featureSum().addSummedColumns(features[0]);

    // accumulate square sum
    tmp_.elementwiseMultiplication(tmp_);
    if (Precursor::weightedAccumulation_)
	tmp_.multiplyColumnsByScalars(weights);
    statistics_->squaredFeatureSum().addSummedColumns(tmp_);

    // accumulate weight
    if (Precursor::weightedAccumulation_)
	statistics_->addToTotalWeight(weights.asum());
    else
	statistics_->addToTotalWeight(features[0].nColumns());
    statistics_->incObservations(features[0].nColumns());
}

//=============================================================================
// explicit template instantiation
namespace Nn {

template class NeuralNetworkTrainer<f32>;
template class NeuralNetworkTrainer<f64>;

template class FrameErrorEvaluator<f32>;
template class FrameErrorEvaluator<f64>;

template class MeanAndVarianceTrainer<f32>;
template class MeanAndVarianceTrainer<f64>;

}

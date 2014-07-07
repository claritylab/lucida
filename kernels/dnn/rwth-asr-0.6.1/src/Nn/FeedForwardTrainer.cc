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
#include "FeedForwardTrainer.hh"

#include <Math/Module.hh>

#include "NeuralNetworkTrainer.hh"
#include "Prior.hh"

using namespace Nn;

//=============================================================================

template<typename T>
const Core::ParameterString FeedForwardTrainer<T>::paramStatisticsFilename(
	"statistics-filename", "filename to write statistics to", "");

template<typename T>
const Core::ParameterBool FeedForwardTrainer<T>::paramDoublePrecisionAccumulator(
	"double-precision-accumulator", "use double precision for accumulated statistics", false);

template<typename T>
const Core::ParameterBool FeedForwardTrainer<T>::paramAccumulateNnOutput(
	"accumulate-nn-output", "accumulate output of neural network", false);

template<typename T>
FeedForwardTrainer<T>::FeedForwardTrainer(const Core::Configuration &c) :
Core::Component(c),
NeuralNetworkTrainer<T>(c),
statisticsFilename_(paramStatisticsFilename(c)),
useDoublePrecisionAccumulator_(paramDoublePrecisionAccumulator(c)),
accumulateNnOutput_(paramAccumulateNnOutput(c)),
statistics_(0),
doublePrecisionStatistics_(0),
lowestTrainableLayerIndex_(0),
timeSync_(0),
timeForwardPass_(0),
timeInitialErrorSignal_(0),
timeBackwardPass_(0),
timeGradient_(0),
timeBaseStatistics_(0),
timeRegularization_(0),
timeEstimation_(0),
timeSyncBatch_(0),
timeForwardPassBatch_(0),
timeInitialErrorSignalBatch_(0),
timeBackwardPassBatch_(0),
timeGradientBatch_(0),
timeBaseStatisticsBatch_(0),
timeRegularizationBatch_(0),
timeEstimationBatch_(0)
{
    if (statisticsFilename_ != "")
	this->log("writing statistics to ") << statisticsFilename_;
    if (useDoublePrecisionAccumulator_){
	if (!estimator().batchMode())
	    this->error("double precision accumulator only possible for batch optimization");
	else
	    this->log("using double precision accumulator");
    }
    if (accumulateNnOutput_)
	this->log("accumulating output of neural network");
}

template<typename T>
FeedForwardTrainer<T>::~FeedForwardTrainer()
{
    if (statistics_)
	delete statistics_;
    if (doublePrecisionStatistics_)
	delete doublePrecisionStatistics_;
}

template<typename T>
void FeedForwardTrainer<T>::initializeTrainer(u32 batchSize, std::vector<u32>& streamSizes) {
    if (Precursor::needInit_) {

	Precursor::initializeTrainer(batchSize, streamSizes);
	if (this->hasNetwork()) {
	    lowestTrainableLayerIndex_ = -1;
	}
	// get trainable layer with lowest index
	lowestTrainableLayerIndex_ = (s32)this->nLayers() - 1;
	for (s32 layer = (s32)this->nLayers() - 1; layer >= 0; layer--) {
	    if (network().getLayer(layer).isTrainable()) {
		lowestTrainableLayerIndex_ = layer;
	    }
	}
	// initialize error signal
	errorSignal_.resize(this->nLayers());
	for (u32 layer = 0; layer < this->nLayers(); layer++) {
	    errorSignal_[layer].resize(network().getLayer(layer).getOutputDimension(), batchSize);
	}
	errorSignalOut_.resize(this->nLayers());
	for (s32 layer = (s32)this->nLayers() - 1; layer > lowestTrainableLayerIndex_; layer--) {
	    std::vector<NnMatrix*> errorSignalOut;
	    for (u32 i = 0; i < network().getLayer(layer).nPredecessors(); i++) {
		errorSignalOut.push_back(&(errorSignal_[network().getLayer(layer).getPredecessor(i)]));
	    }
	    errorSignalOut_.at(layer) = errorSignalOut;
	}
	// initialize statistics
	u32 statisticsType = estimator().requiredStatistics();
	if (this->hasNetwork() && (statisticsChannel_.isOpen() || statisticsFilename_ != "")) // otherwise this information would be lost anyway
	    statisticsType |= Statistics<T>::BASE_STATISTICS;
	if (accumulateNnOutput_)
	    statisticsType |= Statistics<T>::NN_OUTPUT;

	statistics_ = new Statistics<T>(this->nLayers(), statisticsType);
	if (this->hasNetwork())
	    statistics_->initialize(network());
	if (useDoublePrecisionAccumulator_)
	    initializeDoublePrecisionStatistics();
	// initialize computation
	statistics_->initComputation();
	statistics_->reset();
	for (u32 layer = 0; layer < errorSignal_.size(); layer++)
	    errorSignal_[layer].initComputation();
	Precursor::needInit_ = false;
    }
}

template<typename T>
void FeedForwardTrainer<T>::finalize() {
    if (estimator().batchMode() && statistics_) {
	Prior<T> prior(this->config);
	std::string priorFilename = prior.fileName();
	if (priorFilename != "" && statistics().hasClassCounts()){
	    require(this->classWeights_);
	    prior.setFromClassCounts(statistics(), *this->classWeights_);
	    prior.write();
	}

	if (statistics().hasGradient() || statistics().hasBaseStatistics()){
	    if (statisticsFilename_ != ""){
		if (doublePrecisionStatistics_){
		    statistics_->reset();
		    // convert double precision to single precision statistics
		    statistics_->add(*doublePrecisionStatistics_);
		}
		// write statistics
		statistics().finishComputation();
		statistics().write(statisticsFilename_);
	    }
	    else if (statistics().hasGradient())
		this->warning("statistics-filename not set, do not write statistics");
	}

	if (statistics().hasNnOutput()){
	    statistics().finalize();
	    statistics().finishComputation();
	    Math::Vector<T> tmpVector;
	    statistics().nnOutput().convert(tmpVector);
	    this->log("average activations of output layer");
	    this->log() << tmpVector;
	}
	Core::Component::log("total-number-of-observations: ") << statistics_->nObservations();
	if (statistics().hasBaseStatistics()) {
	    Core::Component::log("total-frame-classification-error: ") << (T) statistics_->classificationError();
	    Core::Component::log("total-objective-function: ") << statistics_->objectiveFunction();
	}
    }
    Precursor::finalize();
    if (this->measureTime_){
	this->log("Time for forwarding: ") << timeForwardPass_;
	this->log("Time for initial error signal: ") << timeInitialErrorSignal_;
	this->log("Time for backward pass: ") << timeBackwardPass_;
	this->log("Time for gradient collection: ") << timeGradient_;
	this->log("Time for base statistics: ") << timeBaseStatistics_;
	this->log("Time for applying regularization: ") << timeRegularization_;
	this->log("Time for estimation step: ") << timeEstimation_;
    }
}

// main function to process a mini-batch
template<typename T>
void FeedForwardTrainer<T>::processBatch(std::vector<NnMatrix>& features,
	Math::CudaVector<u32> &alignment, NnVector& weights) {

    // for profiling
    resetBatchTimes();
    timeval start, end;

    // initialization
    if (!estimator().batchMode() || doublePrecisionStatistics_)
	statistics_->reset();

    u32 batchSize = features[0].nColumns();
    setBatchSize(batchSize);
    statistics_->incObservations(batchSize);
    if (weightedAccumulation_){
	TIMER_START(start);
	weights.initComputation();
	TIMER_GPU_STOP_SUM2(start, end, this->measureTime_, timeSyncBatch_, timeSync_)
	statistics_->addToTotalWeight(weights.asum());
    }
    else{
	statistics_->addToTotalWeight(batchSize);
    }


    // count classes
    if (statistics_->hasClassCounts())
	updateClassCounts(alignment, statistics());

    // forward network
    if (statistics_->hasBaseStatistics() || statistics_->hasGradient() || statistics_->hasNnOutput()){
	TIMER_START(start);
	alignment.initComputation();
	// sync required here only to include it in time measurement
	for (u32 i = 0; i < features.size(); ++i)
	    features.at(i).initComputation();
	TIMER_GPU_STOP_SUM2(start, end, this->measureTime_, timeSyncBatch_, timeSync_)

	TIMER_START(start);
	network().forward(features);
	TIMER_GPU_STOP_SUM2(start, end, this->measureTime_, timeForwardPassBatch_, timeForwardPass_)
    }

    // calculate number of classification errors and objective function
    if (statistics_->hasBaseStatistics()){
	TIMER_START(start);
	statistics_->incClassificationErrors(network().getTopLayerOutput().nClassificationErrors(alignment));
	computeObjectiveFunction(alignment, weights);
	TIMER_GPU_STOP_SUM2(start, end, this->measureTime_, timeBaseStatisticsBatch_, timeBaseStatistics_)

	// apply regularization only when not in batch mode
	if (!estimator().batchMode()){
	    TIMER_START(start);
	    statistics_->addToObjectiveFunction(regularizer().objectiveFunction(network(), batchSize));
	    TIMER_GPU_STOP_SUM2(start, end, this->measureTime_, timeRegularizationBatch_, timeRegularization_)
	}
    }

    // compute gradient
    if (statistics_->hasGradient()){

	// reset error signals
	for (u32 layer = 0; layer < errorSignal_.size(); layer++)
	    errorSignal_.at(layer).setToZero();

	// set error signal of top layer
	TIMER_START(start);
	computeInitialErrorSignal(alignment, weights);
	TIMER_GPU_STOP_SUM2(start, end, this->measureTime_, timeInitialErrorSignalBatch_, timeInitialErrorSignal_)
	// error backprop
	errorBackpropagation();

	// collect gradient
	collectGradient();

	// apply regularization only when not in batch mode
	if (!estimator().batchMode()){
	    TIMER_START(start);
	    regularizer().addGradient(network(), statistics());
	    TIMER_GPU_STOP_SUM2(start, end, this->measureTime_, timeRegularizationBatch_, timeRegularization_)
	}
    }

    // accumulate NN output
    if (statistics_->hasNnOutput()){
	if (weightedAccumulation_){
		NnMatrix weightedOutput;
		weightedOutput.copyStructure(network().getTopLayerOutput());
		weightedOutput.initComputation(false);
		weightedOutput.copy(network().getTopLayerOutput());
		weightedOutput.multiplyColumnsByScalars(weights);
		statistics().nnOutput().addSummedColumns(weightedOutput);
	}
	else
	    statistics().nnOutput().addSummedColumns(network().getTopLayerOutput());
    }

    // update (only if has gradient)
    if (statistics_->hasGradient() && !estimator().batchMode()){
	// normalize statistics by batch size
	statistics_->finalize();
	// update model
	if (!estimator().batchMode()) {
	    TIMER_START(start);
	    estimator().estimate(network(), statistics());
	    TIMER_GPU_STOP_SUM2(start, end, this->measureTime_, timeEstimationBatch_, timeEstimation_);
	}
    }

    if (doublePrecisionStatistics_ && estimator().batchMode())
	doublePrecisionStatistics_->add(*statistics_);

    // logging
    if (statisticsChannel_.isOpen() && statistics_->hasBaseStatistics() && !(estimator().batchMode())){
	statisticsChannel_ << Core::XmlOpen("batch-statistics")
	<< Core::XmlFull("frame-classification-error-rate-on-batch", statistics_->classificationError())
	<< Core::XmlFull("objective-function-on-batch", statistics_->objectiveFunction());
	statisticsChannel_ << Core::XmlClose("batch-statistics");
    }
}

template<typename T>
void FeedForwardTrainer<T>::errorBackpropagation(){
    timeval start, end;
    TIMER_START(start);

    // error backpropagation
    for (s32 layer = (s32)network().nLayers() - 1; layer > lowestTrainableLayerIndex_; layer--) {
	network().getLayer(layer).backpropagateWeights(
		errorSignal_.at(layer), errorSignalOut_.at(layer));
	network().getLayer(layer - 1).backpropagateActivations(
		errorSignal_.at(layer - 1),
		errorSignal_.at(layer - 1),
		network().getLayerOutput(layer - 1));
    }

    TIMER_GPU_STOP_SUM2(start, end, this->measureTime_, timeBackwardPassBatch_, timeBackwardPass_);
}

template<typename T>
void FeedForwardTrainer<T>::collectGradient(){
    timeval start, end;
    TIMER_START(start);

    // gradient computation
    for (s32 layer = (s32)network().nLayers() - 1; layer >= lowestTrainableLayerIndex_; layer--) {
	/* update the gradient, if layer is trainable */
	if (network().getLayer(layer).isTrainable()) {
	    for (u32 stream = 0; stream < statistics_->gradientWeights(layer).size(); stream++) {
		NnMatrix &layerInputStream = *(network().getLayerInput(layer)[stream]);
		NnMatrix &gradientWeights  = statistics_->gradientWeights(layer)[stream];
		NnVector &gradientBias     = statistics_->gradientBias(layer);

		// let every layer update the gradients
		network().getLayer(layer).addToWeightsGradient(layerInputStream,
			errorSignal_.at(layer), stream, gradientWeights);
		network().getLayer(layer).addToBiasGradient(layerInputStream,
			errorSignal_.at(layer), stream, gradientBias);
	    }
	}
    }
    TIMER_GPU_STOP_SUM2(start, end, this->measureTime_, timeGradientBatch_, timeGradient_);
}

template<typename T>
void FeedForwardTrainer<T>::updateClassCounts(const Math::CudaVector<u32> &alignment, Statistics<T> &statistics){
    for (u32 i = 0; i < alignment.size(); i++) {
	statistics.incClassCount(alignment.at(i));
    }
}

template<typename T>
void FeedForwardTrainer<T>::setBatchSize(u32 batchSize){
    if (batchSize != this->batchSize()){
	Precursor::setBatchSize(batchSize);
	for (u32 i = 0; i < errorSignal_.size(); i++){
	    u32 nRows = errorSignal_.at(i).nRows();
	    errorSignal_.at(i).resize(nRows, batchSize);
	}
    }
}

template<typename T>
void FeedForwardTrainer<T>::resetBatchTimes(){
    timeSyncBatch_ = 0.0;
    timeForwardPassBatch_= 0.0;
    timeInitialErrorSignalBatch_= 0.0;
    timeBackwardPassBatch_= 0.0;
    timeGradientBatch_= 0.0;
    timeBaseStatisticsBatch_= 0.0;
    timeRegularizationBatch_= 0.0;
    timeEstimationBatch_ = 0.0;
}

template<typename T>
void FeedForwardTrainer<T>::logBatchTimes() const {
    this->log() << Core::XmlOpen("mini-batch-computation-times")
    << Core::XmlFull("sync", timeSyncBatch_)
    << Core::XmlFull("forward-pass", timeForwardPassBatch_)
    << Core::XmlFull("initial-error-signal", timeInitialErrorSignalBatch_)
    << Core::XmlFull("backward-pass", timeBackwardPassBatch_)
    << Core::XmlFull("gradient", timeGradientBatch_)
    << Core::XmlFull("base-statistics", timeBaseStatisticsBatch_)
    << Core::XmlFull("regularization", timeRegularizationBatch_)
    << Core::XmlFull("estimation", timeEstimationBatch_)
    << Core::XmlClose("mini-batch-computation-times");
}


//=============================================================================

template<typename T>
FeedForwardCrossEntropyTrainer<T>::FeedForwardCrossEntropyTrainer(const Core::Configuration &config) :
Core::Component(config),
FeedForwardTrainer<T>(config)
{}

template<typename T>
void FeedForwardCrossEntropyTrainer<T>::computeInitialErrorSignal(Math::CudaVector<u32> &alignment, NnVector& weights) {
    NnMatrix &errorSignal = this->errorSignal_.at(errorSignal_.size()-1);

    switch (network().getTopLayer().getLayerType()) {
    case NeuralNetworkLayer<T>::linearAndSoftmaxLayer:
    case NeuralNetworkLayer<T>::softmaxLayer:
    // softmax - kronecker delta (minimization problem)
    errorSignal.setToZero();
    errorSignal.add(network().getTopLayerOutput());
    errorSignal.addKroneckerDelta(alignment, -1.0);
    break;
    default:
	Core::Component::criticalError(
		"This layer-type is not yet implemented in training. \
		Allowed types: softmax, linear+softmax.");
	break;
    }
    // weight initial error signal, results in a weighting of the gradient
    if (Precursor::weightedAccumulation_)
	errorSignal.multiplyColumnsByScalars(weights);

}

template<typename T>
void FeedForwardCrossEntropyTrainer<T>::computeObjectiveFunction(Math::CudaVector<u32> &alignment, NnVector& weights) {
    if (Precursor::weightedAccumulation_) {
	statistics_->addToObjectiveFunction(
		network().getTopLayerOutput().weightedCrossEntropyObjectiveFunction(alignment, weights));
    } else {
	statistics_->addToObjectiveFunction(
		network().getTopLayerOutput().crossEntropyObjectiveFunction(alignment));
    }
}

//=============================================================================

template<typename T>
FeedForwardSquaredErrorTrainer<T>::FeedForwardSquaredErrorTrainer(const Core::Configuration &config) :
Core::Component(config),
FeedForwardTrainer<T>(config)
{}

template<typename T>
void FeedForwardSquaredErrorTrainer<T>::computeInitialErrorSignal(Math::CudaVector<u32> &alignment, NnVector& weights) {
    NnVector tmp;
    NnMatrix &errorSignal = errorSignal_[errorSignal_.size()-1];
    NnMatrix &netOutput   = network().getTopLayerOutput();

    switch (network().getTopLayer().getLayerType()) {
    case NeuralNetworkLayer<T>::linearLayer:
    errorSignal.setToZero();
    errorSignal.add(netOutput);
    errorSignal.addKroneckerDelta(alignment, -1.0);
    break;

    case NeuralNetworkLayer<T>::linearAndSoftmaxLayer:
    case NeuralNetworkLayer<T>::softmaxLayer:

    // (a) (softmax - kronecker-delta) .* softmax
    errorSignal.setToZero();
    errorSignal.add(network().getTopLayerOutput());
    errorSignal.addKroneckerDelta(alignment, -1.0);
    errorSignal.elementwiseMultiplication(network().getTopLayerOutput());
    // (b) store column sums in tmp vector
    tmp.initComputation();
    tmp.resize(errorSignal_[errorSignal_.size()-1].nColumns(), 0, true);
    tmp.setToZero();
    tmp.addSummedRows(errorSignal_[errorSignal_.size()-1]);
    // (c) redefine error signal: softmax - kronecker-delta
    errorSignal.setToZero();
    errorSignal.add(network().getTopLayerOutput());
    errorSignal.addKroneckerDelta(alignment, -1.0);
    // (d) subtract column sums and multiply with softmax
    errorSignal.addToAllRows(tmp, -1.0);
    errorSignal.elementwiseMultiplication(network().getTopLayerOutput());
    break;

    default:
	Core::Component::criticalError(
		"This layer-type is not yet implemented in training. \
		Allowed types: linear, softmax, linear+softmax.");
	break;
    }
    // weight initial error signal, results in a weighting of the gradient
    if (Precursor::weightedAccumulation_)
	errorSignal.multiplyColumnsByScalars(weights);
}

template<typename T>
void FeedForwardSquaredErrorTrainer<T>::computeObjectiveFunction(Math::CudaVector<u32> &alignment, NnVector& weights) {
    if (Precursor::weightedAccumulation_) {
	statistics_->addToObjectiveFunction(
		network().getTopLayerOutput().weightedSquaredErrorObjectiveFunction(alignment, weights));
    } else {
	statistics_->addToObjectiveFunction(
		network().getTopLayerOutput().squaredErrorObjectiveFunction(alignment));
    }
}

//=============================================================================

template<typename T>
FeedForwardBinaryDivergenceTrainer<T>::FeedForwardBinaryDivergenceTrainer(const Core::Configuration &config) :
Core::Component(config),
FeedForwardTrainer<T>(config)
{}

template<typename T>
void FeedForwardBinaryDivergenceTrainer<T>::computeInitialErrorSignal(Math::CudaVector<u32> &alignment, NnVector& weights) {
    NnMatrix &errorSignal = errorSignal_[errorSignal_.size()-1];
    NnMatrix &netOutput   = network().getTopLayerOutput();

    switch (network().getTopLayer().getLayerType()) {
    case NeuralNetworkLayer<T>::linearAndSigmoidLayer:
    case NeuralNetworkLayer<T>::sigmoidLayer:
    errorSignal.setToZero();
    errorSignal.add(netOutput);
    errorSignal.addKroneckerDelta(alignment, -1.0);
    break;

    case NeuralNetworkLayer<T>::linearAndSoftmaxLayer:
    case NeuralNetworkLayer<T>::softmaxLayer:
    errorSignal.binaryDivergenceSoftmaxGradient(netOutput, alignment);
    break;

    default:
	Core::Component::criticalError(
		"This layer-type is not yet implemented in training. \
		Allowed types: sigmoid, linear+sigmoid.");
	break;
    }
    // weight initial error signal, results in a weighting of the gradient
    if (Precursor::weightedAccumulation_)
	errorSignal.multiplyColumnsByScalars(weights);
}

template<typename T>
void FeedForwardBinaryDivergenceTrainer<T>::computeObjectiveFunction(Math::CudaVector<u32> &alignment, NnVector& weights) {
    if (Precursor::weightedAccumulation_) {
	statistics_->addToObjectiveFunction(
		network().getTopLayerOutput().weightedBinaryDivergenceObjectiveFunction(alignment, weights));
    } else {
	statistics_->addToObjectiveFunction(
		network().getTopLayerOutput().binaryDivergenceObjectiveFunction(alignment));
    }
}

//=============================================================================

template<typename T>
FeedForwardDefaultTrainer<T>::FeedForwardDefaultTrainer(const Core::Configuration &config) :
Core::Component(config),
FeedForwardTrainer<T>(config)
{}


template<typename T>
void FeedForwardDefaultTrainer<T>::initializeTrainer(u32 batchSize, std::vector<u32>& streamSizes){
    Precursor::initializeTrainer(batchSize, streamSizes);
    if (statistics().hasBaseStatistics())
	this->criticalError("Feed-Forward trainer needs a training criterion for accumulating the objective function");
    if (statistics().hasGradient())
	this->criticalError("Feed-Forward trainer needs a training criterion for accumulating the gradient");
}

//=============================================================================

// explicit template instantiation
namespace Nn {

template class FeedForwardTrainer<f32>;
template class FeedForwardTrainer<f64>;

template class FeedForwardCrossEntropyTrainer<f32>;
template class FeedForwardCrossEntropyTrainer<f64>;

template class FeedForwardSquaredErrorTrainer<f32>;
template class FeedForwardSquaredErrorTrainer<f64>;

template class FeedForwardBinaryDivergenceTrainer<f32>;
template class FeedForwardBinaryDivergenceTrainer<f64>;

template class FeedForwardDefaultTrainer<f32>;
template class FeedForwardDefaultTrainer<f64>;


}

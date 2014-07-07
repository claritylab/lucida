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
#include "BufferedAlignedFeatureProcessor.hh"

#include <limits>

#include <Modules.hh>
#include <Math/CudaVector.hh>
#include <Speech/ModelCombination.hh>

#include "FeedForwardTrainer.hh"
#include "NeuralNetworkTrainer.hh"


using namespace Nn;

//=============================================================================

template<typename T>
const Core::ParameterFloat BufferedAlignedFeatureProcessor<T>::paramSilenceWeight(
	"silence-weight", "weight for silence state", 1.0);

template<typename T>
const Core::ParameterBool BufferedAlignedFeatureProcessor<T>::paramWeightedAlignment(
	"weighted-alignment", "use weights from alignment", false);

template<typename T>
BufferedAlignedFeatureProcessor<T>::BufferedAlignedFeatureProcessor(const Core::Configuration &config, bool loadFromFile) :
    Core::Component(config),
    BufferedFeatureExtractor<T>(config, loadFromFile),
    Speech::AlignedFeatureProcessor(config),
    silence_(0),
    acousticModelNeedInit_(true),
    classLabelWrapper_(0),
    alignmentBuffer_(0),
    alignmentWeightsBuffer_(0),
    weightedAlignment_(paramWeightedAlignment(config))
{ }

template<typename T>
void BufferedAlignedFeatureProcessor<T>::initAcousticModel(){
    /* acoustic model to identify labels */
    Speech::ModelCombination modelCombination(
	    select("model-combination"),
	    Speech::ModelCombination::useAcousticModel,
	    Am::AcousticModel::noEmissions | Am::AcousticModel::noStateTransition);
    modelCombination.load();
    acousticModel_ = modelCombination.acousticModel();
    /* set silence */
    Am::Allophone silenceAllophone(acousticModel_->silence(), Am::Allophone::isInitialPhone | Am::Allophone::isFinalPhone);
    silence_ = classIndex(acousticModel_->allophoneStateAlphabet()->index(&silenceAllophone, 0));
    this->log("silence index is ") << silence_;
    this->log("silence accumulation weight is ") << paramSilenceWeight(config);
    this->log("use alignment weights: ") << weightedAlignment_;

    u32 nClasses = acousticModel_->nEmissions();
    this->log("number of classes of acoustic model: ") << nClasses;

    if (classLabelWrapper_)
	delete classLabelWrapper_;
    classLabelWrapper_ = new ClassLabelWrapper(select("class-labels"), nClasses);
    require(classLabelWrapper_->nClassesToAccumulate() > 0);

    /* initialize class weights */
    // so far: only silence weight supported
    classWeights_.resize(classLabelWrapper_->nClassesToAccumulate(), 1.0);
    if (classLabelWrapper_->isClassToAccumulate(silence_)) {
	classWeights_.at(classLabelWrapper_->getOutputIndexFromClassIndex(silence_)) = paramSilenceWeight(config);
    }
    acousticModelNeedInit_ = false;
}

template<typename T>
BufferedAlignedFeatureProcessor<T>::~BufferedAlignedFeatureProcessor() {
    delete classLabelWrapper_;
}

template<typename T>
void BufferedAlignedFeatureProcessor<T>::initBuffer(Core::Ref<const Speech::Feature> f) {
    alignmentBuffer_.resize(PrecursorBuffer::maxBufferSize_);
    if (weightedAlignment_)
	alignmentWeightsBuffer_.resize(PrecursorBuffer::maxBufferSize_);
    PrecursorBuffer::initBuffer(f);
}

template<typename T>
void BufferedAlignedFeatureProcessor<T>::resetBuffer() {
    std::fill(alignmentBuffer_.begin(), alignmentBuffer_.end(), 0);
    if (weightedAlignment_)
	std::fill(alignmentWeightsBuffer_.begin(), alignmentWeightsBuffer_.end(), 0.0);
    PrecursorBuffer::resetBuffer();
}

template<typename T>
void BufferedAlignedFeatureProcessor<T>::processAlignedFeature(Core::Ref<const Speech::Feature> f, Am::AllophoneStateIndex e) {
    processAlignedFeature(f, e, 1.0);
}

template<typename T>
void BufferedAlignedFeatureProcessor<T>::processAlignedFeature(Core::Ref<const Speech::Feature> f, Am::AllophoneStateIndex e, Mm::Weight w) {
    if (acousticModelNeedInit_)
	initAcousticModel();
    if (PrecursorBuffer::needInit_)
	initBuffer(f);
    u32 labelIndex = classIndex(e);
    if (classLabelWrapper_->isClassToAccumulate(labelIndex)) {
	// check consistency
	verify_eq(alignmentBuffer_.size(), BufferedFeatureExtractor<T>::featureBuffer_.at(0).nColumns());
	alignmentBuffer_.at(PrecursorBuffer::nBufferedFeatures_) = classLabelWrapper_->getOutputIndexFromClassIndex(labelIndex);
	if (weightedAlignment_)
	    alignmentWeightsBuffer_.at(PrecursorBuffer::nBufferedFeatures_) = w;
	// collect the feature -> buffer (use BufferedFeatureExtractor)
	BufferedFeatureExtractor<T>::processFeature(f);
    }
}

template<typename T>
void BufferedAlignedFeatureProcessor<T>::generateMiniBatch(std::vector<NnMatrix>& miniBatch, Math::CudaVector<u32>& miniBatchAlignment, std::vector<f64>& miniBatchAlignmentWeights, u32 batchSize) {
    // resize mini batch alignment
    miniBatchAlignment.resize(batchSize, 0, true);
    // fill mini batch alignment
    miniBatchAlignment.finishComputation(false);

    if (weightedAlignment_)
	miniBatchAlignmentWeights.resize(batchSize, 0);

    for (u32 i = 0; i < batchSize; i++) {
	u32 alignmentIndex = PrecursorBuffer::nProcessedFeatures_ + i;
	if (PrecursorBuffer::shuffle_) {
	    alignmentIndex = PrecursorBuffer::shuffledIndices_.at(alignmentIndex);
	}
	miniBatchAlignment.at(i) = alignmentBuffer_.at(alignmentIndex);
	if (weightedAlignment_)
	    miniBatchAlignmentWeights.at(i) = alignmentWeightsBuffer_.at(alignmentIndex);
    }
    PrecursorBuffer::generateMiniBatch(miniBatch, batchSize);
}

template<typename T>
void BufferedAlignedFeatureProcessor<T>::initTrainer(const std::vector<NnMatrix> &miniBatch){
    std::vector<u32> streamSizes;
    for (u32 stream = 0; stream < miniBatch.size(); stream++) {
	streamSizes.push_back(miniBatch.at(stream).nRows());
    }
    PrecursorBuffer::trainer_->initializeTrainer(PrecursorBuffer::batchSize_, streamSizes);
    PrecursorBuffer::trainer_->setClassWeights(&classWeights_);
    if (PrecursorBuffer::trainer_->hasNetwork()){
	if (PrecursorBuffer::trainer_->network().getTopLayer().getOutputDimension() !=
		classLabelWrapper_->nClassesToAccumulate()){
	    this->error("mismatch in size of output layer and number of classes to accumulate: ")
		    << PrecursorBuffer::trainer_->network().getTopLayer().getOutputDimension()
		    << " vs. "
		    << classLabelWrapper_->nClassesToAccumulate();
	}
    }
}

template<typename T>
void BufferedAlignedFeatureProcessor<T>::processBuffer() {

    PrecursorBuffer::prepareProcessBuffer();
    timeval startBatch, endBatch, end;
    bool measureTime = PrecursorBuffer::trainer_->measuresTime();
    std::vector<NnMatrix> miniBatch;
    Math::CudaVector<u32> miniBatchAlignment;
    std::vector<f64> miniBatchAlignmentWeights;
    NnVector weights;
    while (PrecursorBuffer::nProcessedFeatures_ + PrecursorBuffer::batchSize_ <= PrecursorBuffer::nBufferedFeatures_) {
	log("Process mini-batch ") << PrecursorBuffer::nProcessedMiniBatches_ + 1 << " with " << PrecursorBuffer::batchSize_
		<< " features";
	f64 timeMinibatch = 0, timeGenerateMiniBatch = 0;
	TIMER_START(startBatch);
	generateMiniBatch(miniBatch, miniBatchAlignment, miniBatchAlignmentWeights, PrecursorBuffer::batchSize_);
	// determine weights for the mini batch features
	weights.resize(miniBatchAlignment.size(), 0, true);
	weights.finishComputation(false);
	// weight vectors according to class membership
	for (u32 index = 0; index < weights.size(); index++) {
	    weights.at(index) = classWeights_.at(miniBatchAlignment.at(index));
	}
	// additionally weight vectors according to alignment weights
	if (weightedAlignment_) {
	    for (u32 index = 0; index < weights.size(); index++) {
		weights.at(index) *= miniBatchAlignmentWeights.at(index);
	    }
	}
	TIMER_GPU_STOP(startBatch, end, measureTime, timeGenerateMiniBatch)
	// initialize trainer (trainer checks if initialization is needed)
	if (!PrecursorBuffer::trainer_->isInitialized())
	    initTrainer(miniBatch);

	// process mini batch
	PrecursorBuffer::trainer_->processBatch(miniBatch, miniBatchAlignment, weights);
	PrecursorBuffer::nProcessedMiniBatches_++;
	PrecursorBuffer::nProcessedFeatures_ += PrecursorBuffer::batchSize_;
	TIMER_GPU_STOP(startBatch, endBatch, measureTime, timeMinibatch)
	if (measureTime){
	    log("time for generating mini-batch: ") << timeGenerateMiniBatch;
	    log("overall processing time for mini-batch: ") << timeMinibatch;

	    PrecursorBuffer::trainer_->logBatchTimes();
	}
    }
    // process the remaining feature with a smaller mini batch
    // only done for algorithms where the mini batch size is not critical
    u32 nRemainingFeatures = this->nBufferedFeatures_ - this->nProcessedFeatures_;
    if (this->processRemainingFeatures_ && nRemainingFeatures > 0){
	log("Process mini-batch ") << this->nProcessedMiniBatches_ + 1 << " with " << nRemainingFeatures << " features.";
	generateMiniBatch(miniBatch, miniBatchAlignment, miniBatchAlignmentWeights, nRemainingFeatures);
	// determine weights for the mini batch features
	weights.resize(miniBatchAlignment.size(), 0, true);
	weights.finishComputation(false);
	// weight vectors according to class membership
	for (u32 index = 0; index < weights.size(); index++) {
	    weights.at(index) = classWeights_.at(miniBatchAlignment.at(index));
	}
	// additionally weight vectors according to alignment weights
	if (weightedAlignment_) {
	    for (u32 index = 0; index < weights.size(); index++) {
		weights.at(index) *= miniBatchAlignmentWeights.at(index);
	    }
	}

	this->trainer_->setBatchSize(nRemainingFeatures);
	// initialize trainer
	if (!this->trainer_->isInitialized())
	    initTrainer(miniBatch);
	// process mini batch
	this->trainer_->processBatch(miniBatch, miniBatchAlignment, weights);
	this->nProcessedMiniBatches_++;
	this->nProcessedFeatures_ += nRemainingFeatures;
	// reset to old batch size
	this->trainer_->setBatchSize(this->batchSize_);
    }
    PrecursorBuffer::finalizeProcessBuffer();
}

template<typename T>
Mm::EmissionIndex BufferedAlignedFeatureProcessor<T>::classIndex(Am::AllophoneStateIndex e) const {
    if (acousticModel_)
	return acousticModel_->emissionIndex(e);
    else {
	warning("no acoustic model available, using allophone state index as class index!");
	return e;
    }

}


// from BufferedFeatureExtractor
template<typename T>
void BufferedAlignedFeatureProcessor<T>::leaveSegment(Bliss::Segment* segment) {
    PrecursorBuffer::processSegment();
    PrecursorAligned::leaveSegment(segment);
}

template<typename T>
void BufferedAlignedFeatureProcessor<T>::leaveCorpus(Bliss::Corpus* corpus) {
    if (corpus->level() == 0) {
	PrecursorBuffer::processCorpus();
	Core::Component::log("Total number of processed mini-batches: ") << PrecursorBuffer::totalNumberOfProcessedMiniBatches_;
	PrecursorBuffer::trainer_->finalize();
	PrecursorAligned::leaveCorpus(corpus);
    }
}

//=============================================================================

// create the specific type of NeuralNetworkTrainer
template<typename T>
NeuralNetworkTrainer<T>* BufferedAlignedFeatureProcessor<T>::createTrainer(const Core::Configuration& config) {
    return NeuralNetworkTrainer<T>::createSupervisedTrainer(config);
}

//=============================================================================
// explicit template instantiation
namespace Nn {
template class BufferedAlignedFeatureProcessor<f32>;
template class BufferedAlignedFeatureProcessor<f64>;
}

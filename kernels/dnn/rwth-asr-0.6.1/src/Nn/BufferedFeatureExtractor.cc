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
#include <BufferedFeatureExtractor.hh>
#include <Speech/DataSource.hh>

using namespace Nn;

//=============================================================================
template<typename T>
const Core::Choice BufferedFeatureExtractor<T>::choiceBufferType(
	"mini-batch", BufferedFeatureExtractor::minibatch,
	"batch", BufferedFeatureExtractor::batch,
	"utterance", BufferedFeatureExtractor::utterance,
	Core::Choice::endMark());

template<typename T>
const Core::ParameterChoice BufferedFeatureExtractor<T>::paramBufferType(
	"buffer-type", &choiceBufferType, "Type of the feature buffer to be used.", BufferedFeatureExtractor::minibatch);

template<typename T>
const Core::ParameterInt BufferedFeatureExtractor<T>::paramBufferSize(
	"buffer-size", "Maximal Size of the feature buffer.", 16384, 1);

template<typename T>
const Core::ParameterBool BufferedFeatureExtractor<T>::paramShuffleBuffer(
	"shuffle", "Activate shuffling of the feature buffer.", true);

template<typename T>
const Core::ParameterInt BufferedFeatureExtractor<T>::paramBatchSize(
	"batch-size", "Size of a mini-batch.", 1024, 1);

template<typename T>
const Core::ParameterInt BufferedFeatureExtractor<T>::paramRegressionWindowSize(
	"regression-window-size", "window size to compute the derivative of a feature via regression", 5);

template<typename T>
const Core::ParameterInt BufferedFeatureExtractor<T>::paramSlidingWindowSize(
	"window-size", "Size of sliding window", 1);

template<typename T>
const Core::ParameterInt BufferedFeatureExtractor<T>::paramSlidingWindowSizeDerivatives(
	"window-size-derivatives", "Size of sliding window for derivatives (first + first component of second)", 0);


template<typename T>
BufferedFeatureExtractor<T>::BufferedFeatureExtractor(const Core::Configuration &config,bool loadFromFile) :
    Core::Component(config),
    Precursor(config,loadFromFile),
    bufferType_((BufferType) paramBufferType(config)),
    regressionWindowSize_(paramRegressionWindowSize(config)),
    slidingWindowSize_(paramSlidingWindowSize(config)),
    slidingWindowSizeDerivatives_(paramSlidingWindowSizeDerivatives(config)),
    shuffle_(paramShuffleBuffer(config)),
    featureBuffer_(0),
    derivativesBuffer_(0),
    maxBufferSize_(paramBufferSize(config)),
    nBufferedFeatures_(0),
    nProcessedFeatures_(0),
    segmentIndex_(0),
    batchSize_(paramBatchSize(config)),
    derivativesComputed_(false),
    maxBufferedFeatureStreams_(0),
    shuffledIndices_(0),
    processRemainingFeatures_(false),
    needInit_(true),
    nProcessedMiniBatches_(0),
    totalNumberOfProcessedMiniBatches_(0),
    trainer_(0)
{
    if (regressionWindowSize_ % 2 != 1){
	this->error("regression window size must be an odd number but is ") << regressionWindowSize_;
    }
    logProperties();
}

/**	Shuffle the indices of the buffer.
 *
 *	Shuffle the indices of the buffer of maximal @param nFrames.
 *	@param	nFrames	Maximal number of frames to be shuffled
 */
template<typename T>
void BufferedFeatureExtractor<T>::initShuffle() {
    // Resize the index buffer to full size
    if (shuffledIndices_.size() != nBufferedFeatures_)
	shuffledIndices_.resize(nBufferedFeatures_);

    verify(maxBufferSize_ >= nBufferedFeatures_);

    // Fill the index vector with the corresponding index number
    u32 curIndex = 0;
    for (std::vector<u32>::iterator shuffleIndexIterator = shuffledIndices_.begin();
	    shuffleIndexIterator != shuffledIndices_.end(); ++shuffleIndexIterator, ++curIndex) {
	(*shuffleIndexIterator) = curIndex;
    }

    // Shuffle now the indices from 1 to nBufferedFeatures_
    std::random_shuffle(shuffledIndices_.begin(),
	    shuffledIndices_.begin() + nBufferedFeatures_);

    // log some of the random indices (helpful for debugging)
    std::stringstream ss;
    u32 nIndicesForLogging = std::min((size_t) 5, shuffledIndices_.size() / 2);
    for (u32 i = 0; i < nIndicesForLogging; i++)
	ss << shuffledIndices_.at(i) << " ";
    ss << " ... ";
    for (u32 i = 0; i < nIndicesForLogging; i++)
	ss << shuffledIndices_.at(shuffledIndices_.size() - nIndicesForLogging + i) << " ";
    log("accessing data in random order. Shuffled indices: ") << ss.str();
}

/**	Initialize the (feature) buffer.
 *
 *  Initialize the (feature) buffer. The number of elements in the buffer is
 *  limited by @param nFrames * \@parame nFeatures
 *
 *  @param	nFeatures	Dimension of the features
 *  @param	nFrames		Number of frames
 */
template<typename T>
void BufferedFeatureExtractor<T>::initBuffer(std::vector<u32> &nFeatures) {
    segmentIndexBuffer_.resize(maxBufferSize_);
    featureBuffer_.resize(nFeatures.size());
    if (slidingWindowSizeDerivatives_ > 0) {
	derivativesBuffer_.resize(nFeatures.size());
    }
    for (u32 streamIndex = 0; streamIndex < nFeatures.size(); streamIndex++) {
	featureBuffer_[streamIndex].resize(nFeatures[streamIndex], maxBufferSize_);
	if (slidingWindowSizeDerivatives_ > 0) {
	    /* nFeatures[streamIndex] + 1: first derivative of each component (nFeatures[streamIndex])
	     *                             and second derivative of first component (+1)
	     */
	    derivativesBuffer_[streamIndex].resize(nFeatures[streamIndex] + 1, maxBufferSize_);
	}
    }

    nBufferedFeatures_ = 0;
    needInit_ = false;
}

template<typename T>
void BufferedFeatureExtractor<T>::initBuffer(Core::Ref<const Speech::Feature> f){
    size_t nFeatureStreams = f->nStreams();
    std::vector<u32> featureStreamSizes;
    featureStreamSizes.resize(nFeatureStreams);

    u32 curStreamIndex = 0;
    for (Mm::Feature::Iterator streamIterator = f->begin();streamIterator != f->end(); ++streamIterator, ++curStreamIndex) {
	featureStreamSizes[curStreamIndex] = (*streamIterator)->size();
    }

    // init the buffer
    initBuffer(featureStreamSizes);
}

/**	Reset the feature buffer.
 *
 *	Clean up and free the feature buffer. The current number of frames are set
 *	to 0. The buffer has to be initialized again when used.
 *  */
template<typename T>
void BufferedFeatureExtractor<T>::resetBuffer() {
    for (u32 streamIndex = 0; streamIndex < featureBuffer_.size(); streamIndex++) {
	featureBuffer_[streamIndex].setToZero();
    }
    for (u32 streamIndex = 0; streamIndex < derivativesBuffer_.size(); streamIndex++) {
	derivativesBuffer_[streamIndex].setToZero();
    }
    std::fill(segmentIndexBuffer_.begin(), segmentIndexBuffer_.end(), 0);
    nBufferedFeatures_ = 0;
    derivativesComputed_ = false;
    // used for indexing in buffer
    nProcessedFeatures_ = 0;
    nProcessedMiniBatches_ = 0;
}



/**	Update the feature buffer at index.
 *  */
template<typename T>
void BufferedFeatureExtractor<T>::updateBufferedFeature(Core::Ref<const Speech::Feature> f) {
    // Copy the features of each stream (not only the main stream) into the buffer
    u32 streamIndex = 0;
    for (Mm::Feature::Iterator streamIterator = f->begin(); streamIterator != f->end(); ++streamIterator, ++streamIndex) {
	for (u32 i = 0; i < (*streamIterator)->size(); ++i) {
	    featureBuffer_[streamIndex].at(i,nBufferedFeatures_) = (T) (*streamIterator)->at(i);
	}
    }
    // buffer segment index (for windowing)
    segmentIndexBuffer_.at(nBufferedFeatures_) = segmentIndex_;
    nBufferedFeatures_++;
}

template<typename T>
void BufferedFeatureExtractor<T>::processFeature(Core::Ref<const Speech::Feature> f) {
    if (needInit_)
	initBuffer(f);

    // check for buffer overflow
    // TODO handling of utterances is not correct yet !
    if (nBufferedFeatures_ >= maxBufferSize_) {
	// can only happen in utterance mode
	require(bufferType_ == utterance);
	log("Buffer too small for utterance, skip utterance.");
	resetBuffer();
	return;
    }
    // Fill/update the buffer
    updateBufferedFeature(f);

    // Process the buffer?
    if (nBufferedFeatures_ >= maxBufferSize_) {
	// process full buffer only in online mode, else overflow error or enlarge buffer
	switch (bufferType_) {
	case BufferedFeatureExtractor::minibatch:
	    log("Process buffer since it is full. Processing ") << (nBufferedFeatures_ / batchSize_) << " mini-batches.";
	    processBuffer();
	    break;
	case BufferedFeatureExtractor::batch:
	case BufferedFeatureExtractor::utterance:
	    break;
	}
    }
}

template<typename T>
void BufferedFeatureExtractor<T>::getRelativePositionsInSlidingWindow(u32 indexOfCentralFeature, u32 windowSize, std::vector<s32> &positions){
    s32 maxPastSize = windowSize / 2;
    s32 maxFutureSize = windowSize / 2;
    // determine segment boundaries
    s32 leftBoundary = 0;
    s32 rightBoundary = 0;
    u32 segmentIndex = segmentIndexBuffer_.at(indexOfCentralFeature);
    for (s32 i = 0 ; i < maxFutureSize; i++){
	if (indexOfCentralFeature + i + 1 >= nBufferedFeatures_ || segmentIndexBuffer_.at(indexOfCentralFeature + i + 1) != segmentIndex)
	    break;
	else{
	    rightBoundary++;
	}
    }

    for (s32 i = 0 ; i > -maxPastSize; i--){
	bool atBufferBoundary = (u32) (1 - i) > indexOfCentralFeature;
	if (atBufferBoundary || segmentIndexBuffer_.at(indexOfCentralFeature + i - 1) != segmentIndex)
	    break;
	else
	    leftBoundary--;
    }
    positions.resize(windowSize);
    for (s32 i = -maxPastSize; i <= maxFutureSize; i++){
	positions.at(i + maxPastSize) = std::min(std::max(i, leftBoundary), rightBoundary);
    }
}


template<typename T>
void BufferedFeatureExtractor<T>::setWindowedFeature(u32 streamIndex, u32 indexOfCentralFeatureInBuffer, u32 indexInMiniBatch,
	NnMatrix &miniBatch){
    u32 baseFeatureDim = featureBuffer_.at(streamIndex).nRows();
    std::vector<s32> positions;
    getRelativePositionsInSlidingWindow(indexOfCentralFeatureInBuffer, slidingWindowSize_, positions);
    for(u32 i = 0; i < slidingWindowSize_; i++) {
	u32 indexInBuffer = indexOfCentralFeatureInBuffer + positions.at(i);
	miniBatch.copyBlockFromMatrix(featureBuffer_.at(streamIndex), 0, indexInBuffer, i * baseFeatureDim, indexInMiniBatch, baseFeatureDim, 1);
    }
}

template<typename T>
void BufferedFeatureExtractor<T>::setWindowedFeatureDerivatives(u32 streamIndex, u32 indexOfCentralFeatureInBuffer,
	u32 indexInMiniBatch, NnMatrix &miniBatch) {
    u32 windowedFeatureDim = slidingWindowSize_ * featureBuffer_.at(streamIndex).nRows();
    u32 derivativesDim = derivativesBuffer_.at(streamIndex).nRows();
    std::vector<s32> positions;
    getRelativePositionsInSlidingWindow(indexOfCentralFeatureInBuffer, slidingWindowSizeDerivatives_, positions);
    for (u32 i = 0; i < slidingWindowSizeDerivatives_; i++) {
	u32 indexInBuffer = indexOfCentralFeatureInBuffer + positions.at(i);
	miniBatch.copyBlockFromMatrix(derivativesBuffer_.at(streamIndex), 0, indexInBuffer,
		windowedFeatureDim + i * derivativesDim, indexInMiniBatch, derivativesDim, 1);
    }
}

template<typename T>
void BufferedFeatureExtractor<T>::computeFirstDerivative(u32 streamIndex, u32 indexInBuffer) {

    std::vector<s32> positions;
    getRelativePositionsInSlidingWindow(indexInBuffer, regressionWindowSize_, positions);

    // iterate over all feature components
    for (u32 c = 0; c < featureBuffer_.at(streamIndex).nRows(); c++) {
	// compute derivative of component c
	T numerator = 0.0;
	T denominator = 0.0;
	for (u32 i = 0; i < regressionWindowSize_; i++) {
	    T pos = i - (regressionWindowSize_ - 1) / 2.0;
	    numerator += pos * featureBuffer_.at(streamIndex).at(c, indexInBuffer + positions.at(i));
	    denominator += pos * pos;
	}
	derivativesBuffer_[streamIndex].at(c, indexInBuffer) = numerator / denominator;
    }
}

// cf. src/Signal/Regression.cc, derivation of this part unknown, but coefficients are correct!
template<typename T>
void BufferedFeatureExtractor<T>::computeSecondDerivative(u32 streamIndex, u32 indexInBuffer) {

    std::vector<s32> positions;
    getRelativePositionsInSlidingWindow(indexInBuffer, regressionWindowSize_, positions);
    // index for single second derivative component in derivativesBuffer_
    u32 derivativeBufferIndex = derivativesBuffer_[streamIndex].nRows() - 1;

    T numerator = 0.0;
    T denominator = 0.0;
    T tm = 0.0;
    for (u32 i = 0; i < regressionWindowSize_; i++) {
	T pos = i - (regressionWindowSize_ - 1) / 2.0;
	tm += pos * pos;
	denominator += pos * pos * pos * pos;
    }
    denominator = tm * tm - regressionWindowSize_ * denominator;
    for (u32 i = 0; i < regressionWindowSize_; i++) {
	T pos = i - (regressionWindowSize_ - 1) / 2.0;
	numerator += tm * featureBuffer_[streamIndex].at(0, indexInBuffer + positions.at(i));
	numerator -= regressionWindowSize_ * pos * pos * featureBuffer_[streamIndex].at(0, indexInBuffer + positions.at(i));
    }
    derivativesBuffer_[streamIndex].at(derivativeBufferIndex, indexInBuffer) = 2.0 * numerator / denominator;
}

template<typename T>
void BufferedFeatureExtractor<T>::generateMiniBatch(std::vector<NnMatrix>& miniBatch, u32 batchSize) {
    // resize mini batch to number of input streams
    miniBatch.resize(featureBuffer_.size());
    // resize each stream to correct size
    for (u32 streamIndex = 0; streamIndex < miniBatch.size(); streamIndex++) {
	u32 featureDim = slidingWindowSize_ * featureBuffer_.at(streamIndex).nRows()
		+ slidingWindowSizeDerivatives_ * (featureBuffer_.at(streamIndex).nRows() + 1);
	miniBatch.at(streamIndex).resize(featureDim, batchSize);
    }
    // fill mini batch
    for (u32 streamIndex = 0; streamIndex < miniBatch.size(); streamIndex++) {
	miniBatch.at(streamIndex).finishComputation(false);
	for (u32 column = 0; column < batchSize; column++) {
	    u32 featureIndex = nProcessedFeatures_ + column;
	    if (shuffle_){
		verify_lt(featureIndex, shuffledIndices_.size());
		featureIndex = shuffledIndices_.at(featureIndex);
	    }

	    setWindowedFeature(streamIndex, featureIndex, column, miniBatch.at(streamIndex));
	    if (slidingWindowSizeDerivatives_ > 0)
		setWindowedFeatureDerivatives(streamIndex, featureIndex, column, miniBatch.at(streamIndex));
	}
    }
}


template<typename T>
void BufferedFeatureExtractor<T>::prepareProcessBuffer() {
    // create trainer, if not yet done
    if (!trainer_) {
	trainer_ = createTrainer(config);
    }

    processRemainingFeatures_ = trainer_->needsToProcessAllFeatures() || (trainer_->hasEstimator() && trainer_->estimator().batchMode());
    // shuffle, if desired
    if (shuffle_ && (trainer_->hasEstimator() && trainer_->estimator().batchMode())){
	this->log("do not shuffle buffer, because shuffling is irrelevant with batch estimator");
	shuffle_ = false;
    }
    if (shuffle_)
	initShuffle();

    // compute derivatives
    if ((slidingWindowSizeDerivatives_ > 0) && (!derivativesComputed_)) {
	for (u32 streamIndex = 0; streamIndex < featureBuffer_.size(); streamIndex++) {
	    // compute first derivative
	    for (u32 indexInBuffer = 0; indexInBuffer < nBufferedFeatures_; indexInBuffer++) {
		computeFirstDerivative(streamIndex, indexInBuffer);
	    }
	    // compute second derivative
	    for (u32 indexInBuffer = 0; indexInBuffer < nBufferedFeatures_; indexInBuffer++) {
		computeSecondDerivative(streamIndex, indexInBuffer);
	    }
	}
	derivativesComputed_ = true;

    }
}

template<typename T>
void BufferedFeatureExtractor<T>::processBuffer() {

    prepareProcessBuffer();

    while (nProcessedFeatures_ + batchSize_ <= nBufferedFeatures_) {
	std::vector<NnMatrix> miniBatch;
	generateMiniBatch(miniBatch, batchSize_);
	log("Process mini-batch ") << nProcessedMiniBatches_ + 1 << " with " << miniBatch.at(0).nColumns() << " features.";
	// initialize trainer
	if (!trainer_->isInitialized()) {
	    std::vector<u32> streamSizes;
	    for (u32 stream = 0; stream < miniBatch.size(); stream++)
		streamSizes.push_back(miniBatch.at(stream).nRows());
	    trainer_->initializeTrainer(batchSize_, streamSizes);
	}
	// process mini batch
	trainer_->processBatch(miniBatch);
	nProcessedMiniBatches_++;
	nProcessedFeatures_ += batchSize_;
    }
    // process the remaining feature with a smaller mini batch
    // only done for algorithms where the mini batch size is not critical
    u32 nRemainingFeatures = nBufferedFeatures_ - nProcessedFeatures_;
    if (processRemainingFeatures_ && nRemainingFeatures > 0){
	std::vector<NnMatrix> miniBatch;
	generateMiniBatch(miniBatch, nRemainingFeatures);
	log("Process mini-batch ") << nProcessedMiniBatches_ + 1 << " with " << miniBatch.at(0).nColumns() << " features.";
	trainer_->setBatchSize(nRemainingFeatures);
	// initialize trainer
	if (!trainer_->isInitialized()) {
	    std::vector<u32> streamSizes;
	    for (u32 stream = 0; stream < miniBatch.size(); stream++)
		streamSizes.push_back(miniBatch.at(stream).nRows());
	    trainer_->initializeTrainer(nRemainingFeatures, streamSizes);
	}
	// process mini batch
	trainer_->processBatch(miniBatch);
	nProcessedMiniBatches_++;
	nProcessedFeatures_ += nRemainingFeatures;
	// reset to old batch size
	trainer_->setBatchSize(batchSize_);
    }
    finalizeProcessBuffer();
}

template<typename T>
void BufferedFeatureExtractor<T>::finalizeProcessBuffer() {
    log("Processed ") << nProcessedFeatures_ << " features. " << (nBufferedFeatures_ - nProcessedFeatures_)
	    << " remain unprocessed.";
    totalNumberOfProcessedMiniBatches_ += nProcessedMiniBatches_;
    // reset the buffer
    resetBuffer();
}

template<typename T>
void BufferedFeatureExtractor<T>::processSegment() {
    // for sequence (full utterance) training
    if ((bufferType_ == BufferedFeatureExtractor::utterance) && (nBufferedFeatures_ > 0)) {
	// change batch size according to sequence length
	batchSize_ = nBufferedFeatures_;
	processBuffer();
	// ... after processing, reset the buffer
	resetBuffer();
    }
}

template<typename T>
void BufferedFeatureExtractor<T>::processCorpus() {
    // if buffer is not empty, process buffer
    if (nBufferedFeatures_ > 0) {
	log("Process buffer since all features are read.");
	log("Number of features in buffer: ") << nBufferedFeatures_;
	processBuffer();
    }
    resetBuffer();
}

template<typename T>
void BufferedFeatureExtractor<T>::leaveSegment(Bliss::Segment *segment) {
    segmentIndex_++;
    processSegment();
    Precursor::leaveSegment(segment);
}

template<typename T>
void BufferedFeatureExtractor<T>::leaveCorpus(Bliss::Corpus *corpus) {
    if (corpus->level() == 0) {
	processCorpus();
	Core::Component::log("Total number of processed mini-batches: ") << totalNumberOfProcessedMiniBatches_;
	trainer_->finalize();
	Precursor::leaveCorpus(corpus);
    }
}

template<typename T>
void BufferedFeatureExtractor<T>::logProperties() const {
    if (bufferType_ == minibatch){
	this->log("using mini-batch feature buffer");
    }
    else if (bufferType_ == utterance){
	this->log("using utterance feature buffer");
    }
    else if (bufferType_ == batch){
	this->log("using batch feature buffer");
    }
    this->log("maximal buffer size is ") << paramBufferSize(config);
    this->log("batch size is ") << batchSize_;
    this->log("regression window size for computation of derivative features is ") << regressionWindowSize_;
    this->log("sliding window size for computation of windowed features is ") << slidingWindowSize_;
    this->log("sliding window size for computation of windowed derivative features is ") << slidingWindowSizeDerivatives_;
    if (shuffle_)
	this->log("shuffling buffer");
    else
	this->log("do not shuffle buffer");
}

template<typename T>
NeuralNetworkTrainer<T>* BufferedFeatureExtractor<T>::createTrainer(const Core::Configuration& config) {
    return NeuralNetworkTrainer<T>::createUnsupervisedTrainer(config);
}

//=============================================================================
// explicit template instantiation
namespace Nn {
template class BufferedFeatureExtractor<f32>;
template class BufferedFeatureExtractor<f64>;
}

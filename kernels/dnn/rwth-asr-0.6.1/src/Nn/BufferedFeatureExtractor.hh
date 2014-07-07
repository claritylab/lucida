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
#ifndef _NN_BUFFERED_FEATURE_EXTRACTOR_HH
#define _NN_BUFFERED_FEATURE_EXTRACTOR_HH

#include <Core/Types.hh>				// baseline feature types
#include <Mm/Types.hh>					// advanced features types
#include <Speech/Feature.hh>			// speech feature types
#include <Speech/DataExtractor.hh>		// non supervised training (only features)
#include <Speech/CorpusVisitor.hh>

#include "NeuralNetworkLayer.hh"
#include "NeuralNetworkTrainer.hh"

#include <Math/FastMatrix.hh>

namespace Nn {

/**	Extension of the Speech::FeatureExtractor.
 *	Samples/features are collected in a buffer before they are processed.
 *	Shuffling the data is possible.
 *
 */
template<class T>
class BufferedFeatureExtractor : public Speech::FeatureExtractor {
    typedef Speech::FeatureExtractor Precursor;
protected:
    typedef typename Types<T>::NnVector NnVector;
    typedef typename Types<T>::NnMatrix NnMatrix;
public:
    enum BufferType {
	minibatch,		/** each mini-batch contains batch_size frames */
	batch,		/** buffer contains all frames in a single mini-batch */
	utterance,	/** buffer contains whole utterances/speech-segments in a single mini-batch */
    };
protected:
    static const Core::Choice choiceBufferType;
    static const Core::ParameterChoice paramBufferType;
    static const Core::ParameterInt paramBufferSize;
    static const Core::ParameterBool paramShuffleBuffer;
    static const Core::ParameterInt paramBatchSize;
    static const Core::ParameterInt paramRegressionWindowSize;
    static const Core::ParameterInt paramSlidingWindowSize;
    static const Core::ParameterInt paramSlidingWindowSizeDerivatives;

    const BufferType bufferType_;				/** Type of the buffer (single, batch, sequence)*/
    const u32 regressionWindowSize_;
    const u32 slidingWindowSize_;
    const u32 slidingWindowSizeDerivatives_;
    /** Flag if feature shuffling is active */
    bool shuffle_;				                 // not const, because configuration can be changed if obvious

    std::vector< Math::FastMatrix<T> > featureBuffer_;
    std::vector< Math::FastMatrix<T> > derivativesBuffer_;
    std::vector< u32 > segmentIndexBuffer_;

    u32 maxBufferSize_;						/** Maximal size for the feature buffer */
    u32 nBufferedFeatures_;					/** Number of frames in the feature buffer */
    u32 nProcessedFeatures_;					/** Number of processed frames since last call to resetBuffer */
    u32 segmentIndex_;
    u32 batchSize_;						/** Size of a batch in mini-batch mode */
    bool derivativesComputed_;
    u32 maxBufferedFeatureStreams_;				/** Number of maximal feature streams in the buffer */
    std::vector<u32> shuffledIndices_;				/** Shuffled indices */
    bool processRemainingFeatures_;

    bool needInit_;						/** Flag to check for buffer initialization */

protected:
    u32 nProcessedMiniBatches_;					/** number of processed mini-batches, reset at resetBuffer */
    u32 totalNumberOfProcessedMiniBatches_;			/** not reset until corpus completely processed */
    NeuralNetworkTrainer<T>* trainer_;
public:
    BufferedFeatureExtractor(const Core::Configuration &config, bool loadFromFile=true);
    virtual ~BufferedFeatureExtractor() {}

    virtual void processSegment();
    virtual void processCorpus();

    // buffer feature
    virtual void processFeature(Core::Ref<const Speech::Feature> f);

    // process segment if using utterance mode
    virtual void leaveSegment(Bliss::Segment *segment);
    // process buffer
    virtual void leaveCorpus(Bliss::Corpus *corpus);

    /** Returns the maximal size of the (feature) buffer in frames */
    inline u32 bufferSize() const { return maxBufferSize_; }
    /** Return the current number of buffered features */
    inline u32 numberOfBufferedFeatures() const {return nBufferedFeatures_; }
    /** Returns whether data shuffling is active or not */
    inline bool isShuffled() const {return shuffle_; }
    /** Return the index of the random feature in the buffer */
    inline u32 shuffledIndex(u32 index) const {return shuffledIndices_.at(index);}
    /** Return the current size of a mini-batch */
    inline u32 batchSize() const { return batchSize_; }

    // this needs to be virtual, since AlignedNeuralNetworkTrainer overrides the function to create a supervised trainer
    virtual NeuralNetworkTrainer<T>* createTrainer(const Core::Configuration& config);
    virtual void signOn(Speech::CorpusVisitor& corpusVisitor) { Speech::DataExtractor::signOn( corpusVisitor ); }

protected:
    // buffer methods
    virtual void initBuffer(std::vector<u32> &nFeatures);	/** Initialize the feature buffer */
    virtual void initBuffer(Core::Ref<const Speech::Feature> f);	/** Initialize the feature buffer */
    virtual void updateBufferedFeature(Core::Ref<const Speech::Feature> f); /** Update the feature buffer */
    virtual void resetBuffer();
    virtual void prepareProcessBuffer();
    virtual void processBuffer();
    virtual void finalizeProcessBuffer();

    virtual void generateMiniBatch(std::vector<NnMatrix>& miniBatch, u32 batchSize);
    virtual void initShuffle();							/** Initialize the shuffling */
protected:
    // internal methods for feature computation
    void setWindowedFeature(u32 streamIndex, u32 indexInBuffer, u32 indexInMiniBatch, NnMatrix &miniBatch);
    void setWindowedFeatureDerivatives(u32 streamIndex, u32 indexInBuffer, u32 indexInMiniBatch, NnMatrix &miniBatch);
    void getRelativePositionsInSlidingWindow(u32 indexOfCentralFeature, u32 windowSize, std::vector<s32> &positions);
    // first derivative: y' = (sum_i=1^2{ i*(  f(i) -  f(-i)) }) / (2*sum_i=1^2 i^2)
    void computeFirstDerivative(u32 streamIndex, u32 indexInBuffer);
    // second derivative: y" = (sum_i=1^2{ i*( y'(i) - y'(-i)) }) / (2*sum_i=1^2 i^2), only computed for first feature component
    void computeSecondDerivative(u32 streamIndex, u32 indexInBuffer);
protected:
    virtual void logProperties() const;
};

} // namespace Nn

#endif // _NN_BUFFERED_FEATURE_EXTRACTOR_HH

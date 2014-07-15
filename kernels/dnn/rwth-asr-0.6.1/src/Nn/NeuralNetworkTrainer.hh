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
#ifndef _NN_NEURAL_NETWORK_TRAINER_HH
#define _NN_NEURAL_NETWORK_TRAINER_HH

#include <Core/Component.hh>
#include <Speech/CorpusVisitor.hh>				// for unsupervised training
#include <Speech/AlignedFeatureProcessor.hh>			// for supervised training

#include "NeuralNetwork.hh"					// neural network
#include "Estimator.hh"
#include "Regularizer.hh"
#include "Types.hh"

#include <vector>
#include <Math/CudaVector.hh>
#include <Math/Vector.hh>

namespace Nn {

//=============================================================================
/**
 *  Base class for any  neural network trainer
 *  Unsupervised trainers are instantiated in BufferedFeatureExtractor,
 *  supervised trainers are instantiated in BufferedAlignedFeatureProcessor
 */
template<class T>
class NeuralNetworkTrainer : virtual public Core::Component {
    typedef Core::Component Precursor;
    typedef typename Types<T>::NnVector NnVector;
    typedef typename Types<T>::NnMatrix NnMatrix;
public:
    enum TrainerType {
	dummy,
	// supervised trainer types
	feedForwardTrainer,
	frameClassificationErrorAccumulator,
	// unsupervised trainer types
	//  .. currently nothing
	//
	// can be used in both modes
	meanAndVarianceAccumulator,
    };
    enum Criterion {
	none,
	crossEntropy,
	squaredError,
	binaryDivergence,
    };
public:
    static const Core::Choice choiceNetworkTrainer;
    static const Core::ParameterChoice paramNetworkTrainer;
    static const Core::Choice choiceTrainingCriterion;
    static const Core::ParameterChoice paramTrainingCriterion;
    static const Core::ParameterInt paramEpoch;
    static const Core::ParameterBool paramWeightedAccumulation;	// relevant for supervised training
    static const Core::ParameterBool paramMeasureTime;
protected:
    const Criterion criterion_;
    // perform weighted accumulation (according to class-weights, relevant for supervised training only)
    // can be set from outside, therefore not const
    bool weightedAccumulation_;
    // needed for prior estimation from class counts, passed by BufferedAlignedFeatureProcessor
    const Math::Vector<T> *classWeights_;
    // measure runtime
    const bool measureTime_;
    // depends on trainer, but normally true
    bool needsNetwork_;
protected:
    Core::XmlChannel statisticsChannel_; 			// statistics-channel
    bool needInit_;
    // neural network to be trained, use pointer in order to pass a network from outside
    NeuralNetwork<T> *network_;
    // estimator for weights update
    Estimator<T>* estimator_;
    // Regularizer
    Regularizer<T>* regularizer_;
public:
    NeuralNetworkTrainer(const Core::Configuration &config);
    virtual ~NeuralNetworkTrainer();

    // initialization & finalization methods
    virtual void initializeTrainer(u32 batchSize);
    virtual void initializeTrainer(u32 batchSize, std::vector<u32>& streamSizes);
    virtual void finalize();

    // getter and setter methods

    /** get activations of output layer */
    NnMatrix& getOutputActivation() { require(network_); return network_->getTopLayerOutput(); }
    /** returns whether trainer has a network */
    bool hasNetwork() const { return (network_ != 0); }
    /** returns whether trainer has an estimator */
    bool hasEstimator() const { return (estimator_ != 0); }
    /** returns whether frame weights are used */
    bool weightedAccumulation() const { return weightedAccumulation_; }
    /** returns current batch size (equal to size of activations)*/
    u32 batchSize() const { return hasNetwork() ? network_->activationsSize() : 0u ;}
    /** returns whether trainer is initialized */
    bool isInitialized() const { return !needInit_; }
    /** returns whether trainer measures computation time */
    bool measuresTime() const { return measureTime_; }
    /** return reference to network */
    NeuralNetwork<T>& network() { require(network_); return *network_; }
    /** return reference to network */
    u32 nLayers() const { return hasNetwork() ? network_->nLayers() : 0; }
    /** return reference to estimator */
    Estimator<T>& estimator() { require(estimator_); return *estimator_; }
    /** return reference to regularizer */
    Regularizer<T>& regularizer() { require(regularizer_); return *regularizer_; }
    /** sets class weights to values in vector*/
    void setClassWeights(const Math::Vector<T> *vector);

    /** returns whether trainer needs to process all features, i.e. for batch training */
    virtual bool needsToProcessAllFeatures() const { return false; }
    /** resize activations (necessary when batch size has changed) */
    virtual void setBatchSize(u32 batchSize);

    // interface methods

    /** Override this method for unsupervised training */
    virtual void processBatch(std::vector<NnMatrix>& features) {};

    /** Override this method for supervised training */
    virtual void processBatch(std::vector<NnMatrix>& features, Math::CudaVector<u32> &alignment) {};

    /** Override this method to process a batch for all streams with weighted features */
    virtual void processBatch(std::vector<NnMatrix>& features, Math::CudaVector<u32> &alignment, NnVector& weights) {};

    // reset history (for recurrent networks)
    virtual void resetHistory();

    virtual void logBatchTimes() const {}
protected:
    // log configuration
    virtual void logProperties() const;
public:
    // factory methods
    static NeuralNetworkTrainer<T>* createSupervisedTrainer(const Core::Configuration& config);
    static NeuralNetworkTrainer<T>* createUnsupervisedTrainer(const Core::Configuration& config);
};


//=============================================================================
/** only computes frame classification error and objective function
 * (supervised trainer)
 * TODO remove this class, should not be implemented as a trainer
 */
template<class T>
class FrameErrorEvaluator : public NeuralNetworkTrainer<T> {
    typedef NeuralNetworkTrainer<T> Precursor;
protected:
    typedef typename Types<T>::NnVector NnVector;
    typedef typename Types<T>::NnMatrix NnMatrix;
protected:
    using Precursor::statisticsChannel_;
private:
    u32 nObservations_;
    u32 nFrameClassificationErrors_;
    T objectiveFunction_;
public:
    FrameErrorEvaluator(const Core::Configuration &config);
    virtual ~FrameErrorEvaluator() {};
    virtual void initializeTrainer(u32 batchSize, std::vector<u32>& streamSizes);
    NeuralNetwork<T>& network() { return Precursor::network(); }
    virtual void finalize();
    virtual void processBatch(std::vector<NnMatrix>& features, Math::CudaVector<u32> &alignment, NnVector& weights);
    virtual bool needsToProcessAllFeatures() const { return true; }
};

//=============================================================================
/** computes mean and variance of input features
 *
 *  implemented as a NN trainer in order to get GPU support and
 *  to reuse the configuration of the network
 */
template<class T>
class MeanAndVarianceTrainer : public NeuralNetworkTrainer<T> {
    typedef NeuralNetworkTrainer<T> Precursor;
protected:
    typedef typename Types<T>::NnVector NnVector;
    typedef typename Types<T>::NnMatrix NnMatrix;
protected:
    using Precursor::statisticsChannel_;
public:
    static const Core::ParameterString paramMeanFile;
    static const Core::ParameterString paramStandardDeviationFile;
    static const Core::ParameterString paramStatisticsFile;
protected:
    Statistics<T> *statistics_;
    Math::Vector<T> mean_;
    Math::Vector<T> standardDeviation_;
    NnMatrix tmp_;

    std::string meanFile_;
    std::string standardDeviationFile_;
    std::string statisticsFile_;
public:
    MeanAndVarianceTrainer(const Core::Configuration &config);
    virtual ~MeanAndVarianceTrainer();

    /** initialization method */
    virtual void initializeTrainer(u32 batchSize, std::vector<u32>& streamSizes);

    /** returns reference to network */
    NeuralNetwork<T>& network() { return Precursor::network(); }

    /** write statistics */
    virtual void finalize();
    /** compute mean and standard deviation and write it to file */
    virtual void writeMeanAndStandardDeviation(Statistics<T> &statistics);
    /** accumulates mean and variance on mini-batch */
    virtual void processBatch(std::vector<NnMatrix>& features);
    /** accumulates mean and variance on mini-batch, with weighted samples */
    virtual void processBatch(std::vector<NnMatrix>& features, Math::CudaVector<u32> &alignment, NnVector& weights);
    virtual bool needsToProcessAllFeatures() const { return true; }
private:
    void saveVector(std::string& filename, Math::Vector<T>& vector);
};

} // namespace Nn

#endif // _NN_NEURAL_NETWORK_TRAINER_HH

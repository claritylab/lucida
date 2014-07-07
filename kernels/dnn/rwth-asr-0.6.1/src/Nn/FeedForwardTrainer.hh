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
#ifndef _NN_NEURAL_NETWORK_FEED_FORWARD_TRAINER_HH
#define _NN_NEURAL_NETWORK_FEED_FORWARD_TRAINER_HH

#include <cstring>
#include <Core/Component.hh>
#include <Math/CudaMatrix.hh>
#include <Math/CudaVector.hh>

#include "BufferedAlignedFeatureProcessor.hh"
#include "NeuralNetwork.hh"
#include "NeuralNetworkTrainer.hh"
#include "Statistics.hh"
#include "Regularizer.hh"

namespace Nn {

//=============================================================================

/**
 *  Abstract base class for all supervised feed forward trainer
 *  Override computeInitialErrorSignal and computeObjectiveFunction for implementing a new training criterion
 *
 *  Current limitations:
 *  	multiple feature input streams not yet supported
 */
template<class T>
class FeedForwardTrainer : public NeuralNetworkTrainer<T> {
    typedef NeuralNetworkTrainer<T> Precursor;
protected:
    typedef typename Types<T>::NnVector NnVector;
    typedef typename Types<T>::NnMatrix NnMatrix;
    using Precursor::statisticsChannel_;
    using Precursor::weightedAccumulation_;
    using Precursor::measureTime_;
public:
    using Precursor::network;
    using Precursor::estimator;
    using Precursor::regularizer;
protected:
    static const Core::ParameterString paramStatisticsFilename;
    static const Core::ParameterBool paramDoublePrecisionAccumulator;
    static const Core::ParameterBool paramAccumulateNnOutput;
protected:
    const std::string statisticsFilename_;
    const bool useDoublePrecisionAccumulator_;
    const bool accumulateNnOutput_;
    Statistics<T> *statistics_;
    Statistics<f64> *doublePrecisionStatistics_;
    // for each layer an error signal
    std::vector<NnMatrix> errorSignal_;
    // for each layer l: vector of pointers to error signals of layers k which have a connection from k to l
    std::vector<std::vector<NnMatrix*> > errorSignalOut_;
    s32 lowestTrainableLayerIndex_;
private:
    double timeSync_, timeForwardPass_, timeInitialErrorSignal_, timeBackwardPass_, timeGradient_, timeBaseStatistics_, timeRegularization_, timeEstimation_;
    double timeSyncBatch_, timeForwardPassBatch_, timeInitialErrorSignalBatch_, timeBackwardPassBatch_, timeGradientBatch_, timeBaseStatisticsBatch_, timeRegularizationBatch_, timeEstimationBatch_;
public:
    FeedForwardTrainer(const Core::Configuration &config);
    virtual ~FeedForwardTrainer();

    // initialization and finalization methods
    virtual void initializeTrainer(u32 batchSize) { Precursor::initializeTrainer(batchSize); }
    virtual void initializeTrainer(u32 batchSize, std::vector<u32>& streamSizes);
    virtual void finalize();

    /** returns reference to statistics */
    Statistics<T>& statistics() { require(statistics_); return *statistics_; }
    /** process mini-batch of aligned features */
    virtual void processBatch(std::vector<NnMatrix>& features, Math::CudaVector<u32> &alignment, NnVector& weights);
protected:
    // set error signal of top layer (depending on criterion)
    virtual void computeInitialErrorSignal(Math::CudaVector<u32> &alignment, NnVector& weights) = 0;
    // compute objective function (depending on criterion)
    virtual void computeObjectiveFunction(Math::CudaVector<u32> &alignment, NnVector& weights) = 0;
    // backpropagate error signal
    virtual void errorBackpropagation();
    // compute gradient from error signal and activations
    virtual void collectGradient();
    // count classes
    virtual void updateClassCounts(const Math::CudaVector<u32> &alignment, Statistics<T> &statistics);
    // resize activations and error signal
    virtual void setBatchSize(u32 batchSize);
    // initialized double precision accumulator
    virtual void initializeDoublePrecisionStatistics();

    // per-batch time measurements
    virtual void resetBatchTimes();
public:
    virtual void logBatchTimes() const;
};

template<>
inline void FeedForwardTrainer<f32>::initializeDoublePrecisionStatistics(){
    u32 statisticsType = estimator().requiredStatistics();
    if (estimator().batchMode() || statisticsChannel_.isOpen())
	statisticsType |= Statistics<f32>::BASE_STATISTICS;
    doublePrecisionStatistics_ = new Statistics<f64>(network().nLayers(), statisticsType);
    doublePrecisionStatistics_->copyStructure(statistics());
    doublePrecisionStatistics_->initComputation();
}

template<>
inline void FeedForwardTrainer<f64>::initializeDoublePrecisionStatistics(){
    this->warning("option \"double-precision-accumulator\" does not have an effect, because "
	    "double precision is already used for all neural network computations");
}


//=============================================================================

/*
 * Cross entropy trainer for feed forward networks
 */

template<class T>
class FeedForwardCrossEntropyTrainer : public FeedForwardTrainer<T> {
    typedef FeedForwardTrainer<T> Precursor;
public:
    using Precursor::network;
    using Precursor::estimator;
    using Precursor::regularizer;
    using Precursor::statistics;
protected:
    typedef typename Types<T>::NnVector NnVector;
    typedef typename Types<T>::NnMatrix NnMatrix;
protected:
    using Precursor::statisticsChannel_;
    using Precursor::statisticsFilename_;
    using Precursor::statistics_;
    using Precursor::errorSignal_;

    virtual void computeInitialErrorSignal(Math::CudaVector<u32> &alignment, NnVector& weights);
    virtual void computeObjectiveFunction(Math::CudaVector<u32> &alignment, NnVector& weights);
public:
    FeedForwardCrossEntropyTrainer(const Core::Configuration &config);
    virtual ~FeedForwardCrossEntropyTrainer() {}
};

//=============================================================================

/*
 * Squared error trainer for feed forward networks
 */

template<class T>
class FeedForwardSquaredErrorTrainer : public FeedForwardTrainer<T> {
    typedef FeedForwardTrainer<T> Precursor;
protected:
    typedef typename Types<T>::NnVector NnVector;
    typedef typename Types<T>::NnMatrix NnMatrix;
public:
    using Precursor::network;
    using Precursor::estimator;
    using Precursor::regularizer;
    using Precursor::statistics;
protected:
    using Precursor::statisticsChannel_;
    using Precursor::statisticsFilename_;
    using Precursor::statistics_;
    using Precursor::errorSignal_;

    virtual void computeInitialErrorSignal(Math::CudaVector<u32> &alignment, NnVector& weights);
    virtual void computeObjectiveFunction(Math::CudaVector<u32> &alignment, NnVector& weights);
public:
    FeedForwardSquaredErrorTrainer(const Core::Configuration &config);
    virtual ~FeedForwardSquaredErrorTrainer() {}
};

//=============================================================================

/*
 * Binary divergence trainer for feed forward networks
 */

template<class T>
class FeedForwardBinaryDivergenceTrainer : public FeedForwardTrainer<T> {
    typedef FeedForwardTrainer<T> Precursor;
public:
    using Precursor::network;
    using Precursor::estimator;
    using Precursor::regularizer;
    using Precursor::statistics;
protected:
    typedef typename Types<T>::NnVector NnVector;
    typedef typename Types<T>::NnMatrix NnMatrix;
    using Precursor::statisticsChannel_;
    using Precursor::statisticsFilename_;
    using Precursor::statistics_;
    using Precursor::errorSignal_;

    virtual void computeInitialErrorSignal(Math::CudaVector<u32> &alignment, NnVector& weights);
    virtual void computeObjectiveFunction(Math::CudaVector<u32> &alignment, NnVector& weights);
public:
    FeedForwardBinaryDivergenceTrainer(const Core::Configuration &config);
    virtual ~FeedForwardBinaryDivergenceTrainer() {}
};

//=============================================================================

/*
 * Default trainer, empty computeInitialErrorSignal and computeObjectiveFunction
 * useful for all operations, where a training criterion is not required
 */

template<class T>
class FeedForwardDefaultTrainer : public FeedForwardTrainer<T> {
    typedef FeedForwardTrainer<T> Precursor;
public:
    using Precursor::estimator;
    using Precursor::statistics;
protected:
    typedef typename Types<T>::NnVector NnVector;
    typedef typename Types<T>::NnMatrix NnMatrix;
protected:
    virtual void computeInitialErrorSignal(Math::CudaVector<u32> &alignment, NnVector& weights){}
    virtual void computeObjectiveFunction(Math::CudaVector<u32> &alignment, NnVector& weights){}
public:
    FeedForwardDefaultTrainer (const Core::Configuration &config);
    virtual ~FeedForwardDefaultTrainer() {}
    virtual void initializeTrainer(u32 batchSize, std::vector<u32>& streamSizes);
};


//=============================================================================

} // namespace

#endif

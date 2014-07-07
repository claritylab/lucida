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
#ifndef _NN_NEURAL_NETWORK_ESTIMATOR_HH
#define _NN_NEURAL_NETWORK_ESTIMATOR_HH

#include "NeuralNetwork.hh"
#include "Statistics.hh"
#include "Types.hh"

namespace Nn {

/*---------------------------------------------------------------------------*/

/* Base class for weight estimators */

template<typename T>
class Estimator : virtual public Core::Component {
    typedef Core::Component Precursor;
    typedef typename Types<T>::NnVector NnVector;
    typedef typename Types<T>::NnMatrix NnMatrix;
public:
    enum EstimatorType {
	dummy,
	steepestDescentEstimator,
	steepestDescentL1Clipping,
	meanNormalizedSgd,
	meanNormalizedSgdL1Clipping,
	rprop,
	priorEstimator
    };
protected:
    Core::XmlChannel statisticsChannel_;
public:
    static const Core::Choice choiceEstimatorType;
    static const Core::ParameterChoice paramEstimatorType;
    static const Core::ParameterBool paramBatchMode;
    static const Core::ParameterFloat paramLearningRate;
    static const Core::ParameterFloat paramBiasLearningRate;
    static const Core::ParameterBool paramLogStepSize;
protected:
    bool batchMode_;
    T initialLearningRate_;
    T biasLearningRate_;
    bool logStepSize_;
public:
    Estimator(const Core::Configuration& config);
    virtual ~Estimator() {}
    // operate in batch mode ( = pass over full training data)
    virtual bool batchMode() const { return batchMode_ ; }
    virtual void setBatchMode(bool batchMode) { batchMode_ = batchMode; }
    // estimate new model based on previous model and statistics
    virtual void estimate(NeuralNetwork<T>& network, Statistics<T>& statistics)  {};
    // name of estimator
    virtual std::string type() const { return "dummy"; }
    // id of required statistics
    virtual u32 requiredStatistics() const { return Statistics<T>::NONE; }
    // creation method
    static Estimator<T>* createEstimator(const Core::Configuration& config);
};

/*---------------------------------------------------------------------------*/

template<typename T>
class SteepestDescentEstimator : public Estimator<T> {
    typedef Estimator<T> Precursor;
    typedef typename Types<T>::NnVector NnVector;
    typedef typename Types<T>::NnMatrix NnMatrix;
protected:
    using Precursor::statisticsChannel_;
    using Precursor::initialLearningRate_;
    using Precursor::biasLearningRate_;
    using Precursor::logStepSize_;
public:
    static const Core::ParameterBool paramUsePredefinedLearningRateDecay;
    static const Core::ParameterFloat paramLearningRateTau;
    static const Core::ParameterInt paramNumberOfUpdates;
    static const Core::ParameterFloat paramMomentumFactor;
protected:
    bool usePredefinedLearningRateDecay_;
    T tau_;
    u32 nUpdates_;
    T momentumFactor_;
    bool momentum_;
    Statistics<T> *oldDeltas_;			/* for momentum */
public:
    SteepestDescentEstimator(const Core::Configuration& config);
    virtual ~SteepestDescentEstimator();
    virtual void estimate(NeuralNetwork<T>& network, Statistics<T>& statistics);
    virtual std::string type() const { return "steepest-descent"; }
    virtual u32 requiredStatistics() const { return Statistics<T>::GRADIENT ; }
};

/*---------------------------------------------------------------------------*/

template<typename T>
class SteepestDescentL1ClippingEstimator : public SteepestDescentEstimator<T> {
    typedef SteepestDescentEstimator<T> Precursor;
    typedef typename Types<T>::NnVector NnVector;
    typedef typename Types<T>::NnMatrix NnMatrix;
protected:
    using Precursor::statisticsChannel_;
    using Precursor::initialLearningRate_;
    using Precursor::biasLearningRate_;
    using Precursor::logStepSize_;
public:
    SteepestDescentL1ClippingEstimator(const Core::Configuration& config);
    virtual ~SteepestDescentL1ClippingEstimator() {}
    virtual void estimate(NeuralNetwork<T>& network, Statistics<T>& statistics);
    virtual std::string type() const { return "steepest-descent-l1-clipping"; }
    virtual u32 requiredStatistics() const  { return Statistics<T>::GRADIENT ; }
};

template<typename T>
class PriorEstimator : public Estimator<T> {
    typedef Estimator<T> Precursor;
    typedef typename Types<T>::NnVector NnVector;
    typedef typename Types<T>::NnMatrix NnMatrix;
protected:
    using Precursor::statisticsChannel_;
public:
    PriorEstimator(const Core::Configuration& config);
    virtual ~PriorEstimator() {}
    virtual void estimate(NeuralNetwork<T>& network, Statistics<T>& statistics) {}
    virtual std::string type() const { return "prior-estimator"; }
    virtual u32 requiredStatistics() const { return Statistics<T>::CLASS_COUNTS ; }
};


} // namespace

#endif

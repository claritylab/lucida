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
#ifndef _NN_NEURAL_NETWORK_IMPLICIT_NORMALIZATION_ESTIMATOR_HH
#define _NN_NEURAL_NETWORK_IMPLICIT_NORMALIZATION_ESTIMATOR_HH

#include "NeuralNetwork.hh"
#include "Statistics.hh"
#include "Estimator.hh"

namespace Nn {

template<typename T>
class MeanNormalizedSgd : public Estimator<T> {
    typedef Estimator<T> Precursor;
    typedef typename Types<T>::NnVector NnVector;
    typedef typename Types<T>::NnMatrix NnMatrix;
protected:
    using Precursor::statisticsChannel_;
    using Precursor::initialLearningRate_;
    using Precursor::biasLearningRate_;
    using Precursor::logStepSize_;
private:
    bool firstEstimation_;
    void checkForStatistics(NeuralNetwork<T>& network);
public:
    MeanNormalizedSgd(const Core::Configuration& config);
    virtual ~MeanNormalizedSgd() {}
    virtual void estimate(NeuralNetwork<T>& network, Statistics<T>& statistics);
    virtual std::string type() const { return "mean-normalized-steepest-descent"; }
    virtual u32 requiredStatistics() const { return Statistics<T>::GRADIENT ; }
};

/*---------------------------------------------------------------------------*/

template<typename T>
class MeanNormalizedSgdL1Clipping : public MeanNormalizedSgd<T> {
    typedef MeanNormalizedSgd<T> Precursor;
    typedef typename Types<T>::NnVector NnVector;
    typedef typename Types<T>::NnMatrix NnMatrix;
protected:
    using Precursor::statisticsChannel_;
    using Precursor::initialLearningRate_;
    using Precursor::biasLearningRate_;
    using Precursor::logStepSize_;
public:
    MeanNormalizedSgdL1Clipping(const Core::Configuration& config);
    virtual ~MeanNormalizedSgdL1Clipping() {}
    virtual void estimate(NeuralNetwork<T>& network, Statistics<T>& statistics);
    virtual std::string type() const { return "mean-normalized-steepest-descent-l1-clipping"; }
};

} // namespace

#endif

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
#ifndef RPROPESTIMATOR_HH_
#define RPROPESTIMATOR_HH_

#include "Estimator.hh"
#include "Types.hh"

namespace Nn {

template<typename>
class Statistics;

/**
 *
 * Implementation of the Rprop algorithm of Riedmiller & Braun
 *
 * The "backtracking" of the original Rprop algorithm is not
 * implemented, because it is known to be harmful.
 *
 * The implementation is not intended to be used on GPU.
 * In GPU mode, it uses a lot of sychronization and computation on CPU.
 *
 */

template<typename T>
class RpropEstimator : public Estimator<T> {
    typedef Estimator<T> Precursor;
    typedef typename Types<T>::NnVector NnVector;
    typedef typename Types<T>::NnMatrix NnMatrix;
    static const Core::ParameterString paramStepSizesOld;
    static const Core::ParameterString paramStepSizesNew;
    static const Core::ParameterString paramPreviousStatistics;
    static const Core::ParameterFloat paramIncreasingFactor;
    static const Core::ParameterFloat paramDecreasingFactor;
    enum initializationType {
	constant,
	blockwiseAverage,
	minBlockAverage,
    };
    static const Core::Choice choiceInitializationType;
    static const Core::ParameterChoice paramInitializationType;
    static const Core::ParameterFloat paramRelativeInitialStepSize;
    static const Core::ParameterBool paramBlockwiseAveraging;
protected:
    // factors to increase and decrease step sizes
    const T increasingFactor_;
    const T decreasingFactor_;
    const T relativeInitialStepSize_;
    const initializationType initializationType_;
protected:
    // step sizes, abuse gradient field of statistics for storing step sizes
    Statistics<T> *stepSizes_;
    // previous statistics
    Statistics<T> *prevStatistics_;
    bool needInit_;
public:
    RpropEstimator(const Core::Configuration& c);
    virtual ~RpropEstimator();
public:
    virtual void estimate(NeuralNetwork<T>& network, Statistics<T>& statistics);
    virtual u32 requiredStatistics() const { return Statistics<T>::GRADIENT; }
protected:
    virtual void initialize(const NeuralNetwork<T>& network);
    virtual void logProperties() const;
private:
    void updateStepSize(const T &prevGradient, const T& gradient, T& stepSize) const;
    void updateParameter(const T &stepSize, const T& gradient, T& parameter) const;
    T signum(const T &x) const { return (x == 0) ? 0 : ((x > 0) ? 1 : -1); };
};

}

#endif /* RPROPESTIMATOR_HH_ */

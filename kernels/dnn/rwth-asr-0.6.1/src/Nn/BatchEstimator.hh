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
#ifndef _NN_BATCH_ESTIMATOR_HH
#define _NN_BATCH_ESTIMATOR_HH

#include <Core/Component.hh>

#include "Types.hh"

namespace Nn {

/*
 * BatchEstimator:
 *  handles estimation for batch training
 *  statistics are read from file and optionally, regularization is applied
 *
 */

template<typename T>
class Statistics;
template<typename T>
class NeuralNetwork;
template<typename T>
class Estimator;
template<typename T>
class Regularizer;


template<typename T>
class BatchEstimator : virtual public Core::Component {
    typedef Core::Component Precursor;
protected:
    typedef typename Types<T>::NnVector NnVector;
    typedef typename Types<T>::NnMatrix NnMatrix;
protected:
    static const Core::ParameterStringVector paramStatisticsFiles;
protected:
    const std::vector<std::string> statisticsFiles_;
    Statistics<T> *statistics_;
    NeuralNetwork<T> *network_;
    Estimator<T> *estimator_;
    Regularizer<T> *regularizer_;
public:
    BatchEstimator(const Core::Configuration& config);
    virtual ~BatchEstimator();
    virtual void initialize();
    virtual void estimate();
    virtual void finalize();
};

}

#endif

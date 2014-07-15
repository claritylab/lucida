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
#ifndef _NN_NEURAL_NETWORK_ACTIVATIONS_HH
#define _NN_NEURAL_NETWORK_ACTIVATIONS_HH

#include <vector>
#include "NeuralNetworkLayer.hh"
#include "Types.hh"

namespace Nn {

/**
 *
 * Neural network activations.
 *
 */

template <typename T>
class Activations {
    typedef typename Types<T>::NnVector NnVector;
    typedef typename Types<T>::NnMatrix NnMatrix;
private:
    std::vector<NnMatrix> activations_; 			// activations of each layer
    std::vector<std::vector<NnMatrix*> > inputActivations_; 	// input activations of each layer
    std::vector<NnMatrix*> outputActivations_; 			// output activations of each layer
    mutable bool isComputing_;					// in computing state ? (GPU)
public:
    Activations();
    virtual ~Activations(){}

    void swap(Activations &rhs);

    void setNumberOfActivations(u32 size) { activations_.resize(size); }
    u32 numberOfActivations() const { return activations_.size(); }

    void setBatchSize(u32 batchSize);

    void setInputDimension(u32 layer, u32 stream, u32 dim);
    void setOutputDimension(u32 layer, u32 dim);

    NnMatrix& at(u32 index) { require_lt(index, activations_.size()) ; return activations_[index]; }
    const NnMatrix& at(u32 index) const { require_lt(index, activations_.size()) ; return activations_[index]; }

    std::vector<NnMatrix*>& getInput(u32 index);
    const std::vector<NnMatrix*>& getInput(u32 index) const;

    NnMatrix& getOutput(u32 index);
    const NnMatrix& getOutput(u32 index) const;

    void connectOutput(u32 layer, u32 globalPort);
    void connectInput(u32 layer, u32 localPort, u32 globalPort);

    void setNumberOfLayers(u32 size);
    void reset();

    void initComputation(bool sync = true) const;
    void finishComputation(bool sync = true) const;
    bool isComputing() const { return isComputing_; }

//    TODO only for debugging
//    void show() const;

};

} // namespace Nn

#endif // _NN_NEURAL_NETWORK_ACTIVATIONS_HH

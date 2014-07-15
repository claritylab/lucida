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
/* Implementation of neural networks activations container */

#include "Activations.hh"

#include <algorithm>

using namespace Nn;

// constructor
template<typename T>
Activations<T>::Activations() :
  activations_(0),
  inputActivations_(0),
  outputActivations_(0),
  isComputing_(false)
{}

template<typename T>
void Activations<T>::setBatchSize(u32 batchSize){
    for (u32 i = 0; i < activations_.size(); ++i){
	u32 nRows = activations_.at(i).nRows();
	activations_.at(i).resize(nRows, batchSize);
    }
}

template<typename T>
void Activations<T>::setInputDimension(u32 layer, u32 stream, u32 dim){
    require_lt(layer, inputActivations_.size());
    require_lt(stream, inputActivations_.at(layer).size());
    u32 nCols = inputActivations_.at(layer).at(stream)->nColumns();
    inputActivations_.at(layer).at(stream)->resize(dim, nCols);
}

template<typename T>
void Activations<T>::setOutputDimension(u32 layer, u32 dim){
    require_lt(layer, outputActivations_.size());
    u32 nCols = outputActivations_[layer]->nColumns();
    outputActivations_[layer]->resize(dim, nCols);
}

template<typename T>
std::vector<typename Activations<T>::NnMatrix*>& Activations<T>::getInput(u32 index) {
    require_lt(index, inputActivations_.size());
    return inputActivations_[index];
}

template<typename T>
const std::vector<typename Activations<T>::NnMatrix*>& Activations<T>::getInput(u32 index) const {
    require_lt(index, inputActivations_.size());
    return inputActivations_[index];
}

template<typename T>
typename Activations<T>::NnMatrix& Activations<T>::getOutput(u32 index) {
    require_lt(index, outputActivations_.size());
    return *(outputActivations_[index]);
}

template<typename T>
const typename Activations<T>::NnMatrix& Activations<T>::getOutput(u32 index) const {
    require_lt(index, outputActivations_.size());
    return *(outputActivations_[index]);
}

// set number of layers
template<typename T>
void Activations<T>::setNumberOfLayers(u32 size) {
    inputActivations_.resize(size);
    outputActivations_.resize(size);
}

// connect the globalPort with the localPort/position of layer layer
template<typename T>
void Activations<T>::connectInput(u32 layer, u32 localPort, u32 globalPort) {
    require_lt(globalPort, activations_.size());
    if (inputActivations_.size() < layer + 1)
	inputActivations_.resize(layer + 1);
    if (inputActivations_.at(layer).size() < localPort + 1)
	inputActivations_.at(layer).resize(localPort + 1);
    inputActivations_[layer][localPort] = &(activations_[globalPort]);
}

// connect the globalPort with the activation of the layer
template<typename T>
void Activations<T>::connectOutput(u32 layer, u32 globalPort) {
    require_lt(globalPort, activations_.size());
    if (outputActivations_.size() < layer + 1)
	outputActivations_.resize(layer + 1);
    outputActivations_[layer] = &(activations_[globalPort]);
}

template<typename T>
void Activations<T>::reset() {
    for (u32 i = 0; i < activations_.size() ; ++i)
	activations_[i].setToZero();
}

template<typename T>
void Activations<T>::initComputation(bool sync) const {
    for (u32 i = 0; i < activations_.size(); i++) {
	if (!activations_[i].isComputing()) {
	    activations_[i].initComputation(sync);
	}
    }
    isComputing_ = true;
}

template<typename T>
void Activations<T>::finishComputation(bool sync) const {
    for (u32 i = 0; i < activations_.size(); i++) {
	if (activations_[i].isComputing()) {
	    activations_[i].finishComputation(sync);
	}
    }
    isComputing_ = false;
}

template<typename T>
void Activations<T>::swap(Activations<T> &rhs) {
    std::swap(activations_, rhs.activations_);
    std::swap(inputActivations_, rhs.inputActivations_);
    std::swap(outputActivations_, rhs.outputActivations_);
    std::swap(isComputing_, rhs.isComputing_);
}

// only for debugging
//template<typename T>
//void Activations<T>::show() const {
//    std::cout << "------------" << std::endl;
//    std::cout << "ACTIVATIONS" << std::endl;
//    for (u32 i = 0; i < activations_.size(); i++){
//	std::cout << activations_.at(i).nRows() << " x " << activations_.at(i).nColumns() << std::endl;
//    }
//    std::cout << "INPUT ACTIVATIONS" << std::endl;
//    for (u32 i = 0; i < inputActivations_.size(); i++){
//	for (u32 j = 0; j < inputActivations_.at(i).size(); j++){
//	    std::cout << inputActivations_.at(i).at(j)->nRows() << " x " << inputActivations_.at(i).at(j)->nColumns() << std::endl;
//	}
//    }
//    std::cout << "OUTPUT ACTIVATIONS" << std::endl;
//    for (u32 i = 0; i < outputActivations_.size(); i++){
//	std::cout << outputActivations_.at(i)->nRows() << " x " << outputActivations_.at(i)->nColumns() << std::endl;
//    }
//
//}
//=============================================================================
// explicit template instantiation
namespace Nn {
template class Activations<f32>;
template class Activations<f64>;
}

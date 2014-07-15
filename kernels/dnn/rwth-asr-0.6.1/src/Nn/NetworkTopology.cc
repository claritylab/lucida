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
#include <map>

#include "NetworkTopology.hh"

using namespace Nn;


template<typename T>
const u32 NetworkTopologyElement<T>::invalidId = Core::Type<u32>::max;

template<typename T>
NetworkTopologyElement<T>::NetworkTopologyElement(s32 id, NeuralNetworkLayer<T> *layer) :
    id_(id),
    topologicalId_(-1),
    layer_(layer)
{}

template<typename T>
bool NetworkTopologyElement<T>::addPredecessor(u32 elem, u32 portId){
    if (predecessors_.size() < portId + 1)
	predecessors_.resize(portId + 1, invalidId);
    if (predecessors_.at(portId) != elem && predecessors_.at(portId) != invalidId)
	return false;
    predecessors_.at(portId) = elem;
    return true;
}

template<typename T>
void NetworkTopology<T>::clear(){
    featureStreamConnections_.clear();
    layers_.clear();
    layerMapping_.clear();
}

template<typename T>
s32 NetworkTopology<T>::addLayer(const std::string &layerName, NeuralNetworkLayer<T>* layer){
    if (layerMapping_.find(layerName) != layerMapping_.end())
	return -1;
    s32 id = layers_.size();
    layers_.push_back(NetworkTopologyElement<T>(id, layer));
    layerMapping_[layerName] = id;
    return id;
}

template<typename T>
bool NetworkTopology<T>::addConnection(const std::string &layerNameFrom, const std::string &layerNameTo, u32 targetPortId){
    if (layerMapping_.find(layerNameFrom) == layerMapping_.end())
	return false;
    if (layerMapping_.find(layerNameTo) == layerMapping_.end())
	return false;

    NetworkTopologyElement<T> *nodeFrom = &(layers_.at(layerMapping_[layerNameFrom]));
    s32 nodeTo = layerMapping_[layerNameTo];
    // add connection
    nodeFrom->addConnection(nodeTo);
    // add predecessor
    if (!layers_.at(nodeTo).addPredecessor(nodeFrom->id(), targetPortId))
	return false;
    return true;
}

template<typename T>
bool NetworkTopology<T>::addFeatureStreamConnection(u32 featureStreamIndex, const std::string &layerNameTo, u32 targetPortId){
    if (layerMapping_.find(layerNameTo) == layerMapping_.end())
	return false;

    if (featureStreamConnections_.size() < featureStreamIndex + 1)
	featureStreamConnections_.resize(featureStreamIndex + 1);
    featureStreamConnections_.at(featureStreamIndex).push_back(std::make_pair(layerMapping_[layerNameTo], targetPortId));
    return true;
}

template<typename T>
void NetworkTopology<T>::setTopologicalOrdering(std::vector<NeuralNetworkLayer<T>* > &result){
    std::vector<bool> discovered(nLayers(), false);
    std::vector<bool> finished(nLayers(), false);

    std::vector<NetworkTopologyElement<T>* > topologicalOrdering;
    for (u32 layerIdx = 0; layerIdx < layers_.size(); ++layerIdx) {
	if (!discovered.at(layerIdx)){
	    dfsTopologicalOrderingVisit(discovered, finished, topologicalOrdering, layerIdx);
	}
    }

    std::reverse(topologicalOrdering.begin(), topologicalOrdering.end());

    result.clear();
    for (u32 i = 0; i < topologicalOrdering.size(); i++){
	topologicalOrdering.at(i)->setTopologicalId(i);
	result.push_back(topologicalOrdering.at(i)->layer());
    }
}

template<typename T>
void NetworkTopology<T>::dfsTopologicalOrderingVisit(std::vector<bool> &discovered, std::vector<bool> &finished, std::vector<NetworkTopologyElement<T>* > &topologicalOrdering, u32 layerIdx){
    verify_lt(layerIdx, nLayers());

    discovered.at(layerIdx) = true;
    NetworkTopologyElement<T> *layer = &layers_.at(layerIdx);

    for (u32 i = 0; i < layer->nConnections(); i++){
	NetworkTopologyElement<T> *target = &layers_.at(layer->connection(i));
	s32 targetId = target->id();
	if (!discovered.at(targetId)){
	    dfsTopologicalOrderingVisit(discovered, finished, topologicalOrdering, targetId);
	}
    }
    finished.at(layerIdx) = true;
    topologicalOrdering.push_back(layer);
}


template<typename T>
const NetworkTopologyElement<T>* NetworkTopology<T>::layerElement(const std::string &layerName) const {
    std::map<std::string, s32>::const_iterator it = layerMapping_.find(layerName);
    if (it == layerMapping_.end())
	return 0;
    verify_lt((u32)it->second, layers_.size());
    return &(layers_.at(it->second));
}

template<typename T>
NetworkTopologyElement<T>* NetworkTopology<T>::layerElement(const std::string &layerName) {
    std::map<std::string, s32>::iterator it = layerMapping_.find(layerName);
    if (it == layerMapping_.end())
	return 0;
    verify_lt((u32)it->second, layers_.size());
    return &(layers_.at(it->second));
}

//=============================================================================
// explicit template instantiation
namespace Nn {
template class NetworkTopologyElement<f32>;
template class NetworkTopologyElement<f64>;
template class NetworkTopology<f32>;
template class NetworkTopology<f64>;
}

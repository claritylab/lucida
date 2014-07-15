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
#ifndef NETWORKTOPOLOGY_HH_
#define NETWORKTOPOLOGY_HH_

#include <vector>

#include <Core/Types.hh>

#include "NeuralNetworkLayer.hh"

namespace Nn {

template<typename T>
class NetworkTopologyElement {
private:
    static const u32 invalidId;
private:
    s32 id_; 				    		// internal id of node
    s32 topologicalId_;			    		// index in topological order
    NeuralNetworkLayer<T> *layer_; 	    		// pointer to layer
    // connections = edges in network graph
    std::vector<u32> connections_;
    // predecessors in network graph
    // vector at port-id: layer-id of predecessor // TODO replace by map
    std::vector<u32> predecessors_;
public:
    NetworkTopologyElement(s32 id, NeuralNetworkLayer<T> *layer);

    ~NetworkTopologyElement() {};

    // getter and setter methods
    s32 id() const { return id_; }

    s32 topologicalId() const { return topologicalId_; }
    void setTopologicalId(u32 value) { topologicalId_ = value; }

    const NeuralNetworkLayer<T>* layer() const { return layer_; }
    NeuralNetworkLayer<T>* layer() { return layer_; }

    u32 nConnections() const { return connections_.size(); }
    u32 connection(u32 i) const { require_lt(i, connections_.size()); return connections_.at(i); }
    void addConnection(u32  elem) { connections_.push_back(elem); }

    u32 nPredecessors() const { return predecessors_.size(); }
    u32 predecessor(u32 i) const { require_lt(i, predecessors_.size()); return predecessors_.at(i); }
    bool addPredecessor(u32 elem, u32 portId);
};

template<typename T>
class NetworkTopology {
private:
    // connections of features to network topology
    // e.g. featureStreamConnections[i] = [ (layer-k-id, targetPortId-k), (layer-j-id, targetPortId-j) ]
    std::vector<std::vector<std::pair<u32, u32 > > > featureStreamConnections_;

    // vector of nodes in the graph, corresponding to the layers of the network
    std::vector<NetworkTopologyElement<T> > layers_;

    // mapping from selection-name of layer to layer-index
    std::map<std::string, s32> layerMapping_;
public:
    NetworkTopology() {}

    ~NetworkTopology() {}

    /** returns whether a layer with selection name 'layerName' exists */
    bool hasLayer(const std::string &layerName) const { return layerMapping_.find(layerName) != layerMapping_.end(); }

    // getters, setters and access functions
    u32 nLayers() const { return layers_.size(); }

    u32 nFeatureStreams() const { return featureStreamConnections_.size(); }

    u32 nFeatureStreamConnections(u32 streamId) const {
	require_lt(streamId, featureStreamConnections_.size());
	return featureStreamConnections_.at(streamId).size();
    }

    const NetworkTopologyElement<T>* layerElement(u32 i) const { require_lt(i, layers_.size()); return &(layers_.at(i)); }
    NetworkTopologyElement<T>* layerElement(u32 i) { require_lt(i, layers_.size()); return &(layers_.at(i)); }

    const NetworkTopologyElement<T>* layerElement(const std::string &layerName) const;
    NetworkTopologyElement<T>* layerElement(const std::string &layerName);

    std::pair<u32, u32> featureStreamConnection(u32 streamId, u32 connectionId) const {
	require_lt(streamId, featureStreamConnections_.size());
	require_lt(connectionId, featureStreamConnections_.at(streamId).size());
	return featureStreamConnections_.at(streamId).at(connectionId);
    }

    void clear();

    /**
     * add layer 'layer' with selection name 'layerName' to the topology
     * returns index of layer, if successful;  -1, if a layer with this name already exists
     */
    s32 addLayer(const std::string &layerName, NeuralNetworkLayer<T>* layer);

    /**
     *  connect two layers
     */
    bool addConnection(const std::string &layerNameFrom, const std::string &layerNameTo, u32 targetPortId);

    /**
     *  add connection from feature stream to layer
     */
    bool addFeatureStreamConnection(u32 featureStreamIndex, const std::string &layerNameTo, u32 targetPortId);

    /**
     * get topological ordering of network graph
     * set topologicalIds in network-topology elements
     */
    void setTopologicalOrdering(std::vector<NeuralNetworkLayer<T>* > &topologicalOrdering);

private:
    void dfsTopologicalOrderingVisit(std::vector<bool> &discovered, std::vector<bool> &finished, std::vector<NetworkTopologyElement<T>* > &topologicalOrdering, u32 layerIdx);
};

} /* namespace Nn */

#endif /* NETWORKTOPOLOGY_HH_ */

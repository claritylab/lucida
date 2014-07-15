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
#ifndef _NN_NEURAL_NETWORK_HH
#define _NN_NEURAL_NETWORK_HH

#include <vector>

#include <Core/Parameter.hh>
#include <Math/CudaVector.hh>
#include <Math/CudaMatrix.hh>

#include "Activations.hh"
#include "NetworkTopology.hh"
#include "NeuralNetworkLayer.hh"
#include "Types.hh"


namespace Nn {

/**
 *
 *  Neural network implementation
 *
 */
template <class T>
class NeuralNetwork : virtual public Core::Component {
    typedef Core::Component Precursor;
protected:
    typedef typename Types<T>::NnVector NnVector;
    typedef typename Types<T>::NnMatrix NnMatrix;
private:
    static const Core::ParameterString paramNetworkNetworkFilenameOld;
    static const Core::ParameterString paramNetworkNetworkFilenameNew;
    static const Core::ParameterStringVector paramNetworkLinks;
    static const Core::ParameterBool paramIsRecurrent;
    static const Core::ParameterBool paramMeasureTime;
protected:
    const std::string parametersOld_;					/** filename of the old parameters (-> load) */
    const std::string parametersNew_;					/** filename of the new parameters (-> save) */
    const bool isRecurrent_;						/** is a recurrent network (not supported yet)*/
    const bool measureTime_;						/** measure computation time */
protected:
    bool needInit_;
    u32 nTrainableLayers_;
    s32 highestTrainableLayerIndex_;
    // network topology graph
    NetworkTopology<T> topology_;
    /** contains layers in topological order, => network can be forwarded by iterating through the layers in topology_-vector */
    std::vector<NeuralNetworkLayer<T>* > layers_;
    /** layerIndexToTrainableLayerIndex_[layerIndex] = trainableLayerIndex , if trainable, -1, else */
    std::vector<s32> layerIndexToTrainableLayerIndex_;

    mutable bool isComputing_;						/** is in computing state (GPU) */
    u32 batchSize_;							/** number of columns of activations = batch size, e.g. in training */

    // current and previous activations
    Activations<T> activations_;					// contain activation and references for input/output for each layer
    Activations<T> previousActivations_;				// same, but for previous connections (for recurrent networks)
public:
    NeuralNetwork(const Core::Configuration &config);
    virtual ~NeuralNetwork();
public:
    // getters & setters
    u32 nLayers() const { return layers_.size(); }
    u32 nFreeParameters() const;
    u32 nTrainableLayers() const { return nTrainableLayers_; }
    u32 getHighestTrainableLayerIndex() const { return highestTrainableLayerIndex_; }
    s32 layerIndexToTrainableLayerIndex(u32 layerIndex) const { require_lt(layerIndex, layerIndexToTrainableLayerIndex_.size()); return layerIndexToTrainableLayerIndex_.at(layerIndex); }
    bool measuresTime() const { return measureTime_; }
    u32 activationsSize() const { require_gt(layers_.size(), 0); return activations_.getOutput(0).nColumns(); }
    // set batch size
    void resizeActivations(u32 newSize);

    // access methods
    NeuralNetworkLayer<T>& getLayer(u32 index);
    const NeuralNetworkLayer<T>& getLayer(u32 index) const;
    NeuralNetworkLayer<T>& getTopLayer();
    const NeuralNetworkLayer<T>& getTopLayer() const;
    NnMatrix& getTopLayerOutput() { require_gt(layers_.size(), 0); return activations_.getOutput(nLayers() - 1); }
    NnMatrix& getLayerOutput(u32 layer) { require_le(layer, layers_.size()); return activations_.getOutput(layer); }
    std::vector<NnMatrix*>& getLayerInput(u32 layer) { require_le(layer, layers_.size()); return activations_.getInput(layer); }
    void resetPreviousActivations();
    // remove top layer from network
    NeuralNetworkLayer<T>* popLayer();
    // compute l1 norm of all network weights
    T l1norm(bool trainableLayersOnly=false) const;

    // forward multiple input streams
    bool forward(std::vector<NnMatrix>& inputStreams);
    // forward single input stream
    bool forward(NnMatrix& inputStream);
    // forward single input stream (stored in stl vector)
    bool forward(const std::vector<T>& inputStream);
    // applies softmax (non-linear part only) to current activation of final layer (must be a linear+softmax layer)
    bool applySoftmax();

    // finalizes the layers
    void finalize();
    // initialize network with a single input feature stream
    // pass filename only if it should be used instead of the internal configuration parameter 'parameters-old'
    void initializeNetwork(u32 batchSize, const std::string &filename="");
    // more general initialization method for networks with multiple input feature streams
    // the stream sizes are required for resizing the activations corresponding to the input features correctly
    // pass filename only if it should be used instead of the internal configuration parameter 'parameters-old'
    void initializeNetwork(u32 batchSize, std::vector<u32> streamSizes, const std::string &filename="");

    // I/O methods for the network parameters
    void loadNetworkParameters() { loadParameters(parametersOld_); }
    void saveNetworkParameters() { saveParameters(parametersNew_); }
    void saveParameters(const std::string &filename);
    void loadParameters(const std::string &filename);

    // GPU synchronization
    void initComputation(bool sync = true) const;
    void finishComputation(bool sync = true) const;
    bool isComputing() const { return isComputing_; }
protected:
    // set input activations for forwarding
    void setInputActivations(std::vector<NnMatrix>& inputStreams);
    void setInputActivations(NnMatrix& inputStream);
    void setInputActivations(const std::vector<T>& inputStream);
    // forward the layers, assume features are already set
    void forwardLayers();

    // create network layers and set connections
    void buildTopology();
    // initialization methods
    bool createLayer(const std::string &layerName);
    void connectActivations();
    void resizeInputActivations(const std::vector<u32>& streamSizes);
    void setInputDimensions();
    void initParameters(std::string filename);
    // protected in order to enable unit-testing
    bool formatConnection(const std::string& connection, u32& srcPort, std::string& targetLayerName, u32 &targetPort);
private:
    bool formatConnectionPart(const std::string& connection, bool lhs, u32& port, std::string& layerName);
    std::string getTopologyDescription();
};

} // namespace Nn

#endif // _NN_NEURAL_NETWORK_HH

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
/* Implementation of neural networks */

#include "NeuralNetwork.hh"

#include <iostream>
#include <queue>
#include <string>
#include <typeinfo>

#include <Core/Assertions.hh>
#include <Core/Component.hh>
#include <Core/Configuration.hh>
#include <Core/StringUtilities.hh>
#include <Core/Types.hh>
#include <Core/XmlStream.hh>

#include "LinearAndActivationLayer.hh"

using namespace Nn;

//=============================================================================
template<typename T>
const Core::ParameterString NeuralNetwork<T>::paramNetworkNetworkFilenameOld(
	"parameters-old", "Name of the file containing the parameters of the network", "");

template<typename T>
const Core::ParameterString NeuralNetwork<T>::paramNetworkNetworkFilenameNew(
	"parameters-new", "Name of the file to save the parameters to", "");

template<typename T>
const Core::ParameterStringVector NeuralNetwork<T>::paramNetworkLinks(
	"links", "links to other network layers, default is empty", ",", 0,
	Core::Type<s32>::max);

template<typename T>
const Core::ParameterBool NeuralNetwork<T>::paramIsRecurrent(
	"isRecurrent", "is this a recurrent neural network?", false);

template<typename T>
const Core::ParameterBool NeuralNetwork<T>::paramMeasureTime(
	"measure-time", "Measures time for executing methods in NeuralNetworkLayer", false);


// constructor
template<typename T>
NeuralNetwork<T>::NeuralNetwork(const Core::Configuration &config)
: Core::Component(config),
  parametersOld_(paramNetworkNetworkFilenameOld(config)),
  parametersNew_(paramNetworkNetworkFilenameNew(config)),
  isRecurrent_(paramIsRecurrent(config)),
  measureTime_(paramMeasureTime(config)),
  needInit_(true),
  nTrainableLayers_(0),
  highestTrainableLayerIndex_(-1),
  isComputing_(false),
  batchSize_(0)
{}

// destructor
template<typename T>
NeuralNetwork<T>::~NeuralNetwork() {
    for (u32 i = 0; i < nLayers(); i++)
	delete layers_.at(i);
}

/*
 * read the config and create/add layer in the order defined by "links"
 * links define an acyclic graph, layers eventually are stored in layers_ in topological order
 */
template<typename T>
void NeuralNetwork<T>::buildTopology() {
    std::vector<std::string> featureConnections = paramNetworkLinks(select("neural-network"));
    if (featureConnections.size() == 0)
	this->error("no configuration of neural network topology found");

    // build the topology graph from the configuration
    // for each feature stream connection: perform a breadth first search through the network layers
    for (u32 fIndex = 0; fIndex < featureConnections.size(); ++fIndex){
	std::string featureConnection = featureConnections.at(fIndex);
	u32 sourcePort, targetPort;
	std::string targetName;

	formatConnection(featureConnection, sourcePort, targetName, targetPort);
	// create layer
	bool isNew = createLayer(targetName);
	bool check = topology_.addFeatureStreamConnection(sourcePort, targetName, targetPort);
	verify(check);

	// queue for performing breadth first search through network, starting from feature stream fIndex
	std::queue<std::string> layerQueue;
	if (isNew)
	    layerQueue.push(targetName);

	// breadth first search
	while (!layerQueue.empty()){
	    // get top element
	    std::string layerName = layerQueue.front();
	    layerQueue.pop();

	    // get the connections of this layer
	    std::vector<std::string> newConnections = paramNetworkLinks(select(layerName));

	    // for each connection:
	    // check whether it points to a new layer
	    // if yes, create it and add the layer to the queue
	    // add the connection to the topology
	    for (u32 j = 0; j < newConnections.size(); ++j) {
		formatConnection(newConnections.at(j), sourcePort, targetName, targetPort);
		isNew = createLayer(targetName);
		bool check = topology_.addConnection(layerName, targetName, targetPort);
		verify(check);
		if (isNew)
		    layerQueue.push(targetName);
	    }
	}
    }
    // arrange layers in topological order
    topology_.setTopologicalOrdering(layers_);
    this->log("topological ordering of layers: ") << getTopologyDescription();

    // set output activation ids
    for (u32 layerIndex = 0; layerIndex < layers_.size(); ++layerIndex){
	layers_.at(layerIndex)->setOutputActivationIndex(layerIndex + topology_.nFeatureStreams());
    }

    // set input activation ids from feature streams
    for (u32 streamId = 0; streamId < topology_.nFeatureStreams(); ++streamId){
	for (u32 connectionId = 0; connectionId < topology_.nFeatureStreamConnections(streamId); ++connectionId){
	    u32 layerElementId = topology_.featureStreamConnection(streamId, connectionId).first;
	    u32 portId = topology_.featureStreamConnection(streamId, connectionId).second;
	    NetworkTopologyElement<T> *layerElement = topology_.layerElement(layerElementId);
	    verify(layerElement);
	    u32 layerIndex = layerElement->topologicalId();
	    verify_lt(layerIndex, layers_.size());
	    layers_.at(layerIndex)->setInputActivationIndex(streamId, portId);
	}
    }
    // set input activation ids, add predecessors
    for (u32 l = 0; l < topology_.nLayers(); ++l){
	NetworkTopologyElement<T>* layerElement = topology_.layerElement(l);
	NeuralNetworkLayer<T> *layer = layerElement->layer();
	u32 portId = 0;
	for (u32 p = 0;  p < layerElement->nPredecessors(); ++p, ++portId) {
	    NeuralNetworkLayer<T> *predecessorLayer = topology_.layerElement(layerElement->predecessor(p))->layer();
	    layer->setInputActivationIndex(predecessorLayer->getOutputActivationIndex(), portId);
	    layer->addPredecessor(topology_.layerElement(layerElement->predecessor(p))->topologicalId(), portId);
	}
    }
    // connect and resize activations
    connectActivations();

    // compute nTrainableLayers_
    for (u32 layer = 0; layer < nLayers(); layer++) {
	if (layers_.at(layer)->isTrainable()) {
	    nTrainableLayers_++;
	}
    }

    // compute highest trainable layer index
    if (nLayers() > 0){
	for (s32 layer = (s32)nLayers() - 1; layer >= 0; layer--) {
	    if (getLayer(layer).isTrainable()) {
		highestTrainableLayerIndex_ = layer;
		Core::Component::log("highest trainable layer index: ") << layer;
		break;
	    }
	}
    }
}


template<typename T>
bool NeuralNetwork<T>::createLayer(const std::string& layerName) {
    // check if the layer already exists
    if (topology_.hasLayer(layerName))
	return false;

    // create the NN-layer and add it to the topology
    NeuralNetworkLayer<T>* layer = NeuralNetworkLayer<T>::createNeuralNetworkLayer(select(layerName));
    topology_.addLayer(layerName, layer);
    return true;
}

/** connect activations container with the corresponding layer inputs/output
 */
template<typename T>
void NeuralNetwork<T>::connectActivations() {
    // initialize the activations (input/output)
    activations_.setNumberOfActivations(topology_.nLayers() + topology_.nFeatureStreams());
    activations_.setNumberOfLayers(layers_.size());
    for (u32 index = 0; index < layers_.size(); ++index) {
	for (u32 j = 0; j < layers_[index]->nInputActivations(); ++j){
	    activations_.connectInput(index, j, layers_[index]->getInputActivationIndex(j));
	}

	// output activation of the layers
	activations_.connectOutput(index, layers_[index]->getOutputActivationIndex());
	activations_.setOutputDimension(index, layers_[index]->getOutputDimension());
    }

    // for recurrent networks: initialize the previous activations (input/output)
    if (isRecurrent_) {
	previousActivations_.setNumberOfActivations(topology_.nLayers() + topology_.nFeatureStreams());
	previousActivations_.setNumberOfLayers(layers_.size());
	for (u32 index = 0; index < layers_.size(); ++index) {
	    for (u32 j = 0; j < layers_[index]->nInputActivations(); ++j){
		previousActivations_.connectInput(index, j, layers_[index]->getInputActivationIndex(j));
	    }

	    // previous output activation of the layers
	    previousActivations_.connectOutput(index, layers_[index]->getOutputActivationIndex());
	    previousActivations_.setOutputDimension(index, layers_[index]->getOutputDimension());
	}
    }
}

/**
 * Initialize the parameters of the layers
 */
template<typename T>
void NeuralNetwork<T>::initParameters(std::string filename) {
    // load the parameters from file, or random initialization
    if (! filename.empty()) // load parameters from file
	loadParameters(filename);
    else { // initialization of each layer
	Core::Component::log("Empty filename. Initializing parameters ..");
	for (typename std::vector<NeuralNetworkLayer<T>* >::iterator itLayer = layers_.begin();
		itLayer != layers_.end(); ++itLayer) {
	    (*itLayer)->initializeNetworkParameters();
	}
    }
}

/**
 * Get the layer at index given.
 */
template<typename T>
inline NeuralNetworkLayer<T>& NeuralNetwork<T>::getLayer(u32 index) {
    require_le(index, nLayers());
    return *(layers_[index]);
}

template<typename T>
inline const NeuralNetworkLayer<T>& NeuralNetwork<T>::getLayer(u32 index) const {
    require_le(index, nLayers());
    return *(layers_[index]);
}


template<typename T>
inline NeuralNetworkLayer<T>& NeuralNetwork<T>::getTopLayer() {
    require_gt(layers_.size(), 0);
    return *(layers_.back());
}

template<typename T>
inline const NeuralNetworkLayer<T>& NeuralNetwork<T>::getTopLayer() const {
    require_gt(layers_.size(), 0);
    return *(layers_.back());
}


// removes top layer from network and returns a pointer to the removed layer
template<typename T>
NeuralNetworkLayer<T>* NeuralNetwork<T>::popLayer(){
    require_ge(layers_.size(), 1);
    NeuralNetworkLayer<T> *result = layers_.back();
    layers_.resize(layers_.size() - 1);
    return result;
}

template<typename T>
bool NeuralNetwork<T>::forward(std::vector<NnMatrix>& inputStreams) {
    require(isComputing_);

    // for recurrent networks: swap activation and previous activations
    if (isRecurrent_)
	std::swap(activations_, previousActivations_);

    // configure the size of the activations for each input port if batch size has changed
    if (batchSize_ != inputStreams[0].nColumns())
	resizeActivations(inputStreams[0].nColumns());

    // copy the input streams to the corresponding ports and initialize computation
    setInputActivations(inputStreams);

    // do the forwarding
    forwardLayers();

    return true;
}

template<typename T>
bool NeuralNetwork<T>::forward(NnMatrix& inputStream) {
    require(isComputing_);

    // for recurrent networks: swap activation and previous activations
    if (isRecurrent_)
	std::swap(activations_, previousActivations_);

    // configure the size of the activations for each input port if batch size has changed
    if (batchSize_ != inputStream.nColumns())
	resizeActivations(inputStream.nColumns());

    // copy the input streams to the corresponding ports and initialize computation
    setInputActivations(inputStream);

    // do the forwarding
    forwardLayers();

    return true;
}

template<typename T>
bool NeuralNetwork<T>::forward(const std::vector<T>& inputStream) {
    require(isComputing_);
    // for recurrent networks: swap activation and previous activations
    if (isRecurrent_)
	std::swap(activations_, previousActivations_);

    // configure the size of the activations for each input port if batch size has changed
    if (batchSize_ != 1)
	resizeActivations(1);

    // copy the input streams to the corresponding ports and initialize computation
    setInputActivations(inputStream);

    // do the forwarding
    forwardLayers();

    return true;
}

template<typename T>
bool NeuralNetwork<T>::applySoftmax(){
    require(isComputing_);
    require_gt(layers_.size(), 0);
    require_eq(getTopLayerOutput().size(),0);
    LinearAndSoftmaxLayer<T> *layer = dynamic_cast<LinearAndSoftmaxLayer<T>* >(&getTopLayer());
    require(layer);
    require(!layer->evaluatesSoftmax());
    layer->applySoftmax(getTopLayerOutput());
    return true;
}

template<typename T>
void NeuralNetwork<T>::resizeActivations(u32 newSize) {
    activations_.setBatchSize(newSize);
    previousActivations_.setBatchSize(newSize);
}

template<typename T>
T NeuralNetwork<T>::l1norm(bool trainableLayersOnly) const {
    T result = 0;
    for (u32 layer = 0; layer < layers_.size(); layer++) {
	if (!trainableLayersOnly || layers_.at(layer)->isTrainable()){
	    for (u32 stream = 0; stream < layers_.at(layer)->nInputActivations(); stream++) {
		if (layers_.at(layer)->getWeights(stream))
		    result += layers_.at(layer)->getWeights(stream)->l1norm();
	    }
	    if (layers_.at(layer)->getBias())
		result += layers_.at(layer)->getBias()->l1norm();
	}
    }
    return result;
}

template<typename T>
void NeuralNetwork<T>::setInputActivations(std::vector<NnMatrix>& inputStreams) {
    for (u32 index = 0; index < inputStreams.size(); ++index) {
	inputStreams[index].initComputation();
	activations_.at(index).swap(inputStreams[index]);
    }
}

template<typename T>
void NeuralNetwork<T>::setInputActivations(NnMatrix& inputStream) {
    inputStream.initComputation();
    activations_.at(0).swap(inputStream);
}

template<typename T>
void NeuralNetwork<T>::setInputActivations(const std::vector<T>& inputStream) {
    activations_.at(0).copy(&(inputStream.at(0)));
}



// forward the layers, assume features are already set
template<typename T>
void NeuralNetwork<T>::forwardLayers(){
    // process each layer in topological order
    for (u32 index = 0; index < layers_.size(); ++index) {
	// forward step
	layers_[index]->forward(activations_.getInput(index), activations_.getOutput(index));

	// recurrent step
	if (isRecurrent_) {
	    std::vector<NnMatrix*> tmp;
	    tmp.push_back(&(previousActivations_.getOutput(index)));
	    layers_[index]->forwardRecurrent(tmp, activations_.getOutput(index));
	}

	// finalize forwarding
	layers_[index]->finalizeForwarding(activations_.getOutput(index));
    }
}


template<typename T>
void NeuralNetwork<T>::initializeNetwork(u32 batchSize, const std::string &filename) {
    std::vector<u32> streamSizes;
    initializeNetwork(batchSize, streamSizes, filename);
}


template<typename T>
void NeuralNetwork<T>::initializeNetwork(u32 batchSize, std::vector<u32> streamSizes, const std::string &filename) {
    // create/build the network
    buildTopology();

    // if no stream sizes are given, assume single stream with size of first-layer-first-stream input dimension
    if (streamSizes.size() == 0)
	streamSizes.push_back(getLayer(0).getInputDimension(0));

    // number of frames/sequences
    batchSize_ = batchSize;

    // set the size of the input activations, determined from streamSizes
    resizeInputActivations(streamSizes);
    // set batch size
    resizeActivations(batchSize_);

    // define input and output size of layers (matrices of size featureDim x batchSize)
    // input dimensions can not set before, because buildTopology doesn't know the dimensions of the feature streams
    setInputDimensions();

    // initialize the network weights and bias
    if (filename != ""){
	initParameters(filename);
    }
    else
	initParameters(parametersOld_);


    // set measure time for all layers
    if (measureTime_){
	for (u32 l = 0; l < layers_.size(); ++l){
	    layers_.at(l)->setMeasureTime(true);
	}
    }

    // set mapping from layer index to trainable layer index
    layerIndexToTrainableLayerIndex_.resize(nLayers());
    for (u32 layerIndex = 0; layerIndex < layerIndexToTrainableLayerIndex_.size(); ++layerIndex){
	s32 trainableLayer = -1;
	for (u32 layer = 0; layer <= layerIndex; layer++) {
	    if (getLayer(layer).isTrainable()) {
		trainableLayer++;
	    }
	}
	layerIndexToTrainableLayerIndex_[layerIndex] = trainableLayer;
    }

    // initialize (gpu) computation for matrices
    initComputation();

    Core::Component::log("Number of trainable parameters: ") << nFreeParameters();

    return;
}

// calculate number of trainable parameters
template<typename T>
u32 NeuralNetwork<T>::nFreeParameters() const {
    u32 result = 0;
    for (u32 index = 0; index < layers_.size(); ++index) {
	result += layers_.at(index)->getNumberOfFreeParameters();
    }
    return result;
}

template<typename T>
void NeuralNetwork<T>::resizeInputActivations(const std::vector<u32>& streamSizes) {
    for (u32 index = 0; index < streamSizes.size(); ++index) {
	activations_.at(index).resize(streamSizes.at(index), batchSize_);
	if (isRecurrent_)
	    previousActivations_.at(index).resize(streamSizes.at(index), batchSize_);
    }
}


template<typename T>
void NeuralNetwork<T>::setInputDimensions() {
    for (u32 index = 0; index < layers_.size(); ++index) {
	for (u32 j = 0; j < layers_.at(index)->nInputActivations(); ++j){
	    verify_gt(activations_.getInput(index).size(), j);
	    layers_.at(index)->setInputDimension(j, activations_.getInput(index)[j]->nRows());
	}
    }
}


/**	load the network parameters from file
 */
template<typename T>
void NeuralNetwork<T>::loadParameters(const std::string &filename) {
    // determine file suffix
    std::string suffix;
    if ((filename.length() >= 4) && (filename.substr(0,4) == "bin:")) {
	suffix = ".bin";
    } else {
	suffix = ".xml";
    }
    // load the parameters of the network
    for (u32 index = 0; index < layers_.size(); ++index) {
	// create new layer specific filename
	std::ostringstream id;
	id << index; // convert the id to string
	std::ostringstream type;
	if (typeid(T) == typeid(f32)) {
	    type << "f32";
	} else if (typeid(T) == typeid(f64)) {
	    type << "f64";
	}
	std::string newFilename = filename + "-" + type.str() + "-layer-" + id.str() + suffix;
	// load the parameters
	getLayer(index).loadNetworkParameters(newFilename);
    }
}

/**	save the network parameters from file
 */
template<typename T>
void NeuralNetwork<T>::saveParameters(const std::string &filename) {
    if (filename.empty()) {
	log("parameters-new is empty. Do not save parameters.");
    }
    else {
	log("Save parameters to ") << filename << "...";

	// determine file suffix
	std::string suffix;
	if ((filename.length() >= 4) && (filename.substr(0,4) == "bin:")) {
	    suffix = ".bin";
	} else {
	    suffix = ".xml";
	}
	// save the parameters of the network
	for (s32 index = (s32)layers_.size()-1; index >= 0; index--) {
	    // create new layer specific filename
	    std::ostringstream id;
	    id << index; // convert the id to string
	    std::ostringstream type;
	    if (typeid(T) == typeid(f32)) {
		type << "f32";
	    } else if (typeid(T) == typeid(f64)) {
		type << "f64";
	    }
	    std::string newFilename = filename + "-" + type.str() + "-layer-" + id.str() + suffix;
	    log("Saving '") << getLayer(index).getName() << "' to '" << newFilename << "'";
	    getLayer(index).saveNetworkParameters(newFilename);
	}
    }
}

template<typename T>
void NeuralNetwork<T>::resetPreviousActivations() {
    // reset the history/previous activations
    if (isRecurrent_)
	previousActivations_.reset();
}

template<typename T>
void NeuralNetwork<T>::initComputation(bool sync) const {
    activations_.initComputation(sync);
    if (isRecurrent_)
	previousActivations_.initComputation(sync);
    for (u32 layer = 0; layer < nLayers(); layer++) {
	if (!getLayer(layer).isComputing())
	    getLayer(layer).initComputation(sync);
    }
    isComputing_ = true;
}

template<typename T>
void NeuralNetwork<T>::finishComputation(bool sync) const {
    if (isComputing_) {
	activations_.finishComputation(sync);
	if (isRecurrent_)
	    previousActivations_.finishComputation(sync);
	for (u32 layer = 0; layer < nLayers(); layer++) {
	    getLayer(layer).finishComputation(sync);
	}
    }
    isComputing_ = false;
}

template<typename T>
void NeuralNetwork<T>::finalize() {
    for (u32 index = 0; index < layers_.size(); index++)
	layers_.at(index)->finalize();
}



// connections are of format
// "<src-id> -> <layer-name>" for feature stream connections, or
// "<layer-name>" for connections between layers
// for compatibility with old format: "<srd-id> -> <layer-name>:.*" allowed
template<typename T>
bool NeuralNetwork<T>::formatConnection(const std::string& connection, u32& srcPort, std::string& targetLayerName, u32 &targetPort) {

    // default value
    srcPort = 0;
    targetLayerName = "";
    targetPort = 0;
    std::string dummyLayer;
    bool result = true;

    std::vector<std::string> portNames; 		// separate the source-targets using "->"
    std::string lhs, rhs;

    Core::str2vector(connection, portNames, "->");

    if (portNames.size() == 0 || portNames.size() > 2){
	result = false;
    }
    else if (portNames.size() == 1){
	lhs = "";
	rhs = portNames[0];
	result &= formatConnectionPart(rhs, false, targetPort, targetLayerName);
    }
    if (portNames.size() == 2){
	lhs = portNames[0];
	rhs = portNames[1];
	result &= formatConnectionPart(lhs, true, srcPort, dummyLayer);
	result &= formatConnectionPart(rhs, false, targetPort, targetLayerName);
    }
    if (!result)
	this->error("format error in network connection specification: ") << connection;
    return result;
}

template<typename T>
bool NeuralNetwork<T>::formatConnectionPart(const std::string& connection, bool lhs, u32& port, std::string& layerName) {
    // default value
    port = 0;
    layerName = "";
    bool result = true;
    std::vector<std::string> parts = Core::split(connection,  ":");
    if (parts.size() == 0 || parts.size() > 2){
	layerName = "";
	return false;
    }
    else if (parts.size() == 1){
	if (lhs)
	    result &= Core::str2unsigned(parts[0], port);
	else
	    layerName = parts[0];
	return result;
    }
    else { // parts.size() == 2
	result &= Core::str2unsigned(parts[1], port);
	layerName = parts[0];
	return result;
    }
}

template<typename T>
std::string NeuralNetwork<T>::getTopologyDescription(){
    std::stringstream ss;
    for (u32 l = 0; l < layers_.size(); l++){
	NeuralNetworkLayer<T> *layer = layers_.at(l);
	if (layer)
	    ss << layer->getName() << ", ";
	else
	    ss << "*NULL*, ";
    }
    return ss.str();
}

//=============================================================================
// explicit template instantiation
namespace Nn {
template class NeuralNetwork<f32>;
template class NeuralNetwork<f64>;
}

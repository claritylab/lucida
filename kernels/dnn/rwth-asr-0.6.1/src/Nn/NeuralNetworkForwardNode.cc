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
#include "NeuralNetworkForwardNode.hh"

#include <Flow/TypedAggregate.hh>
#include <Flow/Registry.hh>

#include "LinearAndActivationLayer.hh"

using namespace Nn;

//===============================================================================================
Core::ParameterString NeuralNetworkForwardNode::paramId(
	"id", "Changing the id resets the caches for the recurrent connections.");

Core::ParameterInt NeuralNetworkForwardNode::paramBufferSize(
	"buffer-size", "buffer size", 32);

Core::ParameterBool NeuralNetworkForwardNode::paramCheckValues(
	"check-values", "check output of network for finiteness", false);


NeuralNetworkForwardNode::NeuralNetworkForwardNode(const Core::Configuration &c) :
			Core::Component(c),
			Precursor(c),
			bufferSize_(paramBufferSize(config)),
			checkValues_(paramCheckValues(config)),
			needInit_(true),
			measureTime_(false),
			aggregatedFeatures_(false),
			outputDimension_(0),
			network_(c),
			prior_(c)
{
    log("Neural network forward node: using buffer of size ") << bufferSize_;
    if (checkValues_)
	log("checking output of neural network for finiteness");
    measureTime_ = network_.measuresTime();
}

NeuralNetworkForwardNode::~NeuralNetworkForwardNode() {
    network_.finalize();
}

// Changing the segment id resets the caches for the recurrent connections.
bool NeuralNetworkForwardNode::setParameter(const std::string &name, const std::string &value) {
    if ( paramId.match(name) ) {
	network_.resetPreviousActivations();
    }
    return true;
}

/* same as Flow::Node::configureDatatype but without the error message
 * used for checking whether aggregated features or single feature stream is received
 * */
bool NeuralNetworkForwardNode::configureDataType(Core::Ref<const Flow::Attributes> a, const Flow::Datatype *d) {
    // check for valid attribute reference
    if (! a) {
	return false;
    }

    std::string dtn(a->get("datatype"));
    const Flow::Datatype* datatype(0);
    if (! dtn.empty()) { // get the data type from the attributes
	datatype = Flow::Registry::instance().getDatatype(dtn);
    }

    // data type from attribute and given data type do not match
    if (datatype != d) {
	return false;
    }

    // default is true
    return true;
}

// overrides Flow::Node::configure
// input of the node is a vector or an aggregate vector
// output of the node is a single vector stream
bool NeuralNetworkForwardNode::configure() {
    // get the attributes
    Core::Ref<Flow::Attributes> attributes(new Flow::Attributes());
    getInputAttributes(0, *attributes);

    // check the allowed data types (Vector + aggregate Vector)
    if (! (configureDataType(attributes, Flow::Vector<FeatureType>::type()) ||
	    configureDataType(attributes, Flow::TypedAggregate<Flow::Vector<FeatureType> >::type())) ) {
	return false;
    }

    // return attributes (single vector stream)
    attributes->set("datatype", Flow::Vector<FeatureType>::type()->name());
    return putOutputAttributes(0, attributes);
}

// initializes network and buffer
void NeuralNetworkForwardNode::initialize(const std::vector<u32>& nFeatures) {
    if (needInit_) {
	// set number of feature streams
	buffer_.resize(nFeatures.size());

	// matrix = "size x buffer-size" for each feature stream
	for (u32 index = 0; index < nFeatures.size(); ++index)
	    buffer_[index].resize(nFeatures[index], bufferSize_);

	timeStamps_.resize(bufferSize_);
	network_.initializeNetwork(bufferSize_, nFeatures);
	outputDimension_ = network_.getTopLayer().getOutputDimension();
	column_.resize(outputDimension_);

	// remove log-prior from bias
	if (prior_.fileName() != ""){
	    LinearAndSoftmaxLayer<f32> *topLayer = dynamic_cast<LinearAndSoftmaxLayer<f32>* >(&network_.getTopLayer());
	    if (topLayer){
		network_.finishComputation();
		prior_.read();
		topLayer->removeLogPriorFromBias(prior_);
		network_.initComputation();
	    }
	}
	log("l1 norm of all network weights is: ") << network_.l1norm();
    }

    needInit_ = false;
}

// adds a single stream feature to the buffer
void NeuralNetworkForwardNode::addFeatureToBuffer(const Flow::Vector<FeatureType> &feature){
    u32 colIndex = indexBuffer_.size();
    buffer_.at(0).copy(feature, 0, colIndex);
    timeStamps_.at(colIndex) = feature;
    indexBuffer_.push_back(colIndex);
}

// adds a multi stream feature to the buffer
void NeuralNetworkForwardNode::addFeatureToBuffer(const Flow::TypedAggregate<Flow::Vector<FeatureType> > &feature){
    u32 colIndex = indexBuffer_.size();
    for (size_t i = 0; i < buffer_.size(); i++)
	buffer_.at(i).copy(*feature.at(i), 0, colIndex);
    timeStamps_.at(colIndex) = feature;
    indexBuffer_.push_back(colIndex);
}

// network forwarding
void NeuralNetworkForwardNode::processBuffer(){
    bool result = network_.forward(buffer_);
    require(result);
}

// send next feature from buffer to the output of the node
bool NeuralNetworkForwardNode::putNextFeature(){
    require(!indexBuffer_.empty());
    u32 index = indexBuffer_.front();
    indexBuffer_.pop_front();
    Flow::Vector<FeatureType> *out = NULL;
    out = new Flow::Vector<FeatureType>(outputDimension_);
    out->setTimestamp(timeStamps_.at(index));
    column_.initComputation(false);
    network_.getTopLayerOutput().getColumn(index, column_);
    column_.finishComputation();
    if (checkValues_){
	if (!column_.isFinite()){
	    column_.show();
	    this->error("non-finite output of neural network detected");
	}
    }


    Math::copy<FeatureType, FeatureType>(outputDimension_, column_.begin(), 1, &(out->at(0)), 1);
    bool result = putData(0, out);
    return result;
}

// overrides Flow::Node::work
bool NeuralNetworkForwardNode::work(Flow::PortId p) {

    // features from the flow network (single feature stream/aggregate features)
    Flow::DataPtr<Flow::Vector<FeatureType> > ptrFeatures;
    Flow::DataPtr<Flow::TypedAggregate<Flow::Vector<FeatureType> > > ptrAggregateFeatures;

    if (needInit_){
	// get data type of the flow stream
	Core::Ref<Flow::Attributes> attributes(new Flow::Attributes());
	getInputAttributes(p, *attributes);
	aggregatedFeatures_ = configureDataType(attributes, Flow::TypedAggregate<Flow::Vector<FeatureType> >::type());
    }
    bool endOfStream = false;
    // if buffer empty, fill it
    if (indexBuffer_.empty()){
	bool validData = true;
	while (!bufferFull() && validData){
	    // pull feature from incoming connections
	    if (aggregatedFeatures_) {
		validData = getData(0, ptrAggregateFeatures);
		endOfStream = ptrAggregateFeatures == Flow::Data::eos();
	    }
	    else {
		validData = getData(0, ptrFeatures);
		endOfStream = ptrFeatures == Flow::Data::eos();
	    }
	    // init when receiving first feature
	    if (needInit_ && validData) {
		if (!aggregatedFeatures_) { // single feature stream
		    std::vector<u32> nSizes(1);
		    nSizes[0] = ptrFeatures.get()->size();
		    initialize(nSizes);
		}
		else {
		    std::vector<u32> nSizes(ptrAggregateFeatures.get()->size());
		    for (u32 i = 0; i < nSizes.size(); ++i)
			nSizes[i] = (*ptrAggregateFeatures.get())[i]->size();
		    initialize(nSizes);
		}
	    }
	    // add feature to buffer
	    if (validData && aggregatedFeatures_)
		addFeatureToBuffer(*ptrAggregateFeatures.get());
	    else if (validData && !aggregatedFeatures_)
		addFeatureToBuffer(*ptrFeatures.get());
	}
	// forward features in buffer
	if (!indexBuffer_.empty())
	    processBuffer();
    }
    // put next feature in buffer
    // if buffer still empty: eos has been reached or error in other flow node
    if (indexBuffer_.empty()) {
	if (endOfStream)
	    return putData(0, Flow::Data::eos());
	else
	    return false;
    }
    else
	return putNextFeature();

    return false;
}

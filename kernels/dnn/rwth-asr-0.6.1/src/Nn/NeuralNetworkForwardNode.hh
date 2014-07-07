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
#ifndef _NN_NEURAL_NETWORK_FORWARD_NODE_HH
#define _NN_NEURAL_NETWORK_FORWARD_NODE_HH

#include <deque>

#include <Flow/Node.hh>
#include <Flow/Datatype.hh>
#include <Flow/Attributes.hh>
#include <Math/Matrix.hh>
#include <Mm/Types.hh>

#include "ClassLabelWrapper.hh"
#include "Prior.hh"
#include "NeuralNetwork.hh"
#include "NeuralNetworkTrainer.hh"
#include "Types.hh"

namespace Nn {

/**	neural network forward node.
 *
 *	Neural network forwarding as a flow node.
 *	Useful, when output of network is reused, for example for tandem GMM systems
 *
 */
class NeuralNetworkForwardNode : public Flow::SleeveNode {
    typedef Flow::SleeveNode Precursor;
public:
    typedef Mm::FeatureType FeatureType;
protected:
    typedef Types<f32>::NnVector NnVector;
    typedef Types<f32>::NnMatrix NnMatrix;
public:
    static Core::ParameterString paramId;
    static Core::ParameterInt paramBufferSize;
    static Core::ParameterBool paramCheckValues;
private:
    const u32 bufferSize_;			// number of features that are processed at once
    const bool checkValues_;			// check output of network for finiteness
    bool needInit_;				// needs initialization of network
    bool measureTime_;				// measure run time
    bool aggregatedFeatures_;			// features are aggregated (multiple input streams)
    std::vector<NnMatrix> buffer_;		// features are saved in buffer and then processed in batch mode
    std::vector<Flow::Timestamp> timeStamps_;	// flow timestamps corresponding to feature vectors
    std::deque<u32> indexBuffer_;		// organizes the indices of the features in the buffer that need to be processed
    NnVector column_;				// one column of the output
    u32 outputDimension_;			// output dimension of the network
    NeuralNetwork<FeatureType> network_;	// the neural network
    Prior<f32> prior_;				// state prior
public:
    static std::string filterName() { return std::string("neural-network-forward"); };
public:
    NeuralNetworkForwardNode(const Core::Configuration &c);
    virtual ~NeuralNetworkForwardNode();

    void initialize(const std::vector<u32>& nFeatures);

    virtual bool configure();
    virtual bool setParameter(const std::string &name, const std::string &value);
    // override Flow::Node::work
    virtual bool work(Flow::PortId p);
private:
    // checks whether we have aggregated or simple Flow features
    bool configureDataType(Core::Ref<const Flow::Attributes> a, const Flow::Datatype *d);
    // add feature to buffer
    void addFeatureToBuffer(const Flow::Vector<FeatureType> &feature);
    void addFeatureToBuffer(const Flow::TypedAggregate<Flow::Vector<FeatureType> > &feature);
    // do the work
    void processBuffer();
    bool bufferFull() const { return indexBuffer_.size() == bufferSize_; }
    // send feature to ouput port
    bool putNextFeature();

};

}

#endif // _NN_NEURAL_NETWORK_NODE_HH

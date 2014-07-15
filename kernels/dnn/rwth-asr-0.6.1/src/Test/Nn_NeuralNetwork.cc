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
#include <Test/UnitTest.hh>
#include <Nn/NeuralNetwork.hh>
#include <Nn/LinearAndActivationLayer.hh>
#include <Nn/Types.hh>
#include <Math/Matrix.hh>

template<typename T>
class NeuralNetwork : public Nn::NeuralNetwork<T> {
public:
    using Nn::NeuralNetwork<T>::formatConnection;
public:
    NeuralNetwork(Core::Configuration &c) :  Core::Component(c), Nn::NeuralNetwork<T>(c) {}
};

class TestNeuralNetwork : public Test::ConfigurableFixture
{
public:
    NeuralNetwork<f64> *neuralNetwork_;
    std::vector<Nn::Types<f64>::NnMatrix> inputStream_;
    void setUp();
    void tearDown();
};

void TestNeuralNetwork::setUp() {
    setParameter("*.neural-network.links", "0->layer-1:0");
    setParameter("*.layer-1.layer-type", "linear+sigmoid");
    setParameter("*.layer-1.dimension-input", "2");
    setParameter("*.layer-1.dimension-output", "2");
    setParameter("*.layer-1.links", "0->layer-2:0");
    setParameter("*.layer-2.layer-type", "linear+softmax");
    setParameter("*.layer-2.dimension-input", "2");
    setParameter("*.layer-2.dimension-output", "2");
    setParameter("*.channel", "/dev/null");
    setParameter("*.unbufferred", "true");
    neuralNetwork_ = new NeuralNetwork<f64>(config);
    Nn::Types<f64>::NnMatrix* input = new Nn::Types<f64>::NnMatrix(2,4);
    input->at(0,0) = 1.2; input->at(1,0) = 0.7;
    input->at(0,1) = 0.5; input->at(1,1) = 1.0;
    input->at(0,2) = -1.5; input->at(1,2) = 1.1;
    input->at(0,3) = -0.3; input->at(1,3) = -0.7;
    inputStream_.push_back(*input);
    neuralNetwork_->initializeNetwork(inputStream_[0].nColumns());
    neuralNetwork_->finishComputation();
    neuralNetwork_->getLayer(0).getWeights(0)->at(0,0) = -1.7;
    neuralNetwork_->getLayer(0).getWeights(0)->at(0,1) = 0.3;
    neuralNetwork_->getLayer(0).getWeights(0)->at(1,0) = -0.3;
    neuralNetwork_->getLayer(0).getWeights(0)->at(1,1) = 0.9;
    neuralNetwork_->getLayer(0).getBias()->at(0) = 0.5;
    neuralNetwork_->getLayer(0).getBias()->at(1) = 0.7;
    neuralNetwork_->getLayer(1).getWeights(0)->at(0,0) = 0.4;
    neuralNetwork_->getLayer(1).getWeights(0)->at(0,1) = -0.2;
    neuralNetwork_->getLayer(1).getWeights(0)->at(1,0) = 0.6;
    neuralNetwork_->getLayer(1).getWeights(0)->at(1,1) = -0.1;
    neuralNetwork_->getLayer(1).getBias()->at(0) = 1.2;
    neuralNetwork_->getLayer(1).getBias()->at(1) = -0.5;
    neuralNetwork_->initComputation();
}

void TestNeuralNetwork::tearDown() {
    delete neuralNetwork_;
}

TEST_F(Test, TestNeuralNetwork, stringFormats)
{
    std::string connection("0->layer-1:0");
    u32 srcPort, targetPort;
    std::string destName;
    bool check = neuralNetwork_->formatConnection(connection, srcPort, destName, targetPort);
    EXPECT_TRUE(check);
    EXPECT_EQ(destName, std::string("layer-1"));
    EXPECT_EQ(srcPort, 0u);
    EXPECT_EQ(targetPort, 0u);
    connection = "10->foo:1000";
    check = neuralNetwork_->formatConnection(connection, srcPort, destName, targetPort);
    EXPECT_TRUE(check);
    EXPECT_EQ(destName, std::string("foo"));
    EXPECT_EQ(srcPort, 10u);
    EXPECT_EQ(targetPort, 1000u);
    connection = "bar:5";
    check = neuralNetwork_->formatConnection(connection, srcPort, destName, targetPort);
    EXPECT_TRUE(check);
    EXPECT_EQ(destName, std::string("bar"));
    EXPECT_EQ(srcPort, 0u);
    EXPECT_EQ(targetPort, 5u);

}

TEST_F(Test, TestNeuralNetwork, forward)
{
    neuralNetwork_->forward(inputStream_);
    neuralNetwork_->finishComputation();

    EXPECT_DOUBLE_EQ(0.915273, neuralNetwork_->getTopLayerOutput().at(0,0), 0.000001);
    EXPECT_DOUBLE_EQ(0.0847272, neuralNetwork_->getTopLayerOutput().at(1,0), 0.000001);

    EXPECT_DOUBLE_EQ(0.924293, neuralNetwork_->getTopLayerOutput().at(0,1), 0.000001);
    EXPECT_DOUBLE_EQ(0.0757068, neuralNetwork_->getTopLayerOutput().at(1,1), 0.000001);

    EXPECT_DOUBLE_EQ(0.942989, neuralNetwork_->getTopLayerOutput().at(0,2), 0.000001);
    EXPECT_DOUBLE_EQ(0.0570109, neuralNetwork_->getTopLayerOutput().at(1,2), 0.000001);

    EXPECT_DOUBLE_EQ(0.924822, neuralNetwork_->getTopLayerOutput().at(0,3), 0.000001);
    EXPECT_DOUBLE_EQ(0.0751783, neuralNetwork_->getTopLayerOutput().at(1,3), 0.000001);
}

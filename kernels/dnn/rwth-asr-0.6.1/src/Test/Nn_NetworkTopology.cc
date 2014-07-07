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
#include <Nn/NetworkTopology.hh>
#include <Nn/NeuralNetworkLayer.hh>
#include <Nn/ActivationLayer.hh>

class TestNetworkTopology : public Test::ConfigurableFixture
{
public:
    Nn::NetworkTopology<f32> topology_;
    void setUp();
    void tearDown();
};

void TestNetworkTopology::setUp() {

}

void TestNetworkTopology::tearDown() {

}

TEST_F(Test, TestNetworkTopology, addLayer){
    Nn::NeuralNetworkLayer<f32> *layer1 = new Nn::SigmoidLayer<f32>(config);
    Nn::NeuralNetworkLayer<f32> *layer2 = new Nn::SigmoidLayer<f32>(config);
    Nn::NeuralNetworkLayer<f32> *layer3 = new Nn::SigmoidLayer<f32>(config);

    s32 layerId1 = topology_.addLayer("layer-1", layer1);
    EXPECT_NE(layerId1, -1);
    EXPECT_TRUE(topology_.hasLayer("layer-1"));
    s32 layerId2 = topology_.addLayer("layer-2", layer2);
    EXPECT_NE(layerId2, -1);
    EXPECT_TRUE(topology_.hasLayer("layer-2"));
    s32 layerId3 = topology_.addLayer("layer-3", layer3);
    EXPECT_NE(layerId3, -1);
    EXPECT_TRUE(topology_.hasLayer("layer-3"));
    EXPECT_EQ(topology_.nLayers(), 3u);

    delete layer1;
    delete layer2;
    delete layer3;
}

TEST_F(Test, TestNetworkTopology, id){
    Nn::NeuralNetworkLayer<f32> *layer1 = new Nn::SigmoidLayer<f32>(config);
    Nn::NeuralNetworkLayer<f32> *layer2 = new Nn::SigmoidLayer<f32>(config);
    Nn::NeuralNetworkLayer<f32> *layer3 = new Nn::SigmoidLayer<f32>(config);

    s32 layerId1 = topology_.addLayer("layer-1", layer1);
    s32 layerId2 = topology_.addLayer("layer-2", layer2);
    s32 layerId3 = topology_.addLayer("layer-3", layer3);

    EXPECT_EQ(topology_.layerElement(layerId1)->id(), layerId1);
    EXPECT_EQ(topology_.layerElement(layerId2)->id(), layerId2);
    EXPECT_EQ(topology_.layerElement(layerId3)->id(), layerId3);

    delete layer1;
    delete layer2;
    delete layer3;
}

TEST_F(Test, TestNetworkTopology, addConnection){
    Nn::NeuralNetworkLayer<f32> *layer1 = new Nn::SigmoidLayer<f32>(config);
    Nn::NeuralNetworkLayer<f32> *layer2 = new Nn::SigmoidLayer<f32>(config);
    Nn::NeuralNetworkLayer<f32> *layer3 = new Nn::SigmoidLayer<f32>(config);

    s32 layerId1 = topology_.addLayer("layer-1", layer1);
    s32 layerId2 = topology_.addLayer("layer-2", layer2);
    s32 layerId3 = topology_.addLayer("layer-3", layer3);

    topology_.addConnection("layer-1", "layer-2", 0);
    topology_.addConnection("layer-2", "layer-3", 0);

    EXPECT_EQ(topology_.layerElement(layerId1)->nConnections(), 1u);
    EXPECT_EQ(topology_.layerElement(layerId2)->nConnections(), 1u);
    EXPECT_EQ(topology_.layerElement(layerId3)->nConnections(), 0u);

    EXPECT_EQ(topology_.layerElement(topology_.layerElement(layerId1)->connection(0))->id(), layerId2);
    EXPECT_EQ(topology_.layerElement(topology_.layerElement(layerId2)->connection(0))->id(), layerId3);

    delete layer1;
    delete layer2;
    delete layer3;
}

TEST_F(Test, TestNetworkTopology, setTopologicalOrdering){

    // test with simple linear network

    Nn::NeuralNetworkLayer<f32> *layer1 = new Nn::SigmoidLayer<f32>(select("layer-1"));
    Nn::NeuralNetworkLayer<f32> *layer2 = new Nn::SigmoidLayer<f32>(select("layer-2"));
    Nn::NeuralNetworkLayer<f32> *layer3 = new Nn::SigmoidLayer<f32>(select("layer-3"));
    Nn::NeuralNetworkLayer<f32> *layer4 = new Nn::SigmoidLayer<f32>(select("layer-4"));
    Nn::NeuralNetworkLayer<f32> *layer5 = new Nn::SigmoidLayer<f32>(select("layer-5"));
    Nn::NeuralNetworkLayer<f32> *layer6 = new Nn::SigmoidLayer<f32>(select("layer-6"));

    s32 layerId6 = topology_.addLayer("layer-6", layer6);
    s32 layerId5 = topology_.addLayer("layer-5", layer5);
    s32 layerId3 = topology_.addLayer("layer-3", layer3);
    s32 layerId1 = topology_.addLayer("layer-1", layer1);
    s32 layerId2 = topology_.addLayer("layer-2", layer2);
    s32 layerId4 = topology_.addLayer("layer-4", layer4);

    topology_.addConnection("layer-3", "layer-4", 0);
    topology_.addConnection("layer-4", "layer-5", 0);
    topology_.addConnection("layer-5", "layer-6", 0);
    topology_.addConnection("layer-1", "layer-2", 0);
    topology_.addConnection("layer-2", "layer-3", 0);

    // test predecessors

    EXPECT_EQ(topology_.layerElement("layer-1")->nPredecessors(), 0u);
    EXPECT_EQ(topology_.layerElement("layer-2")->nPredecessors(), 1u);
    EXPECT_EQ(topology_.layerElement("layer-3")->nPredecessors(), 1u);
    EXPECT_EQ(topology_.layerElement("layer-4")->nPredecessors(), 1u);
    EXPECT_EQ(topology_.layerElement("layer-5")->nPredecessors(), 1u);
    EXPECT_EQ(topology_.layerElement("layer-6")->nPredecessors(), 1u);

    EXPECT_EQ(topology_.layerElement(topology_.layerElement("layer-2")->predecessor(0))->layer()->name(), std::string("layer-1"));
    EXPECT_EQ(topology_.layerElement(topology_.layerElement("layer-3")->predecessor(0))->layer()->name(), std::string("layer-2"));
    EXPECT_EQ(topology_.layerElement(topology_.layerElement("layer-4")->predecessor(0))->layer()->name(), std::string("layer-3"));
    EXPECT_EQ(topology_.layerElement(topology_.layerElement("layer-5")->predecessor(0))->layer()->name(), std::string("layer-4"));
    EXPECT_EQ(topology_.layerElement(topology_.layerElement("layer-6")->predecessor(0))->layer()->name(), std::string("layer-5"));

    // test topological ordering

    std::vector<Nn::NeuralNetworkLayer<f32>* > topologicalOrdering;
    topology_.setTopologicalOrdering(topologicalOrdering);

    EXPECT_EQ(topologicalOrdering.at(0)->name(), std::string("layer-1"));
    EXPECT_EQ(topologicalOrdering.at(1)->name(), std::string("layer-2"));
    EXPECT_EQ(topologicalOrdering.at(2)->name(), std::string("layer-3"));
    EXPECT_EQ(topologicalOrdering.at(3)->name(), std::string("layer-4"));
    EXPECT_EQ(topologicalOrdering.at(4)->name(), std::string("layer-5"));
    EXPECT_EQ(topologicalOrdering.at(5)->name(), std::string("layer-6"));

    EXPECT_EQ(topology_.layerElement(layerId1)->topologicalId(), 0);
    EXPECT_EQ(topology_.layerElement(layerId2)->topologicalId(), 1);
    EXPECT_EQ(topology_.layerElement(layerId3)->topologicalId(), 2);
    EXPECT_EQ(topology_.layerElement(layerId4)->topologicalId(), 3);
    EXPECT_EQ(topology_.layerElement(layerId5)->topologicalId(), 4);
    EXPECT_EQ(topology_.layerElement(layerId6)->topologicalId(), 5);


    // test with skip connections
    topology_.clear();

    layerId4 = topology_.addLayer("layer-4", layer4);
    layerId2 = topology_.addLayer("layer-2", layer2);
    layerId6 = topology_.addLayer("layer-6", layer6);
    layerId5 = topology_.addLayer("layer-5", layer5);
    layerId3 = topology_.addLayer("layer-3", layer3);
    layerId1 = topology_.addLayer("layer-1", layer1);

    topology_.addConnection("layer-1", "layer-3", 1);
    topology_.addConnection("layer-2", "layer-6", 1);
    topology_.addConnection("layer-3", "layer-4", 0);
    topology_.addConnection("layer-4", "layer-5", 0);
    topology_.addConnection("layer-5", "layer-6", 0);
    topology_.addConnection("layer-1", "layer-2", 0);
    topology_.addConnection("layer-2", "layer-3", 0);


    EXPECT_EQ(topology_.layerElement("layer-1")->nPredecessors(), 0u);
    EXPECT_EQ(topology_.layerElement("layer-2")->nPredecessors(), 1u);
    EXPECT_EQ(topology_.layerElement("layer-3")->nPredecessors(), 2u);
    EXPECT_EQ(topology_.layerElement("layer-4")->nPredecessors(), 1u);
    EXPECT_EQ(topology_.layerElement("layer-5")->nPredecessors(), 1u);
    EXPECT_EQ(topology_.layerElement("layer-6")->nPredecessors(), 2u);

    EXPECT_EQ(topology_.layerElement(topology_.layerElement("layer-2")->predecessor(0))->layer()->name(), std::string("layer-1"));
    EXPECT_EQ(topology_.layerElement(topology_.layerElement("layer-4")->predecessor(0))->layer()->name(), std::string("layer-3"));
    EXPECT_EQ(topology_.layerElement(topology_.layerElement("layer-5")->predecessor(0))->layer()->name(), std::string("layer-4"));

    std::vector<std::string> predecessorNames(2);
    predecessorNames[0] = topology_.layerElement(topology_.layerElement("layer-3")->predecessor(0))->layer()->name();
    predecessorNames[1] = topology_.layerElement(topology_.layerElement("layer-3")->predecessor(1))->layer()->name();
    EXPECT_EQ(predecessorNames[0], std::string("layer-2"));
    EXPECT_EQ(predecessorNames[1], std::string("layer-1"));
    predecessorNames[0] = topology_.layerElement(topology_.layerElement("layer-6")->predecessor(0))->layer()->name();
    predecessorNames[1] = topology_.layerElement(topology_.layerElement("layer-6")->predecessor(1))->layer()->name();
    EXPECT_EQ(predecessorNames[0], std::string("layer-5"));
    EXPECT_EQ(predecessorNames[1], std::string("layer-2"));

    // set topological ordering

    topology_.setTopologicalOrdering(topologicalOrdering);

    EXPECT_EQ(topologicalOrdering.at(0)->name(), std::string("layer-1"));
    EXPECT_EQ(topologicalOrdering.at(1)->name(), std::string("layer-2"));
    EXPECT_EQ(topologicalOrdering.at(2)->name(), std::string("layer-3"));
    EXPECT_EQ(topologicalOrdering.at(3)->name(), std::string("layer-4"));
    EXPECT_EQ(topologicalOrdering.at(4)->name(), std::string("layer-5"));
    EXPECT_EQ(topologicalOrdering.at(5)->name(), std::string("layer-6"));

    EXPECT_EQ(topology_.layerElement(layerId1)->topologicalId(), 0);
    EXPECT_EQ(topology_.layerElement(layerId2)->topologicalId(), 1);
    EXPECT_EQ(topology_.layerElement(layerId3)->topologicalId(), 2);
    EXPECT_EQ(topology_.layerElement(layerId4)->topologicalId(), 3);
    EXPECT_EQ(topology_.layerElement(layerId5)->topologicalId(), 4);
    EXPECT_EQ(topology_.layerElement(layerId6)->topologicalId(), 5);

    delete layer1;
    delete layer2;
    delete layer3;
    delete layer4;
    delete layer5;
    delete layer6;
}

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
#include <Nn/NeuralNetworkLayer.hh>
#include <Math/Matrix.hh>

class NeuralNetworkLayer : public Nn::NeuralNetworkLayer<f64> {
public:
    NeuralNetworkLayer(const Core::Configuration &config) : Core::Component(config), Nn::NeuralNetworkLayer<f64>(config) {}
    virtual void forward(const std::vector<NnMatrix*>& input, NnMatrix& output) {}
};


class TestNeuralNetworkLayer : public Test::ConfigurableFixture
{
public:
    NeuralNetworkLayer *layer_;
    void setUp();
    void tearDown() {}
};

void TestNeuralNetworkLayer::setUp() {
    setParameter("*.statistics-smoothing-method", "exponential-trace");
    layer_ = new NeuralNetworkLayer(config);
}

TEST_F(Test, TestNeuralNetworkLayer, finalizeForwarding) {
    Nn::NeuralNetworkLayer<f64>::NnMatrix batch1;
    Nn::NeuralNetworkLayer<f64>::NnMatrix batch2;
    batch1.resize(2,3);
    batch1.at(0,0) = 1.0; batch1.at(1,0) = -2.0;
    batch1.at(0,1) = -2.0; batch1.at(1,1) = 3.0;
    batch1.at(0,2) = 2.0; batch1.at(1,2) = 0.0;
    batch1.initComputation();
    batch2.resize(2,3);
    batch2.at(0,0) = -2.0; batch2.at(1,0) = 1.0;
    batch2.at(0,1) = 2.0; batch2.at(1,1) = -3.0;
    batch2.at(0,2) = 1.0; batch2.at(1,2) = -1.0;
    batch2.initComputation();

    layer_->finalizeForwarding(batch1);

    layer_->getActivationVariance().finishComputation();
    layer_->getActivationMean().finishComputation();
    EXPECT_DOUBLE_EQ(1.0/3.0, layer_->getActivationMean().at(0), 0.00001);
    EXPECT_DOUBLE_EQ(1.0/3.0, layer_->getActivationMean().at(1), 0.00001);
    EXPECT_DOUBLE_EQ(26.0/9.0, layer_->getActivationVariance().at(0), 0.00001);
    EXPECT_DOUBLE_EQ(38.0/9.0, layer_->getActivationVariance().at(1), 0.00001);

    layer_->getActivationMean().initComputation();
    layer_->getActivationVariance().initComputation();
    layer_->finalizeForwarding(batch2);

    layer_->getActivationVariance().finishComputation();
    layer_->getActivationMean().finishComputation();
    layer_->getActivationSumOfSquares().finishComputation();
    EXPECT_DOUBLE_EQ(1.0/3.0, layer_->getActivationMean().at(0), 0.00001);
    EXPECT_DOUBLE_EQ(-1.0/3.0, layer_->getActivationMean().at(1), 0.00001);
    EXPECT_DOUBLE_EQ(26.0/9.0, layer_->getActivationVariance().at(0), 0.00001);
    EXPECT_DOUBLE_EQ(35.0/9.0, layer_->getActivationVariance().at(1), 0.00001);
}

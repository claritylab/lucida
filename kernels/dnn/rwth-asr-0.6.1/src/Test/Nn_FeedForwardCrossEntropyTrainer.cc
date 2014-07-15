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
#include <Nn/FeedForwardTrainer.hh>
#include <Nn/Types.hh>
#include <Math/Matrix.hh>
#include <Math/CudaMatrix.hh>
#include <Math/CudaVector.hh>

class TestFeedForwardCrossEntropyTrainer : public Test::ConfigurableFixture
{
private:
    Math::Matrix<f64> weightsLayer1_;
    Math::Matrix<f64> weightsLayer2_;
public:
    Math::CudaVector<u32> *alignment_;
    Nn::Types<f64>::NnVector *weights_;
    Nn::FeedForwardCrossEntropyTrainer<f64> *trainer_;
    std::vector< Nn::Types<f64>::NnMatrix > inputStream_;
    void setUp();
    void tearDown();
};

void TestFeedForwardCrossEntropyTrainer::setUp() {
    setParameter("*.neural-network.links", "0->layer-1:0");
    setParameter("*.layer-1.layer-type", "linear");
    setParameter("*.layer-1.dimension-input", "2");
    setParameter("*.layer-1.dimension-output", "2");
    setParameter("*.layer-1.links", "0->layer-2:0");
    setParameter("*.layer-2.layer-type", "sigmoid");
    setParameter("*.layer-2.dimension-output", "2");
    setParameter("*.layer-2.links", "0->layer-3:0");
    setParameter("*.layer-3.layer-type", "linear");
    setParameter("*.layer-3.dimension-output", "2");
    setParameter("*.layer-3.links", "0->layer-4:0");
    setParameter("*.layer-4.layer-type", "softmax");
    setParameter("*.layer-4.dimension-output", "2");
    setParameter("*.estimator", "steepest-descent");
    setParameter("*.channel", "/dev/null");
    setParameter("*.statistics-channel","log-channel");
    setParameter("*.weighted-accumulation", "true");

    alignment_ = new Math::CudaVector<u32>(4);
    weights_ = new Nn::Types<f64>::NnVector(4);
    trainer_ = new Nn::FeedForwardCrossEntropyTrainer<f64>(config);
    Nn::Types<f64>::NnMatrix* input = new Nn::Types<f64>::NnMatrix(2,4);
    input->at(0,0) = 1.2; input->at(1,0) = 0.7; alignment_->at(0) = 0; weights_->at(0) = 0.5;
    input->at(0,1) = 0.5; input->at(1,1) = 1.0; alignment_->at(1) = 0; weights_->at(1) = 0.5;
    input->at(0,2) = -1.5; input->at(1,2) = 1.1; alignment_->at(2) = 1; weights_->at(2) = 1.0;
    input->at(0,3) = -0.3; input->at(1,3) = -0.7; alignment_->at(3) = 1; weights_->at(3) = 1.0;
    inputStream_.push_back(*input);
    trainer_->initializeTrainer(inputStream_[0].nColumns());
    // set layer weights
    bool statusLayer0 = trainer_->network().getLayer(0).isComputing();
    bool statusLayer2 = trainer_->network().getLayer(2).isComputing();
    trainer_->network().getLayer(0).finishComputation();
    trainer_->network().getLayer(2).finishComputation();
    trainer_->network().getLayer(0).getWeights(0)->at(0,0) = -1.7;
    trainer_->network().getLayer(0).getWeights(0)->at(0,1) = 0.3;
    trainer_->network().getLayer(0).getWeights(0)->at(1,0) = -0.3;
    trainer_->network().getLayer(0).getWeights(0)->at(1,1) = 0.9;
    trainer_->network().getLayer(0).getBias()->at(0) = 0.5;
    trainer_->network().getLayer(0).getBias()->at(1) = 0.7;
    trainer_->network().getLayer(2).getWeights(0)->at(0,0) = 0.4;
    trainer_->network().getLayer(2).getWeights(0)->at(0,1) = -0.2;
    trainer_->network().getLayer(2).getWeights(0)->at(1,0) = 0.6;
    trainer_->network().getLayer(2).getWeights(0)->at(1,1) = -0.1;
    trainer_->network().getLayer(2).getBias()->at(0) = 1.2;
    trainer_->network().getLayer(2).getBias()->at(1) = -0.5;
    if (statusLayer0)
	trainer_->network().getLayer(0).initComputation();
    if (statusLayer2)
	trainer_->network().getLayer(2).initComputation();
}

void TestFeedForwardCrossEntropyTrainer::tearDown() {
    delete trainer_;
    delete alignment_;
}

TEST_F(Test, TestFeedForwardCrossEntropyTrainer, processBatch)
{
    trainer_->processBatch(inputStream_, *alignment_, *weights_);
    trainer_->network().finishComputation();
    trainer_->statistics().finishComputation();

    // check result of gradient
    EXPECT_DOUBLE_EQ(-0.122134 / -4, trainer_->statistics().gradientBias(0).at(0), 0.00001);
    EXPECT_DOUBLE_EQ(-0.269484 / -4, trainer_->statistics().gradientBias(0).at(1), 0.00001);
    EXPECT_DOUBLE_EQ(0.0849118 / -4, trainer_->statistics().gradientWeights(0)[0].at(0,0), 0.00001);
    EXPECT_DOUBLE_EQ(0.227247 / -4, trainer_->statistics().gradientWeights(0)[0].at(0,1), 0.00001);
    EXPECT_DOUBLE_EQ(0.0396304 / -4, trainer_->statistics().gradientWeights(0)[0].at(1,0), 0.00001);
    EXPECT_DOUBLE_EQ(-0.00703717 / -4, trainer_->statistics().gradientWeights(0)[0].at(1,1), 0.00001);

    EXPECT_DOUBLE_EQ(-1.78759 / -4, trainer_->statistics().gradientBias(2).at(0), 0.00001);
    EXPECT_DOUBLE_EQ(1.78759 / -4, trainer_->statistics().gradientBias(2).at(1), 0.00001);
    EXPECT_DOUBLE_EQ(-1.57948 / -4, trainer_->statistics().gradientWeights(2)[0].at(0,0), 0.00001);
    EXPECT_DOUBLE_EQ(1.57948 / -4, trainer_->statistics().gradientWeights(2)[0].at(0,1), 0.00001);
    EXPECT_DOUBLE_EQ(-1.12112 / -4, trainer_->statistics().gradientWeights(2)[0].at(1,0), 0.00001);
    EXPECT_DOUBLE_EQ(1.12112 / -4, trainer_->statistics().gradientWeights(2)[0].at(1,1), 0.00001);

    // check result of classification error
    EXPECT_DOUBLE_EQ(0.5, trainer_->statistics().classificationError(), 0);

    // check result of objective function
    EXPECT_DOUBLE_EQ(1.38401, trainer_->statistics().objectiveFunction(), 0.00001);

    // check result of estimation
    Nn::LinearLayer<f64>* layer = &( dynamic_cast< Nn::LinearLayer<f64>& > (trainer_->network().getLayer(0)) );
    EXPECT_DOUBLE_EQ(0.469467, layer->getBias()->at(0), 0.00001);
    EXPECT_DOUBLE_EQ(0.632629, layer->getBias()->at(1), 0.00001);
    EXPECT_DOUBLE_EQ(-1.67877, layer->getWeights(0)->at(0,0), 0.00001);
    EXPECT_DOUBLE_EQ(0.356812, layer->getWeights(0)->at(0,1), 0.00001);
    EXPECT_DOUBLE_EQ(-0.290092, layer->getWeights(0)->at(1,0), 0.00001);
    EXPECT_DOUBLE_EQ(0.898241, layer->getWeights(0)->at(1,1), 0.00001);

    layer = &( dynamic_cast< Nn::LinearLayer<f64>& > (trainer_->network().getLayer(2)) );
    EXPECT_DOUBLE_EQ(0.753102, layer->getBias()->at(0), 0.00001);
    EXPECT_DOUBLE_EQ(-0.0531016, layer->getBias()->at(1), 0.00001);
    EXPECT_DOUBLE_EQ(0.00513122, layer->getWeights(0)->at(0,0), 0.00001);
    EXPECT_DOUBLE_EQ(0.194869, layer->getWeights(0)->at(0,1), 0.00001);
    EXPECT_DOUBLE_EQ(0.31972, layer->getWeights(0)->at(1,0), 0.00001);
    EXPECT_DOUBLE_EQ(0.18028, layer->getWeights(0)->at(1,1), 0.00001);
}

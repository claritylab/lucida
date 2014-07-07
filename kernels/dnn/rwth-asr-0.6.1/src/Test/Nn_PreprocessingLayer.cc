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
#include <Nn/PreprocessingLayer.hh>


class TestPreprocessingLayer : public Test::ConfigurableFixture
{
    typedef Nn::NeuralNetworkLayer<f64>::NnMatrix NnMatrix;
    typedef Nn::NeuralNetworkLayer<f64>::NnVector NnVector;
public:
    void setUp();
    void tearDown();
protected:
    u32 featureDim_;
    u32 batchSize_;
    std::vector<NnMatrix*> input_;
    NnMatrix output_;
};

class MeanAndVarianceNormalizationPreprocessingLayer : public Nn::MeanAndVarianceNormalizationPreprocessingLayer<f64> {
public:
    using Nn::MeanAndVarianceNormalizationPreprocessingLayer<f64>::mean_;
    using Nn::MeanAndVarianceNormalizationPreprocessingLayer<f64>::standardDeviation_;
public:
    MeanAndVarianceNormalizationPreprocessingLayer(const Core::Configuration &c) :
	Core::Component(c),
	Nn::NeuralNetworkLayer<f64>(c),
	Nn::MeanAndVarianceNormalizationPreprocessingLayer<f64>(c)
	{
	this->needInit_ = false;
	}
};

void TestPreprocessingLayer::setUp()
{
    featureDim_ = 3;
    batchSize_ = 2;
    input_.resize(1);
    input_.at(0) = new NnMatrix(featureDim_, batchSize_);
    output_.resize(featureDim_, batchSize_);
    output_.initComputation();
    setParameter("*.dimension-input", "3");
}

void TestPreprocessingLayer::tearDown()
{
    for (u32 layer = 0; layer < input_.size(); ++layer)
	delete input_.at(layer);
}

TEST_F(Test, TestPreprocessingLayer, logarithm)
{
    setParameter("*.dimension-output", "3");
    Nn::NeuralNetworkLayer<f64> *layer = new Nn::LogarithmPreprocessingLayer<f64>(config);
    layer->setInputActivationIndex(0,0);
    input_.at(0)->initComputation(false);
    input_.at(0)->fill(1.0);
    input_.at(0)->finishComputation();
    input_.at(0)->at(0,0) = 2.0;
    input_.at(0)->at(2,1) = 5.0;
    input_.at(0)->initComputation();

    layer->forward(input_,output_);
    output_.finishComputation();

    EXPECT_DOUBLE_EQ(std::log(2.0), output_.at(0,0), 0.000001);
    EXPECT_DOUBLE_EQ(0.0, output_.at(1,0), 0.000001);
    EXPECT_DOUBLE_EQ(0.0, output_.at(2,0), 0.000001);
    EXPECT_DOUBLE_EQ(0.0, output_.at(0,1), 0.000001);
    EXPECT_DOUBLE_EQ(0.0, output_.at(1,1), 0.000001);
    EXPECT_DOUBLE_EQ(std::log(5.0), output_.at(2,1), 0.000001);

    delete layer;

}

TEST_F(Test, TestPreprocessingLayer, meanAndVariance)
{
    setParameter("*.dimension-output", "3");
    MeanAndVarianceNormalizationPreprocessingLayer *normalizationLayer = new MeanAndVarianceNormalizationPreprocessingLayer(config);
    normalizationLayer->mean_.resize(3);
    normalizationLayer->standardDeviation_.resize(3);
    normalizationLayer->mean_.at(0) = 1.0;
    normalizationLayer->mean_.at(1) = 2.0;
    normalizationLayer->mean_.at(2) = -2.0;
    normalizationLayer->standardDeviation_.at(0) = 1.0;
    normalizationLayer->standardDeviation_.at(1) = 0.5;
    normalizationLayer->standardDeviation_.at(2) = -0.5;
    normalizationLayer->mean_.initComputation();
    normalizationLayer->standardDeviation_.initComputation();
    Nn::NeuralNetworkLayer<f64> *layer = normalizationLayer;
    layer->setInputActivationIndex(0,0);
    input_.at(0)->initComputation(false);
    input_.at(0)->fill(1.0);
    input_.at(0)->finishComputation();
    input_.at(0)->at(0,0) = 2.0;
    input_.at(0)->at(2,1) = 5.0;
    input_.at(0)->initComputation();

    layer->forward(input_,output_);
    output_.finishComputation();

    EXPECT_DOUBLE_EQ(1.0, output_.at(0,0), 0.000001);
    EXPECT_DOUBLE_EQ(-2.0, output_.at(1,0), 0.000001);
    EXPECT_DOUBLE_EQ(-6.0, output_.at(2,0), 0.000001);
    EXPECT_DOUBLE_EQ(0.0, output_.at(0,1), 0.000001);
    EXPECT_DOUBLE_EQ(-2.0, output_.at(1,1), 0.000001);
    EXPECT_DOUBLE_EQ(-14.0, output_.at(2,1), 0.000001);

    delete layer;
}

TEST_F(Test, TestPreprocessingLayer, polynomial2nd)
{
    setParameter("*.order", "2");
    setParameter("*.dimension-input", "2");
    setParameter("*.dimension-output", "5");
    input_.at(0)->resize(2, 2);
    output_.resize(5, 2);
    input_.at(0)->at(0,0) = 1.0;
    input_.at(0)->at(1,0) = 2.0;
    input_.at(0)->at(0,1) = -1.0;
    input_.at(0)->at(1,1) = 2.0;
    Nn::NeuralNetworkLayer<f64> *layer = new Nn::PolynomialPreprocessingLayer<f64>(config);
    layer->setInputActivationIndex(0,0);
    input_.at(0)->initComputation();

    layer->forward(input_,output_);
    output_.finishComputation();

    EXPECT_DOUBLE_EQ(1.0, output_.at(0,0), 0.000001);
    EXPECT_DOUBLE_EQ(2.0, output_.at(1,0), 0.000001);
    EXPECT_DOUBLE_EQ(1.0, output_.at(2,0), 0.000001);
    EXPECT_DOUBLE_EQ(2.0, output_.at(3,0), 0.000001);
    EXPECT_DOUBLE_EQ(4.0, output_.at(4,0), 0.000001);

    EXPECT_DOUBLE_EQ(-1.0, output_.at(0,1), 0.000001);
    EXPECT_DOUBLE_EQ(2.0, output_.at(1,1), 0.000001);
    EXPECT_DOUBLE_EQ(1.0, output_.at(2,1), 0.000001);
    EXPECT_DOUBLE_EQ(-2.0, output_.at(3,1), 0.000001);
    EXPECT_DOUBLE_EQ(4.0, output_.at(4,1), 0.000001);
    delete layer;
}

TEST_F(Test, TestPreprocessingLayer, polynomial3rd)
{
    setParameter("*.order", "3");
    setParameter("*.dimension-input", "2");
    setParameter("*.dimension-output", "9");
    input_.at(0)->resize(2, 2);
    output_.resize(9, 2);
    input_.at(0)->at(0,0) = 1.0;
    input_.at(0)->at(1,0) = 2.0;
    input_.at(0)->at(0,1) = -1.0;
    input_.at(0)->at(1,1) = 2.0;
    Nn::NeuralNetworkLayer<f64> *layer = new Nn::PolynomialPreprocessingLayer<f64>(config);
    layer->setInputActivationIndex(0,0);
    input_.at(0)->initComputation();

    layer->forward(input_,output_);
    output_.finishComputation();

    EXPECT_EQ(1.0, output_.at(0,0));
    EXPECT_EQ(2.0, output_.at(1,0));
    EXPECT_EQ(1.0, output_.at(2,0));
    EXPECT_EQ(2.0, output_.at(3,0));
    EXPECT_EQ(4.0, output_.at(4,0));
    EXPECT_EQ(1.0, output_.at(5,0));
    EXPECT_EQ(2.0, output_.at(6,0));
    EXPECT_EQ(4.0, output_.at(7,0));
    EXPECT_EQ(8.0, output_.at(8,0));


    EXPECT_EQ(-1.0, output_.at(0,1) );
    EXPECT_EQ(2.0, output_.at(1,1) );
    EXPECT_EQ(1.0, output_.at(2,1) );
    EXPECT_EQ(-2.0, output_.at(3,1) );
    EXPECT_EQ(4.0, output_.at(4,1) );
    EXPECT_EQ(-1.0, output_.at(5,1));
    EXPECT_EQ(2.0, output_.at(6,1));
    EXPECT_EQ(-4.0, output_.at(7,1));
    EXPECT_EQ(8.0, output_.at(8,1));

    delete layer;
}

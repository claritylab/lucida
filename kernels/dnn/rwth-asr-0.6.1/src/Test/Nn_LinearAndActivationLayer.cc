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
#include <Nn/LinearAndActivationLayer.hh>
#include <Nn/LinearLayer.hh>
#include <Math/Matrix.hh>

template<typename T>
class LinearAndSigmoidLayer : public Nn::LinearAndSigmoidLayer<T> {
    typedef Nn::LinearAndSigmoidLayer<T> Precursor;
public:
    using Precursor::setParameters;
    LinearAndSigmoidLayer(const Core::Configuration &c) : Core::Component(c), Nn::NeuralNetworkLayer<T>(c), Precursor(c) {}
};

template<typename T>
class LinearAndSoftmaxLayer : public Nn::LinearAndSoftmaxLayer<T> {
    typedef Nn::LinearAndSoftmaxLayer<T> Precursor;
public:
    using Precursor::setParameters;
    LinearAndSoftmaxLayer(const Core::Configuration &c) : Core::Component(c), Nn::NeuralNetworkLayer<T>(c), Precursor(c) {}
};


class TestLinearAndSigmoidLayer : public Test::ConfigurableFixture
{
    typedef Nn::NeuralNetworkLayer<f64>::NnVector NnVector;
    typedef Nn::NeuralNetworkLayer<f64>::NnMatrix NnMatrix;
public:
    void setUp();
    void tearDown();
    void initInputAndOutputAndParameters(std::vector<NnMatrix*> &input, NnMatrix& output,
	    Math::Matrix<f64> &parameter);
protected:
    Nn::NeuralNetworkLayer<f64> *layer_;
    std::vector<NnMatrix*> input_;
    NnMatrix* output_;
};

void TestLinearAndSigmoidLayer::setUp()
{
    LinearAndSigmoidLayer<f64> *layer = new LinearAndSigmoidLayer<f64>(config);
    output_ = new NnMatrix();
    Math::Matrix<f64> parameter;
    initInputAndOutputAndParameters(input_, *output_, parameter);
    layer->setInputActivationIndex(0, 0);
    layer->setInputDimension(0, parameter.nColumns() - 1);
    layer->setOutputDimension(parameter.nRows());
    layer->setParameters(parameter);
    layer_ = layer;
}

void TestLinearAndSigmoidLayer::tearDown()
{
    delete layer_;
    delete output_;
    for (u32 layer = 0; layer < input_.size(); ++layer)
	delete input_.at(layer);
}

void TestLinearAndSigmoidLayer::initInputAndOutputAndParameters(
	std::vector<NnMatrix*> &input, NnMatrix &output, Math::Matrix<f64> &parameter) {
    NnMatrix *inputMatrix = new NnMatrix(3,2);
    output.resize(3,2);
    (*inputMatrix).at(0,0) = 2.0; (*inputMatrix).at(1,0) = 2.5; (*inputMatrix).at(2,0) = 3.0;
    (*inputMatrix).at(0,1) = 1.0; (*inputMatrix).at(1,1) = 0.5; (*inputMatrix).at(2,1) = 1.5;
    input.push_back(inputMatrix);
    // define weights
    parameter.resize(3,4);
    parameter[0][0] = 0.1; parameter[0][1] = 0.3; parameter[0][2] = 0.5; parameter[0][3] = 0.7;
    parameter[1][0] = 0.2; parameter[1][1] = 0.4; parameter[1][2] = 0.6; parameter[1][3] = 0.8;
    parameter[2][0] = 0.0; parameter[2][1] = 0.3; parameter[2][2] = 0.6; parameter[2][3] = 0.9;
}

//=============================================================================

class TestLinearAndSoftmaxLayer : public Test::ConfigurableFixture
{
    typedef Nn::NeuralNetworkLayer<f64>::NnVector NnVector;
    typedef Nn::NeuralNetworkLayer<f64>::NnMatrix NnMatrix;
public:
    void setUp();
    void tearDown();
protected:
    LinearAndSoftmaxLayer<f64> *layer_;
    std::vector<NnMatrix*> input_;
    NnMatrix* output_;
};

void TestLinearAndSoftmaxLayer::setUp()
{
    LinearAndSoftmaxLayer<f64> *layer = new LinearAndSoftmaxLayer<f64>(config);
    output_ = new NnMatrix();
    layer_ = layer;
    Math::Matrix<f64> parameter;
    TestLinearAndSigmoidLayer tmp;
    tmp.initInputAndOutputAndParameters(input_, *output_, parameter);
    layer->setInputActivationIndex(0, 0);
    layer->setInputDimension(0, parameter.nColumns() - 1);
    layer->setOutputDimension(parameter.nRows());
    layer->setParameters(parameter);
}

void TestLinearAndSoftmaxLayer::tearDown()
{
    delete layer_;
    delete output_;
    for (u32 layer = 0; layer < input_.size(); ++layer)
	delete input_.at(layer);
}

//=============================================================================

TEST_F(Test, TestLinearAndSigmoidLayer, forward)
{
    layer_->initComputation();
    input_[0]->initComputation();
    output_->initComputation();
    layer_->forward(input_,*output_);
    output_->finishComputation();

    // linear output (without sigmoid): (4.05,4.9,4.8), (1.7,2.1,1.95)

    EXPECT_DOUBLE_EQ(0.98287596668427235, output_->at(0,0), 0.000001);
    EXPECT_DOUBLE_EQ(0.99260845865571812, output_->at(1,0), 0.000001);
    EXPECT_DOUBLE_EQ(0.99183742884684012, output_->at(2,0), 0.000001);

    EXPECT_DOUBLE_EQ(0.84553473491646525, output_->at(0,1), 0.000001);
    EXPECT_DOUBLE_EQ(0.89090317880438707, output_->at(1,1), 0.000001);
    EXPECT_DOUBLE_EQ(0.87544664181258358, output_->at(2,1), 0.000001);
}

//=============================================================================

TEST_F(Test, TestLinearAndSoftmaxLayer, forward)
{
    layer_->initComputation();
    input_[0]->initComputation();
    output_->initComputation();
    layer_->forward(input_,*output_);
    output_->finishComputation();

    // linear output (without softmax): (4.05,4.9,4.8), (1.7,2.1,1.95)

    EXPECT_DOUBLE_EQ(0.18326272967482829, output_->at(0,0), 0.000001);
    EXPECT_DOUBLE_EQ(0.42877006855907612, output_->at(1,0), 0.000001);
    EXPECT_DOUBLE_EQ(0.38796720176609562, output_->at(2,0), 0.000001);

    EXPECT_DOUBLE_EQ(0.26484102115311464, output_->at(0,1), 0.000001);
    EXPECT_DOUBLE_EQ(0.39509637630475053, output_->at(1,1), 0.000001);
    EXPECT_DOUBLE_EQ(0.34006260254213494, output_->at(2,1), 0.000001);
}

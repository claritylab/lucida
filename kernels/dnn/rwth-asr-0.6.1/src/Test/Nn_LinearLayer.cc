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
#include <Core/Directory.hh>
#include <cstdlib>
#include <Nn/NeuralNetwork.hh>
#include <Nn/LinearLayer.hh>
#include <Math/Matrix.hh>

template<typename T>
class LinearLayer : public Nn::LinearLayer<T> {
    typedef Nn::LinearLayer<T> Precursor;
public:
    using Precursor::setParameters;
    LinearLayer(const Core::Configuration &c) : Core::Component(c), Nn::NeuralNetworkLayer<T>(c), Precursor(c) {}
};

class TestLinearLayer : public Test::ConfigurableFixture
{
public:
    Nn::NeuralNetworkLayer<f64>* layer_;
    void setUp();
    void tearDown();
};

void TestLinearLayer::setUp() {
    Math::Matrix<f64> params(3,3);
    // bias
    params[0][0] = 0.0;
    params[1][0] = 1.0;
    params[2][0] = -1.0;
    // weights
    params[0][1] = 1.0;
    params[1][1] = 2.0;
    params[2][1] = 3.0;
    params[0][2] = -1.0;
    params[1][2] = -2.0;
    params[2][2] = -3.0;

    LinearLayer<f64> *layer = new LinearLayer<f64>(config);
    layer_ = layer;
    layer_->setInputActivationIndex(0, 0);
    layer_->setInputActivationIndex(0,0);
    layer_->setInputDimension(0,2);
    layer_->setOutputDimension(3);
    layer->setParameters(params);
}

void TestLinearLayer::tearDown() {}

TEST_F(Test, TestLinearLayer, IOBin) {
    setParameter("*.channel", "/dev/null");
    char *t = std::getenv("TMPDIR");
    std::string tmpdir;
    if (t)
	tmpdir = t;
    else
	tmpdir = "/tmp";
    std::string dirName = tmpdir + "/XXXX/";
    Core::createDirectory(dirName);
    std::string filename = "bin:" + dirName + "model";
    layer_->saveNetworkParameters(filename);
    Nn::NeuralNetworkLayer<f64> *layer2 = new LinearLayer<f64>(config);
    layer2->setInputActivationIndex(0, 0);
    layer2->setInputDimension(0,2);
    layer2->setOutputDimension(3);
    layer2->loadNetworkParameters(filename);

    EXPECT_EQ(layer_->getWeights(0)->nRows(), layer2->getWeights(0)->nRows());
    EXPECT_EQ(layer_->getWeights(0)->nColumns(), layer2->getWeights(0)->nColumns());
    EXPECT_EQ(layer_->getBias()->nRows(), layer2->getBias()->nRows());
    for (u32 i = 0; i < layer_->getWeights(0)->nRows(); i++) {
	for (u32 j = 0; j < layer_->getWeights(0)->nColumns(); j++) {
	    EXPECT_EQ(layer_->getWeights(0)->at(i,j), layer2->getWeights(0)->at(i,j));
	}
    }
    for (u32 i = 0; i < layer_->getBias()->nRows(); i++) {
	EXPECT_EQ(layer_->getBias()->at(i), layer2->getBias()->at(i));
    }
    delete layer2;
    Core::removeDirectory(dirName);
}

TEST_F(Test, TestLinearLayer, IOXml) {
    setParameter("*.channel", "/dev/null");
    char *t = std::getenv("TMPDIR");
    std::string tmpdir;
    if (t)
	tmpdir = t;
    else
	tmpdir = "/tmp";
    std::string dirName = tmpdir + "/XXXX/";
    Core::createDirectory(dirName);
    std::string filename = "xml:" + dirName + "model";
    layer_->saveNetworkParameters(filename);
    Nn::NeuralNetworkLayer<f64> *layer2 = new LinearLayer<f64>(config);
    layer2->setInputActivationIndex(0,0);
    layer2->setInputDimension(0,2);
    layer2->setOutputDimension(3);
    layer2->loadNetworkParameters(filename);

    EXPECT_EQ(layer_->getWeights(0)->nRows(), layer2->getWeights(0)->nRows());
    EXPECT_EQ(layer_->getWeights(0)->nColumns(), layer2->getWeights(0)->nColumns());
    EXPECT_EQ(layer_->getBias()->nRows(), layer2->getBias()->nRows());
    for (u32 i = 0; i < layer_->getWeights(0)->nRows(); i++) {
	for (u32 j = 0; j < layer_->getWeights(0)->nColumns(); j++) {
	    EXPECT_EQ(layer_->getWeights(0)->at(i,j), layer2->getWeights(0)->at(i,j));
	}
    }
    for (u32 i = 0; i < layer_->getBias()->nRows(); i++) {
	EXPECT_EQ(layer_->getBias()->at(i), layer2->getBias()->at(i));
    }
    delete layer2;
    Core::removeDirectory(dirName);
}

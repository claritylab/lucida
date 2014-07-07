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
#include <Nn/NeuralNetworkTrainer.hh>
#include <Nn/Types.hh>

class TestNeuralNetworkTrainer : public Test::ConfigurableFixture {
public:
    void setUp();
    void tearDown();
};

class MeanAndVarianceTrainer : public Nn::MeanAndVarianceTrainer<f32> {
    typedef Nn::MeanAndVarianceTrainer<f32> Precursor;
public:
    using Precursor::statistics_;
public:
    MeanAndVarianceTrainer(const Core::Configuration &c) :
	Core::Component(c),
	Precursor(c) {}
};

void TestNeuralNetworkTrainer::setUp() {
    setParameter("*.channel", "/dev/null");
    setParameter("*.weighted-accumulation", "true");
}
void TestNeuralNetworkTrainer::tearDown() {}

TEST_F(Test, TestNeuralNetworkTrainer, MeanAndVarianceTrainer_processBatch) {
    MeanAndVarianceTrainer trainer(config);
    std::vector<Nn::Types<f32>::NnMatrix > features(1);
    features.at(0).resize(2,3);
    Nn::Types<f32>::NnVector weights(3);
    Nn::Types<u32>::NnVector alignment(3);
    features.at(0).at(0,0) = 1.0;
    features.at(0).at(0,1) = 2.0;
    features.at(0).at(0,2) = 0.0;
    features.at(0).at(1,0) = -3.0;
    features.at(0).at(1,1) = -2.0;
    features.at(0).at(1,2) = -1.0;
    weights.at(0) = 1.0;
    weights.at(1) = 0.5;
    weights.at(2) = 0.5;
    features.at(0).initComputation();
    weights.initComputation();

    std::vector<u32> streamSizes(1, 2);
    trainer.initializeTrainer(3, streamSizes);

    trainer.processBatch(features, alignment, weights);

    trainer.statistics_->finishComputation();
    EXPECT_EQ(3u, trainer.statistics_->nObservations());
    EXPECT_EQ(2.0f, trainer.statistics_->totalWeight());
    EXPECT_EQ(2.0f, trainer.statistics_->featureSum().at(0));
    EXPECT_EQ(-4.5f, trainer.statistics_->featureSum().at(1));
    EXPECT_EQ(3.0f, trainer.statistics_->squaredFeatureSum().at(0));
    EXPECT_EQ(11.5f, trainer.statistics_->squaredFeatureSum().at(1));
}

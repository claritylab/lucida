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
#include <Nn/Statistics.hh>

class TestStatistics : public Test::ConfigurableFixture
{
public:
    Nn::Statistics<f64>* statistics_;
    Nn::Statistics<f64>* statistics2_;
    Nn::NeuralNetwork<f64> *neuralNetwork_;
    void setUp();
    void tearDown();
};

void TestStatistics::setUp() {
    setParameter("*.neural-network.links", "0->layer-1:0");
    setParameter("*.layer-1.layer-type", "linear+sigmoid");
    setParameter("*.layer-1.dimension-input", "2");
    setParameter("*.layer-1.dimension-output", "3");
    setParameter("*.layer-1.links", "0->layer-2:0");
    setParameter("*.layer-2.layer-type", "linear+softmax");
    setParameter("*.layer-2.dimension-input", "3");
    setParameter("*.layer-2.dimension-output", "5");
    setParameter("*.channel", "/dev/null");
    neuralNetwork_ = new Nn::NeuralNetwork<f64>(config);
    neuralNetwork_->initializeNetwork(1);
    statistics_ = new Nn::Statistics<f64>(2,
	    Nn::Statistics<f64>::GRADIENT | Nn::Statistics<f64>::CLASS_COUNTS | Nn::Statistics<f64>::BASE_STATISTICS);
    statistics_->initialize(*neuralNetwork_);

    statistics_->incClassCount(2,4);
    statistics_->incClassCount(7,3);
    statistics_->incClassificationErrors(10);
    statistics_->incObservations(25);
    statistics_->addToTotalWeight(20.0);
    statistics_->addToObjectiveFunction(1.25);

    for (u32 j = 0; j < 3; j++) {
	for (u32 i = 0; i < 2; i++) {
	    statistics_->gradientWeights(0)[0].at(i,j) = i+j;
	}
	statistics_->gradientBias(0).at(j) = j;
    }
    for (u32 j = 0; j < 5; j++) {
	for (u32 i = 0; i < 3; i++) {
	    statistics_->gradientWeights(1)[0].at(i,j) = i+j+1;
	}
	statistics_->gradientBias(1).at(j) = j+1;
    }
}

void TestStatistics::tearDown() {
}

TEST_F(Test, TestStatistics, IOBin) {
    char *t = std::getenv("TMPDIR");
    std::string tmpdir;
    if (t)
	tmpdir = t;
    else
	tmpdir = "/tmp";
    std::string dirName = tmpdir + "/XXXX/";
    Core::createDirectory(dirName);
    std::string filename = "bin:" + dirName + "statistics";
    statistics_->finishComputation();
    statistics_->write(filename);
    Nn::Statistics<f64> newStatistics(*statistics_, true);
    newStatistics.finishComputation();
    newStatistics.read(filename);
    EXPECT_EQ(statistics_->objectiveFunction(), newStatistics.objectiveFunction());
    EXPECT_EQ(statistics_->classificationError(), newStatistics.classificationError());
    EXPECT_EQ(statistics_->nObservations(), newStatistics.nObservations());
    EXPECT_EQ(statistics_->totalWeight(), newStatistics.totalWeight());

    for (u32 i = 0; i < 2; i++) {
	for (u32 j = 0; j < 3; j++) {
	    EXPECT_EQ(statistics_->gradientWeights(0)[0].at(i,j), newStatistics.gradientWeights(0)[0].at(i,j));
	}
	EXPECT_EQ(statistics_->gradientBias(0).at(i), newStatistics.gradientBias(0).at(i));
    }
    for (u32 i = 0; i < 3; i++) {
	for (u32 j = 0; j < 5 ; j++) {
	    EXPECT_EQ(statistics_->gradientWeights(1)[0].at(i,j), newStatistics.gradientWeights(1)[0].at(i,j));
	}
	EXPECT_EQ(statistics_->gradientBias(1).at(i), newStatistics.gradientBias(1).at(i));
    }
    EXPECT_EQ(statistics_->classCount(2), newStatistics.classCount(2));
    EXPECT_EQ(statistics_->classCount(7), newStatistics.classCount(7));

    // read statistics twice (poor test, but better than nothing)
    std::vector<std::string> filenames;
    filenames.push_back(filename);
    filenames.push_back(filename);
    newStatistics.combine(filenames);
    EXPECT_EQ(2*statistics_->objectiveFunction(), newStatistics.objectiveFunction());
    EXPECT_EQ(statistics_->classificationError(), newStatistics.classificationError());
    EXPECT_EQ(2*statistics_->nObservations(), newStatistics.nObservations());

    for (u32 i = 0; i < 2; i++) {
	for (u32 j = 0; j < 3; j++) {
	    EXPECT_EQ(2*statistics_->gradientWeights(0)[0].at(i,j), newStatistics.gradientWeights(0)[0].at(i,j));
	}
	EXPECT_EQ(2*statistics_->gradientBias(0).at(i), newStatistics.gradientBias(0).at(i));
    }
    for (u32 i = 0; i < 3; i++) {
	for (u32 j = 0; j < 5 ; j++) {
	    EXPECT_EQ(2*statistics_->gradientWeights(1)[0].at(i,j), newStatistics.gradientWeights(1)[0].at(i,j));
	}
	EXPECT_EQ(2*statistics_->gradientBias(1).at(i), newStatistics.gradientBias(1).at(i));
    }
    EXPECT_EQ(2*statistics_->classCount(2), newStatistics.classCount(2));
    EXPECT_EQ(2*statistics_->classCount(7), newStatistics.classCount(7));

    Core::removeDirectory(dirName);
}

TEST_F(Test, TestStatistics, add) {
    char *t = std::getenv("TMPDIR");
    std::string tmpdir;
    if (t)
	tmpdir = t;
    else
	tmpdir = "/tmp";
    std::string dirName = tmpdir + "/XXXX/";
    Core::createDirectory(dirName);
    std::string filename1 = "bin:" + dirName + "statistics1";
    std::string filename2 = "bin:" + dirName + "statistics2";
    std::vector<std::string> filenames;
    filenames.push_back(filename1);
    filenames.push_back(filename2);
    statistics_->finishComputation();
    statistics_->write(filename1);
    statistics_->initComputation();
    statistics_->gradientWeights(0)[0].scale(2.0);
    statistics_->gradientWeights(1)[0].scale(3.0);
    statistics_->gradientBias(0).scale(-2.0);
    statistics_->incClassCount(0,10);
    statistics_->finishComputation();
    statistics_->write(filename2);
    statistics_->read(filename1);

    Nn::Statistics<f64> newStatistics(*statistics_, true);
    newStatistics.combine(filenames);

    EXPECT_EQ(2*statistics_->objectiveFunction(), newStatistics.objectiveFunction());
    EXPECT_EQ(statistics_->classificationError(), newStatistics.classificationError());
    EXPECT_EQ(2*statistics_->nObservations(), newStatistics.nObservations());
    for (u32 i = 0; i < 2; i++) {
	for (u32 j = 0; j < 2; j++) {
	    EXPECT_EQ(3*statistics_->gradientWeights(0)[0].at(i,j), newStatistics.gradientWeights(0)[0].at(i,j));
	    EXPECT_EQ(4*statistics_->gradientWeights(1)[0].at(i,j), newStatistics.gradientWeights(1)[0].at(i,j));
	}
	EXPECT_EQ(-statistics_->gradientBias(0).at(i), newStatistics.gradientBias(0).at(i));
	EXPECT_EQ(2*statistics_->gradientBias(1).at(i), newStatistics.gradientBias(1).at(i));
    }
    EXPECT_EQ(newStatistics.classCount(0), 10u);
    EXPECT_EQ(newStatistics.classCount(2), 8u);
    EXPECT_EQ(newStatistics.classCount(7), 6u);

    Core::removeDirectory(dirName);




}

TEST_F(Test, TestStatistics, combineMeanAndVariance) {
    char *t = std::getenv("TMPDIR");
    std::string tmpdir;
    if (t)
	tmpdir = t;
    else
	tmpdir = "/tmp";
    std::string dirName = tmpdir + "/XXXX/";
    Core::createDirectory(dirName);
    std::string filename1 = "bin:" + dirName + "statistics1";
    std::string filename2 = "bin:" + dirName + "statistics2";
    std::vector<std::string> filenames;
    filenames.push_back(filename1);
    filenames.push_back(filename2);

    delete statistics_;
    statistics_ = new Nn::Statistics<f64>(0,
	    Nn::Statistics<f64>::MEAN_AND_VARIANCE);
    statistics_->incObservations(3);
    statistics_->featureSum().resize(3);
    statistics_->squaredFeatureSum().resize(3);
    statistics_->featureSum().at(0) = -1;
    statistics_->featureSum().at(1) = 2;
    statistics_->featureSum().at(2) = 3;
    statistics_->squaredFeatureSum().at(0) = 1;
    statistics_->squaredFeatureSum().at(1) = 4;
    statistics_->squaredFeatureSum().at(2) = 9;
    statistics_->write(filename1);
    statistics_->write(filename2);

    Nn::Statistics<f64> newStatistics(*statistics_, true);
    newStatistics.combine(filenames);

    for (u32 i = 0; i < 3; i++) {
	EXPECT_EQ(2.0 * statistics_->featureSum().at(i), newStatistics.featureSum().at(i));
	EXPECT_EQ(2.0 * statistics_->squaredFeatureSum().at(i), newStatistics.squaredFeatureSum().at(i));
    }
    EXPECT_EQ(newStatistics.nObservations(), 2u * statistics_->nObservations());
    EXPECT_EQ(newStatistics.totalWeight(), 2.0 * statistics_->totalWeight());
    Core::removeDirectory(dirName);
}

TEST_F(Test, TestStatistics, finalizeMeanAndVariance) {
    if (statistics_)
	delete statistics_;
    statistics_ = new Nn::Statistics<f64>(0,
	    Nn::Statistics<f64>::MEAN_AND_VARIANCE);
    statistics_->incObservations(3);
    statistics_->addToTotalWeight(2.0);
    statistics_->featureSum().resize(1);
    statistics_->squaredFeatureSum().resize(1);
    statistics_->featureSum().at(0) = 2;
    statistics_->squaredFeatureSum().at(0) = 3;
    statistics_->initComputation();
    statistics_->finalize();
    statistics_->finishComputation();
    EXPECT_EQ(1.0, statistics_->featureSum().at(0));
    EXPECT_EQ(0.5, statistics_->squaredFeatureSum().at(0));
}

TEST_F(Test, TestStatistics, gradientNorm) {
    statistics_->initComputation();
    f64 result = statistics_->gradientL1Norm();
    EXPECT_EQ(87.0, result);
}

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
#include <Nn/BufferedAlignedFeatureProcessor.hh>
#include <Mm/Types.hh>

class BufferedAlignedFeatureProcessor : public Nn::BufferedAlignedFeatureProcessor<f32> {
public:
    typedef Nn::BufferedFeatureExtractor<f32> PrePrecursor;
    typedef Nn::BufferedAlignedFeatureProcessor<f32> Precursor;
    using PrePrecursor::featureBuffer_;
    using PrePrecursor::segmentIndexBuffer_;
    using PrePrecursor::nBufferedFeatures_;
    using PrePrecursor::batchSize_;
    using Precursor::acousticModelNeedInit_;
    using Precursor::alignmentBuffer_;
    using Precursor::classLabelWrapper_;
    using Precursor::initBuffer;
    using Precursor::generateMiniBatch;
    using Precursor::processAlignedFeature;
    using Precursor::alignmentWeightsBuffer_;
public:
    BufferedAlignedFeatureProcessor(const Core::Configuration &config)
    :  Core::Component(config),
       Precursor(config,false)
       {}

};


class TestBufferedAlignedFeatureProcessor : public Test::ConfigurableFixture
{
public:
    BufferedAlignedFeatureProcessor *trainer_;
    u32 nClasses_;
    Core::Ref<const Speech::Feature> feature1_;
    Core::Ref<const Speech::Feature> feature2_;
    Core::Ref<const Speech::Feature> feature3_;
    void setUp();
    void tearDown();
};

void TestBufferedAlignedFeatureProcessor::setUp(){
    setParameter("*.buffer-size", "3");
    setParameter("*.on-error", "ignore");
    setParameter("*.buffer-type", "batch");
    setParameter("*.channel", "nil");
    setParameter("*.disregard-classes", "5");
    setParameter("*.shuffle", "false");

    nClasses_ = 10;

    trainer_ = new BufferedAlignedFeatureProcessor(config);
    trainer_->acousticModelNeedInit_ = false;
    trainer_->classLabelWrapper_ = new Nn::ClassLabelWrapper(config, nClasses_);

    // input
    Flow::Vector<Mm::FeatureType> *vector1 = new Flow::Vector<Mm::FeatureType>(2);
    vector1->at(0) = 1.0;
    vector1->at(1) = 2.0;
    Flow::DataPtr<Flow::Vector<Mm::FeatureType> > dptr1(vector1);
    Core::Ref<const Speech::Feature> feature1(new Speech::Feature(dptr1));
    feature1_ = feature1;

    Flow::Vector<Mm::FeatureType> *vector2 = new Flow::Vector<Mm::FeatureType>(2);
    vector2->at(0) = 3.0;
    vector2->at(1) = 4.0;
    Flow::DataPtr<Flow::Vector<Mm::FeatureType> > dptr2(vector2);
    Core::Ref<const Speech::Feature> feature2(new Speech::Feature(dptr2));
    feature2_ = feature2;

    Flow::Vector<Mm::FeatureType> *vector3 = new Flow::Vector<Mm::FeatureType>(2);
    vector3->at(0) = 5.0;
    vector3->at(1) = 6.0;
    Flow::DataPtr<Flow::Vector<Mm::FeatureType> > dptr3(vector3);
    Core::Ref<const Speech::Feature> feature3(new Speech::Feature(dptr3));
    feature3_ = feature3;

}

void TestBufferedAlignedFeatureProcessor::tearDown(){
    delete trainer_;
}

TEST_F(Test, TestBufferedAlignedFeatureProcessor, buffer)
{
    trainer_->processAlignedFeature(feature1_, 3);
    EXPECT_EQ(1u, trainer_->nBufferedFeatures_);
    EXPECT_EQ(0u, trainer_->segmentIndexBuffer_.at(0));
    trainer_->processAlignedFeature(feature2_, 1);
    EXPECT_EQ(2u, trainer_->nBufferedFeatures_);
    EXPECT_EQ(0u, trainer_->segmentIndexBuffer_.at(1));
    trainer_->processAlignedFeature(feature3_, 9);
    EXPECT_EQ(3u, trainer_->nBufferedFeatures_);
    EXPECT_EQ(0u, trainer_->segmentIndexBuffer_.at(2));
    EXPECT_EQ(trainer_->classLabelWrapper_->getOutputIndexFromClassIndex(3), trainer_->alignmentBuffer_.at(0));
    EXPECT_EQ(trainer_->classLabelWrapper_->getOutputIndexFromClassIndex(1), trainer_->alignmentBuffer_.at(1));
    EXPECT_EQ(trainer_->classLabelWrapper_->getOutputIndexFromClassIndex(9), trainer_->alignmentBuffer_.at(2));

    f32 val = 1.0f;
    for (u32 i = 0; i < 3; i++){
	for (u32 j = 0; j < 2; j++ ){
	    EXPECT_EQ(val, trainer_->featureBuffer_.at(0).at(j,i));
	    val = val + 1.0f;
	}
    }
}

// with alignment weights

class TestBufferedAlignedFeatureProcessorWithWeights : public Test::ConfigurableFixture
{
public:
    BufferedAlignedFeatureProcessor *trainer_;
    u32 nClasses_;
    Core::Ref<const Speech::Feature> feature1_;
    Core::Ref<const Speech::Feature> feature2_;
    Core::Ref<const Speech::Feature> feature3_;
    Mm::Weight w1_, w2_, w3_; // weights from alignment
    virtual void setUp();
    void tearDown();
};

void TestBufferedAlignedFeatureProcessorWithWeights::setUp(){
    setParameter("*.buffer-size", "3");
    setParameter("*.on-error", "ignore");
    setParameter("*.buffer-type", "batch");
    setParameter("*.channel", "nil");
    setParameter("*.disregard-classes", "5");
    setParameter("*.shuffle", "false");
    setParameter("*.weighted-alignment", "true"); // default = false

    nClasses_ = 10;

    trainer_ = new BufferedAlignedFeatureProcessor(config);
    trainer_->acousticModelNeedInit_ = false;
    trainer_->classLabelWrapper_ = new Nn::ClassLabelWrapper(config, nClasses_);

    // input
    Flow::Vector<Mm::FeatureType> *vector1 = new Flow::Vector<Mm::FeatureType>(2);
    vector1->at(0) = 1.0;
    vector1->at(1) = 2.0;
    Flow::DataPtr<Flow::Vector<Mm::FeatureType> > dptr1(vector1);
    Core::Ref<const Speech::Feature> feature1(new Speech::Feature(dptr1));
    feature1_ = feature1;
    w1_ = 1.0;

    Flow::Vector<Mm::FeatureType> *vector2 = new Flow::Vector<Mm::FeatureType>(2);
    vector2->at(0) = 3.0;
    vector2->at(1) = 4.0;
    Flow::DataPtr<Flow::Vector<Mm::FeatureType> > dptr2(vector2);
    Core::Ref<const Speech::Feature> feature2(new Speech::Feature(dptr2));
    feature2_ = feature2;
    w2_ = 0.31415;

    Flow::Vector<Mm::FeatureType> *vector3 = new Flow::Vector<Mm::FeatureType>(2);
    vector3->at(0) = 5.0;
    vector3->at(1) = 6.0;
    Flow::DataPtr<Flow::Vector<Mm::FeatureType> > dptr3(vector3);
    Core::Ref<const Speech::Feature> feature3(new Speech::Feature(dptr3));
    feature3_ = feature3;
    w3_ = 0.5;

}

void TestBufferedAlignedFeatureProcessorWithWeights::tearDown(){
    delete trainer_;
}

TEST_F(Test, TestBufferedAlignedFeatureProcessorWithWeights, buffer)
{
    trainer_->processAlignedFeature(feature1_, 3, w1_);
    EXPECT_EQ(1u, trainer_->nBufferedFeatures_);
    EXPECT_EQ(0u, trainer_->segmentIndexBuffer_.at(0));
    EXPECT_EQ(w1_, trainer_->alignmentWeightsBuffer_.at(0));

    trainer_->processAlignedFeature(feature2_, 1, w2_);
    EXPECT_EQ(2u, trainer_->nBufferedFeatures_);
    EXPECT_EQ(0u, trainer_->segmentIndexBuffer_.at(1));
    EXPECT_EQ(w2_, trainer_->alignmentWeightsBuffer_.at(1));

    trainer_->processAlignedFeature(feature3_, 5, w3_);
    EXPECT_EQ(2u, trainer_->nBufferedFeatures_); // 2 because 5 is in disregard classes and is therefore dropped

    trainer_->processAlignedFeature(feature3_, 9, w3_);
    EXPECT_EQ(0u, trainer_->segmentIndexBuffer_.at(2));
    EXPECT_EQ(w3_, trainer_->alignmentWeightsBuffer_.at(2));

    EXPECT_EQ(trainer_->classLabelWrapper_->getOutputIndexFromClassIndex(3), trainer_->alignmentBuffer_.at(0));
    EXPECT_EQ(trainer_->classLabelWrapper_->getOutputIndexFromClassIndex(1), trainer_->alignmentBuffer_.at(1));
    EXPECT_EQ(trainer_->classLabelWrapper_->getOutputIndexFromClassIndex(9), trainer_->alignmentBuffer_.at(2));

    f32 val = 1.0f;
    for (u32 i = 0; i < 3; i++){
	for (u32 j = 0; j < 2; j++ ){
	    EXPECT_EQ(val, trainer_->featureBuffer_.at(0).at(j,i));
	    val = val + 1.0f;
	}
    }
}

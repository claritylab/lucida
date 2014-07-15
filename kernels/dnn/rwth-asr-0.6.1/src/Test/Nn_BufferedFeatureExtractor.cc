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
#include <Nn/BufferedFeatureExtractor.hh>
#include <Nn/Types.hh>

class BufferedFeatureExtractor : public Nn::BufferedFeatureExtractor<f32> {
public:
    typedef Nn::BufferedFeatureExtractor<f32> Precursor;
    using Precursor::featureBuffer_;
    using Precursor::segmentIndexBuffer_;
    using Precursor::setWindowedFeature;
    using Precursor::initBuffer;
    using Precursor::generateMiniBatch;
    using Precursor::getRelativePositionsInSlidingWindow;
    using Precursor::nBufferedFeatures_;
    using Precursor::slidingWindowSize_;
    using Precursor::batchSize_;
public:
    BufferedFeatureExtractor(const Core::Configuration &config)
    :  Core::Component(config),
       Precursor(config,false)
       {}

};


class TestBufferedFeatureExtractor : public Test::ConfigurableFixture
{
public:
    BufferedFeatureExtractor *buffer_;
    Core::Ref<const Speech::Feature> feature1_;
    Core::Ref<const Speech::Feature> feature2_;
    Core::Ref<const Speech::Feature> feature3_;
    void setUp();
    void tearDown();
};

void TestBufferedFeatureExtractor::setUp(){
    setParameter("*.buffer-size", "3");
    setParameter("*.log-channel.file", "log");
    setParameter("*.on-error", "ignore");
    setParameter("*.buffer-type", "batch");
    setParameter("*.channel", "nil");
    setParameter("*.window-size", "3");
    setParameter("*.shuffle", "false");
    buffer_ = new BufferedFeatureExtractor(config);

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

void TestBufferedFeatureExtractor::tearDown(){
    delete buffer_;
}

TEST_F(Test, TestBufferedFeatureExtractor, buffer)
{
    buffer_->processFeature(feature1_);
    EXPECT_EQ(1u, buffer_->nBufferedFeatures_);
    EXPECT_EQ(0u, buffer_->segmentIndexBuffer_.at(0));
    buffer_->processFeature(feature2_);
    EXPECT_EQ(2u, buffer_->nBufferedFeatures_);
    EXPECT_EQ(0u, buffer_->segmentIndexBuffer_.at(1));
    buffer_->processFeature(feature3_);
    EXPECT_EQ(3u, buffer_->nBufferedFeatures_);
    EXPECT_EQ(0u, buffer_->segmentIndexBuffer_.at(2));
    f32 val = 1.0f;
    for (u32 i = 0; i < 3; i++){
	for (u32 j = 0; j < 2; j++ ){
	    EXPECT_EQ(val, buffer_->featureBuffer_.at(0).at(j,i));
	    val = val + 1.0f;
	}
    }
}


TEST_F(Test, TestBufferedFeatureExtractor, slidingWindow)
{
    EXPECT_EQ(buffer_->slidingWindowSize_,3u);
    buffer_->batchSize_ = 3;
    buffer_->processFeature(feature1_);
    buffer_->processFeature(feature2_);
    buffer_->processFeature(feature3_);

    // test 1
    buffer_->segmentIndexBuffer_.at(0) = 0;
    buffer_->segmentIndexBuffer_.at(1) = 0;
    buffer_->segmentIndexBuffer_.at(2) = 0;
    EXPECT_EQ(3u, buffer_->nBufferedFeatures_);

    std::vector<Nn::Types<f32>::NnMatrix > minibatch;
    buffer_->generateMiniBatch(minibatch, 3);
    EXPECT_EQ((size_t)1, minibatch.size());
    EXPECT_EQ(6u, minibatch.at(0).nRows());
    EXPECT_EQ(3u, minibatch.at(0).nColumns());
    EXPECT_EQ(1.0f, minibatch.at(0).at(0,0));
    EXPECT_EQ(2.0f, minibatch.at(0).at(1,0));
    EXPECT_EQ(1.0f, minibatch.at(0).at(2,0));
    EXPECT_EQ(2.0f, minibatch.at(0).at(3,0));
    EXPECT_EQ(3.0f, minibatch.at(0).at(4,0));
    EXPECT_EQ(4.0f, minibatch.at(0).at(5,0));
    EXPECT_EQ(1.0f, minibatch.at(0).at(0,1));
    EXPECT_EQ(2.0f, minibatch.at(0).at(1,1));
    EXPECT_EQ(3.0f, minibatch.at(0).at(2,1));
    EXPECT_EQ(4.0f, minibatch.at(0).at(3,1));
    EXPECT_EQ(5.0f, minibatch.at(0).at(4,1));
    EXPECT_EQ(6.0f, minibatch.at(0).at(5,1));
    EXPECT_EQ(3.0f, minibatch.at(0).at(0,2));
    EXPECT_EQ(4.0f, minibatch.at(0).at(1,2));
    EXPECT_EQ(5.0f, minibatch.at(0).at(2,2));
    EXPECT_EQ(6.0f, minibatch.at(0).at(3,2));
    EXPECT_EQ(5.0f, minibatch.at(0).at(4,2));
    EXPECT_EQ(6.0f, minibatch.at(0).at(5,2));

    // test 2
    buffer_->segmentIndexBuffer_.at(0) = 0;
    buffer_->segmentIndexBuffer_.at(1) = 1;
    buffer_->segmentIndexBuffer_.at(2) = 1;

    buffer_->generateMiniBatch(minibatch, 3);
    EXPECT_EQ((size_t)1, minibatch.size());
    EXPECT_EQ(6u, minibatch.at(0).nRows());
    EXPECT_EQ(3u, minibatch.at(0).nColumns());
    EXPECT_EQ(1.0f, minibatch.at(0).at(0,0));
    EXPECT_EQ(2.0f, minibatch.at(0).at(1,0));
    EXPECT_EQ(1.0f, minibatch.at(0).at(2,0));
    EXPECT_EQ(2.0f, minibatch.at(0).at(3,0));
    EXPECT_EQ(1.0f, minibatch.at(0).at(4,0));
    EXPECT_EQ(2.0f, minibatch.at(0).at(5,0));
    EXPECT_EQ(3.0f, minibatch.at(0).at(0,1));
    EXPECT_EQ(4.0f, minibatch.at(0).at(1,1));
    EXPECT_EQ(3.0f, minibatch.at(0).at(2,1));
    EXPECT_EQ(4.0f, minibatch.at(0).at(3,1));
    EXPECT_EQ(5.0f, minibatch.at(0).at(4,1));
    EXPECT_EQ(6.0f, minibatch.at(0).at(5,1));
    EXPECT_EQ(3.0f, minibatch.at(0).at(0,2));
    EXPECT_EQ(4.0f, minibatch.at(0).at(1,2));
    EXPECT_EQ(5.0f, minibatch.at(0).at(2,2));
    EXPECT_EQ(6.0f, minibatch.at(0).at(3,2));
    EXPECT_EQ(5.0f, minibatch.at(0).at(4,2));
    EXPECT_EQ(6.0f, minibatch.at(0).at(5,2));

    // test 3
    buffer_->segmentIndexBuffer_.at(0) = 0;
    buffer_->segmentIndexBuffer_.at(1) = 0;
    buffer_->segmentIndexBuffer_.at(2) = 1;

    buffer_->generateMiniBatch(minibatch, 3);
    EXPECT_EQ((size_t)1, minibatch.size());
    EXPECT_EQ(6u, minibatch.at(0).nRows());
    EXPECT_EQ(3u, minibatch.at(0).nColumns());
    EXPECT_EQ(1.0f, minibatch.at(0).at(0,0));
    EXPECT_EQ(2.0f, minibatch.at(0).at(1,0));
    EXPECT_EQ(1.0f, minibatch.at(0).at(2,0));
    EXPECT_EQ(2.0f, minibatch.at(0).at(3,0));
    EXPECT_EQ(3.0f, minibatch.at(0).at(4,0));
    EXPECT_EQ(4.0f, minibatch.at(0).at(5,0));
    EXPECT_EQ(1.0f, minibatch.at(0).at(0,1));
    EXPECT_EQ(2.0f, minibatch.at(0).at(1,1));
    EXPECT_EQ(3.0f, minibatch.at(0).at(2,1));
    EXPECT_EQ(4.0f, minibatch.at(0).at(3,1));
    EXPECT_EQ(3.0f, minibatch.at(0).at(4,1));
    EXPECT_EQ(4.0f, minibatch.at(0).at(5,1));
    EXPECT_EQ(5.0f, minibatch.at(0).at(0,2));
    EXPECT_EQ(6.0f, minibatch.at(0).at(1,2));
    EXPECT_EQ(5.0f, minibatch.at(0).at(2,2));
    EXPECT_EQ(6.0f, minibatch.at(0).at(3,2));
    EXPECT_EQ(5.0f, minibatch.at(0).at(4,2));
    EXPECT_EQ(6.0f, minibatch.at(0).at(5,2));

    // test 4
    // hack for setting window size although it's const
    const u32 *slidingWindowSizeTmp = &(buffer_->slidingWindowSize_);
    u32 *slidingWindowSize = const_cast<u32*>(slidingWindowSizeTmp);
    *slidingWindowSize = 1;
    EXPECT_EQ(buffer_->slidingWindowSize_, 1u);

    buffer_->segmentIndexBuffer_.at(0) = 0;
    buffer_->segmentIndexBuffer_.at(1) = 1;
    buffer_->segmentIndexBuffer_.at(2) = 1;

    buffer_->generateMiniBatch(minibatch, 3);
    EXPECT_EQ((size_t)1, minibatch.size());
    EXPECT_EQ(2u, minibatch.at(0).nRows());
    EXPECT_EQ(3u, minibatch.at(0).nColumns());
    EXPECT_EQ(1.0f, minibatch.at(0).at(0,0));
    EXPECT_EQ(2.0f, minibatch.at(0).at(1,0));
    EXPECT_EQ(3.0f, minibatch.at(0).at(0,1));
    EXPECT_EQ(4.0f, minibatch.at(0).at(1,1));
    EXPECT_EQ(5.0f, minibatch.at(0).at(0,2));
    EXPECT_EQ(6.0f, minibatch.at(0).at(1,2));


}

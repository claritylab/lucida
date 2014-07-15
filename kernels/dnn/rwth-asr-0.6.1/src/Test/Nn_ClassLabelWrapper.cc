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
#include <Nn/ClassLabelWrapper.hh>
#include <Core/Directory.hh>

class TestClassLabelWrapper : public Test::ConfigurableFixture
{
public:
    u32 nClasses_;
    void setUp();
};

void TestClassLabelWrapper::setUp(){
//    setParameter("*.channel", "nil");
    nClasses_ = 10;

}

TEST_F(Test, TestClassLabelWrapper, initFromConfig){
    setParameter("*.disregard-classes", "5");
    Nn::ClassLabelWrapper *classLabelWrapper = new Nn::ClassLabelWrapper(select("class-labels"), nClasses_);
    // nClasses
    EXPECT_EQ(nClasses_, classLabelWrapper->nClasses());
    // nClassesToAccumulate
    EXPECT_EQ(nClasses_ - 1, classLabelWrapper->nClassesToAccumulate());

    // getOutputIndexFromClassIndex
    EXPECT_EQ(0u, classLabelWrapper->getOutputIndexFromClassIndex(0));
    EXPECT_EQ(1u, classLabelWrapper->getOutputIndexFromClassIndex(1));
    EXPECT_EQ(2u, classLabelWrapper->getOutputIndexFromClassIndex(2));
    EXPECT_EQ(3u, classLabelWrapper->getOutputIndexFromClassIndex(3));
    EXPECT_EQ(4u, classLabelWrapper->getOutputIndexFromClassIndex(4));
    EXPECT_EQ(5u, classLabelWrapper->getOutputIndexFromClassIndex(6));
    EXPECT_EQ(6u, classLabelWrapper->getOutputIndexFromClassIndex(7));
    EXPECT_EQ(7u, classLabelWrapper->getOutputIndexFromClassIndex(8));
    EXPECT_EQ(8u, classLabelWrapper->getOutputIndexFromClassIndex(9));

    // isClassToAccumulate
    EXPECT_EQ(true, classLabelWrapper->isClassToAccumulate(0));
    EXPECT_EQ(true, classLabelWrapper->isClassToAccumulate(1));
    EXPECT_EQ(true, classLabelWrapper->isClassToAccumulate(2));
    EXPECT_EQ(true, classLabelWrapper->isClassToAccumulate(3));
    EXPECT_EQ(true, classLabelWrapper->isClassToAccumulate(4));
    EXPECT_EQ(true, classLabelWrapper->isClassToAccumulate(6));
    EXPECT_EQ(true, classLabelWrapper->isClassToAccumulate(7));
    EXPECT_EQ(true, classLabelWrapper->isClassToAccumulate(8));
    EXPECT_EQ(true, classLabelWrapper->isClassToAccumulate(9));
    EXPECT_EQ(false, classLabelWrapper->isClassToAccumulate(5));

}

TEST_F(Test, TestClassLabelWrapper, IO){
    char *t = std::getenv("TMPDIR");
    std::string tmpdir;
    if (t)
	tmpdir = t;
    else
	tmpdir = "/tmp";
    std::string dirName = tmpdir + "/XXXX/";
    Core::createDirectory(dirName);
    std::string filename = dirName + "label.map";
    setParameter("*.disregard-classes", "5");
    Nn::ClassLabelWrapper *classLabelWrapper = new Nn::ClassLabelWrapper(config, nClasses_);
    bool returnValue = classLabelWrapper->save(filename);
    EXPECT_TRUE(returnValue);

    std::cout << "filename: " << filename << std::endl;
    setParameter("*.load-from-file", filename);
    Nn::ClassLabelWrapper wrapper(config);
    // nClasses
    EXPECT_EQ(nClasses_, wrapper.nClasses());
    // nClassesToAccumulate
    EXPECT_EQ(nClasses_ - 1, wrapper.nClassesToAccumulate());

    // getOutputIndexFromClassIndex
    EXPECT_EQ(0u, wrapper.getOutputIndexFromClassIndex(0));
    EXPECT_EQ(1u, wrapper.getOutputIndexFromClassIndex(1));
    EXPECT_EQ(2u, wrapper.getOutputIndexFromClassIndex(2));
    EXPECT_EQ(3u, wrapper.getOutputIndexFromClassIndex(3));
    EXPECT_EQ(4u, wrapper.getOutputIndexFromClassIndex(4));
    EXPECT_EQ(5u, wrapper.getOutputIndexFromClassIndex(6));
    EXPECT_EQ(6u, wrapper.getOutputIndexFromClassIndex(7));
    EXPECT_EQ(7u, wrapper.getOutputIndexFromClassIndex(8));
    EXPECT_EQ(8u, wrapper.getOutputIndexFromClassIndex(9));

    // isClassToAccumulate
    EXPECT_EQ(true, wrapper.isClassToAccumulate(0));
    EXPECT_EQ(true, wrapper.isClassToAccumulate(1));
    EXPECT_EQ(true, wrapper.isClassToAccumulate(2));
    EXPECT_EQ(true, wrapper.isClassToAccumulate(3));
    EXPECT_EQ(true, wrapper.isClassToAccumulate(4));
    EXPECT_EQ(true, wrapper.isClassToAccumulate(6));
    EXPECT_EQ(true, wrapper.isClassToAccumulate(7));
    EXPECT_EQ(true, wrapper.isClassToAccumulate(8));
    EXPECT_EQ(true, wrapper.isClassToAccumulate(9));
    EXPECT_EQ(false, wrapper.isClassToAccumulate(5));
    Core::removeDirectory(dirName);
}

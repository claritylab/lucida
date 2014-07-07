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
#include <Math/FastVectorOperations.hh>
#include <cmath>

class TestFastVectorWrapper : public Test::Fixture
{
public:
    void setUp();
    void tearDown();
protected:
    int dim_, dim2_;
    double *vector_, *result_;
    float *vectorf_, *resultf_;
    double *vector2_, *result2_;
    float *vectorf2_, *resultf2_;

};

void TestFastVectorWrapper::setUp()
{
    dim_ = 2;
    vector_ = new double[dim_];
    vector_[0] = 1.0;
    vector_[1] = 2.0;
    vectorf_ = new float[dim_];
    vectorf_[0] = 1.0;
    vectorf_[1] = 2.0;
    result_ = new double[dim_];
    resultf_ = new float[dim_];
    dim2_ = 3;
    vector2_ = new double[dim2_];
    vector2_[0] = 1.0;
    vector2_[1] = 2.0;
    vector2_[2] = 3.0;
    vectorf2_ = new float[dim2_];
    vectorf2_[0] = 1.0;
    vectorf2_[1] = 2.0;
    vectorf2_[2] = 3.0;
    result2_ = new double[dim2_];
    resultf2_ = new float[dim2_];

}

void TestFastVectorWrapper::tearDown(){
    delete [] vector_;
    delete [] vectorf_;
    delete [] result_;
    delete [] resultf_;
    delete [] vector2_;
    delete [] vectorf2_;
    delete [] result2_;
    delete [] resultf2_;

}

TEST_F(Test, TestFastVectorWrapper, exp)
{
    Math::vr_exp(dim_, vector_, result_);
    EXPECT_EQ(result_[0], std::exp(vector_[0]));
    EXPECT_EQ(result_[1], std::exp(vector_[1]));

    Math::vr_exp(dim_, vectorf_, resultf_);
    EXPECT_EQ(resultf_[0], std::exp(vectorf_[0]));
    EXPECT_EQ(resultf_[1], std::exp(vectorf_[1]));

    Math::vr_exp(dim2_, vector2_, result2_);
    EXPECT_EQ(result2_[0], std::exp(vector2_[0]));
    EXPECT_EQ(result2_[1], std::exp(vector2_[1]));
    EXPECT_EQ(result2_[2], std::exp(vector2_[2]));

    Math::vr_exp(dim2_, vectorf2_, resultf2_);
    EXPECT_EQ(resultf2_[0], std::exp(vectorf2_[0]));
    EXPECT_EQ(resultf2_[1], std::exp(vectorf2_[1]));
    EXPECT_EQ(resultf2_[2], std::exp(vectorf2_[2]));
}

TEST_F(Test, TestFastVectorWrapper, exp_mt)
{
    Math::mt_vr_exp(dim_, vector_, result_, 2);
    EXPECT_EQ(result_[0], std::exp(vector_[0]));
    EXPECT_EQ(result_[1], std::exp(vector_[1]));

    Math::mt_vr_exp(dim_, vectorf_, resultf_, 2);
    EXPECT_EQ(resultf_[0], std::exp(vectorf_[0]));
    EXPECT_EQ(resultf_[1], std::exp(vectorf_[1]));

    Math::mt_vr_exp(dim2_, vector2_, result2_, 2);
    EXPECT_EQ(result2_[0], std::exp(vector2_[0]));
    EXPECT_EQ(result2_[1], std::exp(vector2_[1]));
    EXPECT_EQ(result2_[2], std::exp(vector2_[2]));

    Math::mt_vr_exp(dim2_, vectorf2_, resultf2_, 2);
    EXPECT_EQ(resultf2_[0], std::exp(vectorf2_[0]));
    EXPECT_EQ(resultf2_[1], std::exp(vectorf2_[1]));
    EXPECT_EQ(resultf2_[2], std::exp(vectorf2_[2]));
}


TEST_F(Test, TestFastVectorWrapper, log)
{
    Math::vr_log(dim_, vector_, result_);
    EXPECT_EQ(result_[0], std::log(vector_[0]));
    EXPECT_EQ(result_[1], std::log(vector_[1]));
    Math::vr_log(dim_, vectorf_, resultf_);
    EXPECT_EQ(resultf_[0], std::log(vectorf_[0]));
    EXPECT_EQ(resultf_[1], std::log(vectorf_[1]));

}

TEST_F(Test, TestFastVectorWrapper, powx)
{
    Math::vr_powx(dim_, vector_, 2.0, result_);
    EXPECT_EQ(result_[0], std::pow(vector_[0], 2.0));
    EXPECT_EQ(result_[1], std::pow(vector_[1], 2.0));
    Math::vr_powx(dim_, vectorf_, (float) 2.0, resultf_);
    EXPECT_EQ(resultf_[0], std::pow(vectorf_[0], (float) 2.0));
    EXPECT_EQ(resultf_[1], std::pow(vectorf_[1], (float) 2.0));

}

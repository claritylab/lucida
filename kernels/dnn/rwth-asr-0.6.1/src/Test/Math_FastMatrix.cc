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
#include <Math/FastMatrix.hh>
#include <Math/FastVector.hh>

class TestFastMatrix : public Test::Fixture
{
public:
    void setUp();
    void tearDown();
};

void TestFastMatrix::setUp(){ }

void TestFastMatrix::tearDown(){ }

TEST_F(Test, TestFastMatrix, resize)
{
    Math::FastMatrix<f64> A;
    EXPECT_TRUE(A.empty());
    A.resize(2,3);
    EXPECT_EQ(A.nRows(),2u);
    EXPECT_EQ(A.nColumns(),3u);
    EXPECT_EQ(A.size(),6u);
    f64 val = 0.0;
    A.setToZero();
    for (u32 i = 0; i < 2; i++){
	for (u32 j = 0; j < 3; j++)
	    val = A.at(i,j);
    }
    EXPECT_EQ(val, 0.0);
}

TEST_F(Test, TestFastMatrix, copyStructure)
{
    Math::FastMatrix<f64> A;
    Math::FastMatrix<f64> B;
    B.resize(2,3);
    EXPECT_EQ(B.nRows(),2u);
    EXPECT_EQ(B.nColumns(),3u);
    EXPECT_EQ(B.size(),6u);
    A.copyStructure(B);
    EXPECT_EQ(A.nRows(),2u);
    EXPECT_EQ(A.nColumns(),3u);
    EXPECT_EQ(A.size(),6u);
    f64 val = 0.0;
    A.setToZero();
    for (u32 i = 0; i < 2; i++){
	for (u32 j = 0; j < 3; j++)
	    val = A.at(i,j);
    }
    EXPECT_EQ(val, 0.0);
}

TEST_F(Test, TestFastMatrix, matrixVectorProduct)
{
    Math::FastMatrix<f64> A;
    Math::FastVector<f64> x;
    Math::FastVector<f64> y;
    A.resize(2,3);
    x.resize(3);
    y.resize(2);
    A.at(0,0) = 1.0;
    A.at(0,1) = 2.0;
    A.at(0,2) = 3.0;
    A.at(1,0) = 4.0;
    A.at(1,1) = 5.0;
    A.at(1,2) = 6.0;
    x.at(0) = 2.0;
    x.at(1) = 4.0;
    x.at(2) = 6.0;
    A.multiply(x, y, false, 0.5, 0.0);
    EXPECT_EQ(14.0, y.at(0));
    EXPECT_EQ(32.0, y.at(1));
}

TEST_F(Test, TestFastMatrix, copyConstructor){
    Math::FastMatrix<f32> x(2,3);
    x.setToZero();
    x.at(0,0) = 1.0f;

    Math::FastMatrix<f32>y(x);
    EXPECT_EQ(x.nRows(), y.nRows());
    EXPECT_EQ(x.nColumns(), y.nColumns());
    EXPECT_EQ(x.at(0,0), y.at(0,0));
}

TEST_F(Test, TestFastMatrix, assignment)
{
    Math::FastMatrix<f32> x(2,3);
    x.setToZero();
    x.at(0,0) = 1.0f;

    Math::FastMatrix<f32>y;
    y = x;
    EXPECT_EQ(x.nRows(), y.nRows());
    EXPECT_EQ(x.nColumns(), y.nColumns());
    EXPECT_EQ(x.at(0,0), y.at(0,0));
}

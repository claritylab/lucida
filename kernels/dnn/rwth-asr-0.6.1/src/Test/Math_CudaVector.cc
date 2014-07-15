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
#include <Math/CudaVector.hh>
#include <Math/CudaMatrix.hh>

class TestCudaVector: public Test::Fixture
{
public:
    void setUp();
    void tearDown();
};

void TestCudaVector::setUp(){ }

void TestCudaVector::tearDown(){ }

TEST_F(Test, TestCudaVector, resize)
{
    Math::CudaVector<f64> x;
    EXPECT_TRUE(x.empty());
    x.initComputation();
    x.resize(2);
    x.finishComputation();
    EXPECT_EQ(x.nRows(),2u);
    EXPECT_EQ(x.size(),2u);
    x.at(0) = -1.0;
    x.at(1) = 1.0;
    EXPECT_EQ(x.at(0), -1.0);
    EXPECT_EQ(x.at(1), 1.0);
    x.initComputation();
    x.resize(3,10);
    x.finishComputation();
    EXPECT_EQ(x.at(0), -1.0);
    EXPECT_EQ(x.at(1), 1.0);
    EXPECT_EQ(x.at(2), 10.0);
    Math::CudaVector<f32> x2;
    EXPECT_TRUE(x2.empty());
    x2.initComputation();
    x2.resize(2);
    x2.finishComputation();
    EXPECT_EQ(x2.nRows(),2u);
    EXPECT_EQ(x2.size(),2u);
    x2.at(0) = -1.0;
    x2.at(1) = 1.0;
    EXPECT_EQ(x2.at(0), (f32)-1.0);
    EXPECT_EQ(x2.at(1), (f32)1.0);
    x2.initComputation();
    x2.resize(3,10);
    x2.finishComputation();
    EXPECT_EQ(x2.at(0), (f32)-1.0);
    EXPECT_EQ(x2.at(1), (f32)1.0);
    EXPECT_EQ(x2.at(2), (f32)10.0);

    Math::CudaVector<u32> x3;
    EXPECT_TRUE(x3.empty());
    x3.initComputation();
    x3.resize(2);
    x3.finishComputation();
    EXPECT_EQ(x3.nRows(),2u);
    EXPECT_EQ(x3.size(),2u);
    x3.at(0) = 2.0;
    x3.at(1) = 4.0;
    EXPECT_EQ(x3.at(0), 2u);
    EXPECT_EQ(x3.at(1), 4u);
    x3.initComputation();
    x3.resize(3,10);
    x3.finishComputation();
    EXPECT_EQ(x3.at(0), 2u);
    EXPECT_EQ(x3.at(1), 4u);
    EXPECT_EQ(x3.at(2), 10u);
    x3.initComputation();
    x3.resize(1);
    x3.finishComputation();
    EXPECT_EQ(x3.size(),1u);
    EXPECT_EQ(x3.at(0), 2u);
}

TEST_F(Test, TestCudaVector, copyStructure)
{
    Math::CudaVector<f64> x;
    Math::CudaVector<f64> y;
    x.initComputation();
    y.initComputation();
    y.resize(2);
    EXPECT_EQ(y.nRows(),2u);
    x.copyStructure(y);
    EXPECT_EQ(x.nRows(),2u);

    Math::CudaVector<f32> x2;
    Math::CudaVector<f32> y2;
    x2.initComputation();
    y2.initComputation();
    y2.resize(2);
    EXPECT_EQ(y2.nRows(),2u);
    x2.copyStructure(y2);
    EXPECT_EQ(x2.nRows(),2u);
}

TEST_F(Test, TestCudaVector, copy)
{
    Math::CudaVector<f64> x;
    Math::CudaVector<f64> y;
    y.resize(2);
    x.resize(2);
    x.at(0) = 2.0;
    x.at(1) = 4.0;
    x.initComputation();
    y.initComputation(false);
    y.copy(x);
    y.finishComputation();
    EXPECT_EQ(2.0, y.at(0));
    EXPECT_EQ(4.0, y.at(1));

    Math::CudaVector<f32> x2;
    Math::CudaVector<f32> y2;
    y2.resize(2);
    x2.resize(2);
    x2.at(0) = 2.0;
    x2.at(1) = 4.0;
    x2.initComputation();
    y2.initComputation(false);
    y2.copy(x2);
    y2.finishComputation();
    EXPECT_EQ((f32)2.0, y2.at(0));
    EXPECT_EQ((f32)4.0, y2.at(1));
}

TEST_F(Test, TestCudaVector, setToZero)
{
    Math::CudaVector<f64> x;
    x.resize(2);
    x.initComputation();
    x.setToZero();
    x.finishComputation();
    EXPECT_EQ(x.size(),2u);
    EXPECT_EQ(x.at(0), 0.0);
    EXPECT_EQ(x.at(1), 0.0);

    Math::CudaVector<f32> x2;
    x2.resize(2);
    x2.initComputation();
    x2.setToZero();
    x2.finishComputation();
    EXPECT_EQ(x2.size(),2u);
    EXPECT_EQ(x2.at(0), (f32)0.0);
    EXPECT_EQ(x2.at(1), (f32)0.0);
}

TEST_F(Test, TestCudaVector, copyConstructor){
    Math::CudaVector<f32> x(2);
    x.at(0) = 1.0f;
    x.at(1) = 0.0f;
    Math::CudaVector<f32>y(x);
    EXPECT_EQ(x.size(), y.size());
    EXPECT_EQ(x.at(0), y.at(0));
    EXPECT_EQ(x.at(1), y.at(1));
}

TEST_F(Test, TestCudaVector, assignment){
    Math::CudaVector<f32> x(2);
    x.at(0) = 1.0f;
    x.at(1) = 0.0f;
    Math::CudaVector<f32>y;
    y = x;
    EXPECT_EQ(x.size(), y.size());
    EXPECT_EQ(x.at(0), y.at(0));
    EXPECT_EQ(x.at(1), y.at(1));
}

TEST_F(Test, TestCudaVector, add)
{
    Math::CudaVector<f64> x;
    Math::CudaVector<f64> y;
    y.resize(2);
    x.resize(2);
    x.at(0) = 1.0;
    x.at(1) = 2.0;
    y.at(0) = 2.0;
    y.at(1) = 4.0;
    x.initComputation();
    y.initComputation();
    x.add(y, 0.5);
    x.finishComputation();
    EXPECT_EQ(2.0, x.at(0));
    EXPECT_EQ(4.0, x.at(1));

    Math::CudaVector<f32> x2;
    Math::CudaVector<f32> y2;
    y2.resize(2);
    x2.resize(2);
    x2.at(0) = 1.0;
    x2.at(1) = 2.0;
    y2.at(0) = 2.0;
    y2.at(1) = 4.0;
    x2.initComputation();
    y2.initComputation();
    x2.add(y2, 0.5f);
    x2.finishComputation();
    EXPECT_EQ((f32)2.0, x2.at(0));
    EXPECT_EQ((f32)4.0, x2.at(1));

    Math::CudaVector<f64> x3;
    Math::CudaVector<f32> y3;
    y3.resize(2);
    x3.resize(2);
    x3.at(0) = 1.0;
    x3.at(1) = 2.0;
    y3.at(0) = 2.0;
    y3.at(1) = 4.0;
    x3.initComputation();
    y3.initComputation();
    x3.add(y3, 0.5f);
    x3.finishComputation();
    EXPECT_EQ(2.0, x3.at(0));
    EXPECT_EQ(4.0, x3.at(1));

}

TEST_F(Test, TestCudaVector, addConstantElementwise)
{
    Math::CudaVector<f64> x;
    x.resize(2);
    x.at(0) = 1.0;
    x.at(1) = 2.0;
    x.initComputation();
    x.addConstantElementwise(2.0);
    x.finishComputation();
    EXPECT_EQ(3.0, x.at(0));
    EXPECT_EQ(4.0, x.at(1));

    Math::CudaVector<f32> x2;
    x2.resize(2);
    x2.at(0) = 1.0;
    x2.at(1) = 2.0;
    x2.initComputation();
    x2.addConstantElementwise(2.0);
    x2.finishComputation();
    EXPECT_EQ(3.0f, x2.at(0));
    EXPECT_EQ(4.0f, x2.at(1));
}

TEST_F(Test, TestCudaVector, scale)
{
    Math::CudaVector<f64> y;
    y.resize(2);
    y.at(0) = 2.0;
    y.at(1) = 4.0;
    y.initComputation();
    y.scale(0.5);
    y.finishComputation();
    EXPECT_EQ(1.0, y.at(0));
    EXPECT_EQ(2.0, y.at(1));

    Math::CudaVector<f32> y2;
    y2.resize(2);
    y2.at(0) = 2.0;
    y2.at(1) = 4.0;
    y2.initComputation();
    y2.scale(0.5);
    y2.finishComputation();
    EXPECT_EQ((f32)1.0, y2.at(0));
    EXPECT_EQ((f32)2.0, y2.at(1));
}

TEST_F(Test, TestCudaVector, dot)
{
    Math::CudaVector<f64> x;
    Math::CudaVector<f64> y;
    y.resize(2);
    x.copyStructure(y);
    x.at(0) = 1.0;
    x.at(1) = 2.0;
    y.at(0) = 2.0;
    y.at(1) = 4.0;
    x.initComputation();
    y.initComputation();
    f64 dotproduct = x.dot(y);
    EXPECT_EQ(10.0, dotproduct);

    Math::CudaVector<f32> x2;
    Math::CudaVector<f32> y2;
    y2.resize(2);
    x2.copyStructure(y2);
    x2.at(0) = 1.0;
    x2.at(1) = 2.0;
    y2.at(0) = 2.0;
    y2.at(1) = 4.0;
    x2.initComputation();
    y2.initComputation();
    f32 dotproduct2 = x2.dot(y2);
    EXPECT_EQ((f32)10.0, dotproduct2);
}

TEST_F(Test, TestCudaVector, columnwiseInnerProduct)
{
    Math::CudaMatrix<f64> A;
    Math::CudaMatrix<f64> B;
    Math::CudaVector<f64> v;
    A.resize(3,2);
    B.resize(3,2);
    v.resize(2);
    A.at(0,0) = 0;
    A.at(0,1) = 1;
    A.at(1,0) = -1;
    A.at(1,1) = -2;
    A.at(2,0) = 3;
    A.at(2,1) = -2;
    B.at(0,0) = 1;
    B.at(0,1) = -4;
    B.at(1,0) = -3;
    B.at(1,1) = 2;
    B.at(2,0) = 4;
    B.at(2,1) = 3;
    A.initComputation();
    B.initComputation();
    v.initComputation();
    v.columnwiseInnerProduct(A, B);
    v.finishComputation();
    EXPECT_EQ(15.0, v.at(0));
    EXPECT_EQ(-14.0, v.at(1));

    Math::CudaMatrix<f32> A2;
    Math::CudaMatrix<f32> B2;
    Math::CudaVector<f32> v2;
    A2.resize(3,2);
    B2.resize(3,2);
    v2.resize(2);
    A2.at(0,0) = 0;
    A2.at(0,1) = 1;
    A2.at(1,0) = -1;
    A2.at(1,1) = -2;
    A2.at(2,0) = 3;
    A2.at(2,1) = -2;
    B2.at(0,0) = 1;
    B2.at(0,1) = -4;
    B2.at(1,0) = -3;
    B2.at(1,1) = 2;
    B2.at(2,0) = 4;
    B2.at(2,1) = 3;
    A2.initComputation();
    B2.initComputation();
    v2.initComputation();
    v2.columnwiseInnerProduct(A2, B2);
    v2.finishComputation();
    EXPECT_EQ(15.0f, v2.at(0));
    EXPECT_EQ(-14.0f, v2.at(1));
}

TEST_F(Test, TestCudaVector, elementwiseMultiplication)
{
    Math::CudaVector<f64> x;
    Math::CudaVector<f64> y;
    x.resize(2);
    y.resize(2);
    x.at(0) = 1.0;
    x.at(1) = 2.0;
    y.at(0) = 2.0;
    y.at(1) = 4.0;
    x.initComputation();
    y.initComputation();
    x.elementwiseMultiplication(y);
    x.finishComputation();
    EXPECT_EQ(2.0, x.at(0));
    EXPECT_EQ(8.0, x.at(1));

    Math::CudaVector<f32> x2;
    Math::CudaVector<f32> y2;
    x2.resize(2);
    y2.resize(2);
    x2.at(0) = 1.0;
    x2.at(1) = 2.0;
    y2.at(0) = 2.0;
    y2.at(1) = 4.0;
    x2.initComputation();
    y2.initComputation();
    x2.elementwiseMultiplication(y2);
    x2.finishComputation();
    EXPECT_EQ((f32)2.0, x2.at(0));
    EXPECT_EQ((f32)8.0, x2.at(1));
}

TEST_F(Test, TestCudaVector, elementwiseDivision)
{
    Math::CudaVector<f64> x;
    Math::CudaVector<f64> y;
    x.resize(2);
    y.resize(2);
    x.at(0) = 1.0;
    x.at(1) = 2.0;
    y.at(0) = 2.0;
    y.at(1) = 4.0;
    x.initComputation();
    y.initComputation();
    y.elementwiseDivision(x);
    y.finishComputation();
    EXPECT_EQ(2.0, y.at(0));
    EXPECT_EQ(2.0, y.at(1));

    Math::CudaVector<f32> x2;
    Math::CudaVector<f32> y2;
    x2.resize(2);
    y2.resize(2);
    x2.at(0) = 1.0;
    x2.at(1) = 2.0;
    y2.at(0) = 2.0;
    y2.at(1) = 4.0;
    x2.initComputation();
    y2.initComputation();
    y2.elementwiseDivision(x2);
    y2.finishComputation();
    EXPECT_EQ((f32)2.0, y2.at(0));
    EXPECT_EQ((f32)2.0, y2.at(1));
}

TEST_F(Test, TestCudaVector, divide)
{
    Math::CudaVector<f64> y;
    y.resize(2);
    y.at(0) = 2.0;
    y.at(1) = 4.0;
    y.initComputation();
    y.divide(2.0);
    y.finishComputation();
    EXPECT_EQ(1.0, y.at(0));
    EXPECT_EQ(2.0, y.at(1));

    Math::CudaVector<f32> y2;
    y2.resize(2);
    y2.at(0) = 2.0;
    y2.at(1) = 4.0;
    y2.initComputation();
    y2.divide(2.0);
    y2.finishComputation();
    EXPECT_EQ((f32)1.0, y2.at(0));
    EXPECT_EQ((f32)2.0, y2.at(1));
}

TEST_F(Test, TestCudaVector, fill)
{
    Math::CudaVector<f64> y;
    y.resize(2);
    y.initComputation();
    y.fill(10.0);
    y.finishComputation();
    EXPECT_EQ(10.0, y.at(0));
    EXPECT_EQ(10.0, y.at(1));

    Math::CudaVector<f32> y2;
    y2.resize(2);
    y2.initComputation();
    y2.fill(10.0);
    y2.finishComputation();
    EXPECT_EQ((f32)10.0, y2.at(0));
    EXPECT_EQ((f32)10.0, y2.at(1));
}

TEST_F(Test, TestCudaVector, l1norm)
{
    Math::CudaVector<f64> y;
    y.resize(2);
    y.at(0) = 2.0;
    y.at(1) = -4.0;
    y.initComputation();
    f64 norm = y.l1norm();
    y.finishComputation();
    EXPECT_EQ(6.0, norm);

    Math::CudaVector<f32> y2;
    y2.resize(2);
    y2.at(0) = 2.0;
    y2.at(1) = -4.0;
    y2.initComputation();
    f32 norm2 = y2.l1norm();
    y2.finishComputation();
    EXPECT_EQ((f32)6.0, norm2);
}

TEST_F(Test, TestCudaVector, normEuclidean)
{
    Math::CudaVector<f64> y;
    y.resize(2);
    y.at(0) = 3.0;
    y.at(1) = -4.0;
    y.initComputation();
    f64 norm = y.normEuclidean();
    y.finishComputation();
    EXPECT_EQ(5.0, norm);

    Math::CudaVector<f32> y2;
    y2.resize(2);
    y2.at(0) = 3.0;
    y2.at(1) = -4.0;
    y2.initComputation();
    f32 norm2 = y2.normEuclidean();
    y2.finishComputation();
    EXPECT_EQ((f32)5.0, norm2);
}

TEST_F(Test, TestCudaVector, addSummedColumns){

    // single precision
    Math::CudaVector<f32> x(2);
    x.at(0) = 0.0;
    x.at(1) = 1.0;
    x.initComputation();

    Math::CudaMatrix<f32> X(2,3);
    X.at(0,0) = 0.0f;
    X.at(0,1) = -1.0f;
    X.at(0,2) = 2.0f;
    X.at(1,0) = -3.0f;
    X.at(1,1) = 4.0f;
    X.at(1,2) = -5.0f;
    X.initComputation();
    x.addSummedColumns(X,2.0f);
    x.finishComputation();
    EXPECT_EQ(2.0f, x.at(0));
    EXPECT_EQ(-7.0f, x.at(1));

    // double precision

    Math::CudaVector<f64> x2(2);
    x2.at(0) = 0.0;
    x2.at(1) = 1.0;
    x2.initComputation();

    Math::CudaMatrix<f64> X2(2,3);
    X2.at(0,0) = 0.0f;
    X2.at(0,1) = -1.0f;
    X2.at(0,2) = 2.0f;
    X2.at(1,0) = -3.0f;
    X2.at(1,1) = 4.0f;
    X2.at(1,2) = -5.0f;
    X2.initComputation();
    x2.addSummedColumns(X2,2.0);
    x2.finishComputation();
    EXPECT_EQ(2.0, x2.at(0));
    EXPECT_EQ(-7.0, x2.at(1));

    // mixed precision

    Math::CudaVector<f64> x3(2);
    x3.at(0) = 0.0;
    x3.at(1) = 1.0;
    x3.initComputation();

    Math::CudaMatrix<f32> X3(2,3);
    X3.at(0,0) = 0.0f;
    X3.at(0,1) = -1.0f;
    X3.at(0,2) = 2.0f;
    X3.at(1,0) = -3.0f;
    X3.at(1,1) = 4.0f;
    X3.at(1,2) = -5.0f;
    X3.initComputation();
    x3.addSummedColumns(X3,2.0f);
    x3.finishComputation();
    EXPECT_EQ(2.0, x3.at(0));
    EXPECT_EQ(-7.0, x3.at(1));

}

TEST_F(Test, TestCudaVector, addSquaredSummedColumns){
    Math::CudaVector<f32> x(2);
    x.at(0) = 0.0;
    x.at(1) = 1.0;
    x.initComputation();

    Math::CudaMatrix<f32> X(2,3);
    X.at(0,0) = 0.0f;
    X.at(0,1) = -1.0f;
    X.at(0,2) = 2.0f;
    X.at(1,0) = -3.0f;
    X.at(1,1) = 4.0f;
    X.at(1,2) = -5.0f;
    X.initComputation();
    x.addSquaredSummedColumns(X,2.0);
    x.finishComputation();
    EXPECT_EQ(10.0f, x.at(0));
    EXPECT_EQ(101.0f, x.at(1));
}

TEST_F(Test, TestCudaVector, addSummedRows){
    Math::CudaVector<f32> x(3);
    x.at(0) = -1.0f;
    x.at(1) = 0.0f;
    x.at(2) = 1.0f;
    x.initComputation();

    Math::CudaMatrix<f32> X(5,3);
    X.at(0,0) = 3.0f;
    X.at(0,1) = -1.0f;
    X.at(0,2) = 2.0f;
    X.at(1,0) = -5.0f;
    X.at(1,1) = 3.0f;
    X.at(1,2) = 0.0f;
    X.at(2,0) = 2.0f;
    X.at(2,1) = 0.0f;
    X.at(2,2) = 1.0f;
    X.at(3,0) = 0.0f;
    X.at(3,1) = 1.0f;
    X.at(3,2) = 5.0f;
    X.at(4,0) = 2.0f;
    X.at(4,1) = -4.0f;
    X.at(4,2) = 8.0f;

    X.initComputation();

    x.addSummedRows(X,2.0);
    x.finishComputation();
    EXPECT_EQ(3.0f, x.at(0));
    EXPECT_EQ(-2.0f, x.at(1));
    EXPECT_EQ(33.0f, x.at(2));

    x.at(0) = -1.0f;
    x.at(1) = 0.0f;
    x.at(2) = 1.0f;
    x.initComputation();

    x.addSummedRows(X);
    x.finishComputation();
    EXPECT_EQ(1.0f, x.at(0));
    EXPECT_EQ(-1.0f, x.at(1));
    EXPECT_EQ(17.0f, x.at(2));


}


TEST_F(Test, TestCudaVector, addSummedRowsFast){
    Math::CudaVector<f32> x(2);
    x.at(0) = -1.0;
    x.at(1) = 1.0;
    x.initComputation();

    Math::CudaMatrix<f32> X(100,2);
    for (u32 i = 0; i < 100; i++){
	X.at(i, 0) = (f32) i;
	X.at(i, 1) = (f32) -(i+1.0f);
    }
//    std::cout << "INIT" << std::endl;
//    X.show();
//    std::cout << "-----" << std::endl;
    X.initComputation();

    Math::CudaMatrix<f32> tmp(32,2);
    tmp.initComputation(false);
//    X.syncAndShow();
//    x.syncAndShow();
    x.addSummedRows(X, tmp, 2.0);
    x.finishComputation();
//    x.show();
    EXPECT_EQ(9899.0f, x.at(0));
    EXPECT_EQ(-10099.0f, x.at(1));
}



TEST_F(Test, TestCudaVector, getMaxOfColumns){
    Math::CudaVector<f32> x(3);
    x.initComputation();

    Math::CudaMatrix<f32> X(5,3);
    X.at(0,0) = 1.0f;
    X.at(0,1) = -1.0f;
    X.at(0,2) = 2.0f;
    X.at(1,0) = -5.0f;
    X.at(1,1) = 3.0f;
    X.at(1,2) = 0.0f;
    X.at(2,0) = 2.0f;
    X.at(2,1) = 0.0f;
    X.at(2,2) = 1.0f;
    X.at(3,0) = 0.0f;
    X.at(3,1) = 1.0f;
    X.at(3,2) = 5.0f;
    X.at(4,0) = 2.0f;
    X.at(4,1) = -4.0f;
    X.at(4,2) = 8.0f;

    X.initComputation();

    x.getMaxOfColumns(X);
    x.finishComputation();
    EXPECT_EQ(2.0f, x.at(0));
    EXPECT_EQ(3.0f, x.at(1));
    EXPECT_EQ(8.0f, x.at(2));
}


TEST_F(Test, TestCudaVector, getMaxOfColumnsFast){
    Math::CudaVector<f32> x(2);
    x.initComputation();

    Math::CudaMatrix<f32> X(100,2);
    for (u32 i = 0; i < 100; i++){
	X.at(i, 0) = (f32) (i % 7);
	X.at(i, 1) = (f32) (i / 6);
    }
    X.initComputation();

    Math::CudaMatrix<f32> tmp(32,2);
    tmp.initComputation(false);

    x.getMaxOfColumns(X, tmp);
    x.finishComputation();
    EXPECT_EQ(6.0f, x.at(0));
    EXPECT_EQ(16.0f, x.at(1));
}

TEST_F(Test, TestCudaVector, swap)
{
    Math::CudaVector<f64> x;
    Math::CudaVector<f64> y;
    x.resize(3);
    y.resize(3);
    x.at(0) = 1.0;
    x.at(1) = -2.0;
    x.at(2) = -4.0;
    y.at(0) = 3.5;
    y.at(1) = -1.5;
    y.at(2) = 0.0;
    x.initComputation();
    y.initComputation();
    x.swap(y);
    x.finishComputation();
    EXPECT_EQ(3.5, x.at(0));
    EXPECT_EQ(-1.5, x.at(1));
    EXPECT_EQ(0.0, x.at(2));

    Math::CudaVector<f32> x2;
    Math::CudaVector<f32> y2;
    x2.resize(3);
    y2.resize(3);
    x2.at(0) = 1.0;
    x2.at(1) = -2.0;
    x2.at(2) = -4.0;
    y2.at(0) = 3.5;
    y2.at(1) = -1.5;
    y2.at(2) = 0.0;
    x2.initComputation();
    y2.initComputation();
    x2.swap(y2);
    x2.finishComputation();
    EXPECT_EQ((f32)3.5, x2.at(0));
    EXPECT_EQ((f32)-1.5, x2.at(1));
    EXPECT_EQ((f32)0.0, x2.at(2));
}

TEST_F(Test, TestCudaVector, isFinite)
{
    Math::CudaVector<f64> x;
    x.resize(4);
    x.setToZero();
    EXPECT_TRUE(x.isFinite());
    x.at(2) = std::log(0.0);
    EXPECT_FALSE(x.isFinite());
    x.at(2) = std::sqrt(-1.0);
    EXPECT_FALSE(x.isFinite());

    Math::CudaVector<f32> x2;
    x2.resize(4);
    x2.setToZero();
    EXPECT_TRUE(x2.isFinite());
    x2.at(2) = std::log(0.0f);
    EXPECT_FALSE(x2.isFinite());
    x2.at(2) = std::sqrt(-1.0f);
    EXPECT_FALSE(x2.isFinite());

}

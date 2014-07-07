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
#include <Math/CudaMatrix.hh>

class TestCudaMatrix : public Test::ConfigurableFixture
{
};

TEST_F(Test, TestCudaMatrix, add)
{
    Math::CudaMatrix<f64> A;
    Math::CudaMatrix<f64> B;
    B.resize(2,3);
    A.copyStructure(B);
    A.at(0,0) = 1.0;
    A.at(0,1) = 2.0;
    A.at(0,2) = 3.0;
    A.at(1,0) = 4.0;
    A.at(1,1) = 5.0;
    A.at(1,2) = 6.0;
    B.at(0,0) = 2.0;
    B.at(0,1) = 4.0;
    B.at(0,2) = 6.0;
    B.at(1,0) = 8.0;
    B.at(1,1) = 10.0;
    B.at(1,2) = 12.0;
    A.initComputation();
    B.initComputation();
    A.add(B, 0.5);
    A.finishComputation();
    EXPECT_EQ(2.0, A.at(0,0));
    EXPECT_EQ(4.0, A.at(0,1));
    EXPECT_EQ(6.0, A.at(0,2));
    EXPECT_EQ(8.0, A.at(1,0));
    EXPECT_EQ(10.0, A.at(1,1));
    EXPECT_EQ(12.0, A.at(1,2));

    Math::CudaMatrix<f32> A2;
    Math::CudaMatrix<f32> B2;
    B2.resize(2,3);
    A2.copyStructure(B2);
    A2.at(0,0) = 1.0;
    A2.at(0,1) = 2.0;
    A2.at(0,2) = 3.0;
    A2.at(1,0) = 4.0;
    A2.at(1,1) = 5.0;
    A2.at(1,2) = 6.0;
    B2.at(0,0) = 2.0;
    B2.at(0,1) = 4.0;
    B2.at(0,2) = 6.0;
    B2.at(1,0) = 8.0;
    B2.at(1,1) = 10.0;
    B2.at(1,2) = 12.0;
    A2.initComputation();
    B2.initComputation();
    A2.add(B2, 0.5f);
    A2.finishComputation();
    EXPECT_EQ((f32)2.0, A2.at(0,0));
    EXPECT_EQ((f32)4.0, A2.at(0,1));
    EXPECT_EQ((f32)6.0, A2.at(0,2));
    EXPECT_EQ((f32)8.0, A2.at(1,0));
    EXPECT_EQ((f32)10.0, A2.at(1,1));
    EXPECT_EQ((f32)12.0, A2.at(1,2));

    Math::CudaMatrix<f64> A3;
    Math::CudaMatrix<f32> B3;
    B3.resize(2,3);
    A3.copyStructure(B3);
    A3.resize(2,3);
    A3.at(0,0) = 1.0;
    A3.at(0,1) = 2.0;
    A3.at(0,2) = 3.0;
    A3.at(1,0) = 4.0;
    A3.at(1,1) = 5.0;
    A3.at(1,2) = 6.0;
    B3.at(0,0) = 2.0;
    B3.at(0,1) = 4.0;
    B3.at(0,2) = 6.0;
    B3.at(1,0) = 8.0;
    B3.at(1,1) = 10.0;
    B3.at(1,2) = 12.0;
    A3.initComputation();
    B3.initComputation();
    A3.add(B3, 0.5f);
    A3.finishComputation();
    EXPECT_EQ(2.0, A3.at(0,0));
    EXPECT_EQ(4.0, A3.at(0,1));
    EXPECT_EQ(6.0, A3.at(0,2));
    EXPECT_EQ(8.0, A3.at(1,0));
    EXPECT_EQ(10.0, A3.at(1,1));
    EXPECT_EQ(12.0, A3.at(1,2));

}

TEST_F(Test, TestCudaMatrix, addMatrixProduct)
{

    // single precision

    Math::CudaMatrix<f32> A;
    Math::CudaMatrix<f32> B;
    Math::CudaMatrix<f32> C;

    A.resize(2,4);
    B.resize(4,3);
    C.resize(2,3);


    A.at(0,0) = 1; A.at(0,1) = 2; A.at(0,2) = 3; A.at(0,3) = 4;
    A.at(1,0) = 0; A.at(1,1) = 1; A.at(1,2) = 2; A.at(1,3) = 3;

    B.at(0,0) = 1; B.at(0,1) = 2; B.at(0,2) = 3;
    B.at(1,0) =-1; B.at(1,1) = 0; B.at(1,2) = 1;
    B.at(2,0) = 0; B.at(2,1) = 1; B.at(2,2) = 0;
    B.at(3,0) = 2; B.at(3,1) = 3; B.at(3,2) = 4;

    C.at(0,0) = 2; C.at(0,1) = 4; C.at(0,2) = 8;
    C.at(1,0) =-6; C.at(1,1) =-4; C.at(1,2) = 10;

    A.initComputation();
    B.initComputation();
    C.initComputation();

    C.addMatrixProduct(A,B,0.5f,3.0f,false,false);

    C.finishComputation();

    EXPECT_EQ(22.0f, C.at(0,0));
    EXPECT_EQ(53.0f, C.at(0,1));
    EXPECT_EQ(67.0f, C.at(0,2));
    EXPECT_EQ(12.0f, C.at(1,0));
    EXPECT_EQ(31.0f, C.at(1,1));
    EXPECT_EQ(44.0f, C.at(1,2));

    // double precision

    Math::CudaMatrix<f64> A2;
    Math::CudaMatrix<f64> B2;
    Math::CudaMatrix<f64> C2;

    A2.resize(2,4);
    B2.resize(4,3);
    C2.resize(2,3);


    A2.at(0,0) = 1; A2.at(0,1) = 2; A2.at(0,2) = 3; A2.at(0,3) = 4;
    A2.at(1,0) = 0; A2.at(1,1) = 1; A2.at(1,2) = 2; A2.at(1,3) = 3;

    B2.at(0,0) = 1; B2.at(0,1) = 2; B2.at(0,2) = 3;
    B2.at(1,0) =-1; B2.at(1,1) = 0; B2.at(1,2) = 1;
    B2.at(2,0) = 0; B2.at(2,1) = 1; B2.at(2,2) = 0;
    B2.at(3,0) = 2; B2.at(3,1) = 3; B2.at(3,2) = 4;

    C2.at(0,0) = 2; C2.at(0,1) = 4; C2.at(0,2) = 8;
    C2.at(1,0) =-6; C2.at(1,1) =-4; C2.at(1,2) = 10;

    A2.initComputation();
    B2.initComputation();
    C2.initComputation();

    C2.addMatrixProduct(A2,B2,0.5,3.0,false,false);

    C2.finishComputation();

    EXPECT_EQ(22.0, C2.at(0,0));
    EXPECT_EQ(53.0, C2.at(0,1));
    EXPECT_EQ(67.0, C2.at(0,2));
    EXPECT_EQ(12.0, C2.at(1,0));
    EXPECT_EQ(31.0, C2.at(1,1));
    EXPECT_EQ(44.0, C2.at(1,2));

}

TEST_F(Test, TestCudaMatrix, addMatrixProductMixedPrecision)
{
    // mixed precision
    for (u32 bunchSize = 1; bunchSize < 6; bunchSize++){
	Math::CudaDataStructure::setMultiprecisionBunchSize(bunchSize);
	Math::CudaMatrix<f32> A;
	Math::CudaMatrix<f32> B;
	Math::CudaMatrix<f64> C;

	A.resize(2,4);
	B.resize(4,3);
	C.resize(2,3);


	A.at(0,0) = 1; A.at(0,1) = 2; A.at(0,2) = 3; A.at(0,3) = 4;
	A.at(1,0) = 0; A.at(1,1) = 1; A.at(1,2) = 2; A.at(1,3) = 3;

	B.at(0,0) = 1; B.at(0,1) = 2; B.at(0,2) = 3;
	B.at(1,0) =-1; B.at(1,1) = 0; B.at(1,2) = 1;
	B.at(2,0) = 0; B.at(2,1) = 1; B.at(2,2) = 0;
	B.at(3,0) = 2; B.at(3,1) = 3; B.at(3,2) = 4;

	C.at(0,0) = 2; C.at(0,1) = 4; C.at(0,2) = 8;
	C.at(1,0) =-6; C.at(1,1) =-4; C.at(1,2) = 10;

	A.initComputation();
	B.initComputation();
	C.initComputation();

	C.addMatrixProduct(A,B,0.5,3.0f,false,false);

	C.finishComputation();

	EXPECT_EQ(22.0, C.at(0,0));
	EXPECT_EQ(53.0, C.at(0,1));
	EXPECT_EQ(67.0, C.at(0,2));
	EXPECT_EQ(12.0, C.at(1,0));
	EXPECT_EQ(31.0, C.at(1,1));
	EXPECT_EQ(44.0, C.at(1,2));

	// mixed precision, B transposed

	B.resize(3,4);
	B.finishComputation();

	B.at(0,0) = 1; B.at(1,0) = 2; B.at(2,0) = 3;
	B.at(0,1) =-1; B.at(1,1) = 0; B.at(2,1) = 1;
	B.at(0,2) = 0; B.at(1,2) = 1; B.at(2,2) = 0;
	B.at(0,3) = 2; B.at(1,3) = 3; B.at(2,3) = 4;

	C.at(0,0) = 2; C.at(0,1) = 4; C.at(0,2) = 8;
	C.at(1,0) =-6; C.at(1,1) =-4; C.at(1,2) = 10;

	A.initComputation();
	B.initComputation();
	C.initComputation();

	C.addMatrixProduct(A,B,0.5,3.0f,false,true);

	C.finishComputation();

	EXPECT_EQ(22.0, C.at(0,0));
	EXPECT_EQ(53.0, C.at(0,1));
	EXPECT_EQ(67.0, C.at(0,2));
	EXPECT_EQ(12.0, C.at(1,0));
	EXPECT_EQ(31.0, C.at(1,1));
	EXPECT_EQ(44.0, C.at(1,2));
    }
}



TEST_F(Test, TestCudaMatrix, sumOfSquares)
{
    Math::CudaMatrix<f64> A;
    A.resize(2,3);
    A.at(0,0) = 2.0;
    A.at(1,0) = -2.0;
    A.at(0,1) = 4.0;
    A.at(1,1) = -8.0;
    A.at(0,2) = 5.0;
    A.at(1,2) = -6.0;
    A.initComputation();
    EXPECT_EQ(149.0, A.sumOfSquares());

    Math::CudaMatrix<f32> A2;
    A2.resize(2,3);
    A2.at(0,0) = 2.0;
    A2.at(1,0) = -2.0;
    A2.at(0,1) = 4.0;
    A2.at(1,1) = -8.0;
    A2.at(0,2) = 5.0;
    A2.at(1,2) = -6.0;
    A2.initComputation();
    EXPECT_EQ((f32)149.0, A2.sumOfSquares());
}

TEST_F(Test, TestCudaMatrix, dot)
{
    Math::CudaMatrix<f32> A;
    Math::CudaMatrix<f32> B;
    A.resize(2,3);
    B.resize(2,3);
    A.at(0,0) = 1.0;
    A.at(0,1) = 2.0;
    A.at(0,2) = 4.0;
    A.at(1,0) = 6.0;
    A.at(1,1) = 0.0;
    A.at(1,2) = 4.0;
    B.at(0,0) = 2.0;
    B.at(0,1) = 0.0;
    B.at(0,2) = 1.0;
    B.at(1,0) = 5.0;
    B.at(1,1) = 6.0;
    B.at(1,2) = 0.0;
    A.initComputation();
    B.initComputation();
    f64 result = A.dot(B);
    EXPECT_EQ(36.0, result);

    Math::CudaMatrix<f32> A2;
    Math::CudaMatrix<f32> B2;
    A2.resize(2,3);
    B2.resize(2,3);
    A2.at(0,0) = 1.0;
    A2.at(0,1) = 2.0;
    A2.at(0,2) = 4.0;
    A2.at(1,0) = 6.0;
    A2.at(1,1) = 0.0;
    A2.at(1,2) = 4.0;
    B2.at(0,0) = 2.0;
    B2.at(0,1) = 0.0;
    B2.at(0,2) = 1.0;
    B2.at(1,0) = 5.0;
    B2.at(1,1) = 6.0;
    B2.at(1,2) = 0.0;
    A2.initComputation();
    B2.initComputation();
    result = A2.dot(B2);
    EXPECT_EQ(36.0, result);
}

TEST_F(Test, TestCudaMatrix, dotWithColumn)
{
    Math::CudaMatrix<f32> A;
    Math::CudaMatrix<f32> B;
    A.resize(2,3);
    B.resize(2,1);
    A.at(0,0) = 1.0;
    A.at(0,1) = 2.0;
    A.at(0,2) = 4.0;
    A.at(1,0) = 6.0;
    A.at(1,1) = 1.0;
    A.at(1,2) = 4.0;
    B.at(0,0) = 2.0;
    B.at(1,0) = 3.0;
    A.initComputation();
    B.initComputation();
    f32 result = A.dotWithColumn(B, 1);
    EXPECT_EQ(7.0f, result);

    Math::CudaMatrix<f64> A2;
    Math::CudaMatrix<f64> B2;
    A2.resize(2,3);
    B2.resize(2,1);
    A2.at(0,0) = 1.0;
    A2.at(0,1) = 2.0;
    A2.at(0,2) = 4.0;
    A2.at(1,0) = 6.0;
    A2.at(1,1) = 1.0;
    A2.at(1,2) = 4.0;
    B2.at(0,0) = 2.0;
    B2.at(1,0) = 3.0;
    A2.initComputation();
    B2.initComputation();
    f64 result2 = A2.dotWithColumn(B2, 1);
    EXPECT_EQ(7.0, result2);

}


TEST_F(Test, TestCudaMatrix, scale)
{
    Math::CudaMatrix<f64> A;
    A.resize(2,3);
    A.at(0,0) = 2.0;
    A.at(0,1) = 0.0;
    A.at(0,2) = 5.0;
    A.at(1,0) = 1.0;
    A.at(1,1) = 1.8;
    A.at(1,2) = 6.0;
    A.initComputation();
    A.scale(0.5);
    A.finishComputation();
    EXPECT_EQ(1.0, A.at(0,0));
    EXPECT_EQ(0.0, A.at(0,1));
    EXPECT_EQ(2.5, A.at(0,2));
    EXPECT_EQ(0.5, A.at(1,0));
    EXPECT_EQ(0.9, A.at(1,1));
    EXPECT_EQ(3.0, A.at(1,2));

    Math::CudaMatrix<f32> A2;
    A2.resize(2,3);
    A2.at(0,0) = 2.0;
    A2.at(0,1) = 0.0;
    A2.at(0,2) = 5.0;
    A2.at(1,0) = 1.0;
    A2.at(1,1) = 1.8;
    A2.at(1,2) = 6.0;
    A2.initComputation();
    A2.scale(0.5);
    A2.finishComputation();
    EXPECT_EQ((f32)1.0, A2.at(0,0));
    EXPECT_EQ((f32)0.0, A2.at(0,1));
    EXPECT_EQ((f32)2.5, A2.at(0,2));
    EXPECT_EQ((f32)0.5, A2.at(1,0));
    EXPECT_EQ((f32)0.9, A2.at(1,1));
    EXPECT_EQ((f32)3.0, A2.at(1,2));
}

TEST_F(Test, TestCudaMatrix, copy)
{
    Math::CudaMatrix<f64> A;
    Math::CudaMatrix<f64> B;
    A.resize(2,3);
    B.resize(2,3);
    A.at(0,0) = 2.0;
    A.at(0,1) = 8.0;
    A.at(0,2) = 4.0;
    A.at(1,0) = 10.0;
    A.at(1,1) = 6.0;
    A.at(1,2) = 12.0;
    B.initComputation();
    A.initComputation();
    B.copy(A);
    B.finishComputation();
    EXPECT_EQ(2.0, B.at(0,0));
    EXPECT_EQ(8.0, B.at(0,1));
    EXPECT_EQ(4.0, B.at(0,2));
    EXPECT_EQ(10.0, B.at(1,0));
    EXPECT_EQ(6.0, B.at(1,1));
    EXPECT_EQ(12.0, B.at(1,2));

    Math::CudaMatrix<f32> A2;
    Math::CudaMatrix<f32> B2;
    A2.resize(2,3);
    B2.resize(2,3);
    A2.at(0,0) = 2.0;
    A2.at(0,1) = 8.0;
    A2.at(0,2) = 4.0;
    A2.at(1,0) = 10.0;
    A2.at(1,1) = 6.0;
    A2.at(1,2) = 12.0;
    B2.initComputation();
    A2.initComputation();
    B2.copy(A2);
    B2.finishComputation();
    EXPECT_EQ((f32)2.0, B2.at(0,0));
    EXPECT_EQ((f32)8.0, B2.at(0,1));
    EXPECT_EQ((f32)4.0, B2.at(0,2));
    EXPECT_EQ((f32)10.0, B2.at(1,0));
    EXPECT_EQ((f32)6.0, B2.at(1,1));
    EXPECT_EQ((f32)12.0, B2.at(1,2));
}

TEST_F(Test, TestCudaMatrix, copyArray)
{
    Math::CudaMatrix<f64> A;
    std::vector<f64> x(3);
    x.at(0) = 1;
    x.at(1) = 2;
    x.at(2) = 3;
    A.resize(2,3);
    A.initComputation();
    A.setToZero();
    A.copy(&(x.at(0)), 1, 1);
    A.finishComputation();
    EXPECT_EQ(0.0, A.at(0,0));
    EXPECT_EQ(0.0, A.at(0,1));
    EXPECT_EQ(2.0, A.at(0,2));
    EXPECT_EQ(0.0, A.at(1,0));
    EXPECT_EQ(1.0, A.at(1,1));
    EXPECT_EQ(3.0, A.at(1,2));

    Math::CudaMatrix<f32> A2;
    std::vector<f32> x2(3);
    x2.at(0) = 1;
    x2.at(1) = 2;
    x2.at(2) = 3;
    A2.resize(2,3);
    A2.initComputation();
    A2.setToZero();
    A2.copy(&(x2.at(0)), 1, 1);
    A2.finishComputation();
    EXPECT_EQ(0.0f, A2.at(0,0));
    EXPECT_EQ(0.0f, A2.at(0,1));
    EXPECT_EQ(2.0f, A2.at(0,2));
    EXPECT_EQ(0.0f, A2.at(1,0));
    EXPECT_EQ(1.0f, A2.at(1,1));
    EXPECT_EQ(3.0f, A2.at(1,2));

}


TEST_F(Test, TestCudaMatrix, multiply)
{
    Math::CudaMatrix<f64> A;
    Math::CudaVector<f64> x;
    Math::CudaVector<f64> y;
    A.resize(2,3);
    x.resize(3);
    y.resize(2);
    A.at(0,0) = 2.0;
    A.at(1,0) = -2.0;
    A.at(0,1) = 4.0;
    A.at(1,1) = -8.0;
    A.at(0,2) = 5.0;
    A.at(1,2) = -6.0;
    x.at(0) = 2.0;
    x.at(1) = -2.0;
    x.at(2) = -1.0;
    y.at(0) = 1.0;
    y.at(1) = 2.0;
    A.initComputation();
    x.initComputation();
    y.initComputation();
    A.multiply(x, y, false, 2, 2, 0);
    y.finishComputation();
    EXPECT_EQ(-16.0, y.at(0));
    EXPECT_EQ(40.0, y.at(1));

    Math::CudaMatrix<f32> A2;
    Math::CudaVector<f32> x2;
    Math::CudaVector<f32> y2;
    A2.resize(2,3);
    x2.resize(3);
    y2.resize(2);
    A2.at(0,0) = 2.0;
    A2.at(1,0) = -2.0;
    A2.at(0,1) = 4.0;
    A2.at(1,1) = -8.0;
    A2.at(0,2) = 5.0;
    A2.at(1,2) = -6.0;
    x2.at(0) = 2.0;
    x2.at(1) = -2.0;
    x2.at(2) = -1.0;
    y2.at(0) = 1.0;
    y2.at(1) = 2.0;
    A2.initComputation();
    x2.initComputation();
    y2.initComputation();
    A2.multiply(x2, y2, false, 2, 2, 0);
    y2.finishComputation();
    EXPECT_EQ((f32)-16.0, y2.at(0));
    EXPECT_EQ((f32)40.0, y2.at(1));
}

TEST_F(Test, TestCudaMatrix, l1norm)
{
    Math::CudaMatrix<f64> A;
    A.resize(2,3);
    A.at(0,0) = 2.0;
    A.at(1,0) = -2.0;
    A.at(0,1) = 4.0;
    A.at(1,1) = -8.0;
    A.at(0,2) = 5.0;
    A.at(1,2) = -6.0;
    A.initComputation();
    EXPECT_EQ(27.0, A.l1norm());

    Math::CudaMatrix<f32> A2;
    A2.resize(2,3);
    A2.at(0,0) = 2.0;
    A2.at(1,0) = -2.0;
    A2.at(0,1) = 4.0;
    A2.at(1,1) = -8.0;
    A2.at(0,2) = 5.0;
    A2.at(1,2) = -6.0;
    A2.initComputation();
    EXPECT_EQ((f32)27.0, A2.l1norm());
}

TEST_F(Test, TestCudaMatrix, argAbsMax)
{
    Math::CudaMatrix<f64> A;
    A.resize(3,2);
    A.at(0,0) = 2.0;
    A.at(1,0) = -2.0;
    A.at(2,0) = 3.0;
    A.at(0,1) = 0.0;
    A.at(1,1) = -3.0;
    A.at(2,1) = 2.0;
    A.initComputation();
    EXPECT_EQ((u32)2, A.argAbsMax(0));
    EXPECT_EQ((u32)1, A.argAbsMax(1));

    Math::CudaMatrix<f32> A2;
    A2.resize(3,2);
    A2.at(0,0) = 2.0;
    A2.at(1,0) = -2.0;
    A2.at(2,0) = 3.0;
    A2.at(0,1) = 0.0;
    A2.at(1,1) = -3.0;
    A2.at(2,1) = 2.0;
    A2.initComputation();
    EXPECT_EQ((u32)2, A2.argAbsMax(0));
    EXPECT_EQ((u32)1, A2.argAbsMax(1));
}

TEST_F(Test, TestCudaMatrix, addOuterProduct)
{
    Math::CudaMatrix<f64> A(2,3);
    Math::CudaVector<f64> x(2);
    Math::CudaVector<f64> y(3);
    A.at(0,0) = 1.0;
    A.at(0,1) = 3.0;
    A.at(0,2) = 8.0;
    A.at(1,0) = 2.0;
    A.at(1,1) = 1.0;
    A.at(1,2) = 0.5;
    x.at(0) = 2.0;
    x.at(1) = 3.0;
    y.at(0) = 2.0;
    y.at(1) = 4.0;
    y.at(2) = 1.0;
    A.initComputation();
    x.initComputation();
    y.initComputation();
    A.addOuterProduct(x,y,0.5);
    A.finishComputation();
    EXPECT_EQ(3.0, A.at(0,0));
    EXPECT_EQ(7.0, A.at(0,1));
    EXPECT_EQ(9.0, A.at(0,2));
    EXPECT_EQ(5.0, A.at(1,0));
    EXPECT_EQ(7.0, A.at(1,1));
    EXPECT_EQ(2.0, A.at(1,2));

    Math::CudaMatrix<f64> B(3,2);
    Math::CudaVector<f64> z(3);
    Math::CudaVector<f64> t(2);
    B.at(0,0) = 8.0;
    B.at(0,1) = 2.0;
    B.at(1,0) = 5.0;
    B.at(1,1) = 1.0;
    B.at(2,0) = 4.0;
    B.at(2,1) = 1.5;
    z.at(0) = 1.0;
    z.at(1) = 0.0;
    z.at(2) = 2.0;
    t.at(0) = 3.0;
    t.at(1) = 8.0;
    B.initComputation();
    z.initComputation();
    t.initComputation();
    B.addOuterProduct(z,t,0.0);
    B.finishComputation();
    EXPECT_EQ(8.0, B.at(0,0));
    EXPECT_EQ(2.0, B.at(0,1));
    EXPECT_EQ(5.0, B.at(1,0));
    EXPECT_EQ(1.0, B.at(1,1));
    EXPECT_EQ(4.0, B.at(2,0));
    EXPECT_EQ(1.5, B.at(2,1));

    Math::CudaMatrix<f32> A2(2,3);
    Math::CudaVector<f32> x2(2);
    Math::CudaVector<f32> y2(3);
    A2.at(0,0) = 1.0;
    A2.at(0,1) = 3.0;
    A2.at(0,2) = 8.0;
    A2.at(1,0) = 2.0;
    A2.at(1,1) = 1.0;
    A2.at(1,2) = 0.5;
    x2.at(0) = 2.0;
    x2.at(1) = 3.0;
    y2.at(0) = 2.0;
    y2.at(1) = 4.0;
    y2.at(2) = 1.0;
    A2.initComputation();
    x2.initComputation();
    y2.initComputation();
    A2.addOuterProduct(x2,y2,0.5);
    A2.finishComputation();
    EXPECT_EQ((f32)3.0, A2.at(0,0));
    EXPECT_EQ((f32)7.0, A2.at(0,1));
    EXPECT_EQ((f32)9.0, A2.at(0,2));
    EXPECT_EQ((f32)5.0, A2.at(1,0));
    EXPECT_EQ((f32)7.0, A2.at(1,1));
    EXPECT_EQ((f32)2.0, A2.at(1,2));

    Math::CudaMatrix<f32> B2(3,2);
    Math::CudaVector<f32> z2(3);
    Math::CudaVector<f32> t2(2);
    B2.at(0,0) = 8.0;
    B2.at(0,1) = 2.0;
    B2.at(1,0) = 5.0;
    B2.at(1,1) = 1.0;
    B2.at(2,0) = 4.0;
    B2.at(2,1) = 1.5;
    z2.at(0) = 1.0;
    z2.at(1) = 0.0;
    z2.at(2) = 2.0;
    t2.at(0) = 3.0;
    t2.at(1) = 8.0;
    B2.initComputation();
    z2.initComputation();
    t2.initComputation();
    B2.addOuterProduct(z2,t2,0.0);
    B2.finishComputation();
    EXPECT_EQ((f32)8.0, B2.at(0,0));
    EXPECT_EQ((f32)2.0, B2.at(0,1));
    EXPECT_EQ((f32)5.0, B2.at(1,0));
    EXPECT_EQ((f32)1.0, B2.at(1,1));
    EXPECT_EQ((f32)4.0, B2.at(2,0));
    EXPECT_EQ((f32)1.5, B2.at(2,1));
}

TEST_F(Test, TestCudaMatrix, getRow)
{
    Math::CudaMatrix<f64> A;
    Math::CudaVector<f64> b;
    A.resize(2,3);
    A.at(0,0) = 6.0;
    A.at(0,1) = -2.0;
    A.at(0,2) = 1.0;
    A.at(1,0) = 3.0;
    A.at(1,1) = 2.0;
    A.at(1,2) = 0.0;
    A.initComputation();
    b.initComputation();
    A.getRow(1,b);
    b.finishComputation();
    EXPECT_EQ(3u, b.size());
    EXPECT_EQ(3.0, b.at(0));
    EXPECT_EQ(2.0, b.at(1));
    EXPECT_EQ(0.0, b.at(2));

    Math::CudaMatrix<f32> A2;
    Math::CudaVector<f32> b2;
    A2.resize(2,3);
    A2.at(0,0) = 6.0;
    A2.at(0,1) = -2.0;
    A2.at(0,2) = 1.0;
    A2.at(1,0) = 3.0;
    A2.at(1,1) = 2.0;
    A2.at(1,2) = 0.0;
    A2.initComputation();
    b2.initComputation();
    A2.getRow(1,b2);
    b2.finishComputation();
    EXPECT_EQ(3u, b2.size());
    EXPECT_EQ((f32)3.0, b2.at(0));
    EXPECT_EQ((f32)2.0, b2.at(1));
    EXPECT_EQ((f32)0.0, b2.at(2));
}

TEST_F(Test, TestCudaMatrix, getColumn)
{
    Math::CudaMatrix<f64> A;
    Math::CudaVector<f64> b;
    A.resize(2,3);
    A.at(0,0) = 0.0;
    A.at(0,1) = 4.0;
    A.at(0,2) = -1.0;
    A.at(1,0) = 6.0;
    A.at(1,1) = 1.0;
    A.at(1,2) = 6.0;
    A.initComputation();
    b.initComputation();
    A.getColumn(2,b);
    b.finishComputation();
    EXPECT_EQ(2u, b.size());
    EXPECT_EQ(-1.0, b.at(0));
    EXPECT_EQ(6.0, b.at(1));

    Math::CudaMatrix<f32> A2;
    Math::CudaVector<f32> b2;
    A2.resize(2,3);
    A2.at(0,0) = 0.0;
    A2.at(0,1) = 4.0;
    A2.at(0,2) = -1.0;
    A2.at(1,0) = 6.0;
    A2.at(1,1) = 1.0;
    A2.at(1,2) = 6.0;
    A2.initComputation();
    b2.initComputation();
    A2.getColumn(2,b2);
    b2.finishComputation();
    EXPECT_EQ(2u, b2.size());
    EXPECT_EQ((f32)-1.0, b2.at(0));
    EXPECT_EQ((f32)6.0, b2.at(1));

    Math::CudaMatrix<f32> A3;
    Math::CudaVector<f32> b3;
    A3.resize(3,2);
    A3.at(0,0) = 0.0;
    A3.at(0,1) = 4.0;
    A3.at(1,0) = 6.0;
    A3.at(1,1) = 1.0;
    A3.at(2,0) = -1.0;
    A3.at(2,1) = 5.0;
    A3.initComputation();
    b3.initComputation();
    A3.getColumn(1,b3);
    b3.finishComputation();
    EXPECT_EQ(3u, b3.size());
    EXPECT_EQ((f32)4.0, b3.at(0));
    EXPECT_EQ((f32)1.0, b3.at(1));
    EXPECT_EQ((f32)5.0, b3.at(2));
}

TEST_F(Test, TestCudaMatrix, copyBlockToMatrix)
{
    Math::FastMatrix<f64> A(4,3);
    Math::CudaMatrix<f64> B(3,4);
    f64 val = 0.0;
    for (int i = 0; i < 4; i++){
	for (int j = 0; j < 3; j++){
	    val = val + 1.0;
	    A.at(i,j) = val;
	}
    }
    for (int i = 0; i < 3; i++){
	for (int j = 0; j < 4; j++){
	    B.at(i,j) = 0.0;
	}
    }
    B.copyBlockFromMatrix(A, 2, 0, 0, 1, 2, 3);
    EXPECT_EQ(7.0, B.at(0,1));
    EXPECT_EQ(8.0, B.at(0,2));
    EXPECT_EQ(9.0, B.at(0,3));
    EXPECT_EQ(10.0, B.at(1,1));
    EXPECT_EQ(11.0, B.at(1,2));
    EXPECT_EQ(12.0, B.at(1,3));
}

TEST_F(Test, TestCudaMatrix, copyBlockToMatrixCuda)
{
    Math::CudaMatrix<f64> A(4,3);
    Math::CudaMatrix<f64> B(3,4);
    f64 val = 0.0;
    for (int i = 0; i < 4; i++){
	for (int j = 0; j < 3; j++){
	    val = val + 1.0;
	    A.at(i,j) = val;
	}
    }
    A.initComputation();
    B.initComputation();
    B.setToZero();
    B.copyBlockFromMatrix(A, 2, 0, 0, 1, 2, 3);
    B.finishComputation();
    EXPECT_EQ(7.0, B.at(0,1));
    EXPECT_EQ(8.0, B.at(0,2));
    EXPECT_EQ(9.0, B.at(0,3));
    EXPECT_EQ(10.0, B.at(1,1));
    EXPECT_EQ(11.0, B.at(1,2));
    EXPECT_EQ(12.0, B.at(1,3));
}


TEST_F(Test, TestCudaMatrix, setToZero)
{
    Math::CudaMatrix<f64> A;
    A.resize(4,3);
    A.initComputation();
    A.setToZero();
    A.finishComputation();
    A.at(0,0) = 4.0;
    A.at(0,1) = 6.0;
    A.at(0,2) = 1.5;
    A.at(1,0) = 7.0;
    A.at(1,1) = 13.0;
    A.at(1,2) = 2.5;
    A.at(2,0) = 5.0;
    A.at(2,1) = -1.0;
    A.at(2,2) = 4.0;
    A.at(3,0) = 3.0;
    A.at(3,1) = -4.0;
    A.at(3,2) = 8.0;
    EXPECT_EQ(A.nRows(),4u);
    EXPECT_EQ(A.nColumns(),3u);
    EXPECT_EQ(A.size(),12u);
    A.initComputation();
    A.setToZero();
    A.finishComputation();
    for (u32 i = 0; i < 4; i++)
	for (u32 j = 0; j < 3; j++)
	    EXPECT_EQ(A.at(i,j), 0.0);

    Math::CudaMatrix<f32> A2;
    A2.resize(4,3);
    A2.initComputation();
    A2.setToZero();
    A2.finishComputation();
    A2.at(0,0) = 4.0;
    A2.at(0,1) = 6.0;
    A2.at(0,2) = 1.5;
    A2.at(1,0) = 7.0;
    A2.at(1,1) = 13.0;
    A2.at(1,2) = 2.5;
    A2.at(2,0) = 5.0;
    A2.at(2,1) = -1.0;
    A2.at(2,2) = 4.0;
    A2.at(3,0) = 3.0;
    A2.at(3,1) = -4.0;
    A2.at(3,2) = 8.0;
    EXPECT_EQ(A2.nRows(),4u);
    EXPECT_EQ(A2.nColumns(),3u);
    EXPECT_EQ(A2.size(),12u);
    A2.initComputation();
    A2.setToZero();
    A2.finishComputation();
    for (u32 i = 0; i < 4; i++)
	for (u32 j = 0; j < 3; j++)
	    EXPECT_EQ(A2.at(i,j), (f32)0.0);

    Math::CudaMatrix<f64> A3;
    A3.resize(4,3);
    A3.setToZero();
    A3.initComputation();
    A3.finishComputation();
    A3.at(0,0) = 4.0;
    A3.at(0,1) = 6.0;
    A3.at(0,2) = 1.5;
    A3.at(1,0) = 7.0;
    A3.at(1,1) = 13.0;
    A3.at(1,2) = 2.5;
    A3.at(2,0) = 5.0;
    A3.at(2,1) = -1.0;
    A3.at(2,2) = 4.0;
    A3.at(3,0) = 3.0;
    A3.at(3,1) = -4.0;
    A3.at(3,2) = 8.0;
    EXPECT_EQ(A3.nRows(),4u);
    EXPECT_EQ(A3.nColumns(),3u);
    EXPECT_EQ(A3.size(),12u);
    A3.initComputation();
    A3.setToZero();
    A3.finishComputation();
    for (u32 i = 0; i < 4; i++)
	for (u32 j = 0; j < 3; j++)
	    EXPECT_EQ(A3.at(i,j), 0.0);

    Math::CudaMatrix<f32> A4;
    A4.resize(4,3);
    A4.setToZero();
    A4.initComputation();
    A4.finishComputation();
    A4.at(0,0) = 4.0;
    A4.at(0,1) = 6.0;
    A4.at(0,2) = 1.5;
    A4.at(1,0) = 7.0;
    A4.at(1,1) = 13.0;
    A4.at(1,2) = 2.5;
    A4.at(2,0) = 5.0;
    A4.at(2,1) = -1.0;
    A4.at(2,2) = 4.0;
    A4.at(3,0) = 3.0;
    A4.at(3,1) = -4.0;
    A4.at(3,2) = 8.0;
    EXPECT_EQ(A4.nRows(),4u);
    EXPECT_EQ(A4.nColumns(),3u);
    EXPECT_EQ(A4.size(),12u);
    A4.initComputation();
    A4.setToZero();
    A4.finishComputation();
    for (u32 i = 0; i < 4; i++)
	for (u32 j = 0; j < 3; j++)
	    EXPECT_EQ(A4.at(i,j), (f32)0.0);

}

TEST_F(Test, TestCudaMatrix, fill)
{
    Math::CudaMatrix<f64> A;
    A.resize(4,3);
    A.initComputation();
    A.fill(0);
    A.finishComputation();
    A.at(0,0) = 4.0;
    A.at(0,1) = 6.0;
    A.at(0,2) = 1.5;
    A.at(1,0) = 7.0;
    A.at(1,1) = 13.0;
    A.at(1,2) = 2.5;
    A.at(2,0) = 5.0;
    A.at(2,1) = -1.0;
    A.at(2,2) = 4.0;
    A.at(3,0) = 3.0;
    A.at(3,1) = -4.0;
    A.at(3,2) = 8.0;
    EXPECT_EQ(A.nRows(),4u);
    EXPECT_EQ(A.nColumns(),3u);
    EXPECT_EQ(A.size(),12u);
    A.initComputation();
    A.fill(10.0);
    A.finishComputation();
    for (u32 i = 0; i < 4; i++)
	for (u32 j = 0; j < 3; j++)
	    EXPECT_EQ(A.at(i,j), 10.0);

    Math::CudaMatrix<f32> A2;
    A2.resize(4,3);
    A2.initComputation();
    A2.fill(0);
    A2.finishComputation();
    A2.at(0,0) = 4.0;
    A2.at(0,1) = 6.0;
    A2.at(0,2) = 1.5;
    A2.at(1,0) = 7.0;
    A2.at(1,1) = 13.0;
    A2.at(1,2) = 2.5;
    A2.at(2,0) = 5.0;
    A2.at(2,1) = -1.0;
    A2.at(2,2) = 4.0;
    A2.at(3,0) = 3.0;
    A2.at(3,1) = -4.0;
    A2.at(3,2) = 8.0;
    EXPECT_EQ(A2.nRows(),4u);
    EXPECT_EQ(A2.nColumns(),3u);
    EXPECT_EQ(A2.size(),12u);
    A2.initComputation();
    A2.fill(10.0);
    A2.finishComputation();
    for (u32 i = 0; i < 4; i++)
	for (u32 j = 0; j < 3; j++)
	    EXPECT_EQ(A2.at(i,j), (f32)10.0);
}

TEST_F(Test, TestCudaMatrix, ensureMinimalValue)
{
    Math::CudaMatrix<f64> A;
    A.resize(4,3);
    A.at(0,0) = 4.0;
    A.at(0,1) = 6.0;
    A.at(0,2) = 1.5;
    A.at(1,0) = 7.0;
    A.at(1,1) = 13.0;
    A.at(1,2) = 2.5;
    A.at(2,0) = 5.0;
    A.at(2,1) = -1.0;
    A.at(2,2) = 4.0;
    A.at(3,0) = 3.0;
    A.at(3,1) = -4.0;
    A.at(3,2) = 8.0;
    A.initComputation();
    A.ensureMinimalValue(2.0);
    A.finishComputation();
    EXPECT_EQ(A.at(0,0), 4.0);
    EXPECT_EQ(A.at(0,1), 6.0);
    EXPECT_EQ(A.at(0,2), 2.0);
    EXPECT_EQ(A.at(1,0), 7.0);
    EXPECT_EQ(A.at(1,1), 13.0);
    EXPECT_EQ(A.at(1,2), 2.5);
    EXPECT_EQ(A.at(2,0), 5.0);
    EXPECT_EQ(A.at(2,1), 2.0);
    EXPECT_EQ(A.at(2,2), 4.0);
    EXPECT_EQ(A.at(3,0), 3.0);
    EXPECT_EQ(A.at(3,1), 2.0);
    EXPECT_EQ(A.at(3,2), 8.0);

    Math::CudaMatrix<f32> A2;
    A2.resize(4,3);
    A2.at(0,0) = 4.0;
    A2.at(0,1) = 6.0;
    A2.at(0,2) = 1.5;
    A2.at(1,0) = 7.0;
    A2.at(1,1) = 13.0;
    A2.at(1,2) = 2.5;
    A2.at(2,0) = 5.0;
    A2.at(2,1) = -1.0;
    A2.at(2,2) = 4.0;
    A2.at(3,0) = 3.0;
    A2.at(3,1) = -4.0;
    A2.at(3,2) = 8.0;
    A2.initComputation();
    A2.ensureMinimalValue(2.0);
    A2.finishComputation();
    EXPECT_EQ(A2.at(0,0), 4.0f);
    EXPECT_EQ(A2.at(0,1), 6.0f);
    EXPECT_EQ(A2.at(0,2), 2.0f);
    EXPECT_EQ(A2.at(1,0), 7.0f);
    EXPECT_EQ(A2.at(1,1), 13.0f);
    EXPECT_EQ(A2.at(1,2), 2.5f);
    EXPECT_EQ(A2.at(2,0), 5.0f);
    EXPECT_EQ(A2.at(2,1), 2.0f);
    EXPECT_EQ(A2.at(2,2), 4.0f);
    EXPECT_EQ(A2.at(3,0), 3.0f);
    EXPECT_EQ(A2.at(3,1), 2.0f);
    EXPECT_EQ(A2.at(3,2), 8.0f);
}

TEST_F(Test, TestCudaMatrix, exp)
{
    Math::CudaMatrix<f64> A;
    A.resize(2,3);
    for (int i = 0; i < 3; i++){
	A.at(0,i) = (double)i;
	A.at(1,i) = (double)-i;
    }
    A.initComputation();
    A.exp();
    A.finishComputation();
    for (int i = 0; i < 3; i++){
	EXPECT_DOUBLE_EQ(A.at(0,i), std::exp((double)i), 0.0000000001);
	EXPECT_DOUBLE_EQ(A.at(1,i), std::exp((double)-i), 0.0000000001);
    }

    Math::CudaMatrix<f32> B;
    B.resize(2,3);
    for (int i = 0; i < 3; i++){
	B.at(0,i) = (float)i;
	B.at(1,i) = (float)-i;
    }
    B.initComputation();
    B.exp();
    B.finishComputation();
    for (int i = 0; i < 3; i++){
	EXPECT_DOUBLE_EQ(B.at(0,i), std::exp((float)i), 0.000001);
	EXPECT_DOUBLE_EQ(B.at(1,i), std::exp((float)-i), 0.000001);
    }
}

TEST_F(Test, TestCudaMatrix, log)
{
    Math::CudaMatrix<f64> A;
    A.resize(2,3);
    for (int i = 0; i < 3; i++){
	A.at(0,i) = (double)(i + 1);
	A.at(1,i) = (double)2*(i + 1);
    }
    A.initComputation();
    A.log();
    A.finishComputation();
    for (int i = 0; i < 3; i++){
	EXPECT_DOUBLE_EQ(A.at(0,i), std::log((double)(i + 1)), 0.0000000001);
	EXPECT_DOUBLE_EQ(A.at(1,i), std::log((double)2*(i + 1)), 0.0000000001);
    }

    Math::CudaMatrix<f32> B;
    B.resize(2,3);
    for (int i = 0; i < 3; i++){
	B.at(0,i) = (float)(i + 1);
	B.at(1,i) = (float)2*(i + 1);
    }
    B.initComputation();
    B.log();
    B.finishComputation();
    for (int i = 0; i < 3; i++){
	EXPECT_DOUBLE_EQ(B.at(0,i), std::log((float)(i + 1)), 0.000001);
	EXPECT_DOUBLE_EQ(B.at(1,i), std::log((float)2*(i + 1)), 0.000001);
    }
}


TEST_F(Test, TestCudaMatrix, sigmoid)
{
    Math::CudaMatrix<f64> A;
    A.resize(3,2);
    A.at(0,0) = 1.0; A.at(0,1) = 2.0;
    A.at(1,0) = -1.0; A.at(1,1) = 3.0;
    A.at(2,0) = 4.0; A.at(2,1) = -4.0;

    A.initComputation();
    A.sigmoid();
    A.finishComputation();

    EXPECT_DOUBLE_EQ(0.7310585786300049, A.at(0,0), 0.000001);
    EXPECT_DOUBLE_EQ(0.88079707797788231, A.at(0,1), 0.000001);
    EXPECT_DOUBLE_EQ(0.2689414213699951, A.at(1,0), 0.000001);
    EXPECT_DOUBLE_EQ(0.95257412682243336, A.at(1,1), 0.000001);
    EXPECT_DOUBLE_EQ(0.98201379003790845, A.at(2,0), 0.000001);
    EXPECT_DOUBLE_EQ(0.017986209962091559, A.at(2,1), 0.000001);

    A.at(0,0) = 0.5; A.at(0,1) = 1.0;
    A.at(1,0) = -0.5; A.at(1,1) = 1.5;
    A.at(2,0) = 2.0; A.at(2,1) = -2.0;

    A.initComputation();
    A.sigmoid(2.0);
    A.finishComputation();

    EXPECT_DOUBLE_EQ(0.7310585786300049, A.at(0,0), 0.000001);
    EXPECT_DOUBLE_EQ(0.88079707797788231, A.at(0,1), 0.000001);
    EXPECT_DOUBLE_EQ(0.2689414213699951, A.at(1,0), 0.000001);
    EXPECT_DOUBLE_EQ(0.95257412682243336, A.at(1,1), 0.000001);
    EXPECT_DOUBLE_EQ(0.98201379003790845, A.at(2,0), 0.000001);
    EXPECT_DOUBLE_EQ(0.017986209962091559, A.at(2,1), 0.000001);

    // test behavior for very large values
    A.resize(1,3);
    A.at(0,0) = 710.0;
    A.at(0,1) = 0.0;
    A.at(0,2) = -710.0;

    A.initComputation();
    A.sigmoid();
    A.finishComputation();

    EXPECT_DOUBLE_EQ(1.0, A.at(0,0), 0.000001);
    EXPECT_DOUBLE_EQ(0.5, A.at(0,1), 0.000001);
    EXPECT_DOUBLE_EQ(0.0, A.at(0,2), 0.000001);


    Math::CudaMatrix<f32> A2;
    A2.resize(3,2);
    A2.at(0,0) = 1.0; A2.at(0,1) = 2.0;
    A2.at(1,0) = -1.0; A2.at(1,1) = 3.0;
    A2.at(2,0) = 4.0; A2.at(2,1) = -4.0;

    A2.initComputation();
    A2.sigmoid();
    A2.finishComputation();

    EXPECT_DOUBLE_EQ(0.7310585786300049, A2.at(0,0), 0.000001);
    EXPECT_DOUBLE_EQ(0.88079707797788231, A2.at(0,1), 0.000001);
    EXPECT_DOUBLE_EQ(0.2689414213699951, A2.at(1,0), 0.000001);
    EXPECT_DOUBLE_EQ(0.95257412682243336, A2.at(1,1), 0.000001);
    EXPECT_DOUBLE_EQ(0.98201379003790845, A2.at(2,0), 0.000001);
    EXPECT_DOUBLE_EQ(0.017986209962091559, A2.at(2,1), 0.000001);

    A2.at(0,0) = 0.5; A2.at(0,1) = 1.0;
    A2.at(1,0) = -0.5; A2.at(1,1) = 1.5;
    A2.at(2,0) = 2.0; A2.at(2,1) = -2.0;

    A2.initComputation();
    A2.sigmoid(2.0);
    A2.finishComputation();

    EXPECT_DOUBLE_EQ(0.7310585786300049, A2.at(0,0), 0.000001);
    EXPECT_DOUBLE_EQ(0.88079707797788231, A2.at(0,1), 0.000001);
    EXPECT_DOUBLE_EQ(0.2689414213699951, A2.at(1,0), 0.000001);
    EXPECT_DOUBLE_EQ(0.95257412682243336, A2.at(1,1), 0.000001);
    EXPECT_DOUBLE_EQ(0.98201379003790845, A2.at(2,0), 0.000001);
    EXPECT_DOUBLE_EQ(0.017986209962091559, A2.at(2,1), 0.000001);

    // test behavior for very large values
    A2.resize(1,3);
    A2.at(0,0) = 710.0;
    A2.at(0,1) = 0.0;
    A2.at(0,2) = -710.0;

    A2.initComputation();
    A2.sigmoid();
    A2.finishComputation();

    EXPECT_DOUBLE_EQ((f32)1.0, A2.at(0,0), 0.000001);
    EXPECT_DOUBLE_EQ((f32)0.5, A2.at(0,1), 0.000001);
    EXPECT_DOUBLE_EQ((f32)0.0, A2.at(0,2), 0.000001);

}

TEST_F(Test, TestCudaMatrix, tanh)
{
    Math::CudaMatrix<f64> A;
    A.resize(2,3);
    A.at(0,0) = 3.0;
    A.at(0,1) = -1.0;
    A.at(0,2) = 4.5;
    A.at(1,0) = 2.0;
    A.at(1,1) = 0.5;
    A.at(1,2) = 8.0;
    EXPECT_EQ(A.nRows(),2u);
    EXPECT_EQ(A.nColumns(),3u);
    EXPECT_EQ(A.size(),6u);
    A.initComputation();
    A.tanh();
    A.finishComputation();
    EXPECT_DOUBLE_EQ(0.995054753687, A.at(0,0), 0.000001);
    EXPECT_DOUBLE_EQ(-0.761594155955, A.at(0,1), 0.000001);
    EXPECT_DOUBLE_EQ(0.999753210848, A.at(0,2), 0.000001);
    EXPECT_DOUBLE_EQ(0.964027580076, A.at(1,0), 0.000001);
    EXPECT_DOUBLE_EQ(0.462117157266, A.at(1,1), 0.000001);
    EXPECT_DOUBLE_EQ(0.99999977493, A.at(1,2), 0.000001);

    Math::CudaMatrix<f32> A2;
    A2.resize(2,3);
    A2.at(0,0) = 3.0;
    A2.at(0,1) = -1.0;
    A2.at(0,2) = 4.5;
    A2.at(1,0) = 2.0;
    A2.at(1,1) = 0.5;
    A2.at(1,2) = 8.0;
    EXPECT_EQ(A2.nRows(),2u);
    EXPECT_EQ(A2.nColumns(),3u);
    EXPECT_EQ(A2.size(),6u);
    A2.initComputation();
    A2.tanh();
    A2.finishComputation();
    EXPECT_DOUBLE_EQ(0.995054753687, A2.at(0,0), 0.000001);
    EXPECT_DOUBLE_EQ(-0.761594155955, A2.at(0,1), 0.000001);
    EXPECT_DOUBLE_EQ(0.999753210848, A2.at(0,2), 0.000001);
    EXPECT_DOUBLE_EQ(0.964027580076, A2.at(1,0), 0.000001);
    EXPECT_DOUBLE_EQ(0.462117157266, A2.at(1,1), 0.000001);
    EXPECT_DOUBLE_EQ(0.99999977493, A2.at(1,2), 0.000001);
}

TEST_F(Test, TestCudaMatrix, elementwiseMultiplication)
{
    Math::CudaMatrix<f64> A;
    Math::CudaMatrix<f64> B;
    A.resize(2,3);
    B.resize(2,3);
    A.at(0,0) = 1.0;
    A.at(0,1) = 5.0;
    A.at(0,2) = 2.0;
    A.at(1,0) = -1.0;
    A.at(1,1) = 0.5;
    A.at(1,2) = 2.0;
    B.at(0,0) = 7.0;
    B.at(0,1) = 12.0;
    B.at(0,2) = 0.5;
    B.at(1,0) = 3.0;
    B.at(1,1) = -2.0;
    B.at(1,2) = 0.0;
    A.initComputation();
    B.initComputation();
    A.elementwiseMultiplication(B);
    A.finishComputation();
    EXPECT_EQ(7.0, A.at(0,0));
    EXPECT_EQ(60.0, A.at(0,1));
    EXPECT_EQ(1.0, A.at(0,2));
    EXPECT_EQ(-3.0, A.at(1,0));
    EXPECT_EQ(-1.0, A.at(1,1));
    EXPECT_EQ(0.0, A.at(1,2));

    Math::CudaMatrix<f32> A2;
    Math::CudaMatrix<f32> B2;
    A2.resize(2,3);
    B2.resize(2,3);
    A2.at(0,0) = 1.0;
    A2.at(0,1) = 5.0;
    A2.at(0,2) = 2.0;
    A2.at(1,0) = -1.0;
    A2.at(1,1) = 0.5;
    A2.at(1,2) = 2.0;
    B2.at(0,0) = 7.0;
    B2.at(0,1) = 12.0;
    B2.at(0,2) = 0.5;
    B2.at(1,0) = 3.0;
    B2.at(1,1) = -2.0;
    B2.at(1,2) = 0.0;
    A2.initComputation();
    B2.initComputation();
    A2.elementwiseMultiplication(B2);
    A2.finishComputation();
    EXPECT_EQ((f32)7.0, A2.at(0,0));
    EXPECT_EQ((f32)60.0, A2.at(0,1));
    EXPECT_EQ((f32)1.0, A2.at(0,2));
    EXPECT_EQ((f32)-3.0, A2.at(1,0));
    EXPECT_EQ((f32)-1.0, A2.at(1,1));
    EXPECT_EQ((f32)0.0, A2.at(1,2));
}

TEST_F(Test, TestCudaMatrix, elementwiseDivision)
{
    Math::CudaMatrix<f64> A;
    Math::CudaMatrix<f64> B;
    A.resize(2,3);
    B.resize(2,3);
    A.at(0,0) = 4.0;
    A.at(0,1) = 6.0;
    A.at(0,2) = 5.0;
    A.at(1,0) = 2.0;
    A.at(1,1) = 9.0;
    A.at(1,2) = 12.0;
    B.at(0,0) = 1.0;
    B.at(0,1) = 3.0;
    B.at(0,2) = 1.0;
    B.at(1,0) = 2.0;
    B.at(1,1) = 2.0;
    B.at(1,2) = 4.0;
    A.initComputation();
    B.initComputation();
    A.elementwiseDivision(B);
    A.finishComputation();
    EXPECT_EQ(4.0, A.at(0,0));
    EXPECT_EQ(2.0, A.at(0,1));
    EXPECT_EQ(5.0, A.at(0,2));
    EXPECT_EQ(1.0, A.at(1,0));
    EXPECT_EQ(4.5, A.at(1,1));
    EXPECT_EQ(3.0, A.at(1,2));

    Math::CudaMatrix<f32> A2;
    Math::CudaMatrix<f32> B2;
    A2.resize(2,3);
    B2.resize(2,3);
    A2.at(0,0) = 4.0;
    A2.at(0,1) = 6.0;
    A2.at(0,2) = 5.0;
    A2.at(1,0) = 2.0;
    A2.at(1,1) = 9.0;
    A2.at(1,2) = 12.0;
    B2.at(0,0) = 1.0;
    B2.at(0,1) = 3.0;
    B2.at(0,2) = 1.0;
    B2.at(1,0) = 2.0;
    B2.at(1,1) = 2.0;
    B2.at(1,2) = 4.0;
    A2.initComputation();
    B2.initComputation();
    A2.elementwiseDivision(B2);
    A2.finishComputation();
    EXPECT_EQ((f32)4.0, A2.at(0,0));
    EXPECT_EQ((f32)2.0, A2.at(0,1));
    EXPECT_EQ((f32)5.0, A2.at(0,2));
    EXPECT_EQ((f32)1.0, A2.at(1,0));
    EXPECT_EQ((f32)4.5, A2.at(1,1));
    EXPECT_EQ((f32)3.0, A2.at(1,2));
}

TEST_F(Test, TestCudaMatrix, addConstantElementwise)
{
    Math::CudaMatrix<f64> A;
    A.resize(3,2);
    A.at(0,0) = 2.0;
    A.at(0,1) = 1.0;
    A.at(1,0) = 5.5;
    A.at(1,1) = -1.0;
    A.at(2,0) = 3.0;
    A.at(2,1) = 0.5;
    A.initComputation();
    A.addConstantElementwise(2.5);
    A.finishComputation();
    EXPECT_EQ(4.5, A.at(0,0));
    EXPECT_EQ(3.5, A.at(0,1));
    EXPECT_EQ(8.0, A.at(1,0));
    EXPECT_EQ(1.5, A.at(1,1));
    EXPECT_EQ(5.5, A.at(2,0));
    EXPECT_EQ(3.0, A.at(2,1));

    Math::CudaMatrix<f32> A2;
    A2.resize(3,2);
    A2.at(0,0) = 2.0;
    A2.at(0,1) = 1.0;
    A2.at(1,0) = 5.5;
    A2.at(1,1) = -1.0;
    A2.at(2,0) = 3.0;
    A2.at(2,1) = 0.5;
    A2.initComputation();
    A2.addConstantElementwise(2.5);
    A2.finishComputation();
    EXPECT_EQ((f32)4.5, A2.at(0,0));
    EXPECT_EQ((f32)3.5, A2.at(0,1));
    EXPECT_EQ((f32)8.0, A2.at(1,0));
    EXPECT_EQ((f32)1.5, A2.at(1,1));
    EXPECT_EQ((f32)5.5, A2.at(2,0));
    EXPECT_EQ((f32)3.0, A2.at(2,1));
}

TEST_F(Test, TestCudaMatrix, elementwiseMultiplicationWithSigmoidDerivative)
{
    Math::CudaMatrix<f64> A;
    Math::CudaMatrix<f64> B;
    A.resize(3,2);
    B.resize(3,2);
    A.at(0,0) = 2.0;
    A.at(0,1) = 0.0;
    A.at(1,0) = 1.0;
    A.at(1,1) = 4.0;
    A.at(2,0) = -1.5;
    A.at(2,1) = 1.0;
    B.at(0,0) = 2.0;
    B.at(0,1) = 9.0;
    B.at(1,0) = -1.0;
    B.at(1,1) = 0.5;
    B.at(2,0) = 3.5;
    B.at(2,1) = 4.0;
    A.initComputation();
    B.initComputation();
    A.elementwiseMultiplicationWithSigmoidDerivative(B);
    A.finishComputation();
    EXPECT_EQ(-4.0, A.at(0,0));
    EXPECT_EQ(0.0, A.at(0,1));
    EXPECT_EQ(-2.0, A.at(1,0));
    EXPECT_EQ(1.0, A.at(1,1));
    EXPECT_EQ(13.125, A.at(2,0));
    EXPECT_EQ(-12.0, A.at(2,1));

    Math::CudaMatrix<f32> A2;
    Math::CudaMatrix<f32> B2;
    A2.resize(3,2);
    B2.resize(3,2);
    A2.at(0,0) = 2.0;
    A2.at(0,1) = 0.0;
    A2.at(1,0) = 1.0;
    A2.at(1,1) = 4.0;
    A2.at(2,0) = -1.5;
    A2.at(2,1) = 1.0;
    B2.at(0,0) = 2.0;
    B2.at(0,1) = 9.0;
    B2.at(1,0) = -1.0;
    B2.at(1,1) = 0.5;
    B2.at(2,0) = 3.5;
    B2.at(2,1) = 4.0;
    A2.initComputation();
    B2.initComputation();
    A2.elementwiseMultiplicationWithSigmoidDerivative(B2);
    A2.finishComputation();
    EXPECT_EQ((f32)-4.0, A2.at(0,0));
    EXPECT_EQ((f32)0.0, A2.at(0,1));
    EXPECT_EQ((f32)-2.0, A2.at(1,0));
    EXPECT_EQ((f32)1.0, A2.at(1,1));
    EXPECT_EQ((f32)13.125, A2.at(2,0));
    EXPECT_EQ((f32)-12.0, A2.at(2,1));
}

TEST_F(Test, TestCudaMatrix, elementwiseMultiplicationWithTanhDerivative)
{
    Math::CudaMatrix<f64> A;
    Math::CudaMatrix<f64> B;
    A.resize(3,2);
    B.resize(3,2);
    A.at(0,0) = 1.0;
    A.at(0,1) = 3.0;
    A.at(1,0) = 0.0;
    A.at(1,1) = 2.0;
    A.at(2,0) = -1.5;
    A.at(2,1) = 3.0;
    B.at(0,0) = 0.5;
    B.at(0,1) = 2.0;
    B.at(1,0) = -1.0;
    B.at(1,1) = 2.5;
    B.at(2,0) = 2.0;
    B.at(2,1) = 5.0;
    A.initComputation();
    B.initComputation();
    A.elementwiseMultiplicationWithTanhDerivative(B);
    A.finishComputation();
    EXPECT_EQ(0.75, A.at(0,0));
    EXPECT_EQ(-9.0, A.at(0,1));
    EXPECT_EQ(0.0, A.at(1,0));
    EXPECT_EQ(-10.5, A.at(1,1));
    EXPECT_EQ(4.5, A.at(2,0));
    EXPECT_EQ(-72.0, A.at(2,1));

    Math::CudaMatrix<f32> A2;
    Math::CudaMatrix<f32> B2;
    A2.resize(3,2);
    B2.resize(3,2);
    A2.at(0,0) = 1.0;
    A2.at(0,1) = 3.0;
    A2.at(1,0) = 0.0;
    A2.at(1,1) = 2.0;
    A2.at(2,0) = -1.5;
    A2.at(2,1) = 3.0;
    B2.at(0,0) = 0.5;
    B2.at(0,1) = 2.0;
    B2.at(1,0) = -1.0;
    B2.at(1,1) = 2.5;
    B2.at(2,0) = 2.0;
    B2.at(2,1) = 5.0;
    A2.initComputation();
    B2.initComputation();
    A2.elementwiseMultiplicationWithTanhDerivative(B2);
    A2.finishComputation();
    EXPECT_EQ((f32)0.75, A2.at(0,0));
    EXPECT_EQ((f32)-9.0, A2.at(0,1));
    EXPECT_EQ((f32)0.0, A2.at(1,0));
    EXPECT_EQ((f32)-10.5, A2.at(1,1));
    EXPECT_EQ((f32)4.5, A2.at(2,0));
    EXPECT_EQ((f32)-72.0, A2.at(2,1));
}

TEST_F(Test, TestCudaMatrix, multiplicationWithSoftmaxDerivative)
{
    Math::CudaMatrix<f64> A;
    Math::CudaMatrix<f64> B;
    A.resize(3,2);
    B.resize(3,2);
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
    A.multiplicationWithSoftmaxDerivative(B);
    A.finishComputation();
    EXPECT_EQ(-15.0, A.at(0,0));
    EXPECT_EQ(-60.0, A.at(0,1));
    EXPECT_EQ(48.0, A.at(1,0));
    EXPECT_EQ(24.0, A.at(1,1));
    EXPECT_EQ(-48.0, A.at(2,0));
    EXPECT_EQ(36.0, A.at(2,1));

    Math::CudaMatrix<f32> A2;
    Math::CudaMatrix<f32> B2;
    A2.resize(3,2);
    B2.resize(3,2);
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
    A2.multiplicationWithSoftmaxDerivative(B2);
    A2.finishComputation();
    EXPECT_EQ(-15.0f, A2.at(0,0));
    EXPECT_EQ(-60.0f, A2.at(0,1));
    EXPECT_EQ(48.0f, A2.at(1,0));
    EXPECT_EQ(24.0f, A2.at(1,1));
    EXPECT_EQ(-48.0f, A2.at(2,0));
    EXPECT_EQ(36.0f, A2.at(2,1));
}

TEST_F(Test, TestCudaMatrix, addToAllColumns)
{
    Math::CudaMatrix<f64> A;
    Math::CudaVector<f64> b;
    A.resize(2,3);
    b.resize(2);
    A.at(0,0) = 1.0;
    A.at(0,1) = 6.0;
    A.at(0,2) = 0.0;
    A.at(1,0) = -2.0;
    A.at(1,1) = 3.5;
    A.at(1,2) = 1.0;
    b.at(0) = 2.0;
    b.at(1) = -1.0;
    A.initComputation();
    b.initComputation();
    A.addToAllColumns(b);
    A.finishComputation();
    EXPECT_EQ(3.0, A.at(0,0));
    EXPECT_EQ(8.0, A.at(0,1));
    EXPECT_EQ(2.0, A.at(0,2));
    EXPECT_EQ(-3.0, A.at(1,0));
    EXPECT_EQ(2.5, A.at(1,1));
    EXPECT_EQ(0.0, A.at(1,2));

    Math::CudaMatrix<f32> A2;
    Math::CudaVector<f32> b2;
    A2.resize(2,3);
    b2.resize(2);
    A2.at(0,0) = 1.0;
    A2.at(0,1) = 6.0;
    A2.at(0,2) = 0.0;
    A2.at(1,0) = -2.0;
    A2.at(1,1) = 3.5;
    A2.at(1,2) = 1.0;
    b2.at(0) = 2.0;
    b2.at(1) = -1.0;
    A2.initComputation();
    b2.initComputation();
    A2.addToAllColumns(b2);
    A2.finishComputation();
    EXPECT_EQ((f32)3.0, A2.at(0,0));
    EXPECT_EQ((f32)8.0, A2.at(0,1));
    EXPECT_EQ((f32)2.0, A2.at(0,2));
    EXPECT_EQ((f32)-3.0, A2.at(1,0));
    EXPECT_EQ((f32)2.5, A2.at(1,1));
    EXPECT_EQ((f32)0.0, A2.at(1,2));
}

TEST_F(Test, TestCudaMatrix, addToAllRows)
{
    Math::CudaMatrix<f64> A;
    Math::CudaVector<f64> b;
    A.resize(2,3);
    b.resize(3);
    A.at(0,0) = 1.0;
    A.at(0,1) = 6.0;
    A.at(0,2) = 0.0;
    A.at(1,0) = -2.0;
    A.at(1,1) = 3.5;
    A.at(1,2) = 1.0;
    b.at(0) = 2.0;
    b.at(1) = -1.0;
    b.at(2) = 4.0;
    A.initComputation();
    b.initComputation();
    A.addToAllRows(b);
    A.finishComputation();
    EXPECT_EQ(3.0, A.at(0,0));
    EXPECT_EQ(5.0, A.at(0,1));
    EXPECT_EQ(4.0, A.at(0,2));
    EXPECT_EQ(0.0, A.at(1,0));
    EXPECT_EQ(2.5, A.at(1,1));
    EXPECT_EQ(5.0, A.at(1,2));

    Math::CudaMatrix<f32> A2;
    Math::CudaVector<f32> b2;
    A2.resize(2,3);
    b2.resize(3);
    A2.at(0,0) = 1.0;
    A2.at(0,1) = 6.0;
    A2.at(0,2) = 0.0;
    A2.at(1,0) = -2.0;
    A2.at(1,1) = 3.5;
    A2.at(1,2) = 1.0;
    b2.at(0) = 2.0;
    b2.at(1) = -1.0;
    b2.at(2) = 4.0;
    A2.initComputation();
    b2.initComputation();
    A2.addToAllRows(b2);
    A2.finishComputation();
    EXPECT_EQ((f32)3.0, A2.at(0,0));
    EXPECT_EQ((f32)5.0, A2.at(0,1));
    EXPECT_EQ((f32)4.0, A2.at(0,2));
    EXPECT_EQ((f32)0.0, A2.at(1,0));
    EXPECT_EQ((f32)2.5, A2.at(1,1));
    EXPECT_EQ((f32)5.0, A2.at(1,2));
}

TEST_F(Test, TestCudaMatrix, multiplyColumnsByScalars)
{
    Math::CudaMatrix<f64> A;
    Math::CudaVector<f64> b;
    A.resize(2,3);
    b.resize(3);
    A.at(0,0) = 2.0;
    A.at(1,0) = -2.0;
    A.at(0,1) = 4.0;
    A.at(1,1) = -8.0;
    A.at(0,2) = 5.0;
    A.at(1,2) = -6.0;
    b.at(0) = 2.0;
    b.at(1) = -2.0;
    b.at(2) = -1.0;
    A.initComputation();
    b.initComputation();
    A.multiplyColumnsByScalars(b);
    A.finishComputation();
    EXPECT_EQ(4.0, A.at(0,0));
    EXPECT_EQ(-4.0, A.at(1,0));
    EXPECT_EQ(-8.0, A.at(0,1));
    EXPECT_EQ(16.0, A.at(1,1));
    EXPECT_EQ(-5.0, A.at(0,2));
    EXPECT_EQ(6.0, A.at(1,2));

    Math::CudaMatrix<f32> A2;
    Math::CudaVector<f32> b2;
    A2.resize(2,3);
    b2.resize(3);
    A2.at(0,0) = 2.0;
    A2.at(1,0) = -2.0;
    A2.at(0,1) = 4.0;
    A2.at(1,1) = -8.0;
    A2.at(0,2) = 5.0;
    A2.at(1,2) = -6.0;
    b2.at(0) = 2.0;
    b2.at(1) = -2.0;
    b2.at(2) = -1.0;
    A2.initComputation();
    b2.initComputation();
    A2.multiplyColumnsByScalars(b2);
    A2.finishComputation();
    EXPECT_EQ((f32)4.0f, A2.at(0,0));
    EXPECT_EQ((f32)-4.0f, A2.at(1,0));
    EXPECT_EQ((f32)-8.0f, A2.at(0,1));
    EXPECT_EQ((f32)16.0f, A2.at(1,1));
    EXPECT_EQ((f32)-5.0f, A2.at(0,2));
    EXPECT_EQ((f32)6.0f, A2.at(1,2));
}

TEST_F(Test, TestCudaMatrix, divideColumnsByScalars)
{
    Math::CudaMatrix<f64> A;
    Math::CudaVector<f64> b;
    A.resize(2,3);
    b.resize(3);
    A.at(0,0) = 2.0;
    A.at(1,0) = -2.0;
    A.at(0,1) = 4.0;
    A.at(1,1) = -8.0;
    A.at(0,2) = 5.0;
    A.at(1,2) = -6.0;
    b.at(0) = 2.0;
    b.at(1) = -2.0;
    b.at(2) = -1.0;
    A.initComputation();
    b.initComputation();
    A.divideColumnsByScalars(b);
    A.finishComputation();
    EXPECT_EQ(1.0, A.at(0,0));
    EXPECT_EQ(-1.0, A.at(1,0));
    EXPECT_EQ(-2.0, A.at(0,1));
    EXPECT_EQ(4.0, A.at(1,1));
    EXPECT_EQ(-5.0, A.at(0,2));
    EXPECT_EQ(6.0, A.at(1,2));

    Math::CudaMatrix<f32> A2;
    Math::CudaVector<f32> b2;
    A2.resize(2,3);
    b2.resize(3);
    A2.at(0,0) = 2.0;
    A2.at(1,0) = -2.0;
    A2.at(0,1) = 4.0;
    A2.at(1,1) = -8.0;
    A2.at(0,2) = 5.0;
    A2.at(1,2) = -6.0;
    b2.at(0) = 2.0;
    b2.at(1) = -2.0;
    b2.at(2) = -1.0;
    A2.initComputation();
    b2.initComputation();
    A2.divideColumnsByScalars(b2);
    A2.finishComputation();
    EXPECT_EQ((f32)1.0f, A2.at(0,0));
    EXPECT_EQ((f32)-1.0f, A2.at(1,0));
    EXPECT_EQ((f32)-2.0f, A2.at(0,1));
    EXPECT_EQ((f32)4.0f, A2.at(1,1));
    EXPECT_EQ((f32)-5.0f, A2.at(0,2));
    EXPECT_EQ((f32)6.0f, A2.at(1,2));
}

TEST_F(Test, TestCudaMatrix, multiplyRowsByScalars)
{
    Math::CudaMatrix<f64> A;
    Math::CudaVector<f64> b;
    A.resize(2,3);
    b.resize(2);
    A.at(0,0) = 2.0;
    A.at(1,0) = -2.0;
    A.at(0,1) = 4.0;
    A.at(1,1) = -8.0;
    A.at(0,2) = 5.0;
    A.at(1,2) = -6.0;
    b.at(0) = 2.0;
    b.at(1) = -2.0;
    A.initComputation();
    b.initComputation();
    A.multiplyRowsByScalars(b);
    A.finishComputation();
    EXPECT_EQ(4.0, A.at(0,0));
    EXPECT_EQ(4.0, A.at(1,0));
    EXPECT_EQ(8.0, A.at(0,1));
    EXPECT_EQ(16.0, A.at(1,1));
    EXPECT_EQ(10.0, A.at(0,2));
    EXPECT_EQ(12.0, A.at(1,2));

    Math::CudaMatrix<f32> A2;
    Math::CudaVector<f32> b2;
    A2.resize(2,3);
    b2.resize(2);
    A2.at(0,0) = 2.0;
    A2.at(1,0) = -2.0;
    A2.at(0,1) = 4.0;
    A2.at(1,1) = -8.0;
    A2.at(0,2) = 5.0;
    A2.at(1,2) = -6.0;
    b2.at(0) = 2.0;
    b2.at(1) = -2.0;
    A2.initComputation();
    b2.initComputation();
    A2.multiplyRowsByScalars(b2);
    A2.finishComputation();
    EXPECT_EQ((f32)4.0f, A2.at(0,0));
    EXPECT_EQ((f32)4.0f, A2.at(1,0));
    EXPECT_EQ((f32)8.0f, A2.at(0,1));
    EXPECT_EQ((f32)16.0f, A2.at(1,1));
    EXPECT_EQ((f32)10.0f, A2.at(0,2));
    EXPECT_EQ((f32)12.0f, A2.at(1,2));
}

TEST_F(Test, TestCudaMatrix, divideRowsByScalars)
{
    Math::CudaMatrix<f64> A;
    Math::CudaVector<f64> b;
    A.resize(2,3);
    b.resize(2);
    A.at(0,0) = 2.0;
    A.at(1,0) = -2.0;
    A.at(0,1) = 4.0;
    A.at(1,1) = -8.0;
    A.at(0,2) = 5.0;
    A.at(1,2) = -6.0;
    b.at(0) = 2.0;
    b.at(1) = -2.0;
    A.initComputation();
    b.initComputation();
    A.divideRowsByScalars(b);
    A.finishComputation();
    EXPECT_EQ(1.0, A.at(0,0));
    EXPECT_EQ(1.0, A.at(1,0));
    EXPECT_EQ(2.0, A.at(0,1));
    EXPECT_EQ(4.0, A.at(1,1));
    EXPECT_EQ(2.5, A.at(0,2));
    EXPECT_EQ(3.0, A.at(1,2));

    Math::CudaMatrix<f32> A2;
    Math::CudaVector<f32> b2;
    A2.resize(2,3);
    b2.resize(2);
    A2.at(0,0) = 2.0;
    A2.at(1,0) = -2.0;
    A2.at(0,1) = 4.0;
    A2.at(1,1) = -8.0;
    A2.at(0,2) = 5.0;
    A2.at(1,2) = -6.0;
    b2.at(0) = 2.0;
    b2.at(1) = -2.0;
    A2.initComputation();
    b2.initComputation();
    A2.divideRowsByScalars(b2);
    A2.finishComputation();
    EXPECT_EQ((f32)1.0f, A2.at(0,0));
    EXPECT_EQ((f32)1.0f, A2.at(1,0));
    EXPECT_EQ((f32)2.0f, A2.at(0,1));
    EXPECT_EQ((f32)4.0f, A2.at(1,1));
    EXPECT_EQ((f32)2.5f, A2.at(0,2));
    EXPECT_EQ((f32)3.0f, A2.at(1,2));
}

TEST_F(Test, TestCudaMatrix, softmax)
{
    Math::CudaMatrix<f64> A;
    A.resize(3,2);
    A.at(0,0) = 4.05; A.at(1,0) = 4.9; A.at(2,0) = 4.8;
    A.at(0,1) = 1.7; A.at(1,1) = 2.1; A.at(2,1) = 1.95;
    A.initComputation();
    A.softmax();
    A.finishComputation();
    EXPECT_DOUBLE_EQ(0.18326272967482829, A.at(0,0), 0.000001);
    EXPECT_DOUBLE_EQ(0.42877006855907612, A.at(1,0), 0.000001);
    EXPECT_DOUBLE_EQ(0.38796720176609562, A.at(2,0), 0.000001);
    EXPECT_DOUBLE_EQ(0.26484102115311464, A.at(0,1), 0.000001);
    EXPECT_DOUBLE_EQ(0.39509637630475053, A.at(1,1), 0.000001);
    EXPECT_DOUBLE_EQ(0.34006260254213494, A.at(2,1), 0.000001);
}

TEST_F(Test, TestCudaMatrix, nClassificationErrors)
{
    Math::CudaMatrix<f32> X(5,3);
    X.at(0,0) = 4.0f;
    X.at(1,0) = 5.0f; // error
    X.at(2,0) = 2.0f;
    X.at(3,0) = 0.0f;
    X.at(4,0) = -10.0f;

    X.at(0,1) = 1.0f;
    X.at(1,1) = 3.0f; // error
    X.at(2,1) = 0.0f;
    X.at(3,1) = 1.0f;
    X.at(4,1) = 4.0f;

    X.at(0,2) = -10.0f;
    X.at(1,2) = 0.0f;
    X.at(2,2) = 1.0f;
    X.at(3,2) = 5.0f;
    X.at(4,2) = 8.0f; // correct

    Math::CudaVector<u32> alignment(3);
    alignment.at(0) = 0;
    alignment.at(1) = 1;
    alignment.at(2) = 4;

    X.initComputation();
    alignment.initComputation();

    u32 result = X.nClassificationErrors(alignment);
    EXPECT_EQ(2u, result);

    Math::CudaMatrix<f64> X2(5,3);
    X2.at(0,0) = 4.0f;
    X2.at(1,0) = 5.0f; // error
    X2.at(2,0) = 2.0f;
    X2.at(3,0) = 0.0f;
    X2.at(4,0) = -10.0f;

    X2.at(0,1) = 1.0f;
    X2.at(1,1) = 3.0f; // error
    X2.at(2,1) = 0.0f;
    X2.at(3,1) = 1.0f;
    X2.at(4,1) = 4.0f;

    X2.at(0,2) = -10.0f;
    X2.at(1,2) = 0.0f;
    X2.at(2,2) = 1.0f;
    X2.at(3,2) = 5.0f;
    X2.at(4,2) = 8.0f; // correct

    X2.initComputation();
    alignment.initComputation();

    result = X2.nClassificationErrors(alignment);
    EXPECT_EQ(2u, result);
}

TEST_F(Test, TestCudaMatrix, crossEntropyObjectiveFunction)
{
    // values do not make sense, but that does not matter here
    Math::CudaMatrix<f64> X(5,3);
    X.at(0,0) = 1.0f;
    X.at(0,1) = 1.0f;
    X.at(0,2) = 2.0f;
    X.at(1,0) = 5.0f;
    X.at(1,1) = 3.0f;
    X.at(1,2) = 0.0f;
    X.at(2,0) = 2.0f;
    X.at(2,1) = 0.0f;
    X.at(2,2) = 1.0f;
    X.at(3,0) = 0.0f;
    X.at(3,1) = 1.0f;
    X.at(3,2) = 5.0f;
    X.at(4,0) = 2.0f;
    X.at(4,1) = 4.0f;
    X.at(4,2) = 8.0f;

    Math::CudaVector<u32> alignment(3);
    alignment.at(0) = 0;
    alignment.at(1) = 1;
    alignment.at(2) = 4;

    X.initComputation();
    alignment.initComputation();

    f32 result = X.crossEntropyObjectiveFunction(alignment);
    f32 checkVal = -std::log(3.0) - std::log(8.0);
    EXPECT_EQ(checkVal, result);

    Math::CudaMatrix<f32> X2(5,3);
    X2.at(0,0) = 1.0f;
    X2.at(0,1) = 1.0f;
    X2.at(0,2) = 2.0f;
    X2.at(1,0) = 5.0f;
    X2.at(1,1) = 3.0f;
    X2.at(1,2) = 0.0f;
    X2.at(2,0) = 2.0f;
    X2.at(2,1) = 0.0f;
    X2.at(2,2) = 1.0f;
    X2.at(3,0) = 0.0f;
    X2.at(3,1) = 1.0f;
    X2.at(3,2) = 5.0f;
    X2.at(4,0) = 2.0f;
    X2.at(4,1) = 4.0f;
    X2.at(4,2) = 8.0f;

    X2.initComputation();
    alignment.initComputation();

    result = X2.crossEntropyObjectiveFunction(alignment);
    checkVal = -std::log(3.0) - std::log(8.0);
    EXPECT_EQ(checkVal, result);
}

TEST_F(Test, TestCudaMatrix, addKroneckerDelta)
{
    // values do not make sense, but that does not matter here
    Math::CudaMatrix<f64> X(5,3);
    X.at(0,0) = 0.0;
    X.at(1,0) = 0.0;
    X.at(2,0) = 0.0;
    X.at(3,0) = 0.0;
    X.at(4,0) = 1.0;
    X.at(0,1) = 1.0;
    X.at(1,1) = 0.0;
    X.at(2,1) = 0.0;
    X.at(3,1) = 0.0;
    X.at(4,1) = 0.0;
    X.at(0,2) = 0.0;
    X.at(1,2) = -2.0;
    X.at(2,2) = 0.0;
    X.at(3,2) = 0.0;
    X.at(4,2) = 0.0;

    Math::CudaVector<u32> alignment(3);
    alignment.at(0) = 0;
    alignment.at(1) = 1;
    alignment.at(2) = 4;

    X.initComputation();
    alignment.initComputation();

    X.addKroneckerDelta(alignment);
    X.finishComputation();
    EXPECT_EQ(1.0, X.at(0,0) );
    EXPECT_EQ(0.0, X.at(1,0) );
    EXPECT_EQ(0.0, X.at(2,0) );
    EXPECT_EQ(0.0, X.at(3,0) );
    EXPECT_EQ(1.0, X.at(4,0) );

    EXPECT_EQ(1.0, X.at(0,1) );
    EXPECT_EQ(1.0, X.at(1,1) );
    EXPECT_EQ(0.0, X.at(2,1) );
    EXPECT_EQ(0.0, X.at(3,1) );
    EXPECT_EQ(0.0, X.at(4,1) );

    EXPECT_EQ(0.0, X.at(0,2) );
    EXPECT_EQ(-2.0, X.at(1,2) );
    EXPECT_EQ(0.0, X.at(2,2) );
    EXPECT_EQ(0.0, X.at(3,2) );
    EXPECT_EQ(1.0, X.at(4,2) );

    Math::CudaMatrix<f32> X2(5,3);
    X2.at(0,0) = 0.0;
    X2.at(1,0) = 0.0;
    X2.at(2,0) = 0.0;
    X2.at(3,0) = 0.0;
    X2.at(4,0) = 1.0;
    X2.at(0,1) = 1.0;
    X2.at(1,1) = 0.0;
    X2.at(2,1) = 0.0;
    X2.at(3,1) = 0.0;
    X2.at(4,1) = 0.0;
    X2.at(0,2) = 0.0;
    X2.at(1,2) = -2.0;
    X2.at(2,2) = 0.0;
    X2.at(3,2) = 0.0;
    X2.at(4,2) = 0.0;

    X2.initComputation();
    alignment.initComputation();

    X2.addKroneckerDelta(alignment);
    X2.finishComputation();
    EXPECT_EQ(1.0f, X2.at(0,0) );
    EXPECT_EQ(0.0f, X2.at(1,0) );
    EXPECT_EQ(0.0f, X2.at(2,0) );
    EXPECT_EQ(0.0f, X2.at(3,0) );
    EXPECT_EQ(1.0f, X2.at(4,0) );

    EXPECT_EQ(1.0f, X2.at(0,1) );
    EXPECT_EQ(1.0f, X2.at(1,1) );
    EXPECT_EQ(0.0f, X2.at(2,1) );
    EXPECT_EQ(0.0f, X2.at(3,1) );
    EXPECT_EQ(0.0f, X2.at(4,1) );

    EXPECT_EQ(0.0f, X2.at(0,2) );
    EXPECT_EQ(-2.0f, X2.at(1,2) );
    EXPECT_EQ(0.0f, X2.at(2,2) );
    EXPECT_EQ(0.0f, X2.at(3,2) );
    EXPECT_EQ(1.0f, X2.at(4,2) );
}

TEST_F(Test, TestCudaMatrix, setToSecondOrderFeatures){
    Math::CudaMatrix<f64> A;
    Math::CudaMatrix<f64> B;
    B.resize(2,3);
    A.resize(5,3);

    B.at(0,0) = 1.0;
    B.at(1,0) = 2.0;
    B.at(0,1) = -1.0;
    B.at(1,1) = -2.0;
    B.at(0,2) = 1.0;
    B.at(1,2) = 3.0;
    A.initComputation();
    B.initComputation();
    A.setToSecondOrderFeatures(B);
    A.finishComputation();
    EXPECT_EQ(A.at(0,0), 1.0);
    EXPECT_EQ(A.at(1,0), 2.0);
    EXPECT_EQ(A.at(2,0), 1.0);
    EXPECT_EQ(A.at(3,0), 2.0);
    EXPECT_EQ(A.at(4,0), 4.0);

    EXPECT_EQ(A.at(0,1), -1.0);
    EXPECT_EQ(A.at(1,1), -2.0);
    EXPECT_EQ(A.at(2,1), 1.0);
    EXPECT_EQ(A.at(3,1), 2.0);
    EXPECT_EQ(A.at(4,1), 4.0);

    EXPECT_EQ(A.at(0,2), 1.0);
    EXPECT_EQ(A.at(1,2), 3.0);
    EXPECT_EQ(A.at(2,2), 1.0);
    EXPECT_EQ(A.at(3,2), 3.0);
    EXPECT_EQ(A.at(4,2), 9.0);


}

TEST_F(Test, TestCudaMatrix, setToThirdOrderFeatures){
    Math::CudaMatrix<f64> A;
    Math::CudaMatrix<f64> B;
    B.resize(2,3);
    A.resize(9,3);

    B.at(0,0) = 1.0;
    B.at(1,0) = 2.0;
    B.at(0,1) = -1.0;
    B.at(1,1) = -2.0;
    B.at(0,2) = 1.0;
    B.at(1,2) = 3.0;
    A.initComputation();
    B.initComputation();
    A.setToThirdOrderFeatures(B);
    A.finishComputation();
    EXPECT_EQ(A.at(0,0), 1.0);
    EXPECT_EQ(A.at(1,0), 2.0);
    EXPECT_EQ(A.at(2,0), 1.0);
    EXPECT_EQ(A.at(3,0), 2.0);
    EXPECT_EQ(A.at(4,0), 4.0);
    EXPECT_EQ(A.at(5,0), 1.0);
    EXPECT_EQ(A.at(6,0), 2.0);
    EXPECT_EQ(A.at(7,0), 4.0);
    EXPECT_EQ(A.at(8,0), 8.0);

    EXPECT_EQ(A.at(0,1), -1.0);
    EXPECT_EQ(A.at(1,1), -2.0);
    EXPECT_EQ(A.at(2,1), 1.0);
    EXPECT_EQ(A.at(3,1), 2.0);
    EXPECT_EQ(A.at(4,1), 4.0);
    EXPECT_EQ(A.at(5,1), -1.0);
    EXPECT_EQ(A.at(6,1), -2.0);
    EXPECT_EQ(A.at(7,1), -4.0);
    EXPECT_EQ(A.at(8,1), -8.0);

    EXPECT_EQ(A.at(0,2), 1.0);
    EXPECT_EQ(A.at(1,2), 3.0);
    EXPECT_EQ(A.at(2,2), 1.0);
    EXPECT_EQ(A.at(3,2), 3.0);
    EXPECT_EQ(A.at(4,2), 9.0);
    EXPECT_EQ(A.at(5,2), 1.0);
    EXPECT_EQ(A.at(6,2), 3.0);
    EXPECT_EQ(A.at(7,2), 9.0);
    EXPECT_EQ(A.at(8,2), 27.0);
}

TEST_F(Test, TestCudaMatrix, l1clipping){
    Math::CudaMatrix<f64> A;
    A.resize(2,3);

    A.at(0,0) = 1.0;
    A.at(0,1) = -2.0;
    A.at(0,2) = -1.0;
    A.at(1,0) = 0.0;
    A.at(1,1) = -7.0;
    A.at(1,2) = 3.0;
    A.initComputation();
    A.l1clipping(2.0);
    A.finishComputation();
    EXPECT_EQ(A.at(0,0), 0.0);
    EXPECT_EQ(A.at(0,1), 0.0);
    EXPECT_EQ(A.at(0,2), 0.0);
    EXPECT_EQ(A.at(1,0), 0.0);
    EXPECT_EQ(A.at(1,1), -5.0);
    EXPECT_EQ(A.at(1,2), 1.0);
}

TEST_F(Test, TestCudaMatrix, isFinite)
{
    Math::CudaMatrix<f64> A;
    A.resize(4,3);
    A.setToZero();
    EXPECT_TRUE(A.isFinite());
    A.at(2,1) = std::log(0.0);
    EXPECT_FALSE(A.isFinite());
    A.at(2,1) = std::sqrt(-1.0);
    EXPECT_FALSE(A.isFinite());

    Math::CudaMatrix<f32> A2;
    A2.resize(4,3);
    A2.setToZero();
    EXPECT_TRUE(A2.isFinite());
    A2.at(2,1) = std::log(0.0f);
    EXPECT_FALSE(A2.isFinite());
    A2.at(2,1) = std::sqrt(-1.0f);
    EXPECT_FALSE(A2.isFinite());

}

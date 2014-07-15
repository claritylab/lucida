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
#include <Math/Blas.hh>

class TestBlasWrapper : public Test::Fixture
{
public:
    void setUp();
    void tearDown();
protected:
    int dim_;
    double *vector_;
    double *vector2_;
    double *matrix_;
};

void TestBlasWrapper::setUp()
{
    dim_ = 3;
    vector_ = new double[dim_];
    vector_[0] = 1.0;
    vector_[1] = 2.0;
    vector_[2] = 1.0;
    vector2_ = new double[dim_];
    vector2_[0] = 2.0;
    vector2_[1] = 3.0;
    vector2_[2] = -1.0;
    matrix_ = new double[dim_ * dim_];
    matrix_[0] = 1.0;
    matrix_[1] = 2.0;
    matrix_[2] = 3.0;
    matrix_[3] = 4.0;
    matrix_[4] = 5.0;
    matrix_[5] = 6.0;
    matrix_[6] = -1.0;
    matrix_[7] = -2.0;
    matrix_[8] = -3.0;
}

void TestBlasWrapper::tearDown(){
    delete [] vector_;
    delete [] matrix_;
}

TEST_F(Test, TestBlasWrapper, axpy)
{
    Math::axpy(dim_, 2.0, vector_, 1, vector2_, 1);
    EXPECT_EQ(4.0, vector2_[0]);
    EXPECT_EQ(7.0, vector2_[1]);
    EXPECT_EQ(1.0, vector2_[2]);

    for (int dim = 0; dim < 10; dim++){
	double *vector1 = new double[dim];
	double *vector2 = new double[dim];
	for (int i = 0; i < dim; i++){
	    vector1[i] = (double) i;
	    vector2[i] = (double) 2.0*i + 1.0;
	}
	Math::axpy(dim, 3.0, vector1, 1, vector2, 1);
	for (int i = 0; i < dim; i++){
	    EXPECT_EQ(5.0 * i + 1.0, vector2[i]);
	}
	delete [] vector1;
	delete [] vector2;
    }
}

TEST_F(Test, TestBlasWrapper, mt_axpy){
    for (int nThreads = 1; nThreads <= 4; nThreads++){
	vector2_[0] = 2.0;
	vector2_[1] = 3.0;
	vector2_[2] = -1.0;
	Math::mt_axpy(dim_, 2.0, vector_, vector2_, nThreads);
	EXPECT_EQ(4.0, vector2_[0]);
	EXPECT_EQ(7.0, vector2_[1]);
	EXPECT_EQ(1.0, vector2_[2]);
    }
    for (int dim = 0; dim < 10; dim++){
	for (int nThreads = 1; nThreads <= 4; nThreads++){
	    double *vector1 = new double[dim];
	    double *vector2 = new double[dim];
	    for (int i = 0; i < dim; i++){
		vector1[i] = (double) i;
		vector2[i] = (double) 2.0*i + 1.0;
	    }

	    Math::mt_axpy(dim, 3.0, vector1, vector2, nThreads);
	    for (int i = 0; i < dim; i++){
		EXPECT_EQ(5.0 * i + 1.0, vector2[i]);
	    }
	    delete [] vector1;
	    delete [] vector2;
	}
    }

}

TEST_F(Test, TestBlasWrapper, scal)
{
    Math::scal(dim_, 2.0, vector_, 1);
    EXPECT_EQ(2.0, vector_[0]);
    EXPECT_EQ(4.0, vector_[1]);
    EXPECT_EQ(2.0, vector_[2]);

    for (int dim = 0; dim < 10; dim++){
	double *vector1 = new double[dim];
	for (int i = 0; i < dim; i++){
	    vector1[i] = (double) i;
	}
	Math::scal(dim, 3.0, vector1, 1);
	for (int i = 0; i < dim; i++){
	    EXPECT_EQ(3.0 * i, vector1[i]);
	}
	delete [] vector1;
    }

}

TEST_F(Test, TestBlasWrapper, mt_scal){
    for (int nThreads = 1; nThreads <= 4; nThreads++){
	vector_[0] = 1.0;
	vector_[1] = 2.0;
	vector_[2] = 3.0;
	Math::mt_scal(dim_, 2.0, vector_, nThreads);
	EXPECT_EQ(2.0, vector_[0]);
	EXPECT_EQ(4.0, vector_[1]);
	EXPECT_EQ(6.0, vector_[2]);
    }
    for (int dim = 0; dim < 10; dim++){
	for (int nThreads = 0; nThreads <= 4; nThreads++){
	    double *vector1 = new double[dim];
	    for (int i = 0; i < dim; i++){
		vector1[i] = (double) i;
	    }
	    Math::mt_scal(dim, 3.0, vector1, nThreads);
	    for (int i = 0; i < dim; i++){
		EXPECT_EQ(3.0 * i, vector1[i]);
	    }
	    delete [] vector1;
	}
    }

}



TEST_F(Test, TestBlasWrapper, ddot)
{
    double result = Math::dot<double>(dim_, vector_, 1, vector2_, 1);
    EXPECT_EQ(7.0, result);
}

TEST_F(Test, TestBlasWrapper, mt_dot)
{
    // TODO test with larger vector
    for (int nThreads = 1; nThreads <= 4; nThreads++){
	double result = Math::mt_dot<double>(dim_, vector_, vector2_, nThreads);
	EXPECT_EQ(7.0, result);
    }
}


TEST_F(Test, TestBlasWrapper, dasum)
{
    double result = Math::asum<double>(dim_, vector_, 1);
    EXPECT_EQ(4.0, result);
}

TEST_F(Test, TestBlasWrapper, dgemv)
{
    double *result = new double[dim_];
    Math::gemv<double>(CblasRowMajor, CblasNoTrans, dim_, dim_, 1.0, matrix_, dim_, vector_, 1, 0.0, result, 1);
    EXPECT_EQ(8.0, result[0]);
    EXPECT_EQ(20.0, result[1]);
    EXPECT_EQ(-8.0, result[2]);
    delete [] result;
}

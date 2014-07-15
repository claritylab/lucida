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
#ifndef MATH_BLAS_HH_
#define MATH_BLAS_HH_

// ACML is included with cblas.h and linking to correct cblas library
#include <string>
#include <Modules.hh>
#if 1
#include <cblas.h>
#include "MultithreadingHelper.hh"
#endif

/*
 * template wrapper for blas routines
 * simplifies usage of blas in template classes
 *
 */

namespace Math {

/*
 *
 *  BLAS LEVEL 1
 *
 */

/**
 * swap ()
 *
 * @param N input vector dimension
 * @param X input vector
 * @param incX increment of X
 * @param Y input vector
 * @param incY increment of X
 *
 */
template<typename T>
inline void swap(const int N, T *X, const int incX, T *Y, const int incY);

template<>
inline void swap(const int N, float *X, const int incX, float *Y, const int incY) {
    cblas_sswap(N, X, incX, Y, incY);
}
template<>
inline void swap(const int N, double *X, const int incX, double *Y, const int incY) {
    cblas_dswap(N, X, incX, Y, incY);
}

/**
 * nrm2 ( Euclidean norm)
 *
 * @return (X^T X)^0.5
 *
 * @param N input vector dimension
 * @param X input vector
 * @param incX increment of X
 *
 */
template <typename T>
inline T nrm2(const int N, const T *X, const int incX);

template<>
inline float nrm2(const int N, const float *X, const int incX){
    return cblas_snrm2(N, X, incX);
}

template<>
inline double nrm2(const int N, const double *X, const int incX){
    return cblas_dnrm2(N, X, incX);
}

/**
 * asum (one-norm)
 *
 * @return sum_i abs(x_i)
 *
 * @param N input vector dimension
 * @param X input vector
 * @param incX increment of X
 *
 */

template<typename T>
inline T asum(const int N, const T *X, const int incX);

template<>
inline double asum<double>(const int N, const  double *X, const int incX){
    return cblas_dasum(N, X, incX);
}

template<>
inline float asum<float>(const int N, const float *X, const int incX){
    return cblas_sasum(N, X, incX);
}

/**
 * iamax
 *
 * @return the first index of the maximum absolute value of vector x
 *
 * @param N input vector dimension
 * @param X input vector
 * @param incX increment of X
 *
 */

template<typename T>
inline T iamax(const int N, const T *X, const int incX);

template<>
inline double iamax<double>(const int N, const  double *X, const int incX){
    return cblas_idamax(N, X, incX);
}

template<>
inline float iamax<float>(const int N, const float *X, const int incX){
    return cblas_isamax(N, X, incX);
}

/**
 * scal ( vector scaling)
 *
 * X = alpha * X
 *
 * @param N input vector dimension
 * @param X input vector
 * @param alpha scaling factor
 * @param incX increment of X
 *
 */

template<typename T>
inline void scal(const int N, const T alpha, T *X, const int incX);

template<>
inline void scal<double>(const int N, const double alpha, double *X, const int incX) {
    cblas_dscal(N,alpha,X,incX);
}

template<>
inline void scal<float>(const int N, const float alpha, float *X, const int incX) {
    cblas_sscal(N,alpha,X,incX);
}

/**
 *  axpy (weighted vector sum)
 *
 *  y += alpha * x
 *
 * @param N input vector dimension
 * @param alpha scaling factor
 * @param X input vector
 * @param incX increment of X
 * @param Y result vector
 * @param incY increment of Y
 *
 */

template<typename T, typename S>
inline void axpy(const int N, const T alpha, const T *X,
	const int incX, S *Y, const int incY);

template<typename T, typename S>
inline void axpy(const int N, const T alpha, const T *X,
	const int incX, S *Y, const int incY) {
    if (incX == 1 && incY == 1){
	for (int i = 0; i < N; i++){
	    Y[i] += alpha * X[i];
	}
    }
    else{
	int ix = 0, iy = 0;
	for (int i = 0; i < N; i++, ix += incX, iy += incY ){
	    Y[iy] += alpha * X[ix];
	}
    }
}

template<>
inline void axpy<double, double>(const int N, const double alpha, const double *X,
	const int incX, double *Y, const int incY){
    cblas_daxpy(N, alpha, X, incX, Y, incY);
}

template<>
inline void axpy<float, float>(const int N, const float alpha, const float *X,
	const int incX, float *Y, const int incY){
    cblas_saxpy(N, alpha, X, incX, Y, incY);
}

/**
 *  dot (dot product)
 *
 * @return x^T y
 *
 * @param N input vector dimension
 * @param X first input vector
 * @param incX increment of X
 * @param Y second input vector
 * @param incY increment of Y
 *
 */

template<typename T, typename S>
inline T dot(const int N, const T *X, const int incX, const S *Y, const int incY, int nThreads = 1);

template<typename T, typename S>
inline T dot(const int N, const T *X, const int incX, const S *Y, const int incY, int nThreads){
    T result = 0.0;
    if (incX == 1 && incY == 1){
	for (int i = 0; i < N; i++){
	    result += X[i] * Y[i];
	}
    }
    else{
	int ix = 0, iy = 0;
	for (int i = 0; i < N; i++, ix += incX, iy += incY){
	    result += X[ix] * Y[iy];
	}
    }
    return result;
}

template<>
inline double dot<double, double>(const int N, const double *X, const int incX,
	const double *Y, const int incY, int nThreads){
    return cblas_ddot(N, X, incX, Y, incY);
}

template<>
inline float dot<float, float>(const int N, const float *X, const int incX,
	const float *Y, const int incY, int nThreads){
    return cblas_sdot(N, X, incX, Y, incY);
}

/**
 *  copy
 *
 * y = x
 *
 * @param N result vector dimension
 * @param X input vector
 * @param incX increment of X
 * @param Y result vector
 * @param incY increment of Y
 *
 */


template<typename T, typename S>
inline void copy(const int N, const T *X, const int incX,
	S *Y, const int incY);

template<typename T, typename S>
inline void copy(const int N, const T *X, const int incX,
	S *Y, const int incY){
    if (incX == 1 && incY == 1){
	for (int i = 0; i < N; i++){
	    Y[i] = X[i];
	}
    }
    else{
	int ix = 0, iy = 0;
	for (int i = 0; i < N; i++, ix += incX, iy += incY){
	    Y[iy] = X[iy];
	}
    }
}


template<>
inline void copy<double, double>(const int N, const double *X, const int incX,
	double *Y, const int incY){
    cblas_dcopy(N, X, incX, Y, incY);
}

template<>
inline void copy<float, float>(const int N, const float *X, const int incX,
	float *Y, const int incY){
    cblas_scopy(N, X, incX, Y, incY);
}


// BLAS LEVEL 2

/*
 * ger (rank-1 update)
 *
 * A = alpha * X * Y^T + A
 *
 *  order:	CblasRowMajor or  CblasColMajor, row major or column major storage of matrix (typically row major in C/C++)
 *  M:		number of rows of A
 *  N:		number of columns of A
 *  alpha:	scalar factor
 *  A:		matrix
 *  lda:	first dimension of A, typically equal to N, allows for discarding columns
 *  X:		input vector1
 *  incX:	increment of X
 *  Y:		input vector2
 *  incY:	increment of Y
 *
 */

template<typename T>
inline void ger(const CBLAS_ORDER order, const int M, const int N,
	const T alpha, const T *X, const int incX,
	const T *Y, const int incY, T *A, const int lda);

template<>
inline void ger(const CBLAS_ORDER order, const int M, const int N,
	const float alpha, const float *X, const int incX,
	const float *Y, const int incY, float *A, const int lda)
{
    cblas_sger(order, M, N, alpha, X, incX, Y, incY, A, lda);
}

template<>
inline void ger(const CBLAS_ORDER order, const int M, const int N,
	const double alpha, const double *X, const int incX,
	const double *Y, const int incY, double *A, const int lda)
{
    cblas_dger(order, M, N, alpha, X, incX, Y, incY, A, lda);
}

/*
 * gemv (matrix vector product)
 *
 *  Y := alpha*A*X + beta*Y,   or   y := alpha*A**T*X + beta*Y
 *
 *  order:	CblasRowMajor or  CblasColMajor, row major or column major storage of matrix (typically row major in C/C++)
 *  TransA:	CblasNoTrans or CblasTrans for original or transposed matrix
 *  M:		number of rows of A
 *  N:		number of columns of A
 *  alpha:	scalar factor
 *  A:		matrix
 *  lda:	first dimension of A, typically equal to N (row major storage), allows for discarding columns
 *  X:		input vector
 *  incX:	increment of X
 *  beta:	scalar factor
 *  Y:		result vector
 */

template<typename T>
inline void gemv(const CBLAS_ORDER order,
	const CBLAS_TRANSPOSE TransA, const int M, const int N,
	const T alpha, const T *A, const int lda,
	const T *X, const int incX, const T beta,
	T *Y, const int incY);


template<>
inline void gemv<double>(const CBLAS_ORDER order,
	const CBLAS_TRANSPOSE TransA, const int M, const int N,
	const double alpha, const double *A, const int lda,
	const double *X, const int incX, const double beta,
	double *Y, const int incY){
    cblas_dgemv(order, TransA, M, N, alpha, A, lda, X, incX, beta, Y, incY);
}

template<>
inline void gemv<float>(const CBLAS_ORDER order,
	const CBLAS_TRANSPOSE TransA, const int M, const int N,
	const float alpha, const float *A, const int lda,
	const float *X, const int incX, const float beta,
	float *Y, const int incY){
    cblas_sgemv(order, TransA, M, N, alpha, A, lda, X, incX, beta, Y, incY);
}


// BLAS LEVEL 3

/*
 * gemv (matrix vector product)
 *
 *  C := alpha*A*B + beta*V,   or   V := alpha*A**T*B + beta*C
 *
 *  order:		CblasRowMajor or  CblasColMajor, row major or column major storage of matrix (typically row major in C/C++)
 *  TransA:		CblasNoTrans or CblasTrans for original or transposed matrix A
 *  TransB:		CblasNoTrans or CblasTrans for original or transposed matrix B
 *  M:			number of rows of C = number of rows of A
 *  N:			number of columns of C = number of columns of B
 *  K:			number of columns of A = number of rows B
 *  alpha:		scalar factor
 *  A,B			input matrices
 *  C			result matrix
 *  lda, ldb, ldc:	first dimensions of A, B, and C typically equal to the number of columns (in case of row major storage), allows for discarding columns
 *  beta:		scalar factor
 */
template<typename T>
inline void gemm(const CBLAS_ORDER order,
	const CBLAS_TRANSPOSE TransA, const CBLAS_TRANSPOSE TransB,
	const int M, const int N, const int K,
	const T alpha, const T* A, const int lda,
	const T* B, const int ldb,
	const T beta, T* C, const int ldc);

template<>
inline void gemm<float>(const CBLAS_ORDER order,
	const CBLAS_TRANSPOSE transMatrixA, const CBLAS_TRANSPOSE transMatrixB,
	const int sizeM, const int sizeN, const int sizeK,
	const float scaleMatrixA, const float* matrixA, const int lda,
	const float* matrixB, const int ldab,
	const float scaleMatrixC, float* matrixC, const int ldc) {
    cblas_sgemm(order, transMatrixA, transMatrixB,
	    sizeM, sizeN, sizeK,
	    scaleMatrixA, matrixA, lda,
	    matrixB, ldab,
	    scaleMatrixC, matrixC, ldc);
}

template<>
inline void gemm<double>(const CBLAS_ORDER order,
	const CBLAS_TRANSPOSE transMatrixA, const CBLAS_TRANSPOSE transMatrixB,
	const int sizeM, const int sizeN, const int sizeK,
	const double scaleMatrixA, const double* matrixA, const int lda,
	const double* matrixB, const int ldb,
	const double scaleMatrixC, double* matrixC, const int ldc) {
    cblas_dgemm(order, transMatrixA, transMatrixB,
	    sizeM, sizeN, sizeK,
	    scaleMatrixA, matrixA, lda,
	    matrixB, ldb,
	    scaleMatrixC, matrixC, ldc);
}

inline std::string getMathLibrary(){
    std::string result;
#if 1
    result = "CBLAS";
#endif
    return result;

}


/**
 *
 *
 * (simplified) multithreaded versions that are not automatically parallelized by
 * corresponding math library
 *
 *
 */

// SCAL

template<typename T>
inline void mt_scal(int N, T alpha, T* X, int nThreads);

#if 1

template<typename T>
inline void mt_scal(int N, T alpha, T *X, int nThreads){
#pragma omp parallel for
    for (int i = 0; i < N; i++){
	X[i] *= alpha;
    }
}

inline void __mt_scal_f(int N, float alpha, const float *dummy, float *X){
    scal(N, alpha, X, 1);
}

template<>
inline void mt_scal(int N, float alpha, float *X, int nThreads){
    const float *dummy = 0;
    mt_sv2v(N, alpha, dummy, X, __mt_scal_f, nThreads);
}

inline void __mt_scal_d(int N, double alpha, const double *dummy, double *X){
    scal(N, alpha, X, 1);
}

template<>
inline void mt_scal(int N, double alpha, double *X, int nThreads){
    const double *dummy = 0;
    mt_sv2v(N, alpha, dummy, X, __mt_scal_d, nThreads);
}

#endif

// AXPY

template<typename T>
inline void mt_axpy(int N, T alpha, const T *X, T *Y, int nThreads);

#if 1

template<typename T>
inline void mt_axpy(int N, T alpha, const T *X, T *Y, int nThreads){
#pragma omp parallel for
    for (int i = 0; i < N; i++){
	Y[i] += alpha * X[i];
    }
}

inline void __mt_axpy_f(int N, float alpha, const float *X, float *Y){
    axpy(N, alpha, X, 1, Y, 1);
}

template<>
inline void mt_axpy(int N, float alpha, const float *X, float *Y, int nThreads){
    mt_sv2v(N, alpha, X, Y, __mt_axpy_f, nThreads);
}

inline void __mt_axpy_d(int N, double alpha, const double *X, double *Y){
    axpy(N, alpha, X, 1, Y, 1);
}

template<>
inline void mt_axpy(int N, double alpha, const double *X, double *Y, int nThreads){
    mt_sv2v(N, alpha, X, Y, __mt_axpy_d, nThreads);
}


#endif

// DOT

template<typename T>
inline T mt_dot(int N, const T *X, const T *Y, int nThreads);

#if 1

template<typename T>
inline T mt_dot(int N, T alpha, const T *X, const T *Y, int nThreads){
    T result = 0.0;
    #pragma omp parallel for reduction(+:result)
    for (int i = 0; i < N; i++){
	result += X[i] * Y[i];
    }
    return result;
}

inline float __mt_dot_f(int N, float dummy, const float *X, const float *Y){
    return dot(N, X, 1, Y, 1);
}

template<>
inline float mt_dot(int N, const float *X, const float *Y, int nThreads){
    return mt_svv2s(N, 0.0f, X, Y, __mt_dot_f, nThreads);
}

inline double __mt_dot_d(int N, double dummy, const double *X, const double *Y){
    return dot(N, X, 1, Y, 1);
}

template<>
inline double mt_dot(int N, const double *X, const double *Y, int nThreads){
    return mt_svv2s(N, 0.0, X, Y, __mt_dot_d, nThreads);
}

#endif

// NRM2

template<typename T>
inline T mt_nrm2(int N, const T *X, int nThreads);

#if 1

template<typename T>
inline T mt_nrm2(int N, T alpha, const T *X, int nThreads){
    T result = 0.0;
    #pragma omp parallel for reduction(+:result)
    for (int i = 0; i < N; i++){
	result += X[i] * X[i];
    }
    return std::sqrt(result);
}

inline float __mt_nrm2_f(int N, float dummy1, const float *X, const float *dummy2){
    return nrm2(N, X, 1);
}

template<>
inline float mt_nrm2(int N, const float *X, int nThreads){
    const float *dummy = 0;
    return mt_svv2s(N, 0.0f, X, dummy, __mt_nrm2_f, nThreads);
}

inline double __mt_nrm2_d(int N, double dummy1, const double *X, const double *dummy2){
    return nrm2(N, X, 1);
}

template<>
inline double mt_nrm2(int N, const double *X, int nThreads){
    const double *dummy = 0;
    return mt_svv2s(N, 0.0, X, dummy, __mt_nrm2_d, nThreads);
}

#endif

// ASUM

template<typename T>
inline T mt_asum(int N, const T *X, int nThreads);

#if 1

template<typename T>
inline T mt_asum(int N, T alpha, const T *X, int nThreads){
    T result = 0.0;
    #pragma omp parallel for reduction(+:result)
    for (int i = 0; i < N; i++){
	result += X[i] * X[i];
    }
    return std::sqrt(result);
}

inline float __mt_asum_f(int N, float dummy1, const float *X, const float *dummy2){
    return asum(N, X, 1);
}

template<>
inline float mt_asum(int N, const float *X, int nThreads){
    const float *dummy = 0;
    return mt_svv2s(N, 0.0f, X, dummy, __mt_asum_f, nThreads);
}

inline double __mt_asum_d(int N, double dummy1, const double *X, const double *dummy2){
    return asum(N, X, 1);
}

template<>
inline double mt_asum(int N, const double *X, int nThreads){
    const double *dummy = 0;
    return mt_svv2s(N, 0.0, X, dummy, __mt_asum_d, nThreads);
}

#endif


} //namespace Math

#endif

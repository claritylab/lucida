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
#ifndef _MATH_LAPACK_LAPACK_HH
#define _MATH_LAPACK_LAPACK_HH

#include "Vector.hh"
#include "Matrix.hh"

namespace Math { namespace Lapack {

    void dgelsd(s32* m, s32* n, s32* nrhs, f64* a, s32* lda, f64* b, s32* ldb,
		f64* s, f64* rcond, s32* rank, f64* work, s32* lwork, s32* iwork, s32* info);


    void dgesdd( char* jobz, s32* m, s32* n, f64* a, s32* lda, f64* s, f64* u,
		 s32 *ldu, f64* vt, s32* ldvt, f64* work, s32* lwork, s32* iwork,
		 s32* info );

    void dggevx(char* balanc, char* jobvl, char* jobvr, char* sense,
		int* n, double* a, int* lda, double* b, int* ldb,
		double* alphar, double* alphai, double* beta, double* vl, int* ldvl, double* vr, int* ldvr,
		int* ilo, int* ihi, double* lscale, double* rscale, double* abnrm, double* bbnrm,
		double* rconde, double* rcondv, double* work, int* lwork, int* iwork, int* bwork, int* info);

    void dsyevx(char *jobz, char *range, char *uplo, int *n, double *a, int *lda, double *vl,
		double *vu, int *il, int *iu, double *abstol, int *m, double *w, double *z,
		int *ldz, double *work, int *lwork, int *iwork, int *ifail, int *info);

    void dsyevd(char *jobz, char *uplo, int *n, double *a, int *lda, double *w,
		double *work, int *lwork, int *iwork, int *liwork, int *info);

    void dsyevr(char *jobz, char *range, char *uplo, int *n, double *a, int *lda, double *vl,
		double *vu, int *il, int *iu, double *abstol, int *m, double *w, double *z,
		int *ldz, int *isuppz, double *work, int *lwork, int *iwork, int *liwork, int *info);

    void dsygvx(int* itype, char* jobz, char* range, char* uplo, int* n, double* a, int* lda, double* b, int* ldb,
		double* vl, double* vu, int* il, int* iu, double *abstol, int* m, double* w, double* z, int* ldz,
		double* work, int* lwork, int* iwork, int* ifail, int* info);

    void dsygvd(int* itype, char* jobz, char* uplo, int* n, double* a, int* lda, double* b, int* ldb,
		double* w, double* work, int* lwork, int* iwork, int* liwork, int* info);


    void gels(char* trans, int* m, int* n, int* nrhs, double* A, int* lda,
	      double* B, int* ldb, double* work, int* lwork, int* info);

    void gels(char* trans, int* m, int* n, int* nrhs, float* A, int* lda,
	      float* B, int* ldb, float* work, int* lwork, int* info);


    void gelss(int* m, int* n, int* nrhs, double* A, int* lda, double* B, int* ldb,
	       double* s, double* rcond, int* rank, double* work, int* lwork, int* info);

    void gelss(int* m, int* n, int* nrhs, float* A, int* lda, float* B, int* ldb,
	       float* s, float* rcond, int* rank, float* work, int* lwork, int* info);

    /** getrf: computes an LU factorization of a general m-by-n matrix A
     *  using partial pivoting with row interchanges.
     *
     *  @param A:
     *    -On entry a general m-by-n matrix;
     *    -On exit the factors L and U from the factorization
     *     A = P*L*U; the unit diagonal elements of L are not stored
     *
     *  @param pivotIndices: for 1 <= i <= min(M,N), row i of the
     *    matrix was interchanged with row pivotIndices(i).
     *
     *  @return:
     *    0: successful exit
     *    i<0: the -i-th argument of the Lapack call had an illegal value
     *    i>0: U(i,i) is exactly zero. The factorization
     *       has been completed, but the factor U is exactly
     *       singular, and division by zero will occur if it is used
     *       to solve a system of equations.
     */
    void getrf(int *m, int *n, float *A, int *lda, int *ipiv, int *info);


    void getrf(int *m, int *n, double *A, int *lda, int *ipiv, int *info);

    template<class T>
    int getrf(Matrix<T> &A, Vector<int> &pivotIndices) {

	int m = A.nRows();
	int n = A.nColumns();
	int lda = A.nRows();
	int info;

	pivotIndices.resize(std::min(A.nRows(), A.nColumns()));

	getrf(&m, &n, A.buffer(), &lda, pivotIndices.buffer(), &info);

	return info;
    }


    /** Solves system of linear equations A*X=B
     *
     * caution: @param LU has to be the LU factorized form of A!
     *          The factors L and U from the factorization A = P*L*U written in the same matrix with
     *          the unit diagonal elements of L not stored.
     *
     *  @param LU: LU factorized form of A (see caution above)
     *  @param B:
     *    on entry, right hand side matrix
     *    on exit, the solution matrix X
     *  @param pivotIndices: LU factoriztion has interchanged
     *    row i of the matrix A with row pivotIndices(i).
     *
     *  @return:
     *    0 if successful
     *    i<0 if -i-th argument of the Lapack call had an illegal value
     */

    void getrs(char *trans, int  *n, int *nrhs, float *A, int *lda, int *ipiv,
	       float *B, int *ldb, int *info);

    void getrs(char *trans, int  *n, int *nrhs, double *A, int *lda, int *ipiv,
	       double *B, int *ldb, int *info);

    template<class T>
    int getrs(Matrix<T> &LU, Matrix<T> &B, Vector<int> &pivotIndices) {

	char trans = 'N';
	int n = std::min(std::min(LU.nColumns(), LU.nRows()), B.nRows());
	int nrhs = B.nColumns();
	int lda = LU.nRows();
	int ldb = B.nRows();
	int info = 1;

	getrs(&trans, &n, &nrhs, LU.buffer(), &lda, pivotIndices.buffer(), B.buffer(), &ldb, &info);

	return info;
    }

    void getri(int  *n, float *A, int *lda, int *ipiv,
	       float *work, int *lwork, int *info);

    void getri(int  *n, double *A, int *lda, int *ipiv,
	       double *work, int *lwork, int *info);

} } //namespace Math::Lapack

#endif //_MATH_LAPACK_LAPACK_HH

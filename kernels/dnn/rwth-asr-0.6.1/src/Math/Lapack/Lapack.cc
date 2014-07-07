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
#include "Lapack.hh"


#define F77NAME(x) x##_


extern "C" {

    void F77NAME(dgelsd)(s32* m, s32* n, s32* nrhs, f64* a, s32* lda, f64* b, s32* ldb,
			 f64* s, f64* rcond, s32* rank, f64* work, s32* lwork, s32* iwork,
			 s32* info);

    void F77NAME(dgesdd) (char* jobz, s32* m, s32* n, f64* a, s32* lda, f64* s,
			  f64* u, s32 *ldu, f64* vt, s32* ldvt, f64* work,
			  s32* lwork, s32* iwork, s32* info );

    void F77NAME(dggevx)(char* balanc, char* jobvl, char* jobvr, char* sense,
			 int* n, double* a, int* lda, double* b, int* ldb,
			 double* alphar, double* alphai, double* beta, double* vl, int* ldvl, double* vr, int* ldvr,
			 int* ilo, int* ihi, double* lscale, double* rscale, double* abnrm, double* bbnrm,
			 double* rconde, double* rcondv, double* work, int* lwork, int* iwork, int* bwork, int* info);

    void F77NAME(dsyevx)(char *jobz, char *range, char *uplo, int *n, double *a, int *lda, double *vl,
			 double *vu, int *il, int *iu, double *abstol, int *m, double *w, double *z,
			 int *ldz, double *work, int *lwork, int *iwork, int *ifail, int *info);

    void F77NAME(dsyevd)(char *jobz, char *uplo, int *n, double *a, int *lda, double *w,
			 double *work, int *lwork, int *iwork, int *liwork, int *info);

    void F77NAME(dsyevr)(char *jobz, char *range, char *uplo, int *n, double *a, int *lda, double *vl,
			 double *vu, int *il, int *iu, double *abstol, int *m, double *w, double *z,
			 int *ldz, int *isuppz, double *work, int *lwork, int *iwork, int *liwork, int *info);

    void F77NAME(dsygvx)(int* itype, char* jobz, char* range, char* uplo, int* n, double* a, int* lda,
			 double* b, int* ldb, double* vl, double* vu, int* il, int* iu, double *abstol,
			 int* m, double* w, double* z, int* ldz, double* work, int* lwork, int* iwork,
			 int* ifail, int* info);

    void F77NAME(dsygvd)(int* itype, char* jobz, char* uplo, int* n, double* a, int* lda, double* b, int* ldb,
			 double* w, double* work, int* lwork, int* iwork, int* liwork, int* info);

    void F77NAME(dgels)(char* trans, int* m, int* n, int* nrhs, double* A, int* lda,
			double* B, int* ldb, double* work, int* lwork, int* info);

    void F77NAME(sgels)(char* trans, int* m, int* n, int* nrhs, float* A, int* lda,
			float* B, int* ldb, float* work, int* lwork, int* info);

    void F77NAME(dgelss)(int* m, int* n, int* nrhs, double* A, int* lda, double* B, int* ldb,
			 double* s, double* rcond, int* rank, double* work, int* lwork, int* info);

    void F77NAME(sgelss)(int* m, int* n, int* nrhs, float* A, int* lda, float* B, int* ldb,
			 float* s, float* rcond, int* rank, float* work, int* lwork, int* info);

    void F77NAME(sgetrf)(int *m, int *n, float *A, int *lda, int *ipiv, int *info);

    void F77NAME(dgetrf)(int *m, int *n, double *A, int *lda, int *ipiv, int *info);

    void F77NAME(sgetrs)(char* trans, int* n, int* nrhs, float* A, int* lda, int *ipiv,
			 float* B, int* ldb, int* info);

    void F77NAME(dgetrs)(char* trans, int* n, int* nrhs, double* A, int* lda, int *ipiv,
			 double* B, int* ldb, int* info);

    void F77NAME(sgetri)(int  *n, float *A, int *lda, int *ipiv,
			 float *work, int *lwork, int *info);

    void F77NAME(dgetri)(int *n, double *A, int *lda, int *ipiv,
			 double *work, int *lwork, int *info);
}

namespace Math { namespace Lapack {

    // Type dependend Lapack definitions:

    void dgelsd(s32* m, s32* n, s32* nrhs, f64* a, s32* lda, f64* b, s32* ldb,
		f64* s, f64* rcond, s32* rank, f64* work, s32* lwork, s32* iwork, s32* info){
	F77NAME(dgelsd)( m, n, nrhs, a, lda, b, ldb, s, rcond, rank, work,lwork, iwork, info);
    }

    void dgesdd( char* jobz, s32* m, s32* n, f64* a, s32* lda, f64* s, f64* u,
	     s32 *ldu, f64* vt, s32* ldvt, f64* work, s32* lwork, s32* iwork,
	     s32* info ){
	F77NAME(dgesdd)(jobz,  m,  n,  a,  lda,  s,  u, ldu,  vt,  ldvt,  work,  lwork,  iwork, info );
    }



    void dggevx(char* balanc, char* jobvl, char* jobvr, char* sense,
		int* n, double* a, int* lda, double* b, int* ldb,
		double* alphar, double* alphai, double* beta, double* vl, int* ldvl, double* vr, int* ldvr,
		int* ilo, int* ihi, double* lscale, double* rscale, double* abnrm, double* bbnrm,
		double* rconde, double* rcondv, double* work, int* lwork, int* iwork, int* bwork, int* info) {
	F77NAME(dggevx)(balanc, jobvl, jobvr, sense, n, a, lda, b, ldb,
			alphar, alphai, beta, vl, ldvl, vr, ldvr,
			ilo, ihi, lscale, rscale, abnrm, bbnrm,
			rconde, rcondv, work, lwork, iwork, bwork, info);
    }

    void dsyevx(char *jobz, char *range, char *uplo, int *n, double *a, int *lda, double *vl,
		double *vu, int *il, int *iu, double *abstol, int *m, double *w, double *z,
		int *ldz, double *work, int *lwork, int *iwork, int *ifail, int *info) {
	F77NAME(dsyevx)(jobz, range, uplo, n, a, lda, vl, vu, il, iu, abstol, m, w, z, ldz,
			work, lwork, iwork, ifail, info);
    }

    void dsyevd(char *jobz, char *uplo, int *n, double *a, int *lda, double *w,
		double *work, int *lwork, int *iwork, int *liwork, int *info) {
	F77NAME(dsyevd)(jobz, uplo, n, a, lda, w, work, lwork, iwork, liwork, info);
    }

    void dsyevr(char *jobz, char *range, char *uplo, int *n, double *a, int *lda, double *vl,
		double *vu, int *il, int *iu, double *abstol, int *m, double *w, double *z,
		int *ldz, int *isuppz, double *work, int *lwork, int *iwork, int *liwork, int *info) {
	F77NAME(dsyevr)(jobz, range, uplo, n, a, lda, vl, vu, il, iu, abstol, m, w, z, ldz, isuppz,
			work, lwork, iwork, liwork, info);
    }

    void dsygvx(int* itype, char* jobz, char* range, char* uplo, int* n, double* a, int* lda, double* b, int* ldb,
		double* vl, double* vu, int* il, int* iu, double *abstol, int* m, double* w, double* z, int* ldz,
		double* work, int* lwork, int* iwork, int* ifail, int* info) {

	F77NAME(dsygvx)(itype, jobz, range, uplo, n, a, lda, b, ldb,
			vl, vu, il, iu, abstol, m, w, z, ldz,
			work, lwork, iwork, ifail, info);
    }

    void dsygvd(int* itype, char* jobz, char* uplo, int* n, double* a, int* lda, double* b, int* ldb,
		double* w, double* work, int* lwork, int* iwork, int* liwork, int* info) {

	F77NAME(dsygvd)(itype, jobz, uplo, n, a, lda, b, ldb,
			w, work, lwork, iwork, liwork, info);
    }


    void gels(char* trans, int* m, int* n, int* nrhs, double* A, int* lda,
	      double* B, int* ldb, double* work, int* lwork, int* info) {

	F77NAME(dgels)(trans, m, n, nrhs, A, lda, B, ldb, work, lwork, info);
    }

    void gels(char* trans, int* m, int* n, int* nrhs, float* A, int* lda,
	      float* B, int* ldb, float* work, int* lwork, int* info) {

	F77NAME(sgels)(trans, m, n, nrhs, A, lda, B, ldb, work, lwork, info);
    }


    void gelss(int* m, int* n, int* nrhs, double* A, int* lda, double* B, int* ldb,
	       double* s, double* rcond, int* rank, double* work, int* lwork, int* info) {

	F77NAME(dgelss)(m, n, nrhs, A, lda, B, ldb, s, rcond, rank, work, lwork, info);
    }

    void gelss(int* m, int* n, int* nrhs, float* A, int* lda, float* B, int* ldb,
	       float* s, float* rcond, int* rank, float* work, int* lwork, int* info) {

	F77NAME(sgelss)(m, n, nrhs, A, lda, B, ldb, s, rcond, rank, work, lwork, info);
    }


    void getrf(int *m, int *n, float *A, int *lda, int *ipiv, int *info) {

	F77NAME(sgetrf)(m, n, A, lda, ipiv, info);
    }

    void getrf(int *m, int *n, double *A, int *lda, int *ipiv, int *info) {

	F77NAME(dgetrf)(m, n, A, lda, ipiv, info);
    }


    void getrs(char *trans, int  *n, int *nrhs, float *A, int *lda, int *ipiv,
		float *B, int *ldb, int *info) {

	F77NAME(sgetrs)(trans, n, nrhs, A, lda, ipiv, B, ldb, info);
    }

    void getrs(char *trans, int *n, int *nrhs, double *A, int *lda, int *ipiv,
		double *B, int *ldb, int *info) {

	F77NAME(dgetrs)(trans, n, nrhs, A, lda, ipiv, B, ldb, info);
    }

    void getri(int  *n, float *A, int *lda, int *ipiv,
		float *work, int *lwork, int *info) {

	F77NAME(sgetri)(n, A, lda, ipiv, work, lwork, info);
    }

    void getri(int *n, double *A, int *lda, int *ipiv,
		double *work, int *lwork, int *info) {

	F77NAME(dgetri)(n, A, lda, ipiv, work, lwork, info);
    }
} } //namespace Math::Lapack

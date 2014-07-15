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
#include <Core/Types.hh>
#include "Svd.hh"


namespace Math { namespace Lapack {

    s32 pseudoInvert(DoubleMatrix& result,const DoubleMatrix& A){
	require(A.nRows()==A.nColumns()); //other cases possible but not implemeted yet
	if(result.nRows()!=A.nRows() || result.nColumns()!=A.nColumns())
	    result.resize(A.nRows(),A.nColumns());
	DoubleMatrix B(A.nRows());
	B.fill(0.0);
	for(u16 rowIdx = 0; rowIdx < B.nRows(); rowIdx++) B[rowIdx][rowIdx]=1.0;
	return solveLinearLeastSquares(result,A,B);
    }

    s32 solveLinearLeastSquares(DoubleVector& result,const DoubleMatrix& A, const DoubleVector& b){
	DoubleMatrix B;
	B.addColumn(b);
	DoubleMatrix tempResult;
	s32 status=solveLinearLeastSquares(tempResult, A, B);
	result=tempResult.column(0);
	return status;
    }

    s32 solveLinearLeastSquares(DoubleMatrix& result,const DoubleMatrix& A, const DoubleMatrix& B ){

	s32 m=A.nRows();
	s32 n=A.nColumns();
	f64 *a= new f64[m*n];
	s32 lda=m;
	s32 nrhs=B.nColumns();
	s32 maxMN=std::max(m,n);
	f64 *b= new f64[maxMN*nrhs];
	s32 ldb=maxMN;
	f64 *s= new f64[n];
	f64 rcond=svdThreshold;
	s32 rank;
	f64* work = new f64[1]; //needed size will be estimated with 1st call;
	s32 lwork=-1;
	s32 *iwork = new s32[1];//needed size will be estimated with 1st call;
	s32 info;

	for(u16 rowIdx = 0; rowIdx < A.nRows(); rowIdx++)
	    for(u16 columnIdx = 0; columnIdx <A.nColumns() ; columnIdx++) {
		a[columnIdx*lda + rowIdx] = A[rowIdx][columnIdx];
	    }

	for(u16 rowIdx = 0; rowIdx < B.nRows() ; rowIdx++)
	    for(u16 columnIdx = 0; columnIdx <B.nColumns(); columnIdx++)
		b[columnIdx*ldb + rowIdx]= B[rowIdx][columnIdx];


	//first call: estimate size of work array
	dgelsd(&m, &n, &nrhs, a, &lda, b, &ldb, s, &rcond, &rank, work, &lwork, iwork, &info);
	lwork=s32(workArrayFactor*work[0]);
	delete[] work;
	delete[] iwork;
	work = new f64[lwork];
	iwork = new s32[lwork];
	dgelsd(&m, &n, &nrhs, a, &lda, b, &ldb, s, &rcond, &rank, work, &lwork, iwork, &info);
	/* from Lapck man page:

	  INFO    (output) INTEGER
		  = 0:  successful exit
		  < 0:  if INFO = -i, the i-th argument had an illegal value.
		  > 0:  the algorithm for computing the SVD failed to converge;
			if INFO = i, i off-diagonal elements of an intermediate bidiagonal form did not
			converge to zero.
	*/

	result.resize(n,nrhs);

	for(u16 rowIdx = 0; rowIdx < result.nRows(); rowIdx++){
	    for(u16 columnIdx = 0; columnIdx < result.nColumns(); columnIdx++) {
		result[rowIdx][columnIdx] = b[columnIdx*ldb + rowIdx];
	    }
	}

	delete[] a;
	delete[] b;
	delete[] s;
	delete[] work;
	delete[] iwork;

	return info;
    }


    s32 svd(DoubleMatrix &U, DoubleVector &W, DoubleMatrix& V, const DoubleMatrix& A){ //returns V, not(!) VT


	char jobz = 'A';
	s32 m=A.nRows();
	s32 n=A.nColumns();
	u16 minDimension=std::min(m,n);
	f64 *a= new f64[m*n];
	s32 lda=m;
	f64 *s= new f64[minDimension];
	f64 *u = new f64[m*m];
	s32 ldu=m;
	f64 *vt = new f64[n*n];
	s32 ldvt=n;
	s32 lwork = -1; //needed size will be estimated with 1st call;
	f64 *work= new f64[1];
	s32 *iwork=new s32[8*minDimension];
	s32 info;


	for(u16 rowIdx = 0; rowIdx < A.nRows(); rowIdx++)
	    for(u16 columnIdx = 0; columnIdx < A.nColumns(); columnIdx++) {
		a[columnIdx*lda + rowIdx] = A[rowIdx][columnIdx];
	    }

	//first call: estimate size of work array
	dgesdd( &jobz, &m, &n,  a, &lda, s, u, &ldu, vt, &ldvt, work, &lwork, iwork, &info );
	lwork=s32(workArrayFactor*work[0]);
	delete[] work;
	work = new f64[lwork];

	dgesdd( &jobz, &m, &n,  a, &lda, s, u, &ldu, vt, &ldvt, work, &lwork, iwork, &info );


	U.resize(m,m);
	V.resize(n,n);
	W.resize(minDimension);

	if(m==n){
	    for(u16 rowIdx = 0; rowIdx < m; rowIdx++){
		for(u16 columnIdx = 0; columnIdx < m; columnIdx++) {
		    U[rowIdx][columnIdx] = u[columnIdx*ldu + rowIdx];
		    V[columnIdx][rowIdx] = vt[columnIdx*ldvt + rowIdx]; //returns V, not VT
		}
		W[rowIdx]=s[rowIdx];
	    }
	}
	else{
	    for(u16 rowIdx = 0; rowIdx < m; rowIdx++){
		for(u16 columnIdx = 0; columnIdx < m; columnIdx++) {
		    U[rowIdx][columnIdx] = u[columnIdx*ldu + rowIdx];
		}
	    }
	    for(u16 rowIdx = 0; rowIdx < n; rowIdx++){
		for(u16 columnIdx = 0; columnIdx < n; columnIdx++) {
		    V[columnIdx][rowIdx] = vt[columnIdx*ldvt + rowIdx]; //returns V, not VT
		}
	    }
	    for( u16 idx=0; idx< minDimension; ++idx) W[idx]=s[idx];
	}
	delete[] a;
	delete[] s;
	delete[] u;
	delete[] vt;
	delete[] work;
	delete[] iwork;
	return info;
    }


} } // namespace Math::Lapack

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
#include <cmath>
#include "nr.h"
#include <Math/Matrix.hh>
#include <Math/Vector.hh>
namespace Math { namespace Nr {

    void ludcmp(Math::Matrix<DP> &a, Math::Vector<int>  &indx, DP &d)
    {
	require(a.nRows()==a.nColumns());
	require(indx.size()==a.nColumns());

	const DP TINY=1.0e-20;
	int i,imax = 0,j,k;
	DP big,dum,sum,temp;

	int n=a.nRows();
	Math::Vector<DP> vv(n);
	d=1.0;
	for (i=0;i<n;i++) {
	    big=0.0;
	    for (j=0;j<n;j++)
		if ((temp=fabs(a[i][j])) > big) big=temp;
	    if (big == 0.0) nrerror("Singular matrix in routine ludcmp");
	    vv[i]=1.0/big;
	}
	for (j=0;j<n;j++) {
	    for (i=0;i<j;i++) {
		sum=a[i][j];
		for (k=0;k<i;k++) sum -= a[i][k]*a[k][j];
		a[i][j]=sum;
	    }
	    big=0.0;
	    for (i=j;i<n;i++) {
		sum=a[i][j];
		for (k=0;k<j;k++) sum -= a[i][k]*a[k][j];
		a[i][j]=sum;
		if ((dum=vv[i]*fabs(sum)) >= big) {
		    big=dum;
		    imax=i;
		}
	    }
	    if (j != imax) {
		for (k=0;k<n;k++) {
		    dum=a[imax][k];
		    a[imax][k]=a[j][k];
		    a[j][k]=dum;
		}
		d = -d;
		vv[imax]=vv[j];
	    }
	    indx[j]=imax;
	    if (a[j][j] == 0.0) a[j][j]=TINY;
	    if (j != n-1) {
		dum=1.0/(a[j][j]);
		for (i=j+1;i<n;i++) a[i][j] *= dum;
	    }
	}
    }



    void lubksb(Math::Matrix<DP> &a, Math::Vector<int>  &indx, Math::Vector<DP> &b)
    {
	require(a.nRows()==a.nColumns());
	require(indx.size()==a.nColumns());
	require(b.size()==a.nColumns());

	int i,ii=0,ip,j;
	DP sum;

	int n=a.nRows();
	for (i=0;i<n;i++) {
	    ip=indx[i];
	    sum=b[ip];
	    b[ip]=b[i];
	    if (ii != 0)
		for (j=ii-1;j<i;j++) sum -= a[i][j]*b[j];
	    else if (sum != 0.0)
		ii=i+1;
	    b[i]=sum;
	}
	for (i=n-1;i>=0;i--) {
	    sum=b[i];
	    for (j=i+1;j<n;j++) sum -= a[i][j]*b[j];
	    b[i]=sum/a[i][i];
	}
    }

} } //namespace Math::Nr

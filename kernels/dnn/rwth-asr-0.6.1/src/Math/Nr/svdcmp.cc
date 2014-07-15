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

    void svdcmp(Math::Matrix<DP> &a, Math::Vector<DP> &w, Math::Matrix<DP> &v)
    {
	require(a.nColumns()==v.nColumns());
	require(v.nColumns()==v.nRows());
	require(w.size()==a.nColumns());
	bool flag;
	int i,its,j,jj,k,l = 0, nm = 0;
	DP anorm,c,f,g,h,s,scale,x,y,z;

	int m=a.nRows();
	int n=a.nColumns();
	Math::Vector<DP>  rv1(n);
	g=scale=anorm=0.0;
	for (i=0;i<n;i++) {
	    l=i+2;
	    rv1[i]=scale*g;
	    g=s=scale=0.0;
	    if (i < m) {
		for (k=i;k<m;k++) scale += fabs(a[k][i]);
		if (scale != 0.0) {
		    for (k=i;k<m;k++) {
			a[k][i] /= scale;
			s += a[k][i]*a[k][i];
		    }
		    f=a[i][i];
		    g = -SIGN(sqrt(s),f);
		    h=f*g-s;
		    a[i][i]=f-g;
		    for (j=l-1;j<n;j++) {
			for (s=0.0,k=i;k<m;k++) s += a[k][i]*a[k][j];
			f=s/h;
			for (k=i;k<m;k++) a[k][j] += f*a[k][i];
		    }
		    for (k=i;k<m;k++) a[k][i] *= scale;
		}
	    }
	    w[i]=scale *g;
	    g=s=scale=0.0;
	    if (i+1 <= m && i != n) {
		for (k=l-1;k<n;k++) scale += fabs(a[i][k]);
		if (scale != 0.0) {
		    for (k=l-1;k<n;k++) {
			a[i][k] /= scale;
			s += a[i][k]*a[i][k];
		    }
		    f=a[i][l-1];
		    g = -SIGN(sqrt(s),f);
		    h=f*g-s;
		    a[i][l-1]=f-g;
		    for (k=l-1;k<n;k++) rv1[k]=a[i][k]/h;
		    for (j=l-1;j<m;j++) {
			for (s=0.0,k=l-1;k<n;k++) s += a[j][k]*a[i][k];
			for (k=l-1;k<n;k++) a[j][k] += s*rv1[k];
		    }
		    for (k=l-1;k<n;k++) a[i][k] *= scale;
		}
	    }
	    anorm=std::max(anorm,(fabs(w[i])+fabs(rv1[i])));
	}
	for (i=n-1;i>=0;i--) {
	    if (i < n-1) {
		if (g != 0.0) {
		    for (j=l;j<n;j++)
			v[j][i]=(a[i][j]/a[i][l])/g;
		    for (j=l;j<n;j++) {
			for (s=0.0,k=l;k<n;k++) s += a[i][k]*v[k][j];
			for (k=l;k<n;k++) v[k][j] += s*v[k][i];
		    }
		}
		for (j=l;j<n;j++) v[i][j]=v[j][i]=0.0;
	    }
	    v[i][i]=1.0;
	    g=rv1[i];
	    l=i;
	}
	for (i=std::min(m,n)-1;i>=0;i--) {
	    l=i+1;
	    g=w[i];
	    for (j=l;j<n;j++) a[i][j]=0.0;
	    if (g != 0.0) {
		g=1.0/g;
		for (j=l;j<n;j++) {
		    for (s=0.0,k=l;k<m;k++) s += a[k][i]*a[k][j];
		    f=(s/a[i][i])*g;
		    for (k=i;k<m;k++) a[k][j] += f*a[k][i];
		}
		for (j=i;j<m;j++) a[j][i] *= g;
	    } else for (j=i;j<m;j++) a[j][i]=0.0;
	    ++a[i][i];
	}
	for (k=n-1;k>=0;k--) {
	    for (its=0;its<30;its++) {
		flag=true;
		for (l=k;l>=0;l--) {
		    nm=l-1;
		    if (fabs(rv1[l])+anorm == anorm) {
			flag=false;
			break;
		    }
		    if (fabs(w[nm])+anorm == anorm) break;
		}
		if (flag) {
		    c=0.0;
		    s=1.0;
		    for (i=l-1;i<k+1;i++) {
			f=s*rv1[i];
			rv1[i]=c*rv1[i];
			if (fabs(f)+anorm == anorm) break;
			g=w[i];
			h=pythag(f,g);
			w[i]=h;
			h=1.0/h;
			c=g*h;
			s = -f*h;
			for (j=0;j<m;j++) {
			    y=a[j][nm];
			    z=a[j][i];
			    a[j][nm]=y*c+z*s;
			    a[j][i]=z*c-y*s;
			}
		    }
		}
		z=w[k];
		if (l == k) {
		    if (z < 0.0) {
			w[k] = -z;
			for (j=0;j<n;j++) v[j][k] = -v[j][k];
		    }
		    break;
		}
		hope(its<29); // no convergence in 30 svdcmp iterations
		x=w[l];
		nm=k-1;
		y=w[nm];
		g=rv1[nm];
		h=rv1[k];
		f=((y-z)*(y+z)+(g-h)*(g+h))/(2.0*h*y);
		g=pythag(f,1.0);
		f=((x-z)*(x+z)+h*((y/(f+SIGN(g,f)))-h))/x;
		c=s=1.0;
		for (j=l;j<=nm;j++) {
		    i=j+1;
		    g=rv1[i];
		    y=w[i];
		    h=s*g;
		    g=c*g;
		    z=pythag(f,h);
		    rv1[j]=z;
		    c=f/z;
		    s=h/z;
		    f=x*c+g*s;
		    g=g*c-x*s;
		    h=y*s;
		    y *= c;
		    for (jj=0;jj<n;jj++) {
			x=v[jj][j];
			z=v[jj][i];
			v[jj][j]=x*c+z*s;
			v[jj][i]=z*c-x*s;
		    }
		    z=pythag(f,h);
		    w[j]=z;
		    if (z) {
			z=1.0/z;
			c=f*z;
			s=h*z;
		    }
		    f=c*g+s*y;
		    x=c*y-s*g;
		    for (jj=0;jj<m;jj++) {
			y=a[jj][j];
			z=a[jj][i];
			a[jj][j]=y*c+z*s;
			a[jj][i]=z*c-y*s;
		    }
		}
		rv1[l]=0.0;
		rv1[k]=f;
		w[k]=x;
	    }
	}
    }


    void svbksb(const CoreMatrix &u, const Math::Vector<DP>  &w, const CoreMatrix &v,
		const Math::Vector<DP> &b, Math::Vector<DP> &x)
    {

	require(u.nColumns()==v.nColumns());
	require(v.nColumns()==v.nRows());
	require(w.size()==u.nColumns());
	require(b.size()==u.nRows());
	require(x.size()==u.nColumns());


	int jj,j,i;
	DP s;

	int m=u.nRows();
	int n=u.nColumns();
	Math::Vector<DP> tmp(n);
	for (j=0;j<n;j++) {
	    s=0.0;
	    if (w[j] != 0.0) {
		for (i=0;i<m;i++) s += u[i][j]*b[i];
		s /= w[j];
	    }
	    tmp[j]=s;
	}
	for (j=0;j<n;j++) {
	    s=0.0;
	    for (jj=0;jj<n;jj++) s += v[j][jj]*tmp[jj];
	    x[j]=s;
	}
    }

} } //namespace Math::Nr

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

namespace Math { namespace Nr {
using namespace std;

DP trapzd(const FunctorBase<DP,DP>& func, const DP a, const DP b, const int n)
{
	DP x,tnm,sum,del;
	static DP s;
	int it,j;

	if (n == 1) {
		return (s=0.5*(b-a)*(func(a)+func(b)));
	} else {
		for (it=1,j=1;j<n-1;j++) it <<= 1;
		tnm=it;
		del=(b-a)/tnm;
		x=a+0.5*del;
		for (sum=0.0,j=0;j<it;j++,x+=del) sum += func(x);
		s=0.5*(s+(b-a)*sum/tnm);
		return s;
	}
}

void polint(Vec_I_DP &xa, Vec_I_DP &ya, const DP x, DP &y, DP &dy)
{
	int i,m,ns=0;
	DP den,dif,dift,ho,hp,w;

	int n=xa.size();
	Vec_DP c(n),d(n);
	dif=fabs(x-xa[0]);
	for (i=0;i<n;i++) {
		if ((dift=fabs(x-xa[i])) < dif) {
			ns=i;
			dif=dift;
		}
		c[i]=ya[i];
		d[i]=ya[i];
	}
	y=ya[ns--];
	for (m=1;m<n;m++) {
		for (i=0;i<n-m;i++) {
			ho=xa[i]-x;
			hp=xa[i+m]-x;
			w=c[i+1]-d[i];
			if ((den=ho-hp) == 0.0) nrerror("Error in routine polint");
			den=w/den;
			d[i]=hp*den;
			c[i]=ho*den;
		}
		y += (dy=(2*(ns+1) < (n-m) ? c[ns+1] : d[ns--]));
	}
}




DP midpnt(const Nr::FunctorBase<DP,DP>& func, const DP a, const DP b, const int n)
{
	int it,j;
	DP x,tnm,sum,del,ddel;
	static DP s;

	if (n == 1) {
		return (s=(b-a)*func(0.5*(a+b)));
	} else {
		for(it=1,j=1;j<n-1;j++) it *= 3;
		tnm=it;
		del=(b-a)/(3.0*tnm);
		ddel=del+del;
		x=a+0.5*del;
sum=0.0;
		for (j=0;j<it;j++) {
			sum += func(x);
			x += ddel;
			sum += func(x);
			x += del;
		}
		s=(s+(b-a)*sum/tnm)/3.0;
		return s;
	}
}


DP qtrap(const Nr::FunctorBase<DP,DP>& func, const DP a, const DP b)
{
	const int JMAX=20;
	const DP EPS=1.0e-8;
	int j;
	DP s,olds=0.0;

	for (j=0;j<JMAX;j++) {
		s=trapzd(func,a,b,j+1);
		//		cout <<" " << s << " "<< fabs(s-olds)<<" " << EPS*fabs(olds)<< endl;
		if (j > 5)
			if (fabs(s-olds) < EPS*fabs(olds) ||
				(s == 0.0 && olds == 0.0)) return s;
		olds=s;
	}
	nrerror("Too many steps in routine qtrap");
	return 0.0;
}


DP qsimp(const Nr::FunctorBase<DP,DP>& func, const DP a, const DP b)
{
	const int JMAX=30;
	const DP EPS=1.0e-10;
	const DP EPS2= 1.0e-10;
	int j;
	DP s,st,ost=0.0,os=0.0;

	for (j=0;j<JMAX;j++) {
	    st=trapzd(func,a,b,j+1);
		s=(4.0*st-ost)/3.0;
		if (j > 5)
			if (fabs(s-os) < EPS*fabs(os) ||
			    //(s == 0.0 && os == 0.0)) return s;
			    (fabs(s)< EPS2 && fabs(os) <EPS2 )) return s;
		os=s;
		ost=st;
	}
	nrerror("Too many steps in routine qsimp");
	return 0.0;
}




DP qromb(const Nr::FunctorBase<DP,DP>& func, DP a, DP b)
{
	const int JMAX=20, JMAXP=JMAX+1, K=15;
	const DP EPS=1.0e-8;
	DP ss,dss;
	Vec_DP s(JMAX),h(JMAXP),s_t(K),h_t(K);
	int i,j;

	h[0]=1.0;
	for (j=1;j<=JMAX;j++) {
		s[j-1]=trapzd(func,a,b,j);
		if (j >= K) {
			for (i=0;i<K;i++) {
				h_t[i]=h[j-K+i];
				s_t[i]=s[j-K+i];
			}
			polint(h_t,s_t,0.0,ss,dss);
			if (fabs(dss) <= EPS*fabs(ss)) return ss;
		}
		h[j]=0.25*h[j-1];
	}
	nrerror("Too many steps in routine qromb");
	return 0.0;
}


DP qromo(const Nr::FunctorBase<DP,DP>& func, const DP a, const DP b,
	 DP choose(const Nr::FunctorBase<DP,DP>&, const DP, const DP, const int))
{
	const int JMAX=20, JMAXP=JMAX+1, K=5;
	const DP EPS=3.0e-9;
	int i,j;
	DP ss,dss;
	Vec_DP h(JMAXP),s(JMAX),h_t(K),s_t(K);

	h[0]=1.0;
	for (j=1;j<=JMAX;j++) {
		s[j-1]=choose(func,a,b,j);
		if (j >= K) {
			for (i=0;i<K;i++) {
				h_t[i]=h[j-K+i];
				s_t[i]=s[j-K+i];
			}
			polint(h_t,s_t,0.0,ss,dss);
			if (fabs(dss) <= EPS*fabs(ss)) return ss;
		}
		h[j]=h[j-1]/9.0;
	}
	nrerror("Too many steps in routine qromo");
	return 0.0;
}




void rkck(Vec_I_DP &y, Vec_I_DP &dydx, const DP x,
	  const DP h, Vec_O_DP &yout, Vec_O_DP &yerr,
	  Nr::DerivativesBase<DP,Vec_I_DP,Vec_O_DP> &derivs)
{
	static const DP a2=0.2, a3=0.3, a4=0.6, a5=1.0, a6=0.875,
		b21=0.2, b31=3.0/40.0, b32=9.0/40.0, b41=0.3, b42 = -0.9,
		b43=1.2, b51 = -11.0/54.0, b52=2.5, b53 = -70.0/27.0,
		b54=35.0/27.0, b61=1631.0/55296.0, b62=175.0/512.0,
		b63=575.0/13824.0, b64=44275.0/110592.0, b65=253.0/4096.0,
		c1=37.0/378.0, c3=250.0/621.0, c4=125.0/594.0, c6=512.0/1771.0,
		dc1=c1-2825.0/27648.0, dc3=c3-18575.0/48384.0,
		dc4=c4-13525.0/55296.0, dc5 = -277.00/14336.0, dc6=c6-0.25;
	int i;

	int n=y.size();
	Vec_DP ak2(n),ak3(n),ak4(n),ak5(n),ak6(n),ytemp(n);
	for (i=0;i<n;i++)
		ytemp[i]=y[i]+b21*h*dydx[i];
	derivs(x+a2*h,ytemp,ak2);
	for (i=0;i<n;i++)
		ytemp[i]=y[i]+h*(b31*dydx[i]+b32*ak2[i]);
	derivs(x+a3*h,ytemp,ak3);
	for (i=0;i<n;i++)
		ytemp[i]=y[i]+h*(b41*dydx[i]+b42*ak2[i]+b43*ak3[i]);
	derivs(x+a4*h,ytemp,ak4);
	for (i=0;i<n;i++)
		ytemp[i]=y[i]+h*(b51*dydx[i]+b52*ak2[i]+b53*ak3[i]+b54*ak4[i]);
	derivs(x+a5*h,ytemp,ak5);
	for (i=0;i<n;i++)
		ytemp[i]=y[i]+h*(b61*dydx[i]+b62*ak2[i]+b63*ak3[i]+b64*ak4[i]+b65*ak5[i]);
	derivs(x+a6*h,ytemp,ak6);
	for (i=0;i<n;i++)
		yout[i]=y[i]+h*(c1*dydx[i]+c3*ak3[i]+c4*ak4[i]+c6*ak6[i]);
	for (i=0;i<n;i++)
		yerr[i]=h*(dc1*dydx[i]+dc3*ak3[i]+dc4*ak4[i]+dc5*ak5[i]+dc6*ak6[i]);
}


void rkqs(Vec_IO_DP &y, Vec_IO_DP &dydx, DP &x, const DP htry,
	  const DP eps, Vec_I_DP &yscal, DP &hdid, DP &hnext,
	  Nr::DerivativesBase<DP,Vec_I_DP,Vec_O_DP> &derivs)
{
	const DP SAFETY=0.9, PGROW=-0.2, PSHRNK=-0.25, ERRCON=1.89e-4;
	int i;
	DP errmax,h,htemp,xnew;

	int n=y.size();
	h=htry;
	Vec_DP yerr(n),ytemp(n);
	for (;;) {
		rkck(y,dydx,x,h,ytemp,yerr,derivs);
		errmax=0.0;
		for (i=0;i<n;i++) errmax=std::max(errmax,fabs(yerr[i]/yscal[i]));
		errmax /= eps;
		if (errmax <= 1.0) break;
		htemp=SAFETY*h*pow(errmax,PSHRNK);
		h=(h >= 0.0 ? std::max(htemp,0.1*h) : std::min(htemp,0.1*h));
		xnew=x+h;
		if (xnew == x) nrerror("stepsize underflow in rkqs");
	}
	if (errmax > ERRCON) hnext=SAFETY*h*pow(errmax,PGROW);
	else hnext=5.0*h;
	x += (hdid=h);
	for (i=0;i<n;i++) y[i]=ytemp[i];
}




} } //namespace Math::Nr

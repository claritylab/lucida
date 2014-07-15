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
//#warning change when ported to gcc-3
#include <limits>
#include <algorithm>
#include <cmath>
#include "ConjugateGradient.hh"

using namespace std;
using namespace Math::Nr;

void Math::Nr::frprmn(Math::Vector<double> &p, const double ftol, int &iter, double &fret,
		      const FunctorBase<Math::Vector<double>,double>& func,
		      const GradientBase<Math::Vector<double>, Math::Vector<double> >& dfunc)
{
    const int ITMAX=100000;
    const double EPS=1.0e-18;
    int j,its;
    double gg,gam,fp,dgg;

    int n=p.size();
    Math::Vector<double> g(n),h(n),xi(n);
    fp=func(p);
    dfunc(p,xi);
    for (j=0;j<n;j++) {
	g[j] = -xi[j];
	xi[j]=h[j]=g[j];
    }
    for (its=0;its<ITMAX;its++) {
	iter=its;
	dlinmin(p,xi,fret,func,dfunc);
	if (2.0*fabs(fret-fp) <= ftol*(fabs(fret)+fabs(fp)+EPS))
	    return;
	fp=fret;
	dfunc(p,xi);
	dgg=gg=0.0;
	for (j=0;j<n;j++) {
	    gg += g[j]*g[j];
	    //		  dgg += xi[j]*xi[j];
	    dgg += (xi[j]+g[j])*xi[j];
	}
	if (gg == 0.0)
	    return;
	gam=dgg/gg;
	for (j=0;j<n;j++) {
	    g[j] = -xi[j];
	    xi[j]=h[j]=g[j]+gam*h[j];
	}
    }
    nrerror("Too many iterations in frprmn");
}


void Math::Nr::dlinmin(Math::Vector<double> &p, Math::Vector<double> &xi, double &fret,
		       const FunctorBase<Math::Vector<double>,double>& func,
		       const GradientBase<Math::Vector<double>, Math::Vector<double> >& dfunc)
{
    const double TOL=2.0e-8;
    int j;
    double xx,xmin,fx,fb,fa,bx,ax;

    int n=p.size();
    ax=0.0;
    xx=1.0;
    mnbrak(ax,xx,bx,fa,fx,fb,f1dim(n,func,p,xi));
    fret=dbrent(ax,xx,bx,f1dim(n,func,p,xi),df1dim(n,func,dfunc,p,xi),TOL,xmin);
    for (j=0;j<n;j++) {
	xi[j] *= xmin;
	p[j] += xi[j];
    }
}



f1dim::f1dim(int ncom, const FunctorBase<Math::Vector<double>,double>& nrfunc,const Math::Vector<double> & pcom,
	     const Math::Vector<double> & xicom):ncom_(ncom),nrfunc_(nrfunc),pcom_(pcom),xicom_(xicom){};

double f1dim::operator()(const double& x) const
{
    int j;

    Math::Vector<double> xt(ncom_);
    for (j=0;j<ncom_;j++)
	xt[j]=pcom_[j]+x*xicom_[j];
    return nrfunc_(xt);
}


df1dim::df1dim(int ncom,
	       const FunctorBase<Math::Vector<double>,double>& nrfunc,
	       const GradientBase<Math::Vector<double>, Math::Vector<double> >& nrdfun,
	       const Math::Vector<double> & pcom,
	       const Math::Vector<double> & xicom)
    :ncom_(ncom),nrfunc_(nrfunc),nrdfun_(nrdfun) ,pcom_(pcom),xicom_(xicom){};


double df1dim::operator()(const double& x) const
{
    int j;
    double df1=0.0;
    Math::Vector<double> xt(ncom_),df(ncom_);
    for (j=0;j<ncom_;j++) xt[j]=pcom_[j]+x*xicom_[j];
    nrdfun_(xt,df);
    for (j=0;j<ncom_;j++) df1 += df[j]*xicom_[j];
    return df1;
}

namespace {
    inline void mov3(double &a, double &b, double &c, const double d, const double e,
		     const double f)
    {
	a=d; b=e; c=f;
    }
}

double Math::Nr::dbrent(const double ax, const double bx, const double cx,
			const FunctorBase<double,double>& f,
			const FunctorBase<double,double>& df,
			const double tol, double &xmin)
{
    const int ITMAX=5000;

    // const double ZEPS=2.220446049e-16 * 1.0e-3;
    // legacy warning: "change when ported to gcc-3"
    const double ZEPS=numeric_limits<double>::epsilon()*1.0e-3;

    bool ok1,ok2;
    int iter;
    double a,b,d=0.0,d1,d2,du,dv,dw,dx,e=0.0;
    double fu,fv,fw,fx,olde,tol1,tol2,u,u1,u2,v,w,x,xm;

    a=(ax < cx ? ax : cx);
    b=(ax > cx ? ax : cx);
    x=w=v=bx;
    fw=fv=fx=f(x);
    dw=dv=dx=df(x);
    for (iter=0;iter<ITMAX;iter++) {
	xm=0.5*(a+b);
	tol1=tol*fabs(x)+ZEPS;
	tol2=2.0*tol1;
	if (fabs(x-xm) <= (tol2-0.5*(b-a))) {
	    xmin=x;
	    return fx;
	}
	if (fabs(e) > tol1) {
	    d1=2.0*(b-a);
	    d2=d1;
	    if (dw != dx) d1=(w-x)*dx/(dx-dw);
	    if (dv != dx) d2=(v-x)*dx/(dx-dv);
	    u1=x+d1;
	    u2=x+d2;
	    ok1 = (a-u1)*(u1-b) > 0.0 && dx*d1 <= 0.0;
	    ok2 = (a-u2)*(u2-b) > 0.0 && dx*d2 <= 0.0;
	    olde=e;
	    e=d;
	    if (ok1 || ok2) {
		if (ok1 && ok2)
		    d=(fabs(d1) < fabs(d2) ? d1 : d2);
		else if (ok1)
		    d=d1;
		else
		    d=d2;
		if (fabs(d) <= fabs(0.5*olde)) {
		    u=x+d;
		    if (u-a < tol2 || b-u < tol2)
			d=SIGN(tol1,xm-x);
		} else {
		    d=0.5*(e=(dx >= 0.0 ? a-x : b-x));
		}
	    } else {
		d=0.5*(e=(dx >= 0.0 ? a-x : b-x));
	    }
	} else {
	    d=0.5*(e=(dx >= 0.0 ? a-x : b-x));
	}
	if (fabs(d) >= tol1) {
	    u=x+d;
	    fu=f(u);
	} else {
	    u=x+SIGN(tol1,d);
	    fu=f(u);
	    if (fu > fx) {
		xmin=x;
		return fx;
	    }
	}
	du=df(u);
	if (fu <= fx) {
	    if (u >= x) a=x; else b=x;
	    mov3(v,fv,dv,w,fw,dw);
	    mov3(w,fw,dw,x,fx,dx);
	    mov3(x,fx,dx,u,fu,du);
	} else {
	    if (u < x) a=u; else b=u;
	    if (fu <= fw || w == x) {
		mov3(v,fv,dv,w,fw,dw);
		mov3(w,fw,dw,u,fu,du);
	    } else if (fu < fv || v == x || v == w) {
		mov3(v,fv,dv,u,fu,du);
	    }
	}
    }
    nrerror("Too many iterations in routine dbrent");
    return 0.0;
}



namespace {
    inline void shft3(double &a, double &b, double &c, const double d)
    {
	a=b;
	b=c;
	c=d;
    }
}

void Math::Nr::mnbrak(double &ax, double &bx, double &cx, double &fa, double &fb, double &fc,
		      const FunctorBase<double,double>& func)
{
    const double GOLD=1.618034,GLIMIT=100.0,TINY=1.0e-20;
    double ulim,u,r,q,fu;

    fa=func(ax);
    fb=func(bx);
    if (fb > fa) {
	SWAP(ax,bx);
	SWAP(fb,fa);
    }
    cx=bx+GOLD*(bx-ax);
    fc=func(cx);
    while (fb > fc) {
	r=(bx-ax)*(fb-fc);
	q=(bx-cx)*(fb-fa);
	u=bx-((bx-cx)*q-(bx-ax)*r)/
	    (2.0*SIGN(std::max(fabs(q-r),TINY),q-r));
	ulim=bx+GLIMIT*(cx-bx);
	if ((bx-u)*(u-cx) > 0.0) {
	    fu=func(u);
	    if (fu < fc) {
		ax=bx;
		bx=u;
		fa=fb;
		fb=fu;
		return;
	    } else if (fu > fb) {
		cx=u;
		fc=fu;
		return;
	    }
	    u=cx+GOLD*(cx-bx);
	    fu=func(u);
	} else if ((cx-u)*(u-ulim) > 0.0) {
	    fu=func(u);
	    if (fu < fc) {
		shft3(bx,cx,u,cx+GOLD*(cx-bx));
		shft3(fb,fc,fu,func(u));
	    }
	} else if ((u-ulim)*(ulim-cx) >= 0.0) {
	    u=ulim;
	    fu=func(u);
	} else {
	    u=cx+GOLD*(cx-bx);
	    fu=func(u);
	}
	shft3(ax,bx,cx,u);
	shft3(fa,fb,fc,fu);
    }
}

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
#ifndef _MATH_NR_CONJUGATE_GRADIENT_HH
#define _MATH_NR_CONJUGATE_GRADIENT_HH

#include <Math/Vector.hh>
#include "nr.h"

namespace Math { namespace Nr {

    class f1dim : public FunctorBase<double,double>{
    private:
	int ncom_;
	const FunctorBase<Math::Vector<double>,double>& nrfunc_;
	const Math::Vector<double> & pcom_;
	const Math::Vector<double> & xicom_;

    public:
	double operator()(const double& x)const;
	f1dim(int ncom,const FunctorBase<Math::Vector<double>,double>& nrfunc,const Math::Vector<double> & pcom,
	      const Math::Vector<double> & xicom );
    };


    class df1dim : public FunctorBase<double,double>{
    private:
	int ncom_;
	const FunctorBase<Math::Vector<double>,double>& nrfunc_;
	const GradientBase<Math::Vector<double>, Math::Vector<double> >&nrdfun_;
	const Math::Vector<double> & pcom_;
	const Math::Vector<double> & xicom_;

    public:
	double operator()(const double& x)const;
	df1dim(int ncom,const FunctorBase<Math::Vector<double>,double>& nrfunc,
	      const GradientBase<Math::Vector<double>, Math::Vector<double> >&nrdfun,
	      const Math::Vector<double> & pcom,
	      const Math::Vector<double> & xicom );
    };


    void frprmn(Math::Vector<double> &p, const double ftol, int &iter, double &fret,
		const FunctorBase<Math::Vector<double>,double>& func,
		const GradientBase<Math::Vector<double>, Math::Vector<double> >& dfunc);

    void linmin(Math::Vector<double> &p, Math::Vector<double> &xi, double &fret,
		const FunctorBase<Math::Vector<double>,double>& func );

    void dlinmin(Math::Vector<double> &p, Math::Vector<double> &xi, double &fret,
		 const FunctorBase<Math::Vector<double>,double>& func,
		 const GradientBase<Math::Vector<double>, Math::Vector<double> >& dfunc);

    double dbrent(const double ax, const double bx, const double cx,
	      const FunctorBase<double,double>& f,
	      const FunctorBase<double,double>& df,
	      const double tol, double &xmin);

    void mnbrak(double &ax, double &bx, double &cx, double &fa, double &fb, double &fc,
		    const FunctorBase<double,double>& func);

} } //namespace Math::Nr

#endif // _MATH_NR_CONJUGATE_GRADIENT_HH

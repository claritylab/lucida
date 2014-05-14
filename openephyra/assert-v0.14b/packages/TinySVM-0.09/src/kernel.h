/*
 TinySVM -- Yet Another Tiny SVM Package

 $Id: kernel.h,v 1.18 2002/08/20 06:31:17 taku-ku Exp $;

 Copyright (C) 2001-2002  Taku Kudo <taku-ku@is.aist-nara.ac.jp>
 All rights reserved.

 This library is free software; you can redistribute it and/or
 modify it under the terms of the GNU Library General Public
 License as published by the Free Software Foundation; either
 version 2 of the License, or (at your option) any later verjsion.

 This library is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 Library General Public License for more details.

 You should have received a copy of the GNU Library General Public
 License along with this library; if not, write to the
 Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 Boston, MA 02111-1307, USA.
*/


#ifndef _KERNEL_H
#define _KERNEL_H
#include "misc.h"
#include "param.h"
#include "base_example.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>

// $Id: kernel.h,v 1.18 2002/08/20 06:31:17 taku-ku Exp $;
namespace TinySVM {

class Kernel
{
protected:
  // constant param
  const int    l;
  const int    d;
  const int    pack_d;
  const int    kernel_type;
  const int    feature_type;
  const int    dot_kernel;
  const int    degree;
  const double param_g;
  const double param_r;
  const double param_s;

  double  (Kernel::*_getKernel)(const feature_node *, const feature_node *) const;
  double  (Kernel::*_getKernel2)(const double) const;

  // linear
  inline double _getKernel_linear(const feature_node *x1, const feature_node *x2) const
  { 
    return dot_normal(x1,x2);
  }

  inline double _getKernel_linear2(const double _x) const
  { 
    return _x; 
  }

  // polynomial
  inline double _getKernel_poly(const feature_node *x1, const feature_node *x2) const 
  { 
    return pow (param_s * dot_normal(x1,x2) + param_r, degree); 
  }

  inline double _getKernel_poly2(const double _x) const 
  { 
    return pow (param_s * _x + param_r, degree); 
  }

  // neural
  inline double _getKernel_neural(const feature_node *x1, const feature_node *x2) const 
  { 
    return tanh (param_s * dot_normal(x1,x2) + param_r); 
  }

  inline double _getKernel_neural2(const double _x) const
  { 
    return tanh (param_s * _x + param_r); 
  }

  // RBF
  inline double _getKernel_rbf(const feature_node *x1, const feature_node *x2) const 
  { 
    return exp (-param_s * norm2(x1,x2));
  }

  inline double _getKernel_rbf2(const double _x) const
  { 
    fprintf (stderr, "Kernel::getKernel_rbf() cannot obtain kernel value only with dot.\n");
    exit (-1);
    return 0.0;
  }

  // ANOVA
  inline double _getKernel_anova(const feature_node *x1, const feature_node *x2) const 
  { 
    register double sum = 0;
    register int zero = d;

    while (x1->index >= 0 && x2->index >= 0) {
      if (x1->index == x2->index) {
	sum += exp (-param_s * (x1->value - x2->value) * (x1->value - x2->value));
	++x1; ++x2;
      } else if (x1->index < x2->index) {
	sum += exp (-param_s * x1->value * x1->value);
	++x1;
      } else {
	sum += exp (-param_s * x2->value * x2->value);
	++x2;
      }
      zero--;
    }

    return pow (sum + (double)zero, degree);
  }

  inline double _getKernel_anova2(const double _x) const
  { 
    fprintf (stderr, "Kernel::getKernel_anova() cannot obtain kernel value only with dot.\n");
    exit (-1);
    return 0.0;
  }

  inline double norm2(const feature_node *x1, const feature_node *x2) const
  {
    register double sum = 0;
     
    while (x1->index >= 0 && x2->index >= 0) {
      if(x1->index == x2->index) {
	sum += (x1->value - x2->value) * (x1->value - x2->value);
	++x1; ++x2;
      } else if (x1->index < x2->index) {
	sum += (x1->value * x1->value);
	++x1;
      } else {
	sum += (x2->value * x2->value);
	++x2;
      }
    }

    while (x1->index >= 0) {
      sum += (x1->value * x1->value);
      ++x1;
    };

    while (x2->index >= 0) {
      sum += (x2->value * x2->value);
      ++x2;
    };
     
    return sum;
  }

  inline double dot_normal(const feature_node *x1, const feature_node *x2) const
  {
    register double sum = 0;
    while (x1->index >= 0 && x2->index >= 0) {
      if (x1->index == x2->index) {
	sum += (x1->value * x2->value);
	++x1; ++x2;
      } else if (x1->index < x2->index) {
	++x1;
      }	else {
	++x2;
      }			
    }
    return sum;
  }

  inline int dot_binary(const feature_node *x1, const feature_node *x2) const
  {
    register int sum = 0;
    while (x1->index >= 0 && x2->index >= 0) {
      if (x1->index == x2->index) {
	sum++; 
	++x1; ++x2;
      } else if (x1->index < x2->index) {
	++x1;
      } else {
	++x2;
      }			
    }
    return sum;
  }

public:
  feature_node ** x;
  double         *y;

  Kernel(const BaseExample& example, const Param& param):
    l(example.l), 
    d(example.d), 
    pack_d(example.pack_d), 
    kernel_type(param.kernel_type),
    feature_type(example.feature_type),
    dot_kernel(param.dot_kernel),
    degree(param.degree),
    param_g(param.param_g),
    param_r(param.param_r),
    param_s(param.param_s)
  {
    // default
    switch (kernel_type) {
    case LINEAR:
      _getKernel  = &Kernel::_getKernel_linear;
      _getKernel2 = &Kernel::_getKernel_linear2;
      break;
    case POLY:
      _getKernel  = &Kernel::_getKernel_poly;
      _getKernel2 = &Kernel::_getKernel_poly2;
      break;
    case NEURAL:
      _getKernel  = &Kernel::_getKernel_neural;
      _getKernel2 = &Kernel::_getKernel_neural2;
      break;
    case RBF:
      _getKernel  = &Kernel::_getKernel_rbf;
      _getKernel2 = &Kernel::_getKernel_rbf2;
      break;
    case ANOVA:
      _getKernel  = &Kernel::_getKernel_anova;
      _getKernel2 = &Kernel::_getKernel_anova2;
      break;
    default:
      fprintf(stderr,"Kernel::Kernel: Unknown kernel function\n");
    }
  }

  // wrapper for getKernel
  inline double getKernel(const feature_node *x1, const feature_node *x2)
  {
    return (this->*_getKernel)(x1, x2);
  }

  inline double getKernel(const double _x)
  {
    return (this->*_getKernel2)(_x);
  }
};
};
#endif

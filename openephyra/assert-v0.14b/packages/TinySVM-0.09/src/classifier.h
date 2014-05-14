/*
 TinySVM -- Yet Another Tiny SVM Package

 $Id: classifier.h,v 1.8 2002/08/20 06:31:16 taku-ku Exp $;

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


#ifndef _CLASSIFIER_H
#define _CLASSIFIER_H
#include "kernel.h"

// $Id: classifier.h,v 1.8 2002/08/20 06:31:16 taku-ku Exp $;
namespace TinySVM {

class Classifier: public Kernel
{
 private:
  double model_bias;
  int    *dot_buf;
  double **binary_kernel_cache;
  int    **fi2si;

  double  (Classifier::*_getDistance) (const feature_node *) const;
  double  _getDistance_binary (const feature_node *) const;
  double  _getDistance_normal (const feature_node *) const;

 public:
  Classifier (const BaseExample &, const Param &);
  ~Classifier ();

  inline double getDistance (const char *s)
  {
    feature_node *node = str2feature_node (s);
    double d = getDistance(node);
    delete [] node;
    return d;
  }
  
  inline double getDistance (const feature_node *_x) 
  {
    return (this->*_getDistance)(_x);
  }
};
};
#endif


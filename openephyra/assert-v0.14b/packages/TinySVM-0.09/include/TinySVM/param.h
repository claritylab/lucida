/*
 TinySVM -- Yet Another Tiny SVM Package

 $Id: param.h,v 1.29 2002/08/20 06:31:17 taku-ku Exp $;

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


#ifndef _PARAM_H
#define _PARAM_H

namespace TinySVM {

class Param
{
public:
  int    kernel_type;
  int    feature_type;
  int    solver_type;
  int    dot_kernel;
  int    degree;
  double param_g;
  double param_r;
  double param_s;
  double cache_size; 
  double C;
  double eps;
  int    verbose;
  int    shrink_size;
  double shrink_eps;
  int    final_check;
  int    svindex;
  double insensitive_loss;
  int    compress;
  char   model[512];

  int set (int,  char **);
  int set (const char *);

  Param();
  ~Param();
};


};
#endif


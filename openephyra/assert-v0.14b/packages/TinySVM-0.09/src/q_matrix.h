/*
 TinySVM -- Yet Another Tiny SVM Package

 $Id: q_matrix.h,v 1.7 2002/08/20 06:31:17 taku-ku Exp $;

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


#ifndef _Q_MATRIX_H
#define _Q_MATRIX_H
#include "kernel.h"
#include "cache.h"

// $Id: q_matrix.h,v 1.7 2002/08/20 06:31:17 taku-ku Exp $;
namespace TinySVM {

class QMatrix: public Kernel
{
 private:
  double *buf;
  int    *binary_kernel_cache;
  double *(QMatrix::*_getQ)(const int, const int);

  double *_getQ_normal        (const int, const int);
  double *_getQ_binary_char   (const int, const int);
  double *_getQ_binary_double (const int, const int);

  Cache <double>        *cache_normal;
  Cache <unsigned char> *cache_binary;

 public:
  int size;
  int hit;
  int miss;
  double cache_size;

  QMatrix(const BaseExample &, const Param &);
  ~QMatrix();

  // misc function
  void update (const int);
  void delete_index (const int);
  void swap_index (const int, const int);
  void rebuildCache(int);
  void set (double *_y, feature_node **_x) { y = _y; x = _x; };

  // main
  inline double *getQ(const int i, const int active_size)
  {
    return (this->*_getQ)(i, active_size);
  }
};


};
#endif


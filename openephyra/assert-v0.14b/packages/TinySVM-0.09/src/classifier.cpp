/*
 TinySVM -- Yet Another Tiny SVM Package

 $Id: classifier.cpp,v 1.4 2002/08/20 06:31:16 taku-ku Exp $;

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


#include "common.h"
#include "classifier.h"

namespace TinySVM {

Classifier::Classifier (const BaseExample & example, const Param & param):
  Kernel (example, param)
{
  try {
     x = example.x;
     y = example.y;
     model_bias = 0;

    if (feature_type == BINARY_FEATURE && dot_kernel) {
      _getDistance = &Classifier::_getDistance_binary;

      dot_buf = new int[l];
      binary_kernel_cache = new double *[l];
      fi2si = new int *[d + 1];
      int *_fisize = new int[d + 1];

      // init
      for (int i = 0; i < (d + 1); i++)
	_fisize[i] = 0;

      // 1st, detemin the size of each array
      for (int i = 0; i < l; i++) {
	feature_node *node = (feature_node *) x[i];
	int feature_num = 0;
	while (node->index >= 0) {
	  _fisize[node->index]++;
	  node++;
	  feature_num++;
	}

	// caluculate cache
	model_bias += y[i] * (this->*_getKernel2) (0.0);
	binary_kernel_cache[i] = new double[feature_num + 1];
	for (int j = 1; j <= feature_num; j++) {
	  binary_kernel_cache[i][j] =
	    y[i] * ((this->*_getKernel2) ((double) j) -
		    (this->*_getKernel2) ((double) (j - 1)));
	}
      }

      // 2nd memory allocate for each array
      for (int i = 0; i < (d + 1); i++) {
	fi2si[i] = new int[_fisize[i] + 1];
	_fisize[i] = 0;
      }

      // 3rd, copy value
      for (int i = 0; i < l; i++) {
	feature_node *node = (feature_node *) x[i];
	while (node->index >= 0) {
	  fi2si[node->index][_fisize[node->index]++] = i;
	  node++;
	}
      }

      // final fill -1 (dumy field)
      for (int i = 0; i < (d + 1); i++)
	fi2si[i][_fisize[i]++] = -1;

      delete[]_fisize;

    } else {
      _getDistance = &Classifier::_getDistance_normal;
    }
  }

  catch (...) {
    fprintf (stderr, "Classifier::Classifier(): Out of memory\n");
    exit (EXIT_FAILURE);
  }
}

Classifier::~Classifier ()
{
  if (feature_type == BINARY_FEATURE && dot_kernel) {
    delete [] dot_buf;
    for (int i = 0; i < l; i++)
      delete [] binary_kernel_cache[i];
    delete [] binary_kernel_cache;

    for (int i = 0; i < (d + 1); i++)
      delete [] fi2si[i];
    delete [] fi2si;
  }
}

double
Classifier::_getDistance_normal (const feature_node * _x) const
{
  register double result = 0.0;
  register feature_node *node = fix_feature_node ((feature_node *) _x);

  for (register int i = 0; i < l; i++)
    result += y[i] * (this->*_getKernel) (x[i], node);
  
  return result;
}

double
Classifier::_getDistance_binary (const feature_node * node) const
{
  register double result = model_bias;
  register int j, k;
  register int *p;

  memset (dot_buf, 0, sizeof (int) * l);

  feature_node *org = (feature_node *) node;
  while (node->index >= 0 && node->index <= d) {
    if (node->value != 1)
      return this->_getDistance_normal (org);
    for (p = fi2si[node->index], j = 0; (k = p[j]) != -1; j++)
      result += (binary_kernel_cache[k][++dot_buf[k]]);
    node++;
  }

  return result;
}

};

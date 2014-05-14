/*
 TinySVM -- Yet Another Tiny SVM Package

 $Id: model.h,v 1.25 2002/08/20 06:31:17 taku-ku Exp $;

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


#ifndef _MODEL_H
#define _MODEL_H
#include "base_example.h"
#include "param.h"
#include "kernel.h"
#include "classifier.h"

// $Id: model.h,v 1.25 2002/08/20 06:31:17 taku-ku Exp $;

namespace TinySVM {

class Model: public BaseExample
{
 private:
  double margin;
  double vc;
  Param  param;
  Classifier *kernel;

 public:
  double b;
  double sphere;
  double loss;
  int    bsv;
   
  int read         (const char *,  const char *mode = "r", const int offset = 0);
  int write        (const char *,  const char *mode = "w", const int offset = 0);
  int clear();

  // classify
  double classify (const char *);
  double classify (const feature_node *);

  // model information
  double estimateMargin ();
  double estimateSphere ();
  double estimateVC ();
  double estimateXA (const double rho = 2.0);

  int compress (); // compress liner model

  int    getSVnum ()            { return this->l; };
  int    getBSVnum ()           { return this->bsv; };
  int    getTrainingDataSize () { return this->svindex_size; };
  double getLoss ()             { return this->loss; };

  Model();
  Model(const Param &);
  ~Model();

  Model& operator=(Model &m);
};

};
#endif


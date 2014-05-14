/*
 TinySVM -- Yet Another Tiny SVM Package

 $Id: oneclass_solver.cpp,v 1.4 2002/08/20 06:31:17 taku-ku Exp $;

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


#include "oneclass_solver.h"
#include "common.h"
#include "example.h"
#include "classifier.h"
#include "timer.h"
#include "qp_solver.h"

// $Id: oneclass_solver.cpp,v 1.4 2002/08/20 06:31:17 taku-ku Exp $;

namespace TinySVM {

Model *
OneClass_Solver::learn ()
{
  try {
    double obj, rho;
    const double *y        = (const double *)example.y;
    const feature_node **x = (const feature_node **)example.x;
    double *alpha  = new double [l];
    double *G      = new double [l];
    double *b      = new double [l];
    Kernel *kernel = new Kernel (example, param); // make temp kernel

    if (param.C < 1.0/l) {
      fprintf (stderr, "OneClass::learn (): C is too small, must be larger than 1/l\n");
      return 0;
    }

    for (int i = 0; i < l; i++) {
      b[i] = 0;
      alpha[i] = 0;
      G[i] = kernel->getKernel (x[0], x[i]);
      if (example.y[i] != +1) {
	fprintf (stderr, "OneClass::learn (): Only positive example is allowed for OneClass\n");
	return 0;
      }
    }
    delete kernel;
    alpha[0] = 1; // first item is 1 and others is 0

    QP_Solver qp_solver;
    qp_solver.solve (example, param, b, alpha, G, rho, obj);

    // make output model
    Model *out_model = new Model (param);
    out_model->b = -rho;

    // copy gradient and alphas
    _clone (out_model->alpha, alpha, l);
    _clone (out_model->G,     G,     l);

    double loss = 0.0;
    int bsv = 0;
    for (int i = 0; i < l; i++) {
      double d = G[i] + rho;
      if (d < -param.eps) loss -= d;
      if (alpha[i] >= param.C - EPS_A) bsv++; // upper bound
      if (alpha[i] >= EPS_A)  // free 
	out_model->add (alpha[i] * y[i], (feature_node *)x[i]);

    }

    out_model->bsv  = bsv;
    out_model->loss = loss;
    out_model->svindex_size = example.l;

    delete [] alpha;
    delete [] G;
    delete [] b;

    fprintf (stdout, "Number of SVs (BSVs)\t\t%d (%d)\n", out_model->l, out_model->bsv);
    fprintf (stdout, "L1 Loss:\t\t\t%g\n", loss);
    fprintf (stdout, "Object value:\t\t\t%g\n", obj);

    return out_model;
  }

  catch (...) {
    fprintf (stderr, "SVM_Solver::learn(): Out of memory\n");
    exit (EXIT_FAILURE);
    return 0; 
  }
}
}

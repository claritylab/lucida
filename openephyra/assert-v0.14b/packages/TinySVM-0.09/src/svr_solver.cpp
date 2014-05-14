/*
 TinySVM -- Yet Another Tiny SVM Package

 $Id: svr_solver.cpp,v 1.4 2002/08/20 06:31:17 taku-ku Exp $;

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


#include "svr_solver.h"
#include "common.h"
#include "example.h"
#include "classifier.h"
#include "qp_solver.h"

// $Id: svr_solver.cpp,v 1.4 2002/08/20 06:31:17 taku-ku Exp $;

namespace TinySVM {

Model *
SVR_Solver::learn ()
{
  try {
    double obj, rho;
    const double *y        = (const double *)example.y;
    const feature_node **x = (const feature_node **)example.x;

    double *alpha = new double [2 * l];
    double *G     = new double [2 * l];
    double *b     = new double [2 * l];

    for (int i = 0; i < l; i++) {
      G[i]   = b[i]   = param.insensitive_loss - y[i];
      G[i+l] = b[i+l] = param.insensitive_loss + y[i];
      alpha[i] = 0;
      alpha[i+l] = 0;
    }

    Example tmp_example;
    for (int i = 0; i < l; i++) tmp_example.add(1,  (feature_node *)x[i]);
    for (int i = 0; i < l; i++) tmp_example.add(-1, (feature_node *)x[i]);

    QP_Solver qp_solver;
    qp_solver.solve(tmp_example, param, b, alpha, G, rho, obj);

    // make output model
    Model *out_model = new Model (param);
    out_model->b = -rho;
    _clone (out_model->alpha, alpha, 2 * l);
    _clone (out_model->G,     G,     2 * l);

    double loss = 0.0;
    int bsv = 0;
    int err = 0;
    for (int i = 0; i < l; i++) {
      double d = (G[i] - G[i+l] - b[i] + b[i+l])/2 + rho;
      double a = alpha[i] - alpha[i+l];
      double l = _max(0.0, fabs(y[i] - d) - param.insensitive_loss); 
      loss += l;
      if (l > 0) err++;
      if (fabs(a) >= param.C - EPS_A) bsv++; // upper bound
      if (fabs(a) > EPS_A)  // free 
	out_model->add (a, (feature_node *)x[i]);
    }

    out_model->bsv =  bsv;
    out_model->loss = loss;
    out_model->svindex_size = 2 * example.l;

    delete [] alpha;
    delete [] G;
    delete [] b;

    fprintf (stdout, "Number of SVs (BSVs)\t\t%d (%d)\n", out_model->l, out_model->bsv);
    fprintf (stdout, "Empirical Risk:\t\t\t%g (%d/%d)\n", 1.0 * err/l, err,l);
    fprintf (stdout, "L1 Loss:\t\t\t%g\n", loss);
    fprintf (stdout, "Object value:\t\t\t%g\n", obj);

    return out_model;
  }

  catch (...) {
    fprintf (stderr, "SVR_Solver::learn(): Out of memory\n");
    exit (EXIT_FAILURE);
    return 0; 
  }
}
}

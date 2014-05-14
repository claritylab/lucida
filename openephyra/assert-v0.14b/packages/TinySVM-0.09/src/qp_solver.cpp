/*
 TinySVM -- Yet Another Tiny SVM Package

 $Id: qp_solver.cpp,v 1.4 2002/08/20 06:31:17 taku-ku Exp $;

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


#include "qp_solver.h"
#include "common.h"
#include "example.h"
#include "classifier.h"

// $Id: qp_solver.cpp,v 1.4 2002/08/20 06:31:17 taku-ku Exp $;

namespace TinySVM {

// swap i and j
inline void
QP_Solver::swap_index(const int i, const int j)
{
  _swap (y[i],            y[j]);
  _swap (x[i],            x[j]);
  _swap (alpha[i],        alpha[j]);
  _swap (status[i],       status[j]);
  _swap (G[i],            G[j]);
  _swap (b[i],            b[j]);
  _swap (shrink_iter[i],  shrink_iter[j]);
  _swap (active2index[i], active2index[j]);
}

int
QP_Solver::solve(const BaseExample &e,
		 const Param &p,
		 double *b_, double *alpha_, double *G_,
		 double &rho, double &obj)
{
  try {
    param        = p;
    C            = p.C;
    eps          = p.eps;
    shrink_size  = p.shrink_size;
    shrink_eps   = p.shrink_eps;
    final_check  = p.final_check;
    l            = e.l;
    active_size  = l;
    iter         = 0;
    hit_old      = 0;

    _clone (alpha, alpha_, l);
    _clone (G, G_, l);
    _clone (b, b_, l);
    _clone (y, e.y, l);
    _clone (x, e.x, l);

    q_matrix = new QMatrix (e, p);
    q_matrix->set (y, x);     

    shrink_iter  = new int [l];
    status       = new int [l];
    active2index = new int [l];

    for (int i = 0; i < l; i++) {
      status[i] = alpha2status(alpha[i]);
      shrink_iter[i]  = 0;
      active2index[i] = i;
    }

    for (;;) {
      learn_sub();
      if (final_check == 0 || check_inactive () == 0)  break; 
      q_matrix->rebuildCache (active_size);
      q_matrix->set (y, x);
      shrink_eps = p.shrink_eps;
    }

    // make result
    for (int i = 0; i < l; i++) {
      alpha_[active2index[i]] = alpha[i];
      G_[active2index[i]]     = G[i];
    }

    // calculate objective value
    obj = 0;
    for (int i = 0; i < l; i++) obj += alpha[i] * (G[i] + b[i]);
    obj /= 2;

    // calculate threshold b
    rho = lambda_eq;

    delete [] status;
    delete [] alpha;
    delete [] x;
    delete [] y;
    delete [] b;
    delete [] G;
    delete [] active2index;
    delete [] shrink_iter;
    delete q_matrix;

    fprintf (stdout, "\nDone! %d iterations\n\n", iter);
    return 1;
  }

  catch (...) {
    fprintf (stderr, "QP_Solver::learn(): Out of memory\n");
    exit (EXIT_FAILURE);
    return 0; 
  }
}

void
QP_Solver::learn_sub()
{
  fprintf (stdout, "%6d examples, cache size: %d\n", 
	   active_size, q_matrix->size);

  while(++iter) {
    /////////////////////////////////////////////////////////
    // Select Working set
    double Gmax1 = -INF;
    int i = -1;
    double Gmax2 = -INF;
    int j = -1;

    for (int k = 0; k < active_size; k++) {
     if (y[k] > 0) {
	if (!is_upper_bound (k)) {
	  if (-G[k] > Gmax1) {
	    Gmax1 = -G[k];
	    i = k;
	  }
	}

	if (!is_lower_bound (k)) {
	  if (G[k] > Gmax2) {
	    Gmax2 = G[k];
	    j = k;
	  }
	}
      } else {
	if (!is_upper_bound (k)) {
	  if (-G[k] > Gmax2) {
	    Gmax2 = -G[k];
	    j = k;
	  }
	}

	if (!is_lower_bound (k)) {
	  if (G[k] > Gmax1) {
	    Gmax1 = G[k];
	    i = k;
	  }
	}
      }
    }

    /////////////////////////////////////////////////////////
    //
    // Solving QP sub problems
    //
    double old_alpha_i = alpha[i];
    double old_alpha_j = alpha[j];

    double *Q_i = q_matrix->getQ (i, active_size);
    double *Q_j = q_matrix->getQ (j, active_size);

    if (y[i] * y[j] < 0) {
      double L = _max (0.0, alpha[j] - alpha[i]);
      double H = _min (C, C + alpha[j] - alpha[i]);
      alpha[j] += (-G[i] - G[j]) / (Q_i[i] + Q_j[j] + 2 * Q_i[j]);
      if (alpha[j] >= H)      alpha[j] = H;
      else if (alpha[j] <= L) alpha[j] = L;
      alpha[i] += (alpha[j] - old_alpha_j);
    } else {
      double L = _max (0.0, alpha[i] + alpha[j] - C);
      double H = _min (C, alpha[i] + alpha[j]);
      alpha[j] += (G[i] - G[j]) / (Q_i[i] + Q_j[j] - 2 * Q_i[j]);
      if (alpha[j] >= H)      alpha[j] = H;
      else if (alpha[j] <= L) alpha[j] = L;
      alpha[i] -= (alpha[j] - old_alpha_j);
    }

    /////////////////////////////////////////////////////////
    //
    // update status
    // 
    status[i] = alpha2status (alpha[i]);
    status[j] = alpha2status (alpha[j]);

    double delta_alpha_i = alpha[i] - old_alpha_i;
    double delta_alpha_j = alpha[j] - old_alpha_j;

    /////////////////////////////////////////////////////////
    //
    // Update O and Calculate \lambda^eq for shrinking, Calculate lambda^eq,
    // (c.f. Advances in Kernel Method pp.175)
    // lambda_eq = 1/|FREE| \sum_{i \in FREE} y_i - \sum_{l} y_i \alpha_i k(x_i,x_j) (11.29)
    //
    lambda_eq = 0.0;
    int size_A = 0;
    for (int k = 0; k < active_size; k++) {
      G[k] += Q_i[k] * delta_alpha_i + Q_j[k] * delta_alpha_j;
      if (is_free (k)) {
	lambda_eq -= G[k] * y[k];
	size_A++;
      }
    }

    /////////////////////////////////////////////////////////
    //
    // Select example for shrinking,
    // (c.f. 11.5 Efficient Implementation in Advances in Kernel Method pp. 175)
    //
    lambda_eq = size_A ? (lambda_eq / size_A) : 0.0;
    double kkt_violation = 0.0;

    for (int k = 0; k < active_size; k++) {
      double lambda_up = -(G[k] + y[k] * lambda_eq);	// lambda_lo = -lambda_up

      // termination criteria (11.32,11.33,11.34)
      if (! is_lower_bound (k) && lambda_up < -kkt_violation) kkt_violation = -lambda_up;
      if (! is_upper_bound (k) && lambda_up >  kkt_violation) kkt_violation =  lambda_up;

      // "If the estimate (11.30) or (11.31) was positive (or above some threshold) at
      // each of the last h iterations, it is likely that this will be true at the  optimal solution" 
      // lambda^up  (11.30) lambda^low = lambda^up * status[k]
      if (lambda_up * status[k] > shrink_eps) {
	if (shrink_iter[k]++ > shrink_size) {
	  active_size--;
	  swap_index (k, active_size); // remove this data from working set
	  q_matrix->swap_index(k, active_size);
	  q_matrix->update(active_size);
	  k--;
	}
      } else {
	// reset iter, if current data dose not satisfy the condition (11.30), (11.31)
	shrink_iter[k] = 0;
      }
    }
    
    /////////////////////////////////////////////////////////
    //
    // Check terminal criteria, show information of iteration
    //
    if (kkt_violation < eps) break;

    if ((iter % 50) == 0) { fprintf (stdout, "."); fflush (stdout); };

    if ((iter % 1000) == 0) {
      fprintf (stdout, " %6d %6d %5d %1.4f %5.1f%% %5.1f%%\n",
	       iter, active_size, q_matrix->size, kkt_violation,
	       100.0 * (q_matrix->hit - hit_old)/2000,
	       100.0 * q_matrix->hit/(q_matrix->hit + q_matrix->miss));
      fflush (stdout);

      // save old hit rate
      hit_old = q_matrix->hit;

      // This shrink eps rule is delivered from svm_light.
      shrink_eps = shrink_eps * 0.7 + kkt_violation * 0.3;
    }
  }
}

int
QP_Solver::check_inactive ()
{
  // final check
  fprintf (stdout, "\nChecking optimality of inactive variables ");
  fflush (stdout);

  // make dummy classifier
  try {
    Model *tmp_model = new Model (param);
    tmp_model->b = -lambda_eq;
    for (int i = 0; i < l; i++) {
      if (! is_lower_bound (i))	tmp_model->add (alpha[i] * y[i], (feature_node *) x[i]);
    }

    int react_num = 0;
    for (int k= l-1; k >= active_size; k--) {
      double lambda_up = 1 - y[k] * tmp_model->classify (x[k]);
      G[k] = y[k] * tmp_model->b - lambda_up;

      // Oops!, must be added to the active example.
      if ( (! is_lower_bound (k) && lambda_up < -eps) ||
	   (! is_upper_bound (k) && lambda_up >  eps) ) {
	swap_index (k, active_size);
	active_size++;
	++k;
	++react_num;
      }
    }

    delete tmp_model;
    fprintf (stdout, " re-activated: %d\n", react_num);
    
    return react_num;
  }

  catch (...) {
    fprintf (stderr, "QP_Solver::check_inactive(): Out of memory\n");
    exit (EXIT_FAILURE);
    return 0; 
  }
}
}

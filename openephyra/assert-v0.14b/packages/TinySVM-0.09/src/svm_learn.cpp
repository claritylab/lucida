/*
 TinySVM -- Yet Another Tiny SVM Package

 $Id: svm_learn.cpp,v 1.4 2002/08/20 06:31:17 taku-ku Exp $;

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
#include "misc.h"
#include "model.h"
#include "example.h"
#include "base_example.h"
#include "kernel.h"
#include "param.h"

// $Id: svm_learn.cpp,v 1.4 2002/08/20 06:31:17 taku-ku Exp $;

int
main (int argc, char **argv)
{
   TinySVM::Param param;

  if (!param.set (argc, argv) || argc < 3) {
    fprintf (stdout, "%s\n", COPYRIGHT);
    fprintf (stdout, "Try `svm_learn --help' for more information.\n");
    exit (EXIT_FAILURE);
  }

  fprintf (stdout, "%s\n", COPYRIGHT);

  if (param.verbose) {
    fprintf (stdout, "solver_type:\t\t%d\n",      param.solver_type);    
    fprintf (stdout, "kernel_type:\t\t%d\n",      param.kernel_type);
    fprintf (stdout, "param_g:\t\t%g\n",          param.param_g);
    fprintf (stdout, "param_s:\t\t%g\n",          param.param_s);
    fprintf (stdout, "param_r:\t\t%g\n",          param.param_r);
    fprintf (stdout, "degree:\t\t\t%d\n",         param.degree);
    fprintf (stdout, "eps:\t\t\t%g\n",            param.eps);
    fprintf (stdout, "C:\t\t\t%g\n",              param.C);
    fprintf (stdout, "insensitive loss:\t%g\n",   param.insensitive_loss);
  }

  TinySVM::Example example;
  if (!example.read (argv[argc - 2])) {
    fprintf (stderr, "%s: %s: No such file or directory\n", argv[0],
	     argv[argc - 2]);
    exit (EXIT_FAILURE);
  }

  if (param.verbose)
    fprintf (stdout, "feature_type:\t\t%s\n\n",
	     example.feature_type == TinySVM::BINARY_FEATURE ? "binary" : "double");

  TinySVM::Model *model = example.learn (param);

  if (!model) {
    fprintf (stderr, "%s: Unexpected error occurs\n", argv[0]);
    exit (EXIT_FAILURE);
  }
   
  if (param.svindex) {
     char *tmp =  new char[strlen(argv[argc-1]) + 5];
     strcpy (tmp, argv[argc-1]);
     strcat (tmp, ".idx");
     if (!model->writeSVindex (tmp)) {
	fprintf (stderr, "%s: %s: permission denied\n", argv[0], tmp);
	exit (EXIT_FAILURE);
     }
     delete [] tmp;
  }

  if (param.compress) model->compress();
  if (!model->write (argv[argc - 1])) {
    fprintf (stderr, "%s: %s: permission denied\n", argv[0], argv[argc - 1]);
    exit (EXIT_FAILURE);
  }

  if (param.verbose) {
    double h = model->estimateVC ();
    fprintf (stdout, "Margin:\t\t\t\t%g\n", model->estimateMargin ());
    fprintf (stdout, "Number of SVs:\t\t\t%d\n", model->getSVnum ());
    fprintf (stdout, "Number of BSVs:\t\t\t%d\n", model->getBSVnum ());
    fprintf (stdout, "Size of training data:\t\t%d\n", model->getTrainingDataSize());
    fprintf (stdout, "L1 Loss (Empirical Risk):\t%g\n", model->getLoss());
    fprintf (stdout, "Estimated VC dimension:\t\t%g\n", h);
    fprintf (stdout, "Estimated xi-alpha(2.0):\t\t%g\n", model->estimateXA(2.0));
    fprintf (stdout, "Estimated VC bound (n=0.05):\t%g\n",
	     sqrt ((h * (log (2 * example.size () / h) + 1) - log (0.05 / 4))
		   / example.size ()));
    fprintf (stdout, "Leave one out bound:\t\t%g (%d/%d)\n",
	     1.0 * model->getSVnum () / example.size (), model->getSVnum (),
	     example.size ());
  }

  delete model;

  return EXIT_SUCCESS;
}

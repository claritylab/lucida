/*
 TinySVM -- Yet Another Tiny SVM Package

 $Id: svm_classify.cpp,v 1.4 2002/08/20 06:31:17 taku-ku Exp $;

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

const char *help_message =
"\nUsage: svm_classify [options] test-file model-file\n\n\
options:\n\
  -V, --verbose          set verbose mode.\n\
  -v, --version          show the version of TinySVM and exit.\n\
  -h, --help             show this help and exit.\n "; 

static const char *short_options = "Vvh";

static struct option long_options[] = {
  {"verbose",   no_argument,       NULL, 'V'},
  {"version",   no_argument,       NULL, 'v'},
  {"help",      no_argument,       NULL, 'h'},
  {NULL, 0, NULL, 0}
};

int
main (int argc, char **argv)
{
  FILE *fp;
  int verbose = 0;

  while (1) {
    int opt = getopt_long (argc, argv, short_options, long_options, NULL);
    if (opt == EOF) break;

    switch (opt) {
    case 'V':
      verbose = 1;
      break;
    case 'v':
      fprintf (stdout, "%s of %s\n%s\n", argv[0], VERSION, COPYRIGHT);
      exit (EXIT_SUCCESS);
    case 'h':
      fprintf (stdout, "%s%s", COPYRIGHT, help_message);
      exit (EXIT_SUCCESS);
    default:
      fprintf (stdout, "%s\n", COPYRIGHT);
      fprintf (stdout, "Try `svm_classify --help' for more information.\n");
      exit (EXIT_SUCCESS);
    }
  }

  if (argc < 3) {
    fprintf (stdout, "%s\n", COPYRIGHT);
    fprintf (stdout, "Try `svm_classify --help' for more information.\n");
    exit (EXIT_FAILURE);
  }

  TinySVM::Model model;
  if (!model.read (argv[argc - 1])) {
    fprintf (stderr, "%s: %s: No such file or directory\n", argv[0], argv[argc - 1]);
    exit (EXIT_FAILURE);
  }

  if (!strcmp (argv[argc - 2], "-")) {
    fp = stdin;
  } else {
    if ((fp = fopen (argv[argc - 2], "r")) == NULL) {
      fprintf (stderr, "%s: %s: No such file or directory\n", argv[0], argv[argc - 2]);
      exit (EXIT_FAILURE);
    }
  }

  char *line;
  int all = 0;
  int correct = 0;
  int res_a = 0;
  int res_b = 0;
  int res_c = 0;
  int res_d = 0;

  while ((line = model.readLine (fp)) != NULL) {
    double y = 0;
    int len = strlen (line);
    int i = 0;
    while (isspace (line[i])) i++;
    y = atof (line + i);
    while (i < len && !isspace (line[i])) i++;
    while (i < len && isspace (line[i])) i++;
    double dist = model.classify ((const char *) (line + i));
    if (verbose) printf ("%g %g\n", y, dist);
    fflush (stdout);

    all++;
    if(dist > 0) {
      if(y > 0) correct++;
      if(y > 0) res_a++; else res_b++;
    } else {
      if(y < 0) correct++;
      if(y > 0) res_c++; else res_d++;
    }
  }
  printf ("Accuracy:   %.5f%% (%d/%d)\n", 100.0 * correct/all , correct, all);
  printf ("Precision:  %.5f%% (%d/%d)\n", 100.0 * res_a/(res_a + res_b), res_a, res_a + res_b);
  printf ("Recall:     %.5f%% (%d/%d)\n", 100.0 * res_a/(res_a + res_c), res_a, res_a + res_c);
  printf ("System/Answer p/p p/n n/p n/n: %d %d %d %d\n", res_a,res_b,res_c,res_d);
  fclose (fp);

  return EXIT_SUCCESS;
}

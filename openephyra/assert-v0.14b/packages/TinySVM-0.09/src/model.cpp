/*
 TinySVM -- Yet Another Tiny SVM Package

 $Id: model.cpp,v 1.4 2002/08/20 06:31:17 taku-ku Exp $;

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


#include "model.h"
#include "common.h"
#include "kernel.h"
#include "classifier.h"
#include "qp_solver.h"
#include "example.h"

// $Id: model.cpp,v 1.4 2002/08/20 06:31:17 taku-ku Exp $;

#define MAKE_KERNEL { if (!kernel) \
      kernel = new Classifier( *(dynamic_cast <BaseExample *>(this)), param); };

namespace TinySVM {

Model::Model ()
{
  margin = vc = sphere = 0;
  svindex_size = bsv = -1;
  kernel = 0;
  alpha = 0;
  G = 0;
  feature_type = BINARY_FEATURE;
}

Model::Model (const Param & p)
{
  margin = vc = sphere = 0;
  svindex_size = bsv = -1;
  kernel = 0;
  alpha = 0;
  G = 0;
  feature_type = BINARY_FEATURE;
  param = p;
}


Model::~Model ()
{
  delete kernel;
}

int
Model::clear ()
{
  delete kernel;
  kernel = 0;
  alpha  = 0;
  G      = 0;
  return BaseExample::clear ();      
}

Model & Model::operator = (Model &m)
{
  if (this != &m) {
    BaseExample::operator = (m);
    margin = vc = 0;
    feature_type = BINARY_FEATURE;
    param = m.param;
    b     = m.b;
    bsv   = m.bsv;
    delete kernel;
    kernel = 0;
  }

  return *this;
}

double
Model::classify (const char *s)
{
  MAKE_KERNEL;
  return kernel->getDistance (s) - b;
}

double
Model::classify (const feature_node * f)
{
  MAKE_KERNEL;
  return kernel->getDistance (f) - b;
}

double
Model::estimateMargin ()
{
  if (margin) return margin;

  MAKE_KERNEL;
  double m = 0;
  for (int i = 0; i < l; i++) 
    m += y[i] * kernel->getDistance (x[i]);

  return (margin = 1.0 / sqrt (m));
}

double
Model::estimateVC ()
{
  if (vc) return vc;
  if (!margin) margin = estimateMargin ();
  if (!sphere) sphere = estimateSphere();
  return (vc = (sphere * sphere) / (margin * margin) + 1);
}

double
Model::estimateXA (const double rho)
{
  MAKE_KERNEL;

 // calculate R_delta, Roughly axpproxmimated
 feature_node *org = new feature_node [1];
 org[0].index = -1;
 org[0].value = 0;

 // I think it is good axproximation for binary vector
 double rsq = -INF;
 for (int i = 0; i < l; i++) {
   rsq = _max(rsq, kernel->getKernel (x[i], x[i]) - kernel->getKernel (x[i], org));
 }

 double result = 0.0;
 for (int i = 0; i < l; i++) {
   double d = kernel->getDistance (x[i]) - b;
   double eps;
   double alpha;
   if (y[i] > 0) { eps = _max(0.0, 1 - d); alpha =  y[i]; }
   else          { eps = _max(0.0, 1 + d); alpha = -y[i]; };
   if ((rho * alpha * rsq + eps) >= 1.0) result++;
 }

 delete [] org;

 return result;
}

double 
Model::estimateSphere()
{
  MAKE_KERNEL;

  feature_node *org = new feature_node[1];
  org[0].index = -1;
  org[0].value = 0;
  double orgsq = kernel->getKernel (org, org);

  double maxd = -INF;
  for (int i = 0; i < l; i++) {
    maxd =
      _max (maxd,kernel->getKernel (x[i],x[i]) - 2 * kernel->getKernel (x[i], org) + orgsq);
  }

  delete [] org;

  return (sphere = sqrt(maxd));
}

int
Model::compress ()
{
  if (param.degree != 1) {
    fprintf(stderr, "Model::compress: cannot compress model.\n");
    return 0;
  }

  double *w = new double [d + 1];
  for (int i = 0; i <= d; i++) w[i] = 0.0;
  for (int i = 0; i < l; i++) {
    for (feature_node *node = x[i]; node->index >= 0; node++) 
      w[node->index] += param.param_s * y[i] * node->value;
  }

  pack_d = 0;
  for (int i = 0; i <= d; i++) if (w[i] != 0) pack_d++;
  feature_node *fn = new feature_node [pack_d + 1];

  int j = 0;
  for (int i = 0; i <= d; i++) {
    if (w[i] != 0) {
      fn[j].index = i;
      fn[j].value = w[i];
      j++;
    }
  }
  fn[j].index = -1;
  fn[j].value = 0;

  delete [] w;
  clear();

  return add(1, fn);
}

int
Model::read (const char *filename, const char *mode, const int offset)
{
  FILE *fp = fopen (filename, mode);
  if (!fp) return 0;

  fseek (fp, offset, SEEK_SET);
  char *buf;
  char tmpbuf[1024];
  char version[512];
  int tmpl;

  // read parameter
  fscanf (fp, "%s Version %s%*[^\n]\n", tmpbuf, version);
  fscanf (fp, "%d%*[^\n]\n",  &param.kernel_type);
  fscanf (fp, "%d%*[^\n]\n",  &param.degree);
  fscanf (fp, "%lf%*[^\n]\n", &param.param_g);
  fscanf (fp, "%lf%*[^\n]\n", &param.param_s);
  fscanf (fp, "%lf%*[^\n]\n", &param.param_r);
  fscanf (fp, "%s%*[^\n]\n",  tmpbuf);

  param.dot_kernel = 1;
  if (param.kernel_type == RBF || param.kernel_type == ANOVA) param.dot_kernel = 0;

  buf = readLine(fp);
  if (sscanf (buf, "%d %d %d %lf%%*[^\n]\n", &tmpl, &bsv, &svindex_size, &loss) != 4) {
    sscanf (buf, "%d%*[^\n]\n", &tmpl); // old version
    svindex_size = -1;
    loss = -1;
  }

  tmpl--;
  fscanf (fp, "%lf%*[^\n]\n", &b);

  int line = 0;
  while ((buf = readLine (fp)) != NULL && line < tmpl) {
    line++;
    if (!this->add (buf)) {
      fprintf (stderr, "WARNING: Format Error in file [%s] line [%d]\n",
	       filename, line);
      fclose (fp);
      return 0;
    }
  }
  fclose (fp);

  // make kernel
  MAKE_KERNEL;

  return 1;
}

int
Model::write (const char *filename, const char *mode, const int offset)
{
  FILE *fp = fopen (filename, mode);
  if (!fp) return 0;

  //   fprintf(fp,"SVM-light Version V3.02\n");
  fprintf (fp, "TinySVM Version %s\n", VERSION);
  fprintf (fp, "%d # kernel type\n", param.kernel_type);
  fprintf (fp, "%d # kernel parameter -d\n", param.degree);
  fprintf (fp, "%.8g # kernel parameter -g\n", param.param_g);
  fprintf (fp, "%.8g # kernel parameter -s\n", param.param_s);
  fprintf (fp, "%.8g # kernel parameter -r\n", param.param_r);
  fprintf (fp, "empty # kernel parameter -u\n");

  // add L1loss, # of training size to the model file since version 0.02
  fprintf (fp, "%d %d %d %.8g # number of SVs/BSVs/number of training data/L1 loss \n", 
	   l+1, bsv, svindex_size, loss);   // triky, svm_light must be l+1

  fprintf (fp, "%.16g # threshold b\n", b);

  for (int i = 0; i < l; i++) {
    fprintf (fp, "%.16g", y[i]);
    for (feature_node * node = x[i]; node->index >= 0; node++)
      fprintf (fp, " %d:%.16g", node->index, node->value);
    fprintf (fp, "\n");
  }

  fclose (fp);
  return 1;
}
}

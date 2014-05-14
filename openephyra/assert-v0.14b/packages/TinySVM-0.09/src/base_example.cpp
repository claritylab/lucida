/*
 TinySVM -- Yet Another Tiny SVM Package

 $Id: base_example.cpp,v 1.5 2002/08/20 06:31:16 taku-ku Exp $;

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


#include "base_example.h"
#include "common.h"

// $Id: base_example.cpp,v 1.5 2002/08/20 06:31:16 taku-ku Exp $;

// misc function
namespace TinySVM {

int
inc_refcount_feature_node (feature_node *f)
{
   int i;
   for (i = 0; f[i].index >= 0; i++);
   return --f[i].index;
}

int
dec_refcount_feature_node (feature_node *f)
{
   int i;
   for (i = 0; f[i].index >= 0; i++);
   return ++f[i].index;
}

int
comp_feature_node (const void *x1, const void *x2)
{
  feature_node *p1 = (feature_node *) x1;
  feature_node *p2 = (feature_node *) x2;
  return (p1->index > p2->index);
}

feature_node *
copy_feature_node (const feature_node * f)
{
  int i;
  for (i = 0; f[i].index >= 0; i++);

  try {
    feature_node *r = new feature_node[i + 1];
    for (i = 0; f[i].index >= 0; i++) {
      r[i].index = f[i].index;
      r[i].value = f[i].value;
    }
    r[i].index = -1;
    return r;
  }

  catch (...) {
    fprintf (stderr, "copy_feature_node(): Out of memory\n");
    exit (EXIT_FAILURE);
    return 0;
  }
}

feature_node *
str2feature_node (const char *s)
{
  int elmnum = 0;
  int len = strlen (s);

  for (int i = 0; i < len; i++) if (s[i] == ':') elmnum++;

  try {
    feature_node *_x = new feature_node[elmnum + 1];

    int j = 0;
    for (int i = 0; j < elmnum && i < len;) {
      while (i < len && isspace (s[i]))	i++;
      _x[j].index = atoi (s + i);
      while (i + 1 < len && s[i] != ':') i++;
      _x[j].value = atof (s + i + 1);
      j++;
      while (i < len && !isspace (s[i])) i++;
    }

    // dumy index
    _x[j].index = -1;
    _x[j].value = 0;
    return _x;
  }

  catch (...) {
    fprintf (stderr, "str2feature_node(): Out of memory\n");
    exit (EXIT_FAILURE);
    return 0;
  }
}

feature_node *
fix_feature_node (feature_node * _x)
{
  register int i;
  register int cindex = -1;
  register int sorted = 1;

  // check sort
  for (i = 0; _x[i].index >= 0; i++) {
     if (cindex >= _x[i].index) sorted = 0;
     cindex = _x[i].index;
  }
   
  // sort
  if (!sorted) qsort ((void *) _x, i, sizeof (feature_node), comp_feature_node);
  return _x;
}

BaseExample::BaseExample ()
{
  l = d = pack_d = strl = 0;
  stre = 0;
  x = 0;
  y = 0;
  alpha = 0;
  G  = 0;
  feature_type = class_type = BINARY_FEATURE;
}

BaseExample::~BaseExample ()
{
  for (int i = 0; i < l; i++) {
     if (x && dec_refcount_feature_node(x[i]) == -1) delete [] x[i];
  }
   
  delete [] x;
  delete [] y;
  delete [] alpha;
  delete [] G;
  delete [] stre;
}

// copy constructor
BaseExample &
BaseExample::operator =(BaseExample & e) 
{
  if (this != &e) {
    clear ();
    for (int i = 0; i < e.l; i++) {
       inc_refcount_feature_node (e.x[i]);
       add (e.y[i], e.x[i]);
    }
    l = e.l;
    pack_d = e.pack_d;
    svindex_size = e.svindex_size;
    if (svindex_size) {
	_clone (alpha, e.alpha, svindex_size);
	_clone (G,     e.G,     svindex_size);
    }
  }

  return *this;
}

int
BaseExample::clear ()
{
  for (int i = 0; i < l; i++) {
     if (x && dec_refcount_feature_node(x[i]) == -1) delete [] x[i];
  }

  delete [] x;
  delete [] y;
  delete [] alpha;
  delete [] G;

  l = d = pack_d = 0;
  x = 0;
  y = 0;
  alpha = 0;
  G = 0;
  return 0;
}
   

char *
BaseExample::readLine (FILE * fp)
{
  long len;
  int c;
  char *tstr;

  try {
    if (! stre) {
      strl = MAXLEN;
      stre = new char[strl];
    }

    len = 0;
    tstr = stre;

    while (1) {
      if (len >= strl) {
	tstr = _resize (tstr, strl, strl + MAXLEN, (char)0);
        strl += MAXLEN;
	stre = tstr;
      }

      c = fgetc (fp);
      if (c == '\n' || c == '\r') {
        tstr[len] = '\0';
        break;
      }

      if (c == EOF && feof (fp)) {
        tstr[len] = '\0';
        if (feof (fp) && len == 0) tstr = 0;
        break;
      }

      tstr[len++] = c;
    }

    return tstr;
  }

  catch (...) {
    fprintf (stderr, "BaseExample::readLine(): Out of memory\n");
    exit (EXIT_FAILURE);
    return 0; 
  }
}
   
int
BaseExample::remove (int i)
{
   if (i < 0 || i >= l || ! x || ! y) {
      fprintf(stderr, "BaseExample::set (): Out of range\n");
      return 0;
   }

  if (dec_refcount_feature_node(x[i]) == -1) delete [] x[i];
  for (int j = i+1; j < l; j++) {
     x[j-1] = x[j];
     y[j-1] = y[j];
  }
     
  return --l;
}

int 
BaseExample::get (int i, double &_y, feature_node *&_x)
{
   if (i < 0 || i >= l || ! x || ! y) {
      fprintf(stderr, "BaseExample::set (): Out of range\n");
      return 0;
   }
   
   _y = y[i];
   _x = x[i];
   return 1;
}

const char *
BaseExample::get (int i)
{
  if (i < 0 || i >= l || ! x || ! y) {
      fprintf(stderr, "BaseExample::get (): Out of range\n");
      return 0;
  }

  try {
    int elmnum;
    feature_node *node = x[i];
    for (elmnum = 0; node[elmnum].index >= 0; elmnum++);

    int len = _min (strl + 1024, (elmnum + 1) * 32);
    if (len > strl) {
      stre = _resize (stre, strl, len, (char)0);
      strl = len;
    }

    sprintf (stre, "%.16g", y[i]);
    char tmp[32];
    for (feature_node *node = x[i]; node->index >= 0; node++) {
      sprintf (tmp, " %d:%.16g", node->index, node->value);
      strcat(stre, tmp);
    }

    return (const char*)stre;
  }

  catch (...) {
    fprintf (stderr, "BaseExample::get (): Out of memory\n");
    exit (EXIT_FAILURE);
    return 0; 
  }
}
     
int 
BaseExample::set (int i, const double _y, feature_node *_x)
{
   if (i < 0 || i >= l || ! x || ! y) {
      fprintf(stderr, "BaseExample::set (): Out of range\n");
      return 0;
   }
   
  if (dec_refcount_feature_node(x[i]) == -1) delete [] x[i];
  _x = fix_feature_node(_x);
  inc_refcount_feature_node(_x);
   
  x[i] = _x;
  y[i] = _y;
  return 1;
}
   
int 
BaseExample::set (int i, const double _y, const char *s)
{
   return set(i, _y, (feature_node *) str2feature_node (s));
}

int 
BaseExample::set (int _i, const char *s)
{
  double _y = 0;
  int len = strlen (s);

  int i;
  for (i = 0; i < len;) {
    while (isspace (s[i])) i++;
    _y = atof (s + i);
    while (i < len && !isspace (s[i])) i++;
    while (i < len && isspace (s[i]))  i++;
    break;
  }

  return set (_i, _y, (const char *) (s + i));
}

int
BaseExample::add (const double _y, feature_node * _x)
{
  try {
    int fnum = 0;
    feature_node *node = fix_feature_node ((feature_node *) _x);
     
    // check contents
    for (int i = 0; (node + i)->index >= 0; i++) {
      if ((node + i)->value != 1) feature_type = DOUBLE_FEATURE; // check feature type
      d = _max (d, (node + i)->index);	// save max dimension
      fnum++;
    }
     
    // incriment refcount
    inc_refcount_feature_node (node);
    pack_d = _max (fnum, pack_d);

    // check class type
    if (_y != +1 && _y != -1) class_type = DOUBLE_FEATURE;

    // resize
    x = _append (x, l, node, (feature_node*)0);
    y = _append (y, l, _y, 0.0);
    l++;

    return 1;
  }

  catch (...) {
    fprintf (stderr, "BaseExample::add(): Out of memory\n");
    exit (EXIT_FAILURE);
    return 0;
  }
}

int
BaseExample::add (const double _y, const char *s)
{
  return add (_y, (feature_node *) str2feature_node (s));
}

int
BaseExample::add (const char *s)
{
  double _y = 0;
  int len = strlen (s);

  int i;
  for (i = 0; i < len;) {
    while (isspace (s[i])) i++;
    _y = atof (s + i);
    while (i < len && !isspace (s[i])) i++;
    while (i < len && isspace (s[i]))  i++;
    break;
  }

  return add (_y, (const char *) (s + i));
}

int
BaseExample::writeSVindex (const char *filename, const char *mode, const int offset)
{
  if (!alpha || !G) return 0;

  FILE *fp = fopen (filename, mode);
  if (!fp) return 0;

  for (int i = 0; i < svindex_size; i++)
    fprintf (fp, "%.16g %.16g\n", alpha[i], G[i]);

  fclose (fp);
  return 1;
}

int
BaseExample::readSVindex (const char *filename, const char *mode, const int offset)
{
  if (l == 0) {
    fprintf(stderr, "Fatal: size == 0, Read model/example file before reading .idx file\n");
    return 0;
  }

  FILE *fp = fopen (filename, mode);
  if (!fp) return 0;

  delete [] alpha;
  delete [] G;

  int _l = 0;
  char *buf;

  while ((buf = readLine (fp)) != NULL) {
    double alpha_, G_;
    if (2 != sscanf (buf, "%lf %lf\n", &alpha_, &G_)) {
      fprintf(stderr, "Fatal: Format error %s, line %d\n", filename, _l);
      fclose (fp);
      return 0;
    }

    alpha = _append (alpha, _l, alpha_, 0.0);
    G     = _append (G,     _l, G_,     0.0);
    _l++;
  }

  fclose (fp);

  //  check size of idx file
  if (l < _l) {
    fprintf(stderr, "Fatal: model/example size (%d) < idx size (%d)\n", l, _l);
    delete [] alpha;
    delete [] G;
    alpha = 0;
    G = 0;
    return 0;
  }

  svindex_size = _l;
  return 1;
}
}

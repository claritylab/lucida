/*
 YamCha -- Yet Another Multipurpose CHunk Annotator

 $Id: svm.cpp,v 1.11 2003/01/06 10:46:35 taku-ku Exp $;

 Copyright (C) 2001  Taku Kudoh <taku-ku.aist-nara.ac.jp>
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
#include "mmap.h"
#include "darts.h"
#include "param.h"
#include "yamcha.h"
#include <cmath>

namespace YamCha 
{
#define  _YAMCHA_INIT_SVM param(0), mode(0), mmap(0), da(0), dot_buf(0), dot_cache(0), \
                          result_(0), result(0), version(0), kernel_type(0), \
                          param_degree(0),  param_g(0), param_r(0), param_s(0), \
                          alpha(0), alpha_size(0), sv_size(0), table_size(0), \
                          dimension_size(0), nonzero_dimension_size(0), fi(0), \
                          table(0), da_size(0), model(0), model_size(0), \
                          class_size(0)

  static inline char *read_ptr (char **ptr, size_t size) 
  {
    char *r = *ptr;
    *ptr += size;
    return r;
  }

  template <class T> static inline void read_static (char **ptr, T& value)
  {
    char *r = read_ptr (ptr, sizeof (T));
    memcpy (&value, r, sizeof (T));
  }

  SVM::SVM():  _YAMCHA_INIT_SVM {}

  SVM::SVM(const char* filename): _YAMCHA_INIT_SVM 
  {
    if (! open (filename)) throw std::runtime_error (_what);
  }

  SVM::~SVM() { close (); }

  bool SVM::close ()
  {
    delete [] alpha;
    delete [] model;
    delete [] result_;
    delete [] result;
    delete [] dot_buf;
    delete [] dot_cache;
    delete mmap;
    delete param; 

    param = 0, mode = 0, mmap = 0, da = 0 , dot_buf = 0, dot_cache = 0, \
    result_ = 0, result = 0, version = 0, kernel_type = 0, \
    param_degree = 0,  param_g = 0, param_r = 0, param_s = 0, \
    alpha = 0, alpha_size = 0, sv_size = 0, table_size = 0, \
    dimension_size = 0, nonzero_dimension_size = 0, fi = 0, \
    table = 0, da_size = 0, model = 0, model_size = 0, \
    class_size = 0;

    return true;
  }

  bool SVM::open (const char *filename)
  {
    try {

      param = new Param;
      mmap  = new Mmap<char>;

      if (! mmap->open (filename)) throw std::runtime_error (mmap->what());
      char *ptr = mmap->begin ();

      // kernel specfic param.
      version = read_ptr (&ptr, 32);

      // check version
      if (atof(version) != MODEL_VERSION) 
      	throw std::runtime_error ("model version is different");

      kernel_type  = read_ptr (&ptr, 32);
      read_static<unsigned int>(&ptr, param_degree);
      read_static<double> (&ptr, param_g);
      read_static<double> (&ptr, param_r);
      read_static<double> (&ptr, param_s);
       
      // model specfic
      read_static<unsigned int>(&ptr, model_size);
      read_static<unsigned int>(&ptr, class_size);
      read_static<unsigned int>(&ptr, alpha_size);
      read_static<unsigned int>(&ptr, sv_size);
      read_static<unsigned int>(&ptr, table_size);
      read_static<unsigned int>(&ptr, dimension_size);
      read_static<unsigned int>(&ptr, nonzero_dimension_size);

      // NOTE: no descrete answre (class_size - 1 == class_size (class_size -1) / 2)
      //       so, we can distingwish the following two mode
      if (model_size == class_size-1 && model_size != 1) {
	mode = 1; 
      } else if (model_size == 1 || model_size == class_size * (class_size-1)/2) {
	mode = 0;
      } else throw std::runtime_error ("model/class size is invalid");
      
      // Double Array 
      read_static<unsigned int>(&ptr, da_size);

      // read model prameters 
      int param_size;
      read_static<int>(&ptr, param_size);
      char *param_str = read_ptr (&ptr, param_size);

      int pos = 0;
      while (pos < param_size) {
	char *key =  (param_str + pos);
	while (param_str[++pos] != '\0') {};
	pos++;
	char *value = param_str + pos;
	param->setProfile (key, value);
	while (param_str[++pos] != '\0') {};
	pos++;
      }

      // class_, list of fixied record (32)
      result  = new Result [class_size];
      for (unsigned int i = 0; i < class_size; i++) {
	result[i].name = read_ptr(&ptr, 32);
      }

      if (mode == 1) class_size--;

      // model
      model = new model_t [model_size];
      for (unsigned int i = 0; i < model_size; i++) {
	read_static<unsigned int>(&ptr, model[i].pos);
	read_static<unsigned int>(&ptr, model[i].neg);
	read_static<double>(&ptr, model[i].b);
      }

      // alpha, tricky, including dummy filelds
      alpha = new std::pair<int, double> [alpha_size + model_size];
      for (unsigned int i = 0; i < alpha_size + model_size; i++) {
	read_static<int> (&ptr, alpha[i].first);
	read_static<double> (&ptr, alpha[i].second);
      }

      // feature index
      fi = (unsigned int *) read_ptr (&ptr, sizeof (unsigned int) * dimension_size);

      // table
      table = (int *)( read_ptr (&ptr, sizeof (int) * table_size) );

      // Double Array
      da = (unit_t *) read_ptr (&ptr, da_size);

      // check size
      if ((unsigned int)(ptr - mmap->begin ()) != mmap->size ()) 
	throw std::runtime_error ("size of model file is invalid.");
    
      // initilize
      dot_cache = new double [nonzero_dimension_size+1];
      for (unsigned int i = 0; i <= nonzero_dimension_size; i++) 
	dot_cache[i] = pow (param_s * i  + param_r, (int)param_degree);

      dot_buf = new unsigned int [sv_size];
      result_ = new double [model_size];

      return true;
    }

    catch (std::exception &e) {
      _what = std::string ("SVM::open(): ") + e.what ();
      close ();
      throw std::runtime_error (_what);
      return false;
    }
  }
  
  Result *SVM::classify (unsigned int size, char **features)
  {
    for (unsigned int i = 0; i < sv_size; i++) dot_buf[i] = 0;
    for (unsigned int i = 0; i < model_size; i++) result_[i] = -(model[i].b);
    for (unsigned int i = 0; i < class_size; i++) result[i].dist = result[i].score = 0.0;

    for (unsigned int k = 0;;) {
    next:
      if (k == size) break;

      char *key = features[k];
      unsigned int len = strlen (key);
      int b = da[0].base;
      unsigned int p;

      for (unsigned int i = 0; i < len; i++) {
	p = b + (unsigned char)key[i] + 1;
	if ((unsigned int)b == da[p].check) {
	  b = da[p].base;
	} else {
	  k++;
	  goto next;
	}
      }

      p = b;
      int n = da[p].base;
      if ((unsigned int)b == da[p].check && n < 0)
	for (int j = fi[-n-1]; table[j] != -1; j++) dot_buf[table[j]]++;
      
      k++;
    }
    
    unsigned int i = 0;
    for (unsigned int j = 0;;j++) {
      if (alpha[j].first == -1) {
	if (++i == model_size) break;
      } else {
	result_[i] += alpha[j].second * dot_cache[dot_buf[alpha[j].first]];
      }
    }
  
    switch (mode) {
    case 0:
      for (unsigned int i = 0; i < model_size; i++) {
	result[result_[i] >= 0 ? model[i].pos : model[i].neg].score++;  // score is votes
	result[model[i].pos].dist += result_[i];
	result[model[i].neg].dist -= result_[i];
      }
      break;
    case 1:
      for (unsigned int i = 0; i < model_size; i++) {
	result[model[i].pos].score = result_[i];
	result[model[i].pos].dist  = result_[i];
      }
      break;
    }

    return result;
  }

  int SVM::getProfileInt (const char *key) 
  {
    if (param) return param->getProfileInt (key);
    return 0;
  }
    
  const std::string SVM::getProfileString (const char *key) 
  {
    if (param) return param->getProfileString (key);
    return std::string ("");
  }

}

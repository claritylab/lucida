/*
 YamCha -- Yet Another Multipurpose CHunk Annotator

 $Id: yamcha.h,v 1.15 2003/01/06 10:46:35 taku-ku Exp $;

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
#ifndef _YAMCHA_H
#define _YAMCHA_H

#ifdef __cplusplus

#include <vector>
#include <string>
#include <iostream>

namespace YamCha 
{
  class FeatureIndex;
  class Param;
  template <class T> class Mmap;

  struct Result {
    char   *name;
    double score; // votes
    double dist;  // distance from the hyperplane
  };

  class SVM
  {
  private:
    struct model_t {
      unsigned int pos;
      unsigned int neg;
      double b;
    };
     
    struct unit_t {
      int base;
      unsigned int check;
    };

    Param                   *param;
    int                     mode;
    Mmap<char>              *mmap;
    unit_t                  *da;
    unsigned int*           dot_buf;
    double*                 dot_cache;
    double*                 result_;
    Result*                 result;
    char*                   version;
    char*                   kernel_type;
    unsigned int            param_degree;     
    double                  param_g;
    double                  param_r;
    double                  param_s;
    std::pair<int, double>* alpha;
    unsigned int            alpha_size;
    unsigned int            sv_size;
    unsigned int            table_size;
    unsigned int            dimension_size;
    unsigned int            nonzero_dimension_size;
    unsigned int*           fi;
    int*                    table;
    unsigned int            da_size;
    model_t                *model;
    unsigned int            model_size;
    unsigned int            class_size;
    std::string            _what;

  public:
    SVM  ();
    SVM  (const char *);
    ~SVM ();
    bool open (const char *);
    bool close ();
    Result* classify                   (unsigned int, char**);
    int               getProfileInt    (const char *);
    const std::string getProfileString (const char *) ;
    unsigned int      getClassSize     () { return class_size; }
    const char* what() { return _what.c_str(); }
  };

  class Chunker
  {
  private:
    FeatureIndex* feature_index;
    SVM*          svm;
    bool          is_reverse;
    bool          is_write_header;
    bool          is_partial;
    bool          is_verbose;
    int           mode;
    unsigned int  column_size;
    unsigned int  class_size;
    char**        features;
    unsigned int  features_size;
    int           (*selector_func) (Chunker *, int i);
    std::string   eos_string;
    std::string   feature;
    std::vector < std::vector <std::string> > context;
    std::vector <std::string> tag;
    std::vector <std::string> bos;
    std::vector <std::string> eos;
    std::vector < std::vector <std::pair <char*, double> > > dist;
    std::string   _what;

    std::string&  getFeature    (int, int);
    void          reverse       ();
    unsigned int  select        (int);
    bool          parseDetail   ();
    bool          parseNormal   ();
    bool          parseSelect   ();
    std::ostream& writeDetail   (std::ostream&);
    std::ostream& writeNormal   (std::ostream&);
    std::ostream& writeSelect   (std::ostream&);

  public:
    bool          open        (Param &);
    bool          open        (int,  char**);
    bool          open        (const char*);
    bool          close       ();
    bool          clear       ();
    unsigned int  addFeature  (char *);
    bool          setSelector (int (*) (Chunker *, int i));
    const char*   getTag      (unsigned int i)                 { return tag[i].c_str(); }
    const char*   getContext  (unsigned int i, unsigned int j) { return context[i][j].c_str(); };
    unsigned int  add         (std::vector <std::string>&);
    unsigned int  add         (std::string&);
    unsigned int  add         (const char* s) { return add (s); }
    unsigned int  size        () { return context.size (); }
    unsigned int  row         () { return context.size (); }
    unsigned int  column      () { return column_size;     }
    bool          parse       (std::istream&, std::ostream&);
    bool          parse       ();
    int           parse       (int, char**);
    std::ostream& write       (std::ostream &) ;
    std::istream& read        (std::istream &);

    const char* what() { return _what.c_str(); }

    Chunker  ();
    Chunker  (Param &);
    Chunker  (int, char**);
    Chunker  (const char*);
    ~Chunker ();
  };
}

#endif
#endif

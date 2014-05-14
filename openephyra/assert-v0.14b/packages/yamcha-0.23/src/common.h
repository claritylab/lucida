/*
 YamCha -- Yet Another Multipurpose CHunk Annotator

 $Id: common.h,v 1.19 2003/01/06 10:46:35 taku-ku Exp $;

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
#ifndef _YAMCHA_COMMON_H
#define _YAMCHA_COMMON_H

// tricky macro for MSVC
#ifdef _MSC_VER
#define for if(0); else for
#endif

// $Id: common.h,v 1.19 2003/01/06 10:46:35 taku-ku Exp $;
#define COPYRIGHT "Yet Another Multipurpose CHunk Annotator\n\
Copyright (C) 2001-2003 Taku Kudo All rights reserved.\n"

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#define MAX_STR_LEN 512
#define MAX_FEATURE_LEN 1024

#ifndef EXIT_FAILURE
#define EXIT_FAILURE 1
#endif

#ifndef EXIT_SUCCESS
#define EXIT_SUCCESS 0
#endif

#include <cstdlib>
#include <vector>
#include <string>
#include <stdexcept>

namespace YamCha {

  using namespace std;

  template <class T> inline T _min(T x,T y) { return (x<y)?x:y; }
  template <class T> inline T _max(T x,T y) { return (x>y)?x:y; }

  static inline int str2int (const char *str) 
  {
    char *nptr;
    int l = strtol (str, &nptr, 10);
    if (str == nptr) throw std::runtime_error (std::string (str) + ": invalid integer");
    return l;
  }

  static inline unsigned int split_string (const std::string &src, const std::string& key, std::vector <std::string>& result)
  {
    result.clear();
    int len =  src.size();
    int i = 0; int si = 0;
    while(i < len) {
      while (i < len && key.find (src[i]) != std::string::npos) { si++; i++; }; 
      while (i < len && key.find (src[i]) == std::string::npos) i++; 
      result.push_back (src.substr (si,i-si));
      si = i;
    }

    return result.size();
  }
}

#endif

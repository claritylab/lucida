/*
 MeCab -- Yet Another Part-of-Speech and Morphological Analyzer

 $Id: param.cpp,v 1.7 2003/01/29 18:30:45 taku-ku Exp $;

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

#include <fstream>
#include <strstream>
#include <cstdio>
#include "param.h"
#include "common.h"
#include <string.h>

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

namespace YamCha {
   
  using namespace std;
  
  bool Param::load (const char *filename)
  {
    std::ifstream ifs (filename);

    if (!ifs) {
       _what = std::string ("Param::load(): ") + 
	 std::string (filename) + ": no such file or directory";
      return false;
    }

    std::string line;
    while (std::getline (ifs, line)) {
      if (! line.size()  || (line.size() && (line[0] == ';' || line[0] == '#'))) continue;

      unsigned int pos = line.find ('=');
      if (pos == std::string::npos) {
	_what =  std::string ("Param::open(): ") + 
	  std::string (filename) + ": format error [" + line +"]";
	return false;
      }
    
      unsigned int s1,s2;
      for (s1 = pos+1; s1 < line.size () && isspace(line[s1]); s1++);
      for (s2 = pos-1; (int)s2 >= 0 && isspace(line[s2]); s2--);
      std::string value = line.substr (s1, line.size()-s1);
      std::string key   = line.substr (0, s2+1);
      setProfile (key.c_str(), value.c_str(), false);
    }

    return true;
  }

  bool Param::open (int argc, char **argv, const Option *opts)
  {
    int ind = 0;

    try {

      if (argc <= 0) {
	systemName = "unknown";
	return true; // this is not error
      }

      systemName = std::string (argv[0]);

      for (unsigned int i = 0; opts[i].name; i++) {
	if (opts[i].default_value) setProfile (opts[i].name, opts[i].default_value);
      }

      for (ind = 1; ind < argc; ind++) {

	if (argv[ind][0] == '-') {

	  // long options
	  if (argv[ind][1] == '-') {

	    char *s;
	    for (s = &argv[ind][2]; *s != '\0' && *s != '='; s++);
	    unsigned int len = (unsigned int)(s - &argv[ind][2]);
	    if (len == 0) return true; // stop the scanning

	    bool hit = false;
	    unsigned int i = 0;
	    for (i = 0; opts[i].name; i++) {
	      unsigned int nlen = strlen (opts[i].name);
	      if (nlen == len && strncmp (&argv[ind][2], opts[i].name, len) == 0) {
		hit = true;
		break;
	      }
	    }

	    if (!hit) throw 0;

	    if (opts[i].arg_description) {
	      if (*s == '=') {
		if (*(s+1) == '\0') throw 1;
		setProfile (opts[i].name, s+1);
	      } else { 
		if (argc == (ind+1)) throw 1;
		setProfile (opts[i].name, argv[++ind]);
	      }
	    } else {
	      if (*s == '=') throw 2;
	      setProfile (opts[i].name, 1);
	    }

	    // short options
	  } else if (argv[ind][1] != '\0') {

	    unsigned int i = 0;
	    bool hit = false;
	    for (i = 0; opts[i].name; i++) {
	      if (opts[i].short_name == argv[ind][1]) {
		hit = true;
		break;
	      }
	    }

	    if (!hit) throw 0;

	    if (opts[i].arg_description) {
	      if (argv[ind][2] != '\0') {
		setProfile (opts[i].name, &argv[ind][2]);
	      } else {
		if (argc == (ind+1)) throw 1;
		setProfile (opts[i].name, argv[++ind]);
	      }
	    } else {
	      if (argv[ind][2] != '\0') throw 2;
	      setProfile (opts[i].name, 1);
	    }
	  }
	} else {
	  rest.push_back (std::string (argv[ind])); // others
	}
      }

      return true;
    }

    catch (std::exception &e) {
      _what = e.what();
      return false;
    }

    catch (int num) {
      switch (num) {
      case 0: _what = std::string ("Param::open(): unrecognized option `" ) + argv[ind] + "`"; break;
      case 1: _what = std::string ("Param::open(): `") + argv[ind] + "` requres an argument";  break;
      case 2: _what = std::string ("Param::open(): `") + argv[ind]  + "` dosen't allow an argument"; break;
      }
      return false;
    }
  }

  void Param::clear ()
  {
    conf.clear (); 
    rest.clear (); 
  };
  
  bool Param::open (const char *arg, const Option *opts)
  {
    char str [1024];
    strncpy (str, arg, 1024);
    char* ptr [64];
    unsigned int size = 1;
    ptr[0] = PACKAGE;
   
    for (char *p = str; *p ; ) {
      while (isspace (*p)) *p++ = '\0';
      if (*p == '\0') break;
      ptr[size++] = p;
      if (size == 64) break;
      while (*p && ! isspace (*p)) p++;
    }
    
    return open (size, ptr, opts);
  }

  void Param::help (std::ostream &os, const Option *opts)
  {
    os << COPYRIGHT << std::endl
       << "Usage: " << PACKAGE << " [options] files\n";

    unsigned int max = 0;
    for (unsigned int i = 0; opts[i].name; i++) {
      unsigned int l = 1 + strlen (opts[i].name);
      if (opts[i].arg_description) l += (1 + strlen (opts[i].arg_description));
      max = _max (l, max);
    }

    for (unsigned int i = 0; opts[i].name; i++) {
      unsigned int l = strlen (opts[i].name);
      if (opts[i].arg_description) l += (1 + strlen (opts[i].arg_description));
      os << "  -"  << opts[i].short_name << ", --"  << opts[i].name;
      if (opts[i].arg_description) os << '=' << opts[i].arg_description;
      for (; l <= max; l++) os << ' ';
      os << opts[i].description  << std::endl;
    }

    os << std::endl;
  }

  void Param::version (std::ostream &os, const Option *)
  {
    os << PACKAGE << " of " << VERSION << std::endl;
  }

  const std::string Param::getProfileString (const char* key, bool check)
  {
    std::string val = conf[std::string(key)];
    if (check && val.empty())
      throw std::runtime_error (std::string("Param::getProfileString(): [") + key + "] is not defined.");
    return val;
  }
    
  int Param::getProfileInt (const char* key, bool check)
  {
    std::string val = conf[std::string(key)];
    if (check && val.empty()) 
      throw std::runtime_error (std::string("Param::getProfileString(): [") + key + "] is not defined.");
    return atoi (val.c_str());
  }

  void Param::setProfile (const char* key, const char* value, bool rewrite)
  {
    std::string keys2 = std::string(key);
    if (rewrite || (! rewrite && conf[keys2].empty()) ) conf[keys2] = value;
  }

  void Param::setProfile (const char* key, int i, bool rewrite)
  {
    std::string keys2 = std::string(key);
    if (rewrite || (! rewrite && conf[keys2].empty())) {
      std::ostrstream ss; ss << i;
      conf[std::string(key)] = ss.str();
      ss.freeze(false);
    }
  }
}

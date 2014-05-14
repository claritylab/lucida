/*
 YamCha -- Yet Another Multipurpose CHunk Annotator

 $Id: chunker.cpp,v 1.15 2003/01/29 22:10:42 taku-ku Exp $;

 Copyright (C) 2001  Taku Kudoh <taku-ku@aist-nara.ac.jp>
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
#include <vector>
#include <stdexcept>
#include <string>
#include <strstream>
#include <map>
#include <algorithm>
#include <functional>
#include <fstream>
#include "feature_index.h"
#include "param.h"
#include "yamcha.h"
#include "common.h"
#include <string.h>

namespace YamCha 
{
#define _INIT_CHUNKER feature_index(0), svm(0), \
                      is_reverse (0), is_write_header (0), \
                      is_partial (0), is_verbose(0), mode(0), \
                      column_size(0), class_size(0), features (0), \
                      features_size(0), selector_func (0)

#define CHUNKER_ERROR  std::ostrstream os; \
                      os << "Tagger::open(): " << param.what () << "\n\n" \
                         <<  COPYRIGHT << "\ntry '--help' for more information.\n" << std::ends; \
                      _what = os.str(); os.freeze (false);

  static const Option long_options[] = 
  {
    {"model",          'm', 0, "FILE", "use FILE as model file" },
    {"feature",        'F', 0, "PAT",  "use PAT as the feature template pattern"},
    {"eos-string" ,    'e', 0, "STR",  "use STR as sentence-boundary marker" },
    {"verbose",        'V', 0, 0,      "verbose mode" },     
    {"candidate",      'C', 0, 0,      "partial chunking model"} ,
    {"backward",       'B', 0, 0,      "select features from the end of sentence" },
    {"output",         'o', 0, "FILE", "use FILE as output file" },     
    {"version",        'v', 0, 0,      "show the version and exit" },
    {"help",           'h', 0, 0,      "show this help and exit" },
    {0,0,0,0,0}
  };
  
  Chunker::Chunker(): _INIT_CHUNKER {};

  Chunker::Chunker (Param &p): _INIT_CHUNKER 
  { 
    if (! open (p)) throw std::runtime_error (_what);
  }

  Chunker::Chunker (int argc, char** argv): _INIT_CHUNKER 
  { 
    if (! open (argc, argv)) throw std::runtime_error (_what);
  }

  Chunker::Chunker (const char* arg): _INIT_CHUNKER 
  { 
    if (! open (arg)) throw std::runtime_error (_what);
  }

  bool Chunker::open (int argc, char **argv)
  {
    Param param;

    if (! param.open (argc, argv, long_options)) {
      CHUNKER_ERROR;
      return false;
    }

    return open (param);
  }

  bool Chunker::open (const char *arg)
  {
    Param param;

    if (! param.open (arg, long_options)) {
      CHUNKER_ERROR;
      return false;
    }

    return open (param);
  }

  bool Chunker::open (Param &param)
  {
    try {

      if (param.getProfileInt ("help")) {
        std::ostrstream ostrs;
        param.help (ostrs, long_options);
	ostrs << std::ends; 
        std::runtime_error e (ostrs.str());
	ostrs.freeze (false);
	throw e; 
      }

      if (param.getProfileInt ("version")) {
        std::ostrstream ostrs;
        param.version (ostrs, long_options);
	ostrs << std::ends; 	 
        std::runtime_error e (ostrs.str());
	ostrs.freeze (false);
	throw e; 
      }

      close ();

      feature           = param.getProfileString ("feature");
      is_partial        = param.getProfileInt    ("candidate");
      is_verbose        = param.getProfileInt    ("verbose");
      eos_string        = param.getProfileString ("eos-string");
      std::string model = param.getProfileString ("model");

      if (model != "") {

	mode = 0;
	
	svm = new SVM;
	if (! svm->open (model.c_str())) throw std::runtime_error (svm->what());
	
	feature_index = new FeatureIndex;
	feature_index->setFeature (svm->getProfileString ("features"), 
				   svm->getProfileString ("tag_features"));

	column_size = svm->getProfileInt ("column_size");
	if (column_size == 0) column_size = feature_index->getColumnSize ();
	if (column_size == 0) throw std::runtime_error (std::string ("column size is 0 or unknown: ") + model);

	if (svm->getProfileString("parsing_direction") == "backward") 
	  is_reverse = true;

	class_size = svm->getClassSize ();

      } else if (feature != "") {
	
	mode       = 1;
	is_reverse = param.getProfileInt ("backward");
	
      } else {
	throw std::runtime_error ("unknown action mode");
      }

      features = new char * [MAX_FEATURE_LEN];
      for (unsigned int i = 0; i < MAX_FEATURE_LEN; i++) 
	features[i] = new char [MAX_STR_LEN];

      return true;
    }

    catch (std::exception &e) {
      _what = std::string ("Chunker::open(): ") + e.what ();
      throw std::runtime_error (_what);
      return false;
    }
  }

  int Chunker::parse (int argc, char **argv)
  {
    try {
   
      Param param;
    
      if (! param.open (argc, argv, long_options)) {
	CHUNKER_ERROR;
	throw std::runtime_error (_what);
      }

      if (param.getProfileInt ("help")) {
	param.help (std::cout, long_options);
	return EXIT_SUCCESS;
      }

      if (param.getProfileInt ("version")) {
	param.version (std::cout, long_options);
	return EXIT_SUCCESS;
      }

      if (! open (param)) throw std::runtime_error (_what);

      std::ostream *ofs = &std::cout;
      std::string outputFileName = param.getProfileString ("output");

      if (! outputFileName.empty()) {
	ofs = new std::ofstream (outputFileName.c_str());
	if (! *ofs) throw std::runtime_error (outputFileName + ", cannot open");
      }
     
      const std::vector <std::string>& rest = param.getRestArg (); 
     
      if (rest.size()) {
	for (unsigned int i = 0; i < rest.size(); i++) {
	  std::ifstream ifs (rest[i].c_str ());
	  if (!ifs) throw std::runtime_error (rest[i] + ", cannot open");
	  while (parse (ifs, *ofs)) {};
	}
      } else {
	while (parse (std::cin, *ofs)) {};
      }
  
      if (ofs != &std::cout) delete ofs;

      return EXIT_SUCCESS;
    }

    catch (std::exception &e) {
      std::cerr << "FATAL: " << e.what () << std::endl;
      return EXIT_FAILURE;
    }
  }

  Chunker::~Chunker()
  {
    close ();
  }

  bool Chunker::close () 
  {
    if (features) {
      for (unsigned int i = 0; i < MAX_FEATURE_LEN; i++) delete [] features[i];
      delete [] features;
    }
    features = 0;
    features_size = 0;

    delete svm;
    svm = 0;

    delete feature_index;
    feature_index = 0;

    is_reverse      = false;
    is_write_header = false;
    is_partial      = false;
    is_verbose      = false;
    mode            = 0;
    selector_func   = 0;
    class_size      = 0;

    clear ();

    return true;
  }

  bool Chunker::clear ()
  {
    tag.clear();
    context.clear();
    dist.clear ();
    features_size = 0;
    return true;
  }

  std::string& Chunker::getFeature(int i, int j)
  {
    if (i < 0) {

      for (int k = - static_cast<int>(bos.size())-1; k >= i; k--) {
	char buf [32];
	std::ostrstream os (buf, 32);
	os << k << "__BOS__" << std::ends;
	bos.push_back(std::string(buf));
      }

      return bos[-i-1];

    } else if (i >= static_cast<int>(context.size())) {

      for (int k = 1 + eos.size(); k <= (i - static_cast<int>(context.size()) + 1); k++) {
	char buf [32];
	std::ostrstream os (buf, 32);
	os << '+' << k << "__EOS__" << std::ends;
	eos.push_back (std::string(buf));
      }

      return eos[i-context.size()];

    } else {

      return context[i][j];

    }
  }

  unsigned int Chunker::select (int i)
  {
    features_size = 0;
    if (selector_func) (*selector_func) (this, i);

    unsigned int l = features_size;

    for (unsigned int j = 0; j < feature_index->features.size(); j++) {
      std::ostrstream os (features[l], MAX_STR_LEN);
      os << "F:";
      if (feature_index->features[j].first >= 0) os << '+';
      os << feature_index->features[j].first
	 << ':'  << feature_index->features[j].second 
	 << ':'  <<  getFeature (i + feature_index->features[j].first, 
				 feature_index->features[j].second) << std::ends;
      l++;
    }

    for (unsigned int j = 0; j < feature_index->tags.size(); j++) {
      int k = i + feature_index->tags[j];
      if (k >= 0) {
	std::ostrstream os (features[l], MAX_STR_LEN);
	os << "T:" << feature_index->tags[j] << ':' << tag[k] << std::ends;
	l++;
      }
    }

    return l;
  }

  void Chunker::reverse()
  {
    if (! is_reverse) return;
    std::reverse (context.begin(), context.end());
    std::reverse (tag.begin(),     tag.end());
    std::reverse (dist.begin(),    dist.end());
  }

  bool Chunker::setSelector (int (*func)(Chunker *, int))
  {
    selector_func  = func;
    return true;
  }

  unsigned int Chunker::addFeature (char *s)
  {
    strncpy (features[features_size], s, MAX_STR_LEN);
    features_size++;
    return features_size;
  }

  unsigned int Chunker::add (std::vector <std::string> &s)
  {
    context.push_back (s);
    return context.size ();
  }

  unsigned int Chunker::add (std::string &line) 
  {
    std::vector <std::string> column;
    unsigned int s = split_string (line, "\t ", column);
    if (column_size == 0) column_size = s;
    for (; s < column_size; s++) column.push_back ("");
    return add (column);
  }

  std::istream& Chunker::read (std::istream &is)
  {
    try {

      clear();

      std::string line;

      for (;;) {

	if (! std::getline (is, line)) {
	  is.clear (std::ios::eofbit|std::ios::badbit);
	  return is;
	}

	if (line == "\t" || line == "" || line == "EOS") break;
	add (line);
      }

      return is;
    }

    catch (std::exception &e) {
      _what = std::string ("Chunker::read(): ") + e.what ();
      is.clear (std::ios::eofbit|std::ios::badbit);
      return is;
    }
  }

  std::ostream& Chunker::write (std::ostream &os)
  {
    try {
      switch (mode) {
      case 0: return is_verbose ? writeDetail (os) : writeNormal (os);
      case 1: return writeSelect (os);
      }
      return os;
    }

    catch (std::exception &e) {
      _what = std::string ("Chunker::write(): ") + e.what ();
      os.clear (std::ios::eofbit|std::ios::badbit);
      return os;
    }
  }

  bool Chunker::parse (std::istream &is, std::ostream &os)
  {
    if (! read (is)) return false;
    if (! parse())   return false;
    write (os);
    return true;
  }

  bool Chunker::parse ()
  {
    try {

      switch (mode) {
      case 0: return  is_verbose ? parseDetail () : parseNormal ();
      case 1: return parseSelect ();
      }

      return true;
    }

    catch (std::exception &e) {
      _what = std::string ("Chunker::parse(): ") + e.what ();
      throw std::runtime_error (_what);
      return false;
    }
  }

  bool Chunker::parseSelect ()
  {
    if (column_size <= 1) 
      throw std::runtime_error ("answer tags are not defined");

    if (! feature_index) {
      feature_index = new FeatureIndex;
      feature_index->setFeature (feature, column_size-1);
    }

     
    for (unsigned int i = 0; i < size(); i++) 
      tag.push_back (context[i][column_size-1]); // push last column

    reverse ();

    return true;
  }

  std::ostream& Chunker::writeSelect (std::ostream &os) 
  {
    if (! is_write_header) {

      if (column_size <= 1) 
	throw std::runtime_error ("answer tags are not defined");

      if (! feature_index) {
	feature_index = new FeatureIndex;
	feature_index->setFeature (feature, column_size-1);
      }

      os << "Version: "           << VERSION << std::endl;
      os << "Package: "           << PACKAGE << std::endl;
      os << "Parsing_Direction: " << (is_reverse ? "backward" : "forward") << std::endl;
      os << "Feature_Parameter: " << feature << std::endl;
      os << "Column_Size: "       << column_size-1 << std::endl; // NOTE: must -1; last colum is ANSWER

      os << "Tag_Features:";
      for (unsigned int i = 0; i < feature_index->tags.size(); i++) 
	os << ' ' << feature_index->tags[i];
      os << std::endl;

      os << "Features:";
      for (unsigned int i = 0; i < feature_index->features.size(); i++) 
	os << ' ' << feature_index->features[i].first << ":" << feature_index->features[i].second;

      os << std::endl << std::endl;

      is_write_header = true;
    }

     for (unsigned int i = 0; i < size(); i++) {
      os << tag[i];
      unsigned int size = select (i);
      for (unsigned int j = 0; j < size; j++) os << ' ' << features[j];
      os << std::endl;
    }

    os << std::endl;

    return os;
  }
}

#define _YAMCHA_PARSE_DETAIL
#include "chunkersub.h"
#undef _YAMCHA_PARSE_DETAIL
#include "chunkersub.h"

//
//  CRF++ -- Yet Another CRF toolkit
//
//  $Id: stream_wrapper.h 1588 2007-02-12 09:03:39Z taku $;
//
//  Copyright(C) 2005-2007 Taku Kudo <taku@chasen.org>
//
#ifndef CRFPP_STREAM_WRAPPER_H_
#define CRFPP_STREAM_WRAPPER_H_

#include <iostream>
#include <fstream>
#include <cstring>
#include "common.h"

namespace CRFPP {

class istream_wrapper {
 private:
  std::istream* is;
 public:
  std::istream &operator*() const  { return *is; }
  std::istream *operator->() const { return is;  }
  std::istream *get() { return is; }
  explicit istream_wrapper(const char* filename): is(0) {
    if (std::strcmp(filename, "-") == 0)
      is = &std::cin;
    else
      is = new std::ifstream(WPATH(filename));
  }

  ~istream_wrapper() {
    if (is != &std::cin) delete is;
  }
};

class ostream_wrapper {
 private:
  std::ostream* os;
 public:
  std::ostream &operator*() const  { return *os; }
  std::ostream *operator->() const { return os;  }
  std::ostream *get() { return os; }
  explicit ostream_wrapper(const char* filename): os(0) {
    if (std::strcmp(filename, "-") == 0)
      os = &std::cout;
    else
      os = new std::ofstream(WPATH(filename));
  }

  ~ostream_wrapper() {
    if (os != &std::cout) delete os;
  }
};
}

#endif

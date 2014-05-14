
#ifndef ECARGS_H
#define ECARGS_H

#include <list.h>
#include "ECString.h"

class ECArgs
{
 public:
  ECArgs(int argc, char *argv[]);
  int nargs() { return nargs_; }
  bool isset(char c);
  ECString value(char c);
  ECString arg(int n) { return argList[n]; }
 private:
  int nargs_;
  int nopts_;
  ECString argList[32];
  list<ECString> optList;
};
  

#endif /* ! ECARGS_H */

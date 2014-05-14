
#ifndef FEAT_H
#define FEAT_H

#include "Feature.h"

class FeatureTree;
#define ISCALE 1
#define PARSE 2

class Feat
{
public:
  friend ostream& operator<< ( ostream& os, Feat& t );
  float& g() { return g_; }
  //float& lambda() { return uVals[1]; }
  //float& u(int i) { return uVals[i+1]; }
  //int& cnt() { return cnt_; }
  int& ind() { return ind_; }
  int ind_;
  //int cnt_;
  //float* uVals;
  float g_;
  static int Usage;
};

#endif /* ! FEAT_H */

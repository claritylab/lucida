
#ifndef FEATURETREE_H
#define FEATURETREE_H

#include <assert.h>
#include <fstream.h>
#include <iostream.h>
#include "Feat.h"
#include "FBinaryArray.h"

class FeatureTree;

#define ROOTIND -99
#define AUXIND -9
#define NULLIND 9999999
class FeatureTree
{
public:
  FeatureTree() :auxNd(NULL), back(NULL),
    ind_(NULLIND), count(0) {}
  FeatureTree(int i) : ind_(i), auxNd(NULL), back(NULL),
    count(0){}
  FeatureTree(int i, FeatureTree* b)
    : ind_(i), auxNd(NULL), back(b) {}
  FeatureTree(istream& is);
  //FeatureTree(istream& is, istream& idxs, int asVal);
  void read(istream& is, FTypeTree* ftt);
  int  readOneLevel0(istream& is, int c);
  FeatureTree* follow(int val, int auxCnt);
  static FeatureTree* roots(int which) { return roots_[which]; }
  void         printFfCounts(int asVal, int depth, ostream& os);
  friend ostream&  operator<<(ostream& os, const FeatureTree& ft);

  int ind() const { return ind_; }
  int ind_;
  int count;
  //int specFeatures;
  FeatureTree* back;
  FeatureTree* auxNd;
  FBinaryArray feats;
  FTreeBinaryArray subtree;
 private:
  static FeatureTree* roots_[20];
  void othReadFeatureTree(istream& is, FTypeTree* ftt, int cnt);
  void printFfCounts2(int asVal, int depth, ostream& os);
};

#endif /* ! FEATURETREE_H */

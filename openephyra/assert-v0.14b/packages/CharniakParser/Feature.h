#ifndef FEATUREFUNCTION_H
#define FEATUREFUNCTION_H

#include "ECString.h"
#include <list.h>
#include <assert.h>

#define MAXNUMFS 30
#define NUMCALCS 9

#define RCALC 0
#define HCALC 1
#define UCALC 2
#define MCALC 3
#define LCALC 4
#define LMCALC 5
#define RUCALC 6
#define RMCALC 7
#define TTCALC 8

class FTypeTree;

class FTypeTree
{
 public:
  FTypeTree() : back(NULL),left(NULL), right(NULL), n(-1) {}
  FTypeTree(int fi) : back(NULL), left(NULL), right(NULL), n(fi) {}
  int n;
  FTypeTree* left;
  FTypeTree* right;
  FTypeTree* back;
};

class FullHist;

  /*  Currently what goes in Funs.
    0 t  tree_term 0 |
    1 l  tree_parent_term
    2 u  tree_pos
    3 h  tree_head
    4 ph tree_parent_head
    */

/*
  num  name  function
  0    t      0
  1    l      1
  2    u      2
  */
class SubFeature
{
 public:
  SubFeature(int i, ECString& nm, int fnn, list<int> fl)
    : num(i), name(nm), usf(fnn), fun(Funs[fnn]), featList(fl) {}
  static SubFeature*& fromInt(int i, int which) { return array_[which][i]; }
  int num;
  ECString name;
  int (*fun)(FullHist*);
  int usf;
  list<int> featList;
  static int total[NUMCALCS];
  static int (*Funs[MAXNUMFS])(FullHist*);
  static int (*PRFuns[2])(int);
  static int      ufArray[NUMCALCS][MAXNUMFS];
  static int      splitPts[NUMCALCS][MAXNUMFS];
 private:
  static SubFeature* array_[NUMCALCS][MAXNUMFS];
};

/*
  num name ff startpos
  1   rt   0  0
  2   rtl  1  1
  3   rtu  2  1
  */
class Feature
{
 public:
  Feature(int i, ECString& nm, int ff, int pos, int cpr)
    : num(i), name(nm), subFeat(ff), startPos(pos), auxCnt(0),
      condPR(cpr) {}
  static Feature*& fromInt(int i, int which)
    {
      assert(i > 0);
      return array_[which][i-1];
    }
  int num;
  ECString name;
  int subFeat;
  int usubFeat;
  int startPos;
  int condPR;
  int auxCnt;
  static void assignCalc(ECString conditioned);
  static int total[NUMCALCS];
  static int conditionedFeatureInt[NUMCALCS];
  static void init(ECString& path, ECString& conditioned);
  static void readLam(int which, ECString tmp, ECString path);
  static int whichInt;
  static int assumedFeatVal;
  static int (*conditionedEvent)(FullHist*);
  static int (*assumedSubFeat)(FullHist*);
  //e.g., when processing rules for NP, it would be 55;
  static float getLambda(int wi, int featInt, int bucketInt)
    {return lambdas_[wi][featInt-1][bucketInt];}
  static void setLambda(int wi, int featInt, int bucketInt, float val)
    { lambdas_[wi][featInt-1][bucketInt] = val;}
  static FTypeTree ftTree[NUMCALCS];
  static FTypeTree* ftTreeFromInt[NUMCALCS][MAXNUMFS];
  static void  createFTypeTree(FTypeTree* ft, int n, int which);
 private:
  static Feature* array_[NUMCALCS][MAXNUMFS];
  static float* lambdas_[NUMCALCS][MAXNUMFS];
};


#endif				/* ! FEATUREFUNCTION_H */

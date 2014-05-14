
#include <fstream.h>
#include <iostream.h>
#include "Feature.h"
#include "ECString.h"
#include "utils.h"
#include "FeatureTree.h"

SubFeature* SubFeature::array_[NUMCALCS][MAXNUMFS];
int SubFeature::total[NUMCALCS];
int (*SubFeature::Funs[MAXNUMFS])(FullHist*);
int (*SubFeature::PRFuns[2])(int);
int      SubFeature::ufArray[NUMCALCS][MAXNUMFS];
int      SubFeature::splitPts[NUMCALCS][MAXNUMFS];
Feature* Feature::array_[NUMCALCS][MAXNUMFS];
int Feature::total[NUMCALCS];
float*     Feature::lambdas_[NUMCALCS][MAXNUMFS];
int Feature::conditionedFeatureInt[NUMCALCS];
int  Feature::whichInt;
int Feature::assumedFeatVal;
int (*Feature::conditionedEvent)(FullHist*);
int (*Feature::assumedSubFeat)(FullHist*);
FTypeTree Feature::ftTree[NUMCALCS];
FTypeTree* Feature::ftTreeFromInt[NUMCALCS][MAXNUMFS];

int MinCount = 2;

void
Feature::
assignCalc(ECString conditioned)
{
  if(conditioned == "h") whichInt = HCALC;
  else if(conditioned == "u") whichInt = UCALC;
  else if(conditioned == "r") whichInt = RCALC;
  else if(conditioned == "ru") whichInt = RUCALC;
  else if(conditioned == "rm") whichInt = RMCALC;
  else if(conditioned == "tt") whichInt = TTCALC;
  else if(conditioned == "l") whichInt = LCALC;
  else if(conditioned == "lm") whichInt = LMCALC;
  else
    {
      assert(conditioned == "m");
      whichInt = MCALC;
    }
}

void
Feature::
init(ECString& path, ECString& conditioned)
{
  assignCalc(conditioned);
  int f;
  for(f = 0 ; f < MAXNUMFS ; f++)
    {
      float* vec = new float[15];
      lambdas_[whichInt][f] = vec;
      for(int k = 0 ; k < 15 ; k++) vec[k] = 0.0;
    }
  ECString dataECString(path);
  dataECString += "featInfo.";
  dataECString += conditioned;
  ifstream dataStrm(dataECString.c_str());
  assert(dataStrm);

  int auxCnts[MAXNUMFS];
  int i;
  for(i = 0 ; i < MAXNUMFS ; i++) auxCnts[i] = 0;
  Feature::ftTreeFromInt[whichInt][0] = &(Feature::ftTree[whichInt]);

  int conditionedInt;
  dataStrm >> conditionedInt;
  conditionedFeatureInt[whichInt] = conditionedInt;
  int num;
  for(num = 0 ;  ; num++)
    {
      int n, subf, pos, cpr;
      ECString nm;
      ECString tmp;
      dataStrm >> tmp;
      if(tmp == "--") break;
      n = atoi(tmp.c_str());
      dataStrm >> nm;
      dataStrm >> subf;
      dataStrm >> pos;
      dataStrm >> tmp;
      
      if(tmp == "|")
	cpr = -1;
      else
	{
	  cpr = atoi(tmp.c_str());
	  dataStrm >> tmp;
	  assert(tmp == "|");
	}
      array_[whichInt][n-1] = new Feature(n, nm, subf, pos, cpr);
      array_[whichInt][n-1]->auxCnt = auxCnts[pos];
      auxCnts[pos]++;
      createFTypeTree(Feature::ftTreeFromInt[whichInt][pos], n, whichInt);
    }
  Feature::total[whichInt] = num;
  for(num = 0 ;  ; num++)
    {
      int n, fn;
      ECString nm;
      ECString tmp;
      dataStrm >> tmp;
      if(tmp == "--") break;
      n = atoi(tmp.c_str());
      dataStrm >> nm;
      dataStrm >> fn;
      list<int> featList;
      for( ; ; )
	{
	  dataStrm >> tmp;
	  if(tmp == "|") break;
	  int f = atoi(tmp.c_str());
	  featList.push_back(f);
	}
      SubFeature::fromInt(n, whichInt)
	= new SubFeature(n, nm, fn, featList);
      assert(SubFeature::fromInt(n, whichInt));
    }
  SubFeature::total[whichInt] = num;
  /* set the universal function num on feats from their subfeats */
  for(num = 0 ; num < Feature::total[whichInt] ; num++)
    {
      Feature* f = array_[whichInt][num];
      f->usubFeat = SubFeature::fromInt(f->subFeat,whichInt)->usf;
    }
  /* set up the table from universal subfeat nums to subfeat nums */
  for(num = 0 ; num < MAXNUMFS ; num++)
    SubFeature::ufArray[whichInt][num] = -1;
  for(num = 0 ; num < SubFeature::total[whichInt] ; num++)
    {
      SubFeature* sf = SubFeature::fromInt(num,whichInt);
      SubFeature::ufArray[whichInt][sf->usf] = num;
    }
}

void
Feature::
readLam(int which, ECString tmp, ECString path)
{
  ECString ftstr(path);
  ftstr += tmp;
  ftstr += ".lambdas";
  ifstream fts(ftstr.c_str());
  assert(fts);
  int b,f;
  for(b = 1; b < 15 ; b++)
     {
       int bb;
       assert(fts);
       fts >> bb ;
       //cerr << bb << endl;
       assert(bb == b);
       for(f = 2; f <= Feature::total[which] ; f++)
	 {
	   float lam;
	   assert(fts);
	   fts >> lam;
	   //cerr << which << " " << f << " " << b << " " << lam << endl;
	   Feature::setLambda(which,f,b,lam);
	 }
     }
} 

void
Feature::
createFTypeTree(FTypeTree* posftTree, int n, int which)
{
  assert(posftTree);
  if(!posftTree->left)
    {
      posftTree->left = new FTypeTree(n);
      Feature::ftTreeFromInt[which][n] = posftTree->left;
    }
  else if(!posftTree->right)
    {
      posftTree->right = new FTypeTree(AUXIND);
      createFTypeTree(posftTree->right, n, which);
    }
  else createFTypeTree(posftTree->right, n, which);
}     

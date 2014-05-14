
#include "ccInd.h"
#include "InputTree.h"
#include "Term.h"

int
ccIndFromTree(InputTree* tree)
{
  InputTreesIter  subTreeIter = tree->subTrees().begin();
  ECString trm = tree->term();
  bool sawComma = false;
  int numTrm = 0;
  int pos = 0;
  for( ; subTreeIter != tree->subTrees().end() ; subTreeIter++ )
    {
      InputTree* subTree = *subTreeIter;
      if(pos != 0 && (subTree->term() == "CC" || subTree->term() == "CONJP"))
	return 1;
      if(subTree->term() == trm) numTrm++;
      if(pos != 0 && (subTree->term() == "," || subTree->term() == ":"))
	sawComma = true;
      pos++;
    }
  /* counts NP -> NP, NP as a conjuncion ??? */
  if(sawComma && numTrm >= 2) return 1;
  return 0;
}

int
ltocc(int lspec, int ccInd)
{
  static int npcc = -1;
  static int vpcc = -1;
  static int scc = -1;
  if(npcc < 0)
    {
      npcc = Term::get("G1")->toInt();
      vpcc = Term::get("G2")->toInt();
      scc = Term::get("G3")->toInt();
    }
  if(!ccInd) return lspec;
  ECString nm = Term::fromInt(lspec)->name();
  if(nm == "NP") return npcc;
  else if(nm == "VP") return vpcc;
  else if(nm == "S") return scc;
  else return lspec;
}


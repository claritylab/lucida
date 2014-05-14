#include <set.h>
#include "headFinder.h"
#include "Term.h"
#include "InputTree.h"

set<ECString,less<ECString> > head1s;
set<ECString,less<ECString> > head2s;

void
readHeadInfo(ECString& path)
{
  ECString headStrg(path);
  headStrg += "headInfo.txt";
  ifstream headStrm(headStrg.c_str());
  assert(headStrm);

  ECString next, next2;
  headStrm >> next;
  assert(next == "1");
  int whichHeads = 1;
  while(headStrm)
    {
      headStrm >> next;
      if(!headStrm) break;
      if(next == "2")
	{
	  whichHeads = 2;
	  continue;
	}
      
      headStrm >> next2;
      if(!headStrm) error("Bad format for headInfo.txt");
      next += next2;
      if(whichHeads == 1) head1s.insert(next);
      else head2s.insert(next);
    }
}

int
headPriority(ECString lhsString, ECString rhsString, int ansPriority) 
{
  const Term* rhsTerm = Term::get(rhsString);
  if(!rhsTerm) return 11;
  ECString both(lhsString);
  both += rhsString;
  if(lhsString == "PP" && ansPriority == 1) return 10;//make fst IN head of PP
  if(head1s.find(both) != head1s.end()) return 1;
  else if(ansPriority <= 2) return 10;
  else if(rhsString == lhsString)
    return 2; //lhs constit. e.g. np -> NP , np;
  else if(head2s.find(both) != head2s.end()) return 3;
  else if(ansPriority == 3) return 10;
  else if(rhsTerm->terminal_p() && !rhsTerm->isPunc()) return 4;
  else if(ansPriority == 4) return 10;
  else if(!rhsTerm->terminal_p() && rhsTerm->name() != "PP")
    return 5;
  else if(ansPriority == 5) return 10;
  else if(!rhsTerm->terminal_p()) return 6;
  else if(ansPriority == 6) return 10;
  else return 7;
}


int
headPosFromTree(InputTree* tree)
{
  int   ansPriority = 10;
  ECString lhsString(tree->term());
  if(lhsString == "") lhsString = "S1";
  int   pos = -1;
  int   ans = -1;

  ConstInputTreesIter subTreeIter = tree->subTrees().begin();
  InputTree   *subTree;
  for( ; subTreeIter != tree->subTrees().end() ; subTreeIter++ )
    {
      subTree = *subTreeIter;
      pos++;
      ECString rhsString(subTree->term());
      int nextPriority = headPriority(lhsString, rhsString, ansPriority);
      if(nextPriority <= ansPriority)
	{
	  ans = pos;
	  ansPriority = nextPriority;
	}
    }
  return ans;
}

 
#include "AnswerTree.h"

void
AnswerTree::
deleteSubTrees()
{
  cerr << "Should never be called" << endl;
  std::list<AnswerTree*>::iterator ati = subtrees.begin();
  for(; ati != subtrees.end() ; ati++)
    {
      AnswerTree* at = *ati;
      if(!at) continue;
      at->deleteSubTrees();
      delete at;
    }
}

AnswerTreePair&
atpFind(CntxArray& ca, AnswerTreeMap& atm)
{
  AnswerTreeMap::iterator atpi = atm.find(ca);
  if(atpi == atm.end())
    {
      AnswerTreePair atp(-1.0, (AnswerTree*)NULL);
      atm[ca] = atp;
      return atm[ca];
    }
  else return (*atpi).second;
}


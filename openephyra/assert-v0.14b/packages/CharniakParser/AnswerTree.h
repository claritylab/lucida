
#ifndef ANSWERTREE_H
#define ANSWERTREE_H

#include "Edge.h"
#include <list>
#include "CntxArray.h"

class AnswerTree;
class
AnswerTree
{
public:
  AnswerTree(Edge* ed) : e(ed){}
  void extend(AnswerTree* at) { subtrees.push_back(at); }
  void deleteSubTrees();
  Edge* e;
  std::list<AnswerTree*> subtrees;
};

typedef pair<double,AnswerTree*> AnswerTreePair;
typedef map<CntxArray, AnswerTreePair, less<CntxArray> > AnswerTreeMap;

AnswerTreePair& atpFind(CntxArray& hi, AnswerTreeMap& atm);

#endif


#ifndef FULLHIST_H
#define FULLHIST_H

#include "Edge.h"
#include <list.h>
#include "Wrd.h"

class FullHist;
class
FullHist
{
public:
  FullHist() : e(NULL), back(NULL) {}
  FullHist(Edge* e1) : e(e1), back(NULL){}
  FullHist(int tint, FullHist* fh) : term(tint), back(fh), pos(-1),hd(NULL){}
  FullHist* extendByEdge(Edge* e1);
  FullHist* extendBySubConstit();
  FullHist* retractByEdge();
  friend ostream& operator<<(ostream& os, const FullHist& fh);
  int hpos;
  int pos;
  int term;
  int preTerm;
  Edge* e;
  const Wrd* hd;
  FullHist* back;
  list<FullHist*> subtrees;  
  list<FullHist*>::iterator sti;
};

#endif

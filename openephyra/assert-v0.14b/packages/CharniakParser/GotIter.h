
#ifndef GOTITER_H
#define GOTITER_H

#include "Edge.h"
#include "Item.h"

class           GotIter
{
 public:
  GotIter(Edge* edge);
  bool     next(Item*& itm);
 private:
  Edge*        whereIam;
};

class           LeftRightGotIter
{
 public:
  LeftRightGotIter(Edge* edge);
  bool    next(Item*& itm);
  Item*   index(int i) const { assert(i < 128); return lrarray[i]; }
  int     size() const { return size_; }
 private:
  void         makelrgi(Edge* edge);
  Item*        lrarray[128];
  int          pos_;
  int          size_;
};

class           SuccessorIter
{
 public:
  SuccessorIter(Edge* edge) : edge_(edge), edgeIter( edge->sucs().begin() ) {}
  bool    next(Edge*& itm);
 private:
  Edge*  edge_;
  list<Edge*>::iterator edgeIter;
};
    
class          NeedmeIter
{
 public:
  NeedmeIter(Item* itm);
  bool    next(Edge*& e);
 private:
  int          stackptr;
  Edge*        stack[64000]; //??? was 32;
};
  
#endif	/* ! GOTITER_H */

#include "GotIter.h"

GotIter::
GotIter(Edge* edge) : whereIam( edge )
{}


bool
GotIter::
next(Item*& itm)
{
  if(!whereIam || !whereIam->item()) return false;
  else
    {
      itm = whereIam->item();
      whereIam = whereIam->pred();
      return true;
    }
}

LeftRightGotIter::
LeftRightGotIter(Edge* edge)
{
  makelrgi(edge);
}

void
LeftRightGotIter::
makelrgi(Edge* ri)
{
  GotIter gi(ri);
  Item* itm;
  bool finishedRight = false;
  int spos = ri->start();
  /* gotiter return a b head c d in the order d c a b head */
  list<Item*>::iterator lri;
  list<Item*> lrlist;
  while(gi.next(itm))
    {
      //cerr << "lrgi " << *itm << endl;
      if(finishedRight || itm->start() == spos)
	{
	  // if 1st consits is STOP(3, 3) then next can have same start pos.
	  if(itm->start() == spos && !finishedRight)
	    {
	      finishedRight = true;
	      lri = lrlist.begin();
	    }
	  lri = lrlist.insert(lri, itm);
	  lri++;
	}
      else lrlist.push_front(itm);
    }
  lri = lrlist.begin();
  int i = 0;
  for( ; lri != lrlist.end() ; lri++)
    {
      assert(i < 127);
      lrarray[i] = (*lri);
      i++;
    }
  size_ = i;
  pos_ = 0;
}


bool
LeftRightGotIter::
next(Item*& itm)
{
  if(pos_ >= size_) return false;
  itm = lrarray[pos_];
  pos_++;
  return true;
}

bool
SuccessorIter::
next(Edge*& edge)
{
  if(edgeIter == edge_->sucs_.end()) return false;
  edge = *edgeIter;
  //cerr << "Si: " << *edge_ << " has " << *edge << endl; 
  edgeIter++;
  return true;
}
    
NeedmeIter::
NeedmeIter(Item* itm)
{
  stackptr = -1;
  //cerr << "nmi for " << *itm << endl;
  list<Edge*>::iterator eiter = itm->needme().begin();
  for( ; eiter != itm->needme().end() ; eiter++)
    {
      stackptr++;
      assert(stackptr < 64000); //was 32;
      stack[stackptr] = *eiter;
    }
}

bool
NeedmeIter::
next(Edge*& e)
{
  if(stackptr < 0 ) return false;
  e = stack[stackptr];
  //cerr << "nminext = " << *e << endl;
  stackptr--;
  SuccessorIter si(e);
  Edge* suc;
  while(si.next(suc))
    {
      stackptr++;
      assert(stackptr < 64000); //was 32.
      //cerr << "nmisuc " << *suc << endl;
      stack[stackptr] = suc;
    }
  return true;
}
	

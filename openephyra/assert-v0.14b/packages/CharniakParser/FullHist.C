
#include "FullHist.h"
#include "GotIter.h"

FullHist*
FullHist::
extendBySubConstit()
{
  list<FullHist*>::iterator& bsti = back->sti;
  bsti++;
  if(bsti != back->subtrees.end()) return *bsti;
  else return back;
}

FullHist*
FullHist::
extendByEdge(Edge* e1)
{
  //cerr << "ebe " << *e1 << endl;
  e = e1;
  LeftRightGotIter gi(e1);
  Item* itm;
  while(gi.next(itm))
    {
      int termInt = itm->term()->toInt();
      //cerr << "ebei " << termInt << endl;
      FullHist* st = new FullHist(termInt, this);
      subtrees.push_back(st);
    }
  sti = subtrees.begin();
  //cerr << "ebe ret " << **sti << endl;
  return *sti;
}

FullHist*
FullHist::
retractByEdge()
{
  assert(sti == subtrees.end());
  list<FullHist*>::iterator stI = subtrees.begin();
  for(; stI != subtrees.end() ; stI++)
    {
      delete *stI;
    }
  while(!subtrees.empty())
    subtrees.pop_front();
  return this;
}
  
  
ostream&
operator<<(ostream& os, const FullHist& fh)
{
  FullHist* bfh = fh.back;
  if(bfh)
    {
      os << bfh->term << "/" << *bfh->hd << "--";
    }
  os << fh.term << "/" ;
  if(fh.hd) os << *fh.hd;
  return os;
}

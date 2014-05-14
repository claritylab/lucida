
#include "Edge.h"
#include "Item.h"	// need Item, Items definitions
#include "GotIter.h"
#include <algo.h>
#include "Bchart.h"
#include <math.h>

float Edge::DemFac = 0.999;
#define EDGE_CHUNKSIZE		1000
//int Edge::numEdges = 0;

//Edge *	Edge::edge_cache_;


Edge::
~Edge()
{   
  //numEdges--;
}

bool
Edge::
check()
{
  GotIter gi(this); 
  Item* f;
  while(gi.next(f))
    assert(f->finish() == 0 || f->finish() > f->start());
  return true;
}


int
Edge::
ccInd()
{

  const Term* trm = lhs();
  bool sawComma = false;
  int numTrm = 0;

  Item* itm;
  LeftRightGotIter gi(this);  
  int pos = 0;
  while( gi.next(itm) )
    {
      const Term* subtrm = itm->term();
      if(subtrm == Term::stopTerm) continue;
      if(subtrm == trm) numTrm++;
      const ECString& nm = subtrm->name();
      if(pos != 0 && (nm == "CC" || nm == "CONJP")) return 1;
      if(pos != 0 && (nm == "," || nm == ":")) sawComma = true;
      pos++;
    }
  if(sawComma && numTrm >= 2) return 1;
  return 0;
}


Edge::
Edge(ConstTerm* trm)
: lhs_( trm ), loc_( -1 ),
  pred_(NULL), finishedParent_( NULL ), item_(NULL),
  status_(0), start_(-1),
  heapPos_(-1),
  demerits_(0),
  prob_(1.2) // encourage constits???
{
  //numEdges++;
  if(lhs_->name() == "VP") prob_ *= 1.4; //???;
  // VPs are badly modeled by system;
}

Edge::
Edge( Edge& src, Item& itm, int right )
: lhs_( src.lhs_ ), loc_( src.loc_ ),  start_(src.start_),
  finishedParent_( NULL ),
  status_(right),
  item_(&itm),
  heapPos_(-1),
  demerits_(src.demerits_),
  leftMerit_(src.leftMerit()),
  rightMerit_(src.rightMerit()),
  prob_(src.prob())
{
  //numEdges++;
  if(start_ == -1)
    start_ = itm.start();
  if(loc_ == -1) loc_ = itm.finish();
  if(right) loc_ = itm.finish();
  else start_ = itm.start();

  if(!src.item_) //it has no item
    {
      pred_ = NULL;
    }
  else
    {
      pred_ = &src;
      pred_->sucs_.push_front(this);
      //cerr << *pred_ << " has suc " << *this << endl;
    }
  prob_ *= itm.prob();
}

Edge::
Edge( Item& itm )
: lhs_( itm.term() ), loc_( itm.finish() ),  finishedParent_( &itm ),
  status_(2), pred_(NULL), start_(itm.start()),
  item_(NULL),
  heapPos_(-1),
  demerits_(0),
  prob_( itm.prob() ),
  leftMerit_(1),
  rightMerit_(1)
{
  //numEdges++;
}

void 
Edge::
print( ostream& os )
{
    if(!item_) //dummy rule
      {
	if(finishedParent_)
	  {
	    os << *finishedParent_ << " -> ";
	  }
	else
	  {
	    os << *lhs_ << " -> ";
	  }
      }
    else 
    {
        os << *lhs_ << "(" << start() << ", " << loc_ << ") -> ";
	LeftRightGotIter gi(this);
	const Term * tempRHS;
	Item* itm;
	while( gi.next(itm) )
	  {
	    if(itm->term() == Term::stopTerm) continue;
	    os << *itm << " ";
	  }
    }
}

void
Edge::
setmerit()
{
  merit_ = prob_*leftMerit_*rightMerit_*pow(DemFac,demerits_);
}

int
Edge::
headPos(int i /*=0*/)
{
  if(!pred()) return i;
  Edge* prd = pred();
  //cerr << "H " << *item() << endl;
  if(prd->start() > start()) return prd->headPos(i+1);
  else if(prd->start() == start()
	  && item()->term() == Term::stopTerm
	  && item()->start() == start())
    {
      //cerr << "HPST " << *(prd->item()) << " " << i << endl;
      return prd->headPos(i+1);
    }
  else return pred()->headPos(i);
}

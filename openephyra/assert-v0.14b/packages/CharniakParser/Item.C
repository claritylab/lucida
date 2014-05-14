#include "Item.h"
#include <iostream.h>

Item::
~Item()
{
  //cerr << "Deleting " << *this << endl;
  PosIter pi = posAndheads_.begin();
  for( ; pi != posAndheads_.end() ; pi++)
    {
      HeadMap& hm = (*pi).second;
      HeadMap::iterator hmi = hm.begin();
      for( ; hmi != hm.end() ; hmi++)
	{
	  //cerr << "HMI " << (*hmi).first << endl;
	  ItmGHeadInfo& igh = (*hmi).second;
	  AnswerTreeMap& atm = igh.second;
	  AnswerTreeMap::iterator atmi = atm.begin();
	  for( ; atmi != atm.end(); atmi++)
	    {
	      AnswerTreePair& atp= (*atmi).second;
	      if(atp.second) delete atp.second;
	    }
	}
    }
}

Item::
Item(
     //const Word* hd,
     const Term * _term, int _start, int _finish)
: //head_ (hd),
  term_( _term ), start_( _start ), finish_( _finish ),
  needme_(), ineed_(),
  prob_( 1.0 ), word_( NULL ), storeP_( 0.0 ), poutside_( 0.0 )
{}

ostream&
operator<< (ostream& os, const Item& item)
{
    os << *item.term() << "(" << item.start();
    os << ", " << item.finish();
    //os << ", " << item.head()->lexeme();
    os << ")";
    return os;
}

int
Item::
operator== (const Item& item) const
{
    return ( //head_ == item.head() &&
	    term_ == item.term()
	    && start_ == item.start()
	    && finish_ == item.finish());
}

void            
Item::
check()
{
  assert(start() < finish() || !finish());
  list<Edge*>::iterator nmIter = needme_.begin();
  list<Edge*>::iterator inIter = ineed_.begin();
  for( ; nmIter != needme_.end() ; nmIter++)
    (*nmIter)->check();
  for( ; inIter != ineed_.end() ; inIter++)
    (*inIter)->check();
}

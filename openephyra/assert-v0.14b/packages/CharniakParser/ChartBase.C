
#include "ChartBase.h"
#include <math.h>
#include "GotIter.h"

const double ChartBase::badParse = -1.0L;
int ChartBase::ruleiCountTimeout_ = 25000;
Items     ChartBase::regs[400][400];
list<Edge*> ChartBase::waitingEdges[2][400];

bool
ChartBase::
finalPunc(const char* wrd)
{
  if(strlen(wrd) >1) return false;
  if(strpbrk(wrd, ".!?:")) return true;
  return false;
}


extern float endFactor;
extern float midFactor;

float
ChartBase::
endFactorComp(Edge* dnrl)
{
  int start = dnrl->start();
  int finish = dnrl->loc();
  int effVal = effEnd(finish);
  ECString tnm(dnrl->lhs()->name());
  if((tnm == "S1" || tnm == "S") && finish == wrd_count_
     && start == 0)
    return endFactor;
  else if(effVal==1)
    return endFactor;
  else if(effVal == 0) return midFactor;
  else return .95;  //if effVal == 2, currently not used;
}

int
ChartBase::
effEnd(int pos)
{
  bool ans;
  const char* wrd = sentence_[pos].lexeme().c_str();
  if(pos > endPos) ans = 0;
  else if(pos == endPos) ans = 1;  //in case no final punc;
  else if(ChartBase::finalPunc(wrd) || !strcmp(wrd, ";")) ans = 1;
  else if(pos > wrd_count_ -3) ans = 0;
  else if(!strcmp(wrd,","))
    {
      if(sentence_[pos+1].lexeme() == "''")
	ans = 1; // ,'' acts like end of sentence;
      else ans = 0;  //ans = 2 for alt version???
    }
  else ans = 0;
  return ans;
}


ChartBase::
ChartBase(SentRep & sentence)
: crossEntropy_(0.0L), 
  wrd_count_(0),
  ruleiCounts_(0),
  popedEdgeCount_(0),
  sentence_( sentence )
{
#ifdef DEBUG
    extern int	rulei_high_water;
    rulei_high_water = 0;
#endif /* DEBUG */
    wrd_count_ = sentence.length();
    endPos = wrd_count_;
    const char* endwrd = sentence_[wrd_count_-1].lexeme().c_str();
    if(finalPunc(endwrd)) endPos = wrd_count_-1;
    else if(wrd_count_ > 2)
      {
	endwrd = sentence[wrd_count_-2].lexeme().c_str();
	if(finalPunc(endwrd)) endPos = wrd_count_-2;
	else
	  {
	    endwrd = sentence[wrd_count_-3].lexeme().c_str();
	    if(finalPunc(endwrd)) endPos = wrd_count_-3;
	  }
      }
}

// virtual
ChartBase::
~ChartBase()
{
  register int    i, j;

  for (i = 0; i <= wrd_count_; i++)
    {
      while(!waitingEdges[0][i].empty()) waitingEdges[0][i].pop_front();
      while(!waitingEdges[1][i].empty()) waitingEdges[1][i].pop_front();
      for (j = 0; j < wrd_count_; j++)
	{
	  free_chart_items(regs[i][j]);
	}
    }
  for(i = 0 ; i < pretermNum ; i++)
    {
      assert(pretermItems[i]);
      delete pretermItems[i];
    }
}

void
ChartBase::
free_edges(list<Edge*>& edges)
{
  list<Edge*>::iterator lei = edges.begin();
  for( ; lei != edges.end() ; lei++)
    delete (*lei);
}

void
ChartBase::
set_Alphas()
{
  Item           *snode = get_S();
  double         tempAlpha[400];
  
  if( !snode || snode->prob() == 0.0 )
    {
      warn( "estimating the counts on a zero-probability sentence" );
      return;
    }
  double sAlpha = 1.0/snode->prob();
  snode->poutside() = sAlpha;
  
  /* for each position in the 2D chart, starting at top*/
  /* look at every bucket of length j */
  for (int j = wrd_count_-1 ; j >= 0 ; j--)
    {
      for (int i = 0 ; i <= wrd_count_ - j ; i++)
	{
	  Items il = regs[j][i];
	  list<Item*>::iterator ili = il.begin();
	  Item* itm;
	  for(; ili != il.end(); ili++ )
	    {
	      itm = *ili;
	      if(itm != snode) itm->poutside() = 0; //init outside probs to 0;
	    }
	  
	  bool valuesChanging = true;
	  /* do alpha calulcations until values settle down */
	  ili = il.begin();
	  while(valuesChanging)
	    {
	      valuesChanging = false;
	      int            tempPos = 0;  //position in tempAlpha;
	      ili = il.begin();
	      for(; ili != il.end(); ili++ )
		{
		  itm = *ili;
		  if(itm == snode) continue;
		  double itmalpha = 0;

		  NeedmeIter nmi(itm);
		  Edge* e;
		  while( nmi.next(e) )
		    {
		      const Item* lhsItem = e->finishedParent();
		      if(lhsItem) itmalpha += lhsItem->poutside()
			                        * e->prob();
		    }
		  assert(tempPos < 400);
		  double val = itmalpha/itm->prob();
		  tempAlpha[tempPos++] = val;
		} 
	      /* at this point the new alpha values are stored in tempAlpha */
	      int temppos = 0;
	      ili = il.begin();
	      for(; ili != il.end(); ili++ )
		{
		  itm = *ili;
		  if(itm == snode) continue;
		  /* the start symbol for the entire sentence has poutside =1*/
		  if(i == 0 && j ==wrd_count_-1 &&
		     itm->term()->name() == "S1")
		    itm->poutside() = sAlpha;
		  else
		    {
		      double oOutside = itm->poutside();
		      double nOutside = tempAlpha[temppos];
		      if(nOutside == 0)
			{
			  if(oOutside != 0) error("Alpha went down");
			}
		      else if(oOutside/nOutside < .95)
			{
			  itm->poutside() = nOutside;
			  valuesChanging = true;
			  //cerr << "alpha*beta " << *itm << " = "
			  //<< (itm->poutside() * itm->prob()) << endl;
			}
		    }
		  temppos++;
		}
	      if(temppos != tempPos)
		{
		  cerr << "temppos = " << temppos << " and tempPos = "
		    << tempPos << " ";
		  error("Funnly situation in setAlphas");
		}
	    }
	}
    }
}

void
ChartBase::
free_chart_items(Items& itms)
{
    Item          *temp;

    int i = 0;
    while( !itms.empty() )
      {
	temp = itms.front();
	//temp->check();
	itms.pop_front();
	
	if(!temp->term()->terminal_p()) delete temp; //???;
      }
}


Item *
ChartBase::
get_S() const
{
    ECString temp("S1");
    const Term *    sterm = Term::get(temp);
    Item           *itm;

    Items il = regs[wrd_count_ - 1][0];
    Items::iterator ili = il.begin();
    for(; ili != il.end(); ili++ )
      {
	itm = *ili;
	if( itm->term() == sterm )
	    return itm;
      }
    return NULL;
}

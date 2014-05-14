#include "Bchart.h"
#include <math.h>
#include <iostream.h>
#include <stdiostream.h>
#include <fstream.h>
#include "GotIter.h"
#include "FeatureTree.h"

int   Bchart::printDebug_ = 0;
float Bchart::denomProbs[1000];
Item* Bchart::dummyItem = NULL;
int   Bchart::posStarts_[80][30];
Item* Bchart::stops[400];
extern LeftRightGotIter* globalGi;
extern int (*edgeFnsArray[19])(FullHist*);
float Bchart::bucketLims[14] =
  {0, .003, .01, .033, .09, .33, 1.01, 2.01, 5.1, 12, 30, 80, 200, 600};
float Bchart::pHcapgt_[100];
float Bchart::pHhypgt_[100];
Wwegt* Bchart::pHegt_;
float Bchart::pHugt_[100];
float Bchart::pT_[50];
int Bchart::egtSize_ = 0;
int    Bchart::curDemerits_[400][400];
map< ECString, int, less<ECString> > Bchart::wordMap;
float Bchart::timeFactor = 7.5;


Bchart::
Bchart(SentRep & sentence)
: ChartBase( sentence )
{
  pretermNum = 0;
  heap = new EdgeHeap();
  int len = sentence.length();
  int i,j;
  assert(len <= 400);
  char temp[250];
  for(i = 0 ; i < len ; i++)
    {
      ECString wl = toLower(sentence[i].lexeme().c_str(), temp);
      int val = wtoInt(wl);
      sentence_[i].toInt() = val;
    }
  for(i = 0 ; i < 400 ; i++)
    for(j = 0 ; j < 400 ; j++) curDemerits_[i][j] = 0;
  initDenom();
}

/// virtual
Bchart::
~Bchart()
{
  int i;
  for(i = 0 ; i < alreadyPopedNum ; i++)
    delete alreadyPoped[i];
  delete heap;
}

double
Bchart::
parse()
{
    alreadyPopedNum = 0;
    SentRepIter     sri(sentence_);
    
    bool   haveS = false;

    for (;;)
    {
      //check();
      if( ruleiCounts_ > ruleiCountTimeout_ )
	{
	  break;
	}

      if(get_S() && !haveS)
	{
	  // once we have found a parse, the total edes is set to edges * 3.5;
	  haveS = true;
	  if(printDebug(10)) cerr << "Found S " << popedEdgeCount_ << endl;
	  popedEdgeCountAtS_ = popedEdgeCount_;
	  totEdgeCountAtS_ = ruleiCounts_;
	  int newTime = (int)(ruleiCounts_ * timeFactor);  
	  if(newTime < ruleiCountTimeout_)
	    ruleiCountTimeout_ = newTime;
	}
      // We keep track of number of ruleis to decide when time out on parsing.;
      /* get best thing off of keylist */
      Edge* edge = heap->pop(); 
      if (!edge) break;
      int stus = edge->status();
      int cD = curDemerits_[edge->start()][edge->loc()];
      if(edge->demerits() < cD - 5 && !haveS)
	{
	  edge->demerits() = cD;
	  edge->setmerit();
	  heap->insert(edge);
	  continue;
	}
      if(alreadyPopedNum >= 400000)
	{
	  cerr << "alreadyPoped got too large" << endl;
	  break;
	}
      if(printDebug() > 60)
	{
	  cerr << popedEdgeCount_ << "\tPop";
	  if(stus == 0) cerr << "< ";
	  else if(stus == 2) cerr << ". ";
	  else cerr << "> ";
	  cerr << *edge << "\t" << edge->prob() << "\t" << edge->merit();
	  cerr << endl;
	}
      popedEdgeCount_++;
      alreadyPoped[alreadyPopedNum++] = edge;
      if(!haveS) addToDemerits(edge);
      /* and add it to chart */
      //heap->check();
      switch (stus)
	{
	  case 0 : add_edge(edge, 0); break; //0 => continuing left;
	  case 1 : add_edge(edge, 1); break; //1 => continung right;
	  case 2 : addFinishedEdge(edge);
	}
    }

    /* at this point we are done looking for edges etc. */
    Item           *snode = get_S();
    /* No "S" node means the sentence was unparsable. */
    if (!snode)
	return badParse;
    double          ans = snode->prob();

    if (ans <= 0.0L)
	error("zero probability parse?");
    /*
    ans = -log2(ans);
    if (ans == quiet_nan(0L))
	error("log returned quiet_nan()");
    */
    static double	nat_log_2 = log( 2.0 );
    ans = -log( ans )/ nat_log_2;
    crossEntropy_ = ans;
    return ans;
}

/* add_edge does 3 things.  Basic bookkeeping on ineeds and needmes, 
   (2) looks to see if the edge can be immediagely extended, and
   (3) adds a new art, for the position it is now looking at if there
   is not already one in the chart.
*/   
   
void
Bchart::
add_edge(Edge* edge, int right)
{
  if(printDebug() > 250)
    cerr << "add_edge " << *edge << endl;

  int     loc = right ? edge->loc() : edge->start() ;
  int     i;

  int     stoploc = right ? loc : loc-1;
  extend_rule(edge, stops[loc], right); 
  // Iterate over i = the length of the constituent -1.;
  // looking for a reg item of length i and starting position start;
  if(right)
    for( i = 0 ; i < wrd_count_ - loc ; i++ )
      already_there_extention(i, loc, right, edge);
  else
    for( i = 0 ; i < loc ; i++)
      already_there_extention(loc - i -1, i, right, edge);

  assert(loc >= 0 && loc < 400);
  waitingEdges[right][loc].push_back( edge ); 
}

void
Bchart::
already_there_extention(int i, int start, int right, Edge* edge)
{
  assert(i >= 0 && i < 400 && start >= 0 && start < 400);
  Items::iterator regsiter = regs[i][start].begin();
  for( ; regsiter != regs[i][start].end() ; regsiter++)
    {
      Item* item = *regsiter;
      extend_rule( edge, item, right );  
    }
}

void
Bchart::
put_in_reg(Item * itm)
{
    int             st = itm->start();
    int             diff = itm->finish() - st - 1;

    if( diff < 0 || st < 0  || diff > 400 || st > 400)
	error( "illegal indices in put_in_reg" );
    regs[diff][st].push_back( itm );
}

void
Bchart::
add_reg_item(Item * itm )
{
    if(printDebug() > 250) cerr << "add_reg_item " << *itm << endl;
    put_in_reg(itm);
    add_starter_edges(itm); 
//
// Look at the art for this item (i.e., it has the same Term and start). For
// each of the dotted rules which hope to have this item following the 'dot', 
// extend the rule.  
//
    
  for(int right = 0 ; right < 2 ; right++)
    {
      int pos = right ? itm->start() : itm->finish();
      //cerr<< "Look for " << *itm << " " << pos << " " << right << endl;
      Edges::iterator edgeIter = waitingEdges[right][pos].begin();
      for( ; edgeIter != waitingEdges[right][pos].end() ; edgeIter++ )
	{
	  Edge* edge = *edgeIter;
	  extend_rule(edge, itm, right);
	}
    }
}
  

//also need to make lhs of edge a term*, and give edge a start member.
void
Bchart::
add_starter_edges(Item* itm)
{
  if(printDebug() > 140)
    cerr << "add_starter_edges " << *itm << endl;
  ConstTerm* poslhs;
  int ht = itm->term()->toInt();
  int i;
  for(i = 0 ; ; i++)
    {
      int rt = posStarts(ht,i);
      if(rt < 0) break;
      poslhs = Term::fromInt(rt);
      Edge*  nedge = new Edge(poslhs);//???;
      extend_rule(nedge, itm, 0);  //adding head is like extending left;
    }
}

/*
 * To extend a rule we either make a new rule inst and add it to the chart,
 * or, if the rule becomes finished, we add a reg item corresponding to the
 * lhs of the rule to the keylist (we can also do both, because really
 * edges correspond to clusters of rules, one of which might be completed
 * and the others not.
 */

void
Bchart::
extend_rule(Edge* edge, Item * item, int right)
{
    if(printDebug() > 140)
      cerr << "extend_rule " << *edge << " " << *item << endl;
    Edge*          newEdge = new Edge(*edge, *item, right);
    const Term* itemTerm = item->term();
    LeftRightGotIter lrgi(newEdge);
    globalGi = &lrgi;
	
    if(edge->loc() == edge->start())
      {
	newEdge->prob() *= meEdgeProb(item->term(), newEdge, MCALC); 
	/*stoprightp is p of stopping after seeing what currently
	  passes for the rhs of the edge */
	newEdge->rightMerit() = computeMerit(newEdge,RUCALC);
	delete edge; // just created;
      }
    else if(right)
      {
	newEdge->prob() *=     meEdgeProb(item->term(),newEdge, RCALC);
      }
    else newEdge->prob() *= meEdgeProb(item->term(),newEdge, LCALC);
    if(right)
      {
	newEdge->rightMerit()  = computeMerit(newEdge,RMCALC);
      }
    else
      {
	/* this is the left boundary stat for constituents that are
	   continuing left,  given the label and
	   whatever currently appears on the left boundary of the constit.
	   we only need this when going left */
	newEdge->leftMerit() = computeMerit(newEdge,LMCALC);
      }

    if(itemTerm == Term::stopTerm) newEdge->status() = right ? 2 : 1;

    if(newEdge->status() == 2) newEdge->prob() *= endFactorComp(newEdge);

    if(printDebug() > 250 )
      cerr << "Constructed " << *newEdge << "\t"
	<< newEdge->leftMerit() << "\t"
	  << newEdge->prob() << "\t" << newEdge->rightMerit() << endl;
    int tmp = curDemerits_[newEdge->start()][newEdge->loc()];
    newEdge->demerits() = tmp;
    newEdge->setmerit(); 
    globalGi = NULL;
    if(newEdge->merit() == 0)
      {
	assert(alreadyPopedNum < 450000);
	alreadyPoped[alreadyPopedNum++] = newEdge;
	Edge* prd = newEdge->pred();
	if(prd) prd->sucs().pop_front();
	return;
      }
    ++ruleiCounts_;
    heap->insert(newEdge);

    if(itemTerm != Term::stopTerm) item->needme().push_back(newEdge);
}

void
Bchart::
addFinishedEdge(Edge* newEdge)
{
  if(printDebug() > 250)
    cerr << "addFinishedEdge " << *newEdge << endl;
  if(newEdge->finishedParent()
     && (newEdge->finishedParent()->term()->terminal_p()))
    {
      add_reg_item(newEdge->finishedParent());
      return;
    }
  Item           *regi;
  regi = in_chart( NULL, newEdge->lhs(),  
		  newEdge->start(), newEdge->loc());
  if(regi)
    {
      /* redoP is a crutial function.  It uses to probability of the edge
	 to see what the new prob of regi should be, and if it is over
	 the threshold for propogating probs, It will recursively
	 do this up the chart. */
      redoP(regi, newEdge->prob());
    }
  else
    {
      regi = new Item(newEdge->lhs(),
		      newEdge->start(), newEdge->loc());
      regi->prob() = newEdge->prob();  
      //regi->headp() = newEdge->headp();
      add_reg_item(regi);
    }
  
  if(newEdge->finishedParent())
    assert(newEdge->finishedParent() == regi);
  else newEdge->setFinishedParent( regi );    

  regi->ineed().push_back( newEdge );
  /* setFinishedParent tells newEdge that the consitutent that it
     build is regi */
}

Item*
Bchart::
in_chart(const Wrd* hd, const Term * trm, int start, int finish)
{
    Item           *itm;

    if( finish <= 0 || start < 0 || finish - start - 1 < 0 )
      {
	cerr << "For " << *trm << "(" << start << ", " << finish << ")"
	  << endl;
	error( "bogus boundary params in in_chart" );
      }
    Items::iterator regsIter = regs[finish - start - 1][start].begin();
    for( ; regsIter != regs[finish - start - 1][start].end(); ++regsIter )
    {
      itm = *regsIter;
      if (itm->term() == trm &&
	  //itm->head() == hd &&
	  itm->start() == start
	  && itm->finish() == finish)
	return itm;
    }
    return NULL;
}

void
Bchart::
redoP(Edge* edge, double probRatio)
{
  //cerr << "rpEdge " << *edge << endl;
  double oldEdgeP = edge->prob();
  //if(oldEdgeP == 0) cerr << "Zprob " << *edge << endl;
  if(edge->heapPos() >= 0) heap->del(edge);
  edge->prob() *= probRatio;
  edge->setmerit();
  if(edge->heapPos() >= 0) heap->insert(edge);
  //heap->check();
  if(edge->finishedParent())
    {
      redoP(edge->finishedParent(), edge->prob()-oldEdgeP);
    }
}

const double storeCutoff = .01;

/* probDiff is the new probability which should be added to the prob of
   item (which for an initially created item will be zero.  If probDiff
   plus whatever previously unused prob in item->storeP is over threshold
   then recurse */
void
Bchart::
redoP(Item *item, double probDiff)
{
  double oldItemP = item->prob();
  
  double itemStoreP = item->storeP() + probDiff;
  item->storeP() = itemStoreP;
  if ( oldItemP != 0.0 )
    if( itemStoreP / oldItemP  < storeCutoff )
      {
	return;
      }
  item->prob() += itemStoreP;
  //cerr << "P( " << *item << " ) goes from  " << oldItemP
    //<< " -> " << item->prob() << endl;
  item->storeP() = 0.0;
  if( oldItemP == 0.0 )
    {
      return;
    }
  double pRatio = item->prob() / oldItemP;
  
  NeedmeIter nmi(item);
  Edge* edge;
  while(nmi.next(edge))
    {
      redoP(edge,pRatio);
    }
}  

void
Bchart::
check()
{
  // for each position in the 2D chart, starting at top
  // look at every bucket of length j 
  for (int j = wrd_count_-1 ; j >= 0 ; j--)
    {
      for (int i = 0 ; i <= wrd_count_ - j ; i++)
	{
	  Items::iterator regsiter =  regs[j][i].begin();
	  Item* itm;
	  for( ; regsiter !=  regs[j][i].end(); ++regsiter )
	    {
	      itm = *regsiter;
	      itm->check();
	    }
	}
    }
}

int&
Bchart::
posStarts(int i, int j)
{
  assert(i < 80);
  assert(j < 30);
  return posStarts_[i][j];
}

void
Bchart::
setPosStarts()
{
  int i,j,k,l;
  for(i = 0 ; i < 80 ; i++)
    for(j = 0 ; j < 30 ; j++) posStarts(i,j) = -1;
  
  int numFor[80];
  for(i = 0 ; i < 80 ; i++) numFor[i] = 0;
  FeatureTree* ft = FeatureTree::roots(MCALC);
  for(k = 0 ; k < ft->subtree.size() ; k++)
    {
      FeatureTree* ft2 = ft->subtree.index(k);
      i = ft2->ind(); // i = rule term
      for(l = 0 ; l < ft2->feats.size() ; l++)
	{
	  Feat* f = ft2->feats.index(l);
	  j = f->ind(); //j = rule head term;
	  assert(numFor[j] < 30);
	  posStarts(j,numFor[j]) = i;
	  numFor[j]++;
	}
    }
}

float
Bchart::
meEdgeProb(const Term* trm, Edge* edge, int whichInt)
{
  FullHist fh(edge);
  float ans =  meFHProb(trm, fh, whichInt);
  return ans;
}

float
Bchart::
meFHProb(const Term* trm, FullHist& fh, int whichInt)
{
  Edge* edge = fh.e;
  int pos = 0;
  /* the left to right position we are working on is either the far left (0)
     or the far right */
  if(!globalGi) {}
  //else if(edge->item() != globalGi->index(0)) ;
  else if(whichInt == RUCALC || whichInt == RMCALC || whichInt == RCALC)
    pos = globalGi->size()-1;
  fh.pos = pos;

  int cVal = trm->toInt();
  if(printDebug() > 138)
    {
      cerr << "meP " << *trm << " " << cVal << " " << whichInt << " ";
      if(edge) cerr << *edge << endl;
      else cerr << fh.preTerm  << endl;
    }
  int subfVals[MAXNUMFS];
  FeatureTree* ginfo[MAXNUMFS];  
  ginfo[0] = FeatureTree::roots(whichInt);
  assert(ginfo[0]);
  float smoothedPs[MAXNUMFS];

  float ans = 1;

  for(int i = 1 ; i <= Feature::total[whichInt] ; i++)
    {
      ginfo[i] = NULL;
      Feature* feat = Feature::fromInt(i, whichInt); 
      /* e.g., g(rtlu) starts from where g(rtl) left off (after tl)*/
      int searchStartInd = feat->startPos;

      FeatureTree* strt = ginfo[searchStartInd];
      if(!strt)
	{
	  continue;
	}
      SubFeature* sf = SubFeature::fromInt(feat->subFeat, whichInt);
      int usf = sf->usf;
      int nfeatV = (edgeFnsArray[usf])(&fh);
      FeatureTree* histPt = strt->follow(nfeatV, feat->auxCnt); 
      ginfo[i] = histPt;
      if(i == 1)
	{
	  smoothedPs[0] = 1;
	  assert(histPt);
	  Feat* f =histPt->feats.find(cVal);
	  if(!f)
	    {
	      return 0.0;
	    }
	  smoothedPs[1] = f->g();
	  if(printDebug() > 238)
	    {
	      cerr << i << " " << nfeatV << " " << smoothedPs[1] << endl;
	    }
	  for(int j = 2; j <= Feature::total[whichInt] ; j++)
	    smoothedPs[j] = 0;
	  ans = smoothedPs[1];
	  continue;
	}
      if(nfeatV < -1)
	{
	  if(printDebug() > 128)
	    {
	      cerr<<"p"<<whichInt<< "(" << cVal << "|";
	      if(edge) cerr << *edge;
	      else cerr << fh.preTerm;
	      cerr << ") = " << ans << endl;
	    }
	  return ans;
	}
      if(!histPt)
	{
	  continue;
	}
      float estm = histPt->count * smoothedPs[1];
      int b = bucket(estm);

      Feat* ft = histPt->feats.find(cVal);
      float unsmoothedVal;
      if(!ft) unsmoothedVal = 0;
      else unsmoothedVal = ft->g();
      float lam = Feature::getLambda(whichInt, i, b);
      float uspathprob = lam*unsmoothedVal;
      float osmoothedVal = smoothedPs[searchStartInd];
      //float osmoothedVal = smoothedPs[i-1]; //for deleted interp.
      float smpathprob = (1-lam)*osmoothedVal;
      float nsmoothedVal = uspathprob+smpathprob;
      if(printDebug() > 238)
	{
	  cerr << i << " " << nfeatV << " " << usf << " "
	       << estm << " " << b <<" "<<unsmoothedVal << " " << lam << " " 
	       << nsmoothedVal <<  endl;
	}
      smoothedPs[i] = nsmoothedVal;
      ans *= (nsmoothedVal/osmoothedVal);
    }
  if(printDebug() > 128)
    {
      cerr<<"p"<<whichInt<< "(" << cVal << "|";
      if(edge) cerr << *edge;
      else cerr << fh.preTerm;
      cerr << ") = " << ans << endl;
    }
  return ans;
}

void
Bchart::
addToDemerits(Edge* edge)
{
  int st = edge->start();
  int fn = edge->loc();
  for(int i = st ; i < fn ; i++)
    {
      // e.g., for st = 3, fn = 5, we store at 3,4 3,5 and 4,5
      for(int j = i+1 ; j <= fn ; j++)
	curDemerits_[i][j]++;
    }
}

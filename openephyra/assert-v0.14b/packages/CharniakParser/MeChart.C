
#include "MeChart.h"
#include "fhSubFns.h"
#include "edgeSubFns.h"
#include "GotIter.h"
#include "InputTree.h"
#include "CntxArray.h"

bool sufficiently_likely(Edge* edge);
bool sufficiently_likely(const Item* itm);

extern float endFactor;
extern float midFactor;
LeftRightGotIter* globalGi = NULL;

int depth = 0;

void prDp()
{
  for(int i = 0 ; i < depth ; i++)
    cerr << " ";
}
  
void
MeChart::
init(ECString path)
{
  Feat::Usage = PARSE;
  addEdgeSubFeatureFns();
  addSubFeatureFns();

  ECString tmpA[NUMCALCS] = {"r","h","u","m","l","lm","ru","rm","tt"};

  for(int which = 0 ; which < NUMCALCS ; which++)
    {
      ECString tmp = tmpA[which];
      Feature::init(path, tmp); 
      ECString ftstr(path);
      ftstr += tmp;
      ftstr += ".g";
      ifstream fts(ftstr.c_str());
      if(!fts) cerr << "could not find " << ftstr << endl;
      assert(fts);
      FeatureTree* ft = new FeatureTree(fts); //puts it in root;
      Feature::readLam(which, tmp, path);
    }
  assert(CntxArray::sz == (Feature::total[UCALC] -1));
} 

AnswerTree*
MeChart::
findMapParse()
{
  if(printDebug() > 8)
    {
      prDp();
      cerr << "In findMapParse" << endl;
    }
  Item* s = topS();
  assert(s);
  fillInHeads();
  int s1Int = s->term()->toInt();
  FullHist s1Fh(s1Int,NULL);
  AnswerTreePair& atp = bestParse(s, &s1Fh);
  AnswerTree* ans = atp.second;
  //cerr << "SProb = " << atp.first << endl; //???;
  return ans;
}

AnswerTreePair&
MeChart::
bestParse(Item* itm, FullHist* h)
{
  AnswerTreePair& atp = recordedBP(itm, h);
  if(atp.first >= 0)
    {
      if(printDebug() > 19)
	{
	  prDp();
	  cerr << "already known bestParse(" << *itm << ", ...) has p = "
	       << atp.first << endl;
	}
      return atp;
    }
  atp.first = 0;
  if(printDebug() > 10)
    {
      prDp();
      cerr << "bestParse(" << *itm << ", ...)" << endl;
    }
  int itermInt = itm->term()->toInt();
  PosMap& pm = itm->posAndheads();
  PosIter pi = pm.begin();
  double bestP = 0;
  AnswerTree* bestAT = NULL;
  ECString bestW;
  int bestPos = -1;
  for( ; pi != pm.end() ; pi++ )
    {
      int posInt = (*pi).first;
      if(printDebug() > 16)
	{
	  prDp();
	  cerr << "consider Pos(" << *itm << ") = " << posInt << endl;
	}
      HeadMap& hm = (*pi).second;
      /* we are using collected counts for p(u|t) */
      float hposprob = 1;
      float chposprob = 1;
      /* if we have reached a preterminal, then termInt == posInt
	 and p(posInt|termInt) == 1 */
      if( itermInt != posInt)
	{
	  hposprob = meProb(posInt, h, UCALC); 
	  if(hposprob == 0) hposprob = .00001; //??? this can happen;
	  if(printDebug() > 16)
	    {
	      prDp();
	      cerr <<  "p(pos) = " <<  hposprob << endl;
	    }
	}
      h->preTerm = posInt;
      HeadIter hi = hm.begin(); 
      for( ;hi != hm.end();hi++)
	{
	  const Wrd& subhw = (*hi).first;
	  int wrdInt = subhw.toInt();
	  ECString subh = subhw.lexeme();
	  if(printDebug() > 16)
	    {
	      prDp();
	      cerr << "consider head(" << *itm << ") = " << subh << endl;
	    }
	  float hprob = 0;
	  if(wrdInt >= 0)
	    {
	      hprob = pCapgt(&subhw,posInt); 
	      hprob *= (1 - pHugt(posInt)); 
	      float hprob2 = meHeadProb(wrdInt, h);
	      hprob *= hprob2;
	    }
	  //hprob can be zero if lower case NNPS.
	  if(wrdInt < 0 || hprob == 0)
	    {
	      hprob = psutt(&subhw,posInt);
	    }
	  if(printDebug() > 16)
	    {
	      prDp();
	      cerr << "p(hd) = "<< hprob << endl;
	    }
	  float hhprob = (hposprob * hprob);
	  double nextP = hhprob; 
	  h->hd = &subhw;
	  AnswerTreePair&  
	    atp2 = bestParseGivenHead(posInt,subhw,itm,h,(*hi).second); 
	  double bestProbGHead = atp2.first;  
	  nextP *= bestProbGHead;
	  if(nextP > bestP)
	    {
	      //cerr << "setting bestP to " << nextP << endl;
	      bestP = nextP;
	      bestW = subh;
	      bestPos = posInt;
	      bestAT = atp2.second;
	      atp.first = nextP;
	      atp.second = bestAT;
	    }
	}
    }
  //assert(bestP > 0);
  //  atp.first = bestP;
  if(printDebug() > 10)
    {
      prDp();
      int subfv[MAXNUMFS];
      getHt(h, subfv);
      CntxArray ca(subfv);
      cerr << "Bestp for " << *itm << " = " << bestP
	   << " using " << bestW << " " << bestPos << " " << ca << endl;
    }
  //atp.second = bestAT;
  return atp;
}

int
firstPassE(Edge* e)
{
  return 1;
  /*
  Edge *e1 = e->pred();
  assert(e1);
  e1 = e1->pred();
  assert(e1);
  if(e1->item()->term()->terminal_p()) return 1;
  e1 = e1->pred();
  if(e1) return 1;
  return 0;
  */
}
    
AnswerTreePair&
MeChart::
bestParseGivenHead(int posInt, const Wrd& wd, Item* itm,
		   FullHist* h, ItmGHeadInfo& ighInfo)
{
  EdgeSet& es = ighInfo.first;
  AnswerTreeMap&  atm = ighInfo.second;
  int subfVals[MAXNUMFS];
  AnswerTreePair& atp = recordedBPGH(itm, atm, h);
  if(atp.first >= 0)
    {
      if(printDebug() > 19)
	{
	  int subfv[MAXNUMFS];
	  getHt(h, subfv);
	  CntxArray ca(subfv);
	  prDp();
	  cerr << "bpknown for " << posInt << ", " << wd
	       << ", " << *itm << ") : " << atp.first << " " << ca <<endl;
	}
      return atp;
    }
  if(itm->term()->terminal_p())
    {
      atp.first = 1;
      atp.second = NULL;
      return atp;
    }
  atp.first = 0;
  if(printDebug() > 10)
    {
      prDp();
      cerr << "bestParseGivenHead(" << posInt << ", " << wd
	   << ", " << *itm  << ")" << endl;
    }
  double bestP = 0;
  AnswerTree* bestAT = NULL;
  EdgeSetIter ei = es.begin();
  for(int passNum = 0 ; passNum < 2 ; passNum++)
    {
      ei = es.begin();
  for( ; ei != es.end() ; ei++)
    {
      Edge* e = *ei;
      int fp = firstPassE(e);
      if(fp && passNum == 1) continue;
      else if(!fp && passNum == 0) continue;
      if(!sufficiently_likely(e))
	{
	  continue;
	}

      float edgePg = 1;
      int finish = e->loc();

      int effVal = effEnd(finish);
      if(itm->term()->name() == "S1") edgePg = 1;
      else if(effVal == 1)
	edgePg = endFactor;
      else if(effVal == 0) edgePg = midFactor;
      h->e = e;
      if(printDebug() > 20)
	{
	  prDp();
	  cerr << "consid " << *e << endl;
	}
      
      float prob = meRuleProb(e,h);
      prob *= 1.1; //encourage constits;  //???
      AnswerTree*  nat = new AnswerTree(e);

      int headPos = h->hpos; //set during meRuleProb
      double nextP = prob * edgePg;
      Item* sitm;
      LeftRightGotIter gi(e); 
      int pos = 0;
      depth++;
      h = h->extendByEdge(e);
      while( gi.next(sitm) )
	{
	  //cerr << "Looking at " << *sitm << endl;
	  if(sitm->term() == Term::stopTerm)
	    {
	      pos++;
	      h = h->extendBySubConstit(); 
	      continue;
	    }
	  if(pos == headPos)
	    {
	      h->preTerm = posInt; 
	      h->hd = &wd;
	      ItmGHeadInfo& ighi = sitm->posAndheads()[posInt][wd]; 

	      AnswerTreePair
		atp = bestParseGivenHead(posInt,wd,sitm,h,ighi);
	      nextP *= atp.first;
	      nat->extend(atp.second);
	    }
	  else
	    {
	      AnswerTreePair atp = bestParse(sitm, h);
	      nextP *= atp.first;
	      nat->extend(atp.second);
	    }
	  if(printDebug() > 39)
	    {
	      prDp();
	      cerr << "FullHist from " << *h;
	    }
	  h = h->extendBySubConstit(); 
	  if(printDebug() > 39)
	    cerr << " -> " << *h << endl;
	  pos++;
	}
      if(printDebug() > 20)
	{
	  prDp();
	  cerr << "P(" << *e << " | " << wd << " ) = " << nextP;
	  cerr << "\n"; //???;
	}
      depth--;
      if(nextP <= bestP)
	{
	  if(printDebug() > 20) cerr << endl;
	  delete nat;
	}
      else
	{
	  if(bestAT) delete bestAT;
	  bestAT = nat;
	  bestP = nextP;
	  atp.first = bestP;
	  atp.second = bestAT;
	  if(printDebug() > 20) cerr << " NB" << endl;
	}
      h->retractByEdge(); 
    }
    }
  //assert(bestP > 0);
  //atp.first = bestP;
  //atp.second = bestAT;
  if(printDebug() > 10)
    {
      int subfv[MAXNUMFS];
      getHt(h, subfv);
      CntxArray ca(subfv);
      prDp();
      cerr << "Bestpgh for " << *itm << ", " << wd << " = " << bestP
	   << " " << ca << endl;
    }
  return atp;
}
void
MeChart::
fillInHeads()
{
  for (int j = 0 ; j <  wrd_count_ ; j++)
    {
      // now look at every bucket of length j 
      for (int i = 0 ; i < wrd_count_ - j ; i++)
	{
	  list<Item*>::iterator itmitr =regs[j][i].begin();
	  list<Item*> doover;
	  Item* itm;
	  for( ; itmitr != regs[j][i].end() ; itmitr++)
	    {
	      itm = *itmitr;
	      if(!sufficiently_likely(itm)) continue;
	      const Term* trm = itm->term();
	      int trmInt = trm->toInt();
	      if(trm->terminal_p())
		{
		  HeadMap& hm = itm->posAndheads()[trmInt]; 
		  hm[*itm->word()];
		  continue;
		}
	      else doover.push_back(itm);
	      headsFromEdges(itm);
	    }
	  bool cont = true;
	  int timesAgain = 0;
	  //;while(cont && timesAgain++ < 2)
	  while(cont && timesAgain++ < 4)
	    {
	      cont = false;
	      list<Item*>::iterator lii = doover.begin();
	      for( ; lii != doover.end() ; lii++)
		{
		  bool tmp = headsFromEdges(*lii);
		  if(tmp) cont = tmp;
		}
	      timesAgain++;
	    }
	}
    }
}

bool
MeChart::
headsFromEdges(Item* itm)
{
  bool ans = false;
  list<Edge*>::iterator eli = itm->ineed().begin();
  Edge* e;
  // for each edge we look for all of its possible head preterms, and all
  // of the possible heads, and file this edge for that case 
  for( ; eli != itm->ineed().end() ; eli++)
    {
      e = *eli;
      //cerr << *e << endl;
      if(!sufficiently_likely(e)) continue;
      Item* ehd = headItem(e);
      PosIter epi = ehd->posAndheads().begin();
      if(epi == ehd->posAndheads().end()) continue;
      for( ; epi != ehd->posAndheads().end() ; epi++ )
	{
	  int posInt = (*epi).first;
	  
	  if(itm->posAndheads().find(posInt) == itm->posAndheads().end())
	    ans = true;
	  HeadMap& ihm = itm->posAndheads()[posInt];
	  HeadMap& ehm = (*epi).second;
	  HeadIter ehi = ehm.begin();
	  for( ; ehi != ehm.end() ; ehi++ )
	    {
	      const Wrd& hd = (*ehi).first;
	      if(ihm.find(hd) == ihm.end())
		{
		  if(printDebug() > 16)
		    {
		      prDp();
		      cerr << "attach hd " << *itm << " " << hd << endl;
		    }
		  ans = true;
		}
	      EdgeSet& se = ihm[hd].first;
	      //cerr << "Insrting " << *e << " for itm = " << *itm << endl;
	      se.insert(e);
	    }
	}
    }
  return ans;
}

Item *
MeChart::
headItem(Edge* edge)
{
  if(!sufficiently_likely(edge)) return NULL;
  GotIter gotiter(edge);
  Item* ans;
  Item* next;
  while(gotiter.next(next))  //the head will be the the last thing in gotiter;
    ans = next;
  return next;  
}
     
AnswerTreePair& 
MeChart::
recordedBPGH(Item* itm, AnswerTreeMap& atm, FullHist* h)
{
  int subfv[MAXNUMFS];
  int i;
  for(i = 0 ; i < MAXNUMFS ; i++) subfv[i] = -1;

  if(!itm->term()->terminal_p())
    {
      getHt(h, subfv);
    }
  CntxArray ca(subfv);
  ca.d[CntxArray::sz-1] = -1; //set i field to don't care. 
  return atpFind(ca, atm);
}

AnswerTreePair& 
MeChart::
recordedBP(Item* itm, FullHist* h)
{
  int subfv[MAXNUMFS];
  getHt(h, subfv);
  CntxArray ca(subfv);
  return itm->stored(ca); 
}

float
MeChart::
meHeadProb(int wInt, FullHist* h)
{
  float ans = meProb(wInt, h, HCALC);
  return ans*10;
}

float
MeChart::
meRuleProb(Edge* edge, FullHist* h)
{
  if(printDebug() > 30)
    {
      prDp();
      cerr << "In meruleprob " << *h << " " << *edge
	   << " " << edge->headPos() <<endl;
    }
  int i;
  int hpos = edge->headPos(); 
  h->hpos = hpos;
  LeftRightGotIter gi(edge);
  globalGi = &gi;
  Item* got;
  float ans = 1;
  for(i=0 ;  ; i++ )
    {
      if(i >= gi.size()) break;
      got = gi.index(i);
      h->pos = i;
      int cVal = got->term()->toInt();
      int whichInt = LCALC;
      if(h->pos == hpos) whichInt = MCALC;
      else if(h->pos > hpos) whichInt = RCALC;
      ans *= meProb(cVal, h, whichInt);
      if(ans == 0) break;
      float tot = 0;
      //for(int qq = 0 ; qq < 75 ; qq++) tot += meProb(qq, h, whichInt);
      //ans /= tot;
    }
  if(printDebug() > 30)
    {
      prDp();
      cerr << "merp = " << ans << endl;
    }
  globalGi = NULL;
  return ans;
}

float
MeChart::
meProb(int cVal, FullHist* h, int whichInt)
{
  if(printDebug() > 68)
    {
      prDp();
      cerr << "meP" << whichInt << "(" << cVal << " | " << *h << ")" <<endl;
    }
  int subfVals[MAXNUMFS];
  FeatureTree* ginfo[MAXNUMFS];  
  ginfo[0] = FeatureTree::roots(whichInt);
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
      int nfeatV = (*(sf->fun))(h);
      FeatureTree* histPt = strt->follow(nfeatV, feat->auxCnt); 
      ginfo[i] = histPt;
      if(i == 1)
	{
	  smoothedPs[0] = 1;
	  Feat* f =histPt->feats.find(cVal);
	  if(!f)
	    {
	      if(printDebug() > 60)
		{
		  prDp();
		  cerr << "Zero p" << feat->name << " " << nfeatV << endl;
		}
	      if(whichInt == HCALC) return 0.001; //???;
	      return 0.0;
	    }
	  smoothedPs[1] = f->g();
	  if(printDebug() > 68)
	    {
	      prDp();
	      cerr << i << " " << nfeatV << " " << smoothedPs[1] << endl;
	    }
	  for(int j = 2; j <= Feature::total[whichInt] ; j++)
	    smoothedPs[j] = 0;
	  ans = smoothedPs[1];
	  continue;
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
      if(printDebug() > 68)
	{
	  prDp();
	  cerr << i << " " << nfeatV << " "
	       << estm << " " << b <<" "<<unsmoothedVal << " " << lam << " " 
	       << nsmoothedVal <<  endl;
	}
      smoothedPs[i] = nsmoothedVal;
      ans *= (nsmoothedVal/osmoothedVal);
    }
  if(whichInt == HCALC) ans *= 600;
  if(printDebug() > 30)
    {
      prDp();
      cerr<<"p"<<whichInt<< "(" << cVal << "|" << *h << ") = " << ans << endl;
    }
  return ans;
}


void
MeChart::
getHt(FullHist* h, int* subfVals)
{
  int i;
  int whichTree = UCALC;
  //cerr << "getHt(" << *h << ", " << whichTree << ")" << endl;
  for(i = 1 ; i < MAXNUMFS ; i++) subfVals[i] = -1;
  for(i = 1 ; i <= Feature::total[whichTree] ; i++)
    {
      Feature* ft = Feature::fromInt(i, whichTree); 
      int sfInt = ft->subFeat;
      SubFeature* sf = SubFeature::fromInt(sfInt, whichTree);
      int val = (*sf->fun)(h);
      subfVals[sfInt] = val;
    }
  //cerr << "done getHt" << endl;
}
    
bool
sufficiently_likely(const Item* itm)
{
  double pout = itm->poutside();
  double pin = itm->prob();
  double factor = .0008;
  if((pout * pin) > factor) return true;  
  return false;
}

bool
sufficiently_likely(Edge* edge)
{
  Item* fp = edge->finishedParent();
  if(!fp) return false;
  if(!sufficiently_likely(fp)) return false;
  GotIter gotIter(edge);
  Item* got;
  while( gotIter.next(got) )
    {
      if(got->term() == Term::stopTerm) continue;
      if(!sufficiently_likely(got)) return false;
    }
  //return true;
  //double factorE = .00008;
  double factorE = .00001;
  double pout = fp->poutside();
  double pin = edge->prob();
  if(pout*pin > factorE) return true;
  //cerr << "Edge filtered " << *edge << endl;
  return false;
}

  /* the general rule for the order in which to process constits
     of the same length, since there may be rules of the form
     SBAR -> S, etc, is:
     z) terminals processed first
     a) S1 is processed last;
     b) SBAR is processed after S;
     bc) S ans SQ are processed after VP;
     c) otherwise in order of decreasing probability.
     */

bool
MeChart::
okDecendent(Item* chld, FullHist* h)
{
  //cerr << "okd1 " << *h << endl;
  FullHist* parHist = h->back;
  while(parHist)
    {
      Item* par = parHist->e->finishedParent();
      assert(par);
      if(par->start() != chld->start()) return true;
      if(par->finish() != chld->finish()) return true;
      const Term* ptrm  = par->term();
      if(ptrm->name() == "S1") return true;
      if(par == chld)
	{
	  if(printDebug() > 28)
	    cerr << "Rejecting " << *par << " over " << *chld << endl;
	  return false;
	}
      parHist = parHist->back;
    }
  error("never get here");
  return false;
}


#include "Term.h"
#include "Feature.h"
#include "FullHist.h"
#include "Edge.h"
#include "headFinder.h"
#include "Bchart.h"
#include "ccInd.h"
#include "GotIter.h"

extern LeftRightGotIter* globalGi;

int
fh_term(FullHist* fh)
{
  return fh->term;
}

int
fh_parent_term(FullHist* fh)
{
  static int s1int = 0;
  if(!s1int)
    {
      ECString s1nm("S1");
      s1int = Term::get(s1nm)->toInt();
    }
  FullHist* par = fh->back;
  if(!par) return s1int;
  else return  par->term;
}

int
toBe(const string& parw)
{
  if(parw == "was" || parw == "is" || parw == "be" || parw == "been"
     || parw == "are" || parw == "were" || parw == "being")
    return 1;
  else return 0;
}

int
fh_parent_pos(FullHist* fh)
{
  static int stopint = 0;
  if(!stopint)
    {
      ECString stopnm("STOP");
      stopint = Term::get(stopnm)->toInt();
    }
  FullHist* par = fh->back;
  if(!par) return stopint;
  int ans = par->preTerm;
  if(ans < 2 && toBe(par->hd->lexeme())) return 48;
  return ans;
}

int
fh_term_before(FullHist* fh)
{
  static int stopint = 0;
  if(!stopint)
    {
      ECString stopnm("STOP");
      stopint = Term::get(stopnm)->toInt();
    }
  FullHist* par = fh->back;
  if(!par) return stopint;
  list<FullHist*>::iterator iti = par->subtrees.begin();
  for( ; iti != par->subtrees.end() ; iti++ )
    {
      FullHist* st = *iti;
      if(st != fh) continue;
      if(iti == par->subtrees.begin())
	{
	  return stopint;
	}
      iti--;
      st = *iti;
      assert(st);
      return st->term;
    }
  error("Should never get here");
  return -1;
}

int
fh_term_after(FullHist* fh)
{
  static int stopint = 0;
  if(!stopint)
    {
      ECString stopnm("STOP");
      stopint = Term::get(stopnm)->toInt();
    }
  FullHist* par = fh->back;
  if(!par) return stopint;
  list<FullHist*>::iterator iti = par->subtrees.begin();
  for( ; iti != par->subtrees.end() ; iti++ )
    {
      FullHist* st = *iti;
      if(st != fh) continue;
      iti++;
      if(iti == par->subtrees.end())
	{
	  return stopint;
	}
      st = *iti;
      assert(st);
      return st->term;
    }
  error("Should never get here");
  return -1;
}

int
fh_pos(FullHist* fh)
{
  return fh->preTerm;
}


int
fh_head(FullHist* tree)
{
  int ans = tree->hd->toInt();
  assert(ans >= -1);
  return ans;
}

int
fh_parent_head(FullHist* tree)
{
  FullHist* pt = tree->back;
  static int dummyInt = -1;
  if(dummyInt < 0)
    {
      ECString temp("^^");
      dummyInt = Bchart::wtoInt(temp);
    }
  
  if(!pt) return dummyInt;
  int ans = pt->hd->toInt();
  assert(ans >= -1);
  return ans;
}

int
fh_grandparent_head(FullHist* tree)
{
  static int rootInt = -1;
  if(rootInt < 0)
    {
      ECString temp("^^");
      rootInt = Bchart::wtoInt(temp);
    }
  FullHist* pt = tree->back;
  if(!pt) return rootInt;
  pt = pt->back;
  if(!pt) return rootInt;
  
  int ans = pt->hd->toInt();
  assert(ans >= -1);
  return ans;
}

int
fh_grandparent_term(FullHist* fh)
{
  static int s1int = 0;
  if(!s1int)
    {
      ECString s1nm("S1");
      s1int = Term::get(s1nm)->toInt();
    }
  FullHist* par = fh->back;
  if(!par) return s1int;
  FullHist* gpar = par->back;
  if(!gpar) return s1int;
  else return  gpar->term;
}

int
fh_grandparent_pos(FullHist* fh)
{
  static int stopint = 0;
  if(!stopint)
    {
      ECString stopnm("STOP");
      stopint = Term::get(stopnm)->toInt();
    }
  FullHist* par = fh->back;
  if(!par) return stopint;
  par = par->back;
  if(!par) return stopint;
  return par->preTerm;
}

int
fh_ccparent_term(FullHist* h)
{
  static int s1int = 0;
  if(!s1int)
    {
      ECString s1nm("S1");
      s1int = Term::get(s1nm)->toInt();
    }
  FullHist* par = h->back;
  if(!par) return s1int;
  int trmInt = par->term;
  if(trmInt != h->term) return trmInt;
  int ccedtrmInt = ltocc(trmInt,par->e->ccInd());
  return ccedtrmInt;
}

int
fh_ccInd(FullHist* fh)
{
  FullHist* par = fh->back;
  if(!par) return 0;
  int trmInt = par->term;
  if(trmInt != fh->term) return 0;
  int ccedInd = par->e->ccInd();
  return ccedInd;
}

int
fh_size(FullHist* fh)
{
  static int bucs[9] = {1, 3, 6, 10, 15, 21, 28, 36, 999};
  int sz = fh->e->loc() - fh->e->start();
  for(int i = 0 ; i < 9 ; i++)
    if(sz <= bucs[i]) return i;
  assert("Never get here");
  return -1;
}

extern Bchart* curChart;

int
fh_effEnd(FullHist* h)
{
  //return 0; //dummy;
  if(h->term == Term::rootTerm->toInt()) return 1;
  FullHist* par = h->back;
  assert(par->e);
  return curChart->effEnd(par->e->loc());
}

int
fh_true(FullHist* h) {return 1;}


int
fh_ngram(FullHist* fh, int n, int l)
{
  //cerr << "fhng " << n << " " << l << " "
    //   << fh->pos << " " << *fh->e << endl;
  static int stopTermInt = -1;
  if(stopTermInt < 0)
    stopTermInt = Term::stopTerm->toInt();

  int pos = fh->pos;
  int hpos = fh->hpos; //???;
  int m = pos + (n * l);
  if(m < 0) return stopTermInt;
  if(m > hpos && l > 0)
    {
      return stopTermInt;
    }
  LeftRightGotIter* lrgi = globalGi;
  assert(lrgi);
  if(m >= lrgi->size()) return stopTermInt;
  Item* got = lrgi->index(m);
  assert(got);
  int ans = got->term()->toInt();
  return ans;
}

int
fh_left0(FullHist* fhh)
{
  return fh_ngram(fhh, 0, 0);
}

int
fh_left1(FullHist* fhh)
{
  return fh_ngram(fhh, 1, 1);
}

int
fh_left2(FullHist* fhh)
{
  return fh_ngram(fhh, 2, 1);
}

int
fh_left3(FullHist* fhh)
{
  return fh_ngram(fhh, 3, 1);
}

int
fh_right1(FullHist* fhh)
{
  return fh_ngram(fhh, 1, -1);
}

int
fh_right2(FullHist* fhh)
{
  return fh_ngram(fhh, 2, -1);
}

int
fh_right3(FullHist* fhh)
{
  return fh_ngram(fhh, 3, -1);
}

int
fh_noopenQr(FullHist* fh)
{
  Edge* edge = fh->e;
  int pos = fh->pos;
  LeftRightGotIter*  lrgi = globalGi;
  Item* got;
  int i;
  bool sawOpen = false;
  for(i = 0 ; i < lrgi->size() ; i++)
    {
      if(i == pos) break;
      //if(i >= pos-3) break; //??? -3 because we already know about last3;
      got = lrgi->index(i);
      const Term* trm = got->term();
      if(trm->name() == "``") sawOpen = true;
      else if(trm->name() == "''") sawOpen = false;
    }
  if(sawOpen) return 0;
  else return 1;
}

int
fh_noopenQl(FullHist* fh)
{
  Edge* edge = fh->e;
  int pos = fh->pos;
  int hpos = fh->hpos;
  LeftRightGotIter*  lrgi = globalGi;
  Item* got;
  int i;
  bool sawOpen = false;
  
  for(i = hpos ; i >= 0 ; i--)
    {
      if(i == pos) break;
      //if(i <= (pos+3)) break; //??? +3 because we already know about next 3;
      got = lrgi->index(i);
      const Term* trm = got->term();
      if(trm->name() == "''") sawOpen = true;
      else if(trm->name() == "``") sawOpen = false;
    }
  if(sawOpen) return 0;
  else return 1;
}

int
fh_Bl(FullHist* treeh)
{
  error("fh_Bl should never be called");
  return -1;
}

int
fh_Br(FullHist* treeh)
{
  error("fh_Br should never be called");
  return -1;
}

void
addSubFeatureFns()
{
  /*
    0 t  fh_term
    1 l  fh_parent_term
    2 u  fh_pos
    3 h  fh_head
    4 i  fh_parent_head
    5 T  fh_true
    6 v  fh_parent_pos
    7 b  fh_term_before
    8 a  fh_term_after
    9 m  fh_grandparent_term
    10 w fh_grandparent_pos
    11 j fh_grandparent_head
    12 c fh_ccparent_term
    13 L1 fh_left1
    14 L1 fh_left2
    15 R1 fh_right1
    16 R1 fh_right2
    17 Qr fh_noopenQr
    18 L0 fh_left0;
    19 L3 fh_left3
    20 R3 fh_right3
    21 Qr fh_noopenQl
    22 Bl fh_Bl
    23 Br fh_Br
    24 e  fh_effEnd
    */
  int (*funs[25])(FullHist*)
    = {fh_term, fh_parent_term, fh_pos, fh_head,
       fh_parent_head, fh_true, fh_parent_pos, fh_term_before, fh_term_after,
       fh_grandparent_term,fh_grandparent_pos,fh_grandparent_head,
       fh_ccparent_term, fh_left1, fh_left2, fh_right1, fh_right2,
       fh_noopenQr, fh_left0,fh_left3,fh_right3,fh_noopenQl,fh_Bl,
       fh_Br,fh_effEnd};
  int i;
  for(i = 0 ; i < 25 ; i++) SubFeature::Funs[i] = funs[i];
}



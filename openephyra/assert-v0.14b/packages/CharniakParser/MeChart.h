
#ifndef MECHART_H
#define MECHART_H

#include "AnswerTree.h"
#include "Bchart.h"
#include "FullHist.h"
#include "FeatureTree.h"
#include "Feature.h"
#include "Item.h"

class MeChart : public Bchart
{
 public:
  MeChart(SentRep & sentence)
     : Bchart( sentence ) {}
  static void  init(ECString path);
  AnswerTree*      findMapParse();
  AnswerTreePair& bestParse(Item* itm, FullHist* h);
  AnswerTreePair& bestParseGivenHead(int posInt, const Wrd& wd, Item* itm,
				     FullHist* h, ItmGHeadInfo& ighInfo);
  void  fillInHeads();
  bool  headsFromEdges(Item* itm);
  bool  headsFromEdge(Edge* e);
  Item * headItem(Edge* edge);
  void  getHt(FullHist* h, int* subfVals);
  float getpHst(const ECString& hd, int t);
  AnswerTreePair& recordedBP(Item* itm, FullHist* h);
  AnswerTreePair& recordedBPGH(Item* itm, AnswerTreeMap& atm, FullHist* h);
  float meHeadProb(int wInt, FullHist* h);
  float meProb(int val, FullHist* h, int which);
  float meRuleProb(Edge* e, FullHist* h);
  void  getRelFeats(int c, int c2, int which, Feat* relFeat[],
		    FeatureTree* fts[], FullHist* h, int facPos);
  bool  okDecendent(Item* chld, FullHist* ph);

  int     ccbucket(float val, float* buckets , int sz);
  static void    initCCArrays(ECString path);
  static void    initccarray(ifstream& is, float lenArray[6][8]);
  float   ccLenProb(Edge* edge, int effend);
};

#endif

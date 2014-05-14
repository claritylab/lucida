
#ifndef BCHART_H
#define BCHART_H

#include "ChartBase.h"
#include "EdgeHeap.h"
#include "Wrd.h"
#include "FullHist.h"

#define Termstar const Term*

struct Wwegt  
{
  int t;
  char e[2];
  float p;
};

class Bchart;

class           Bchart : public ChartBase
{
public:
    Bchart(SentRep& sentence);
    virtual ~Bchart();
    virtual double  parse();
    static int&      printDebug() { return printDebug_; }
    static bool      printDebug(int val) { return val < printDebug_; }
    static void     readTermProbs(ECString& path);
    static int      wtoInt(ECString& str);
    int     extraTime; //if no parse is found on regular time;
    static  Item*    dummyItem;
    static float timeFactor;
    static float    denomProbs[1000];  //Restricts to sentences of len < 1000;
    void            check();
    static void     setPosStarts();
    bool prned();
    bool issprn(Edge* e);
protected:
    void            add_reg_item(Item * itm);
    void            addFinishedEdge(Edge* newEdge);
    void            add_starter_edges(Item* itm);
    float           meEdgeProb(const Term* trm, Edge* edge, int whichInt);
    float           meFHProb(const Term* trm, FullHist& fh, int whichInt);
    static int printDebug_;

    void            extend_rule(Edge* rule, Item * itm, int right);
    void            already_there_extention(int i, int start, int right,
					    Edge* edge);
    void            add_edge(Edge* rli, int left);
    void            put_in_reg(Item * item);
    void            addWordsToKeylist( );
    Item           *in_chart(const Wrd* hd, const Term * trm,
			     int start, int finish);

    void            redoP(Edge* edge, double probRatio);
    void            redoP(Item *item, double probDiff);

    float           computeMerit(Edge* edge, int whichCalc);

    void  initDenom();
    list<float>& wordPlist(Wrd* word, int word_num);
    double  psktt(Wrd* shU, int t);
    double  pCapgt(const Wrd* shU, int t);
    float   pHst(int w, int t);
    double  psutt(const Wrd* shU, int t);
    float   pegt(ECString& sh, int t);
    void    getpHst(const ECString& hd, int t);
    double pHypgt(const ECString& shU, int t);
    static float&  pHcapgt(int i) { return pHcapgt_[i]; }
    static float&  pHhypgt(int i) { return pHhypgt_[i]; }
    static float&  pHugt(int i) { return pHugt_[i]; }
    static float& pT(int tInt)
      {
	int val = tInt-Term::lastTagInt()-1;
	assert(val >= 0 && val < 50);
	return pT_[val];
      }
    int     bucket(float val);
    int    greaterThan(Wwegt& wwegt, char e[2], int t);
    float  pHegt(ECString& es, int t);
    float  computepTgT(int t1,int t2);
    void   addToDemerits(Edge* edge);
    static Item*    stops[400];
    EdgeHeap*       heap;
    int             alreadyPopedNum;
    Edge*           alreadyPoped[450000]; //was 350000;
    static int&     posStarts(int i, int j);
    static int      posStarts_[80][30];
    static int     compat_left_[30][80];
    static int     compat_right_[30][80];
    static int     curDemerits_[400][400];

  static int egtSize_;
  static float bucketLims[14];
  static float pT_[50];
  static float pHcapgt_[100];
  static float pHhypgt_[100];
  static float pHugt_[100];
  static float unigrams_[80];
  static float bigrams_[50][50];

  static Wwegt* pHegt_;
  list<float> wordPlists[400];
  static map< ECString, int, less<ECString> > wordMap;
};

#endif	/* ! BCHART_H */


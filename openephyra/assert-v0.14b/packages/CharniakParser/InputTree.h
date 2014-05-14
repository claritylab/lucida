
#ifndef INPUTTREE_H
#define INPUTTREE_H

#include <list.h>
#include "ECString.h"
#include "utils.h"
#include "ParseStats.h"
#include "SentRep.h"

class InputTree;
typedef list<InputTree*> InputTrees;
typedef InputTrees::iterator InputTreesIter;
typedef InputTrees::const_iterator ConstInputTreesIter;
typedef pair<ECString,ECString> EcSPair;
typedef list<EcSPair> EcSPairs;
typedef EcSPairs::iterator EcSPairsIter;

class  InputTree
{
 public:
  InputTree(const InputTree& p);
  InputTree(istream& is);
  InputTree() : start_(0), finish_(0), word_(""), term_("") {}
  InputTree(int s, int f, const ECString w, const ECString t, const ECString n,
	    InputTrees& subT, InputTree* par, InputTree* headTr)
    : start_(s), finish_(f), word_(w), term_(t), ntInfo_(n),
      subTrees_(subT), parent_(par), headTree_(headTr){}
  InputTree(const ECString w, int i)
    : start_(i), finish_(i+1), word_(w), term_(w), ntInfo_(""),
      parent_(NULL), headTree_(NULL){}
  ~InputTree();

  friend istream& operator >>( istream& is, InputTree& parse );
  friend ostream& operator <<( ostream& os, const InputTree& parse );
  int         start() const { return start_; }
  int         length() const { return (finish() - start()); }
  int         finish() const { return finish_; }
  const ECString word() const { return word_; }  
  const ECString term() const { return term_; }
  const ECString ntInfo() const { return ntInfo_; }
  const ECString head() { return headTree_->word(); }
  const ECString hTag() { return headTree_->term(); }
  InputTrees& subTrees() { return subTrees_; }
  InputTree*& headTree() { return headTree_; }
  InputTree*  parent() { return parent_; }
  InputTree*&  parentSet() { return parent_; }
  void         mapCompat(  InputTree& correct, ParseStats& parseStats,
			   SentRep& sr );
  bool         compat( int strt, int fnsh,  SentRep& sr ) ;
  bool         exact( int strt, int fnsh );
  bool         lexact( int strt, int fnsh, ECString trm );
  void         mapExact(  InputTree& computed,
			 ParseStats& parseStats );
  void         precision(  InputTree& correct, ParseStats& parseStats,
			   SentRep& sent);
  bool         punctuationClose(int pos1, int pos2,  SentRep& sent);
  bool         lexact2( int strt, int fnsh,
			ECString trm,  SentRep& sent );

  void        make(list<ECString>& str);
  static int  pageWidth;     
  static ECString tempword[128];
  static int      tempwordnum;
  static int      equivPos[101];
  static void   init();

  static int equivInt(int x);
  static int puncEquiv(int i, SentRep& sr);
  static void setEquivInts(SentRep& sr);

 protected:
  void        readParse(istream& is);
  InputTree*     newParse(istream& is, int& strt, InputTree* par);
  ECString&  readNext( istream& is );
  void        parseTerm(istream& is, ECString& a, ECString& b);
  void        printproper( ostream& os ) const;
  void        prettyPrint(ostream& os, int start, bool startLine) const;
  int         spaceNeeded() const;
  
  InputTree*  parent_;
  int         start_;
  int         finish_;
  ECString   word_;
  ECString   term_;
  ECString   ntInfo_;
  InputTree*  headTree_;
  InputTrees  subTrees_;
};

InputTree* ithInputTree(int i, const list<InputTree*> l);

#endif /* ! INPUTTREE_H */



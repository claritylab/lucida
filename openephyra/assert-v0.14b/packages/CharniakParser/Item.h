
#ifndef ITEM_H 
#define ITEM_H

#include "Wrd.h"
#include "Edge.h"
#include <map.h>
#include <set.h>
#include "AnswerTree.h"
#include "CntxArray.h"

class Term;
class Word;
class ostream;

typedef set<Edge*, less<Edge*> > EdgeSet;
typedef EdgeSet::iterator EdgeSetIter;
typedef pair<EdgeSet,AnswerTreeMap> ItmGHeadInfo;
typedef map<Wrd, ItmGHeadInfo, less<Wrd> > HeadMap;
typedef map<int,HeadMap, less<int> > PosMap;
typedef HeadMap::iterator HeadIter;
typedef PosMap::iterator PosIter;

class           Item
{
public:
    Item( //const Wrd* hd,
	 const Term * _term, int _start, int _finish );
    ~Item();
    int		    operator== (const Item& item) const;
    friend ostream& operator<< ( ostream& os, const Item& item );
    const Term *    term() const { return term_; }
    const Wrd*     word() const { return word_; }
    const Wrd*&    word() { return word_; }
    int             start() const {return start_;}
    int             finish() const {return  finish_;}
    list<Edge*>&  needme() {return needme_;}
    list<Edge*>&  ineed() {return ineed_;}
    void            check();
    double            prob() const {return prob_;}
    double            poutside() const {return poutside_;}
    double            storeP() const {return storeP_;}
    double &          prob() {return prob_;}
    double &          poutside() {return poutside_;}
    double &          storeP() {return storeP_;}
    AnswerTreePair&   stored(CntxArray& ca) { return atpFind(ca, stored_); }
    PosMap&          posAndheads() { return posAndheads_; }
 private:
    Item( const Item& );
    Item&	    operator= (const Item&);
    int             start_;
    int             finish_;
    const Term *    term_;
    const Wrd *    word_;
    list<Edge*>  needme_;	/* A list of rules requiring a term starting
				 * at start */

    list<Edge*>  ineed_;	// needme = rules predicted by this (art) item
				// ineed = rules that predict this (art) item
    double           prob_;
    double           poutside_;
    double           storeP_;	
    AnswerTreeMap    stored_;
    PosMap           posAndheads_;
};

typedef list<Item*> Items;
typedef Item *	Item_star;

#endif /* !ITEM_H */

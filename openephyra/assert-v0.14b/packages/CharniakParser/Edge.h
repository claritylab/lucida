
#ifndef EDGE_H
#define EDGE_H

#include "Term.h"
#include "utils.h"

class Item;

class Edge
{
public:
    friend class GotIter;
    friend class SuccessorIter;
    friend class LeftRightGotIter;
    Edge(ConstTerm* trm);
    Edge( Edge& edge, Item& itm, int right );
    Edge( Item& itm );

    Edge( const Edge& src ) { error("edge copying no longer exists"); }

    Edge() {}
    ~Edge();
    bool            check(); 

    int 	    operator== (const Edge& rhs) { return this == &rhs; }
    bool	    finished() const;
    ConstTerm *	    lhs() const { return lhs_; }
    int 	    heapPos() const {   return heapPos_;   }
    int&	    heapPos() {   return heapPos_;   }
    int		    start() const {   return start_;   }
    list<Edge*>	    sucs() const { return sucs_; }
    list<Edge*>&    sucs() { return sucs_; }
    int		    loc() const {   return loc_;   }
    Item*           item() const { return item_; }
    Edge*           pred() const { return pred_; }
    float&          prob() { return prob_; }
    float           prob() const { return prob_; }

    int             headPos(int i = 0);

    float           leftMerit() const { return leftMerit_; }
    float&          leftMerit() { return leftMerit_; }
    float           rightMerit() const { return rightMerit_; }
    float&          rightMerit() { return rightMerit_; }
    int             demerits() const { return demerits_; }
    int&            demerits() { return demerits_; }

    float	    merit() { return merit_; }
    void	    setmerit();
    int             status() const { return status_; }
    int&            status() { return status_; }
    void	    print( ostream& os );
    friend ostream& operator<< (ostream& os, Edge& edge )
		    { edge.print( os ); return os;}
    void            setFinishedParent( Item* par )
                      { finishedParent_ = par ; }
    Item           *finishedParent() { return finishedParent_; }
    int            ccInd();
    static int      numEdges;
    static float    DemFac;

private:
    int             status_; 
    int             start_;
    int             loc_;
    ConstTerm 	   *lhs_;
    Item           *finishedParent_;
    Item           *item_;
    Edge*           pred_;
    int             heapPos_;
    int             demerits_;

    float          leftMerit_;
    float          rightMerit_;
    float          prob_;
    float          merit_;
    list<Edge*> sucs_;
};

typedef list<Edge*> Edges;

#endif	/* ! EDGE_H */

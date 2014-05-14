
#ifndef CHARTBASE_H
#define CHARTBASE_H

#include "Edge.h"
#include "Item.h"
#include "SentRep.h"

class ostream;

class           ChartBase
{
public:
    ChartBase(SentRep& sentence);
    virtual ~ChartBase();

    enum Err { OK, OVERFLW, FAILURE };

    // parsing functions, what the class is all about.
    virtual double  parse() = 0;

    // extracting information about the parse.
    void            set_Alphas();
    const Items&    items( int i, int j ) const
		    {   return regs[ i ][ j ];   }
    int             edgeCount() const    { return ruleiCounts_; }
    int             popedEdgeCount() const    { return popedEdgeCount_; }
    int             popedEdgeCountAtS() const    { return popedEdgeCountAtS_; }
    int             totEdgeCountAtS() const    { return totEdgeCountAtS_; }

    // printing information about the parse.
    const Item*     mapProbs();
    static bool finalPunc(const char* wrd);

    Item*           topS() { return get_S(); }
    static int&	    ruleCountTimeout()  {   return ruleiCountTimeout_;   }
    static const double
		    badParse;	// error return value for parse(), crossEntropy
    SentRep&        sentence_;
    int             effEnd(int pos);
protected:
    Item           *get_S() const;  
    static Items           regs[400][400];
    static list<Edge*> waitingEdges[2][400];
    double          crossEntropy_;
    int             wrd_count_;
    int             popedEdgeCount_;
    int             totEdgeCountAtS_;
    int             popedEdgeCountAtS_;
    int             ruleiCounts_; // keeps track of how many edges have been
                                // created --- used to time out the parse
    Item*           pretermItems[4000];
    int             pretermNum;
    int		    endPos;
    static int      ruleiCountTimeout_ ; //how many rulei's before we time out.
    float           endFactorComp(Edge* dnrl);

private:
    void            free_chart_items(Items& itms);
    void            free_chart_itm(Item * itm);
    void            free_edges(list<Edge*>& edges);
};


#endif	/* ! CHARTBASE_H */


#ifndef TERM_H
#define TERM_H

#include "ECString.h"
#include <list>
#include <map>
#include <assert.h>
#include <iostream>
#include <fstream>

class Term;

typedef Term *		Term_p;
typedef const Term *	Const_Term_p;
typedef const Term     ConstTerm;

#define Terms list<ConstTerm*>
#define ConstTerms const list<ConstTerm*>
#define TermsIter list<Term*>::iterator
#define ConstTermsIter list<ConstTerm*>::const_iterator
typedef map<ECString, Term*, less<ECString> >  TermMap;

class Term 
{
public:
    Term();			// provided only for maps.
    Term( const ECString s, int terminal, int n );
    Term( const Term& src );
    int              toInt() const { return num_; }
    const ECString& name() const {  return name_;  }
    int	  terminal_p() const { return terminal_p_; }
    friend ostream& operator<< ( ostream& os, const Term& t );
    friend ostream& operator>> ( istream& os, const Term& t );
    int		operator== (const Term& rhs ) const;
    bool   isPunc() const { return (terminal_p_ == 2) ? true : false ; }
    bool   openClass() const { return (terminal_p_ == 3) ? true : false ; }
    static Const_Term_p get(const ECString& getname);
    static void  init(ECString & prefix);
    static const Term* fromInt(int i) 
      { assert(i < 400); return array_[i]; }
    static int  lastTagInt() { return lastTagInt_; }
    static int  lastNTInt() { return lastNTInt_; }
    static Term* stopTerm;
    static Term* startTerm;
    static Term* rootTerm;
private:
    ECString* namePtr() { return (ECString*)&name_; }
    int    	terminal_p_;
    int		num_;
    const ECString name_;
    static Term*  array_[400];
    static TermMap termMap_ ;
    static int    lastTagInt_;
    static int    lastNTInt_;
};
  

#endif /* ! TERM_H */

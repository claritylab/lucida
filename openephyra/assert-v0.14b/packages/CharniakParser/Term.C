
#include "Term.h"
#include "utils.h"

Term*  Term::array_[400];
map<ECString, Term*,less<ECString> > Term::termMap_;
int    Term::lastTagInt_ = 0;
int    Term::lastNTInt_ = 0;
Term*  Term::stopTerm;
Term*  Term::startTerm;
Term*  Term::rootTerm;

Term::
Term()
: num_( -1 ), terminal_p_( 0 )
{}

Term::
Term(const ECString s, int terminal, int num )
: name_( s ), num_( num ), terminal_p_( terminal )
{}

Term::
Term( const Term& src )
: name_( src.name() ), 
  //num_(src.toInt()),
  terminal_p_( src.terminal_p() )
{}

ostream&
operator<< ( ostream& os, const Term& t )
{
    os << t.name();
    return os;
}

int
Term::
operator== ( const Term& rhs ) const
{
    if( this == &rhs || name_ == rhs.name() )
	return 1;
    return 0;
}


void
Term::
init(ECString & prefix)
{
  ECString          fileName(prefix);
  fileName += "terms.txt";
  ifstream           stream(fileName.c_str(), ios::in);
  if (!stream)
    {
      cerr << "Can't open terms file " << fileName << endl;;
      return;
    }
  
  ECString          termName;
  int ind, n;
  n = 0;
  bool seenNTs = false;
  while (stream >> termName)
    {
      stream >> ind;
      Term* nextTerm = new Term(termName, ind, n);
      termMap_[nextTerm->name()] = nextTerm;
      if(termName == "STOP") Term::stopTerm = nextTerm;
      else if(termName == "G4") Term::startTerm = nextTerm;
      else if(termName == "S1") Term::rootTerm = nextTerm;
      array_[n] = nextTerm;
      if(!ind && !seenNTs)
	{
	  assert(n > 0);
	  lastTagInt_ = n-1;
	  seenNTs = true;
	}
      n++;
      assert(n < 400);
    }
  assert(!ind);
  lastNTInt_ = n-1;
  //lastNTInt_ = n-4;  //??? hack to ignore G1 and G2 and G3;
  stream.close();
}

Const_Term_p
Term::
get(const ECString& nm)
{
  TermMap::iterator ti = termMap_.find(nm);
  if( ti == termMap_.end()) return NULL;
  return (*ti).second;
}



#include "Feat.h"
#include "utils.h"

int Feat::Usage = 0;
/*
Feat::
Feat() //: cnt_(0)
{
  if(Usage == ISCALE || Usage == PARSE)
    {
      if(Usage == ISCALE)
	{
	  uVals = new float[MAXNUMFS+2];
	  for(int i = 0 ; i < MAXNUMFS+2 ; i++)
	    uVals[i] = 0;
	}
      else
	uVals = new float[1];
      g() = 1;
    }
  else uVals = NULL;
}
*/
ostream&
operator<< ( ostream& os, Feat& t )
{
  os << "{" << t.ind()/*<< "," << t.cnt()*/ << "}";
  return os;
}

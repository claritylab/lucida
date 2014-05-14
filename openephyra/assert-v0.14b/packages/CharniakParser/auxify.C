#include "auxify.h"
#include <iostream.h>
#include "Term.h"
#include "ECString.h"

char* 	suffixes[] = {
"'VE",
"'M",
"'LL",
"'D",
"'S",
"'RE",
0
};

char* 	auxgs[] = {
"BEIN",
"HAVING",
"BEING",
0
};


char* 	auxs[] = {
"MAHT",
"SHULD",
"WILL",
"WAS",
"OUGHTA",
"AHM",
"NEED",
"MAYE",
"WILLYA",
"WHADDYA",
"HATH",
"HAVE",
"WERE",
"IS",
"HAS",
"MUST",
"DID",
"HAD",
"DO",
"MIGHT",
"WOULD",
"SHALL",
"SHOULD",
"OUGHT",
"COULD",
"DOES",
"HAFTA",
"BE",
"KIN",
"CAN",
"ART",
"BEEN",
"DONE",
"ARE",
"DOO",
"MAY",
"AM",
0
};

bool
hasAuxSuf( ECString word )
{
    size_t pos = word.find_first_of("\'");
    if(pos == -1) return false;
    ECString apostrophe = word.substr(pos, word.length()-pos);
    for( int i = 0; suffixes[i]; i++)
    {
	if( apostrophe == suffixes[i] ) 
	    return true;
    }
    return false;
}

bool
isAux( ECString word )
{
    for( int i = 0; auxs[i]; i++)
    {
	if( word == auxs[i] )
	    return true;
    }
    return false;
}

bool
isAuxg( ECString word )
{
    for( int i = 0; auxgs[i]; i++)
    {
	if( word == auxgs[i] ) 
	    return true;
    }
    return false;
}

char* verbs[] = {
"VB",
"VBD",
"VBG",
"VBN",
"VBP",
"VBZ",
0
};

bool
isVerb( ECString tag )
{
    for( int i = 0; verbs[i]; i++)
	if( tag == verbs[i] ) 
	    return true;
    return false;
}

char*
toUpper(const char* str, char* temp)
{
  int l = strlen(str);
  assert(l < 128);
  for(int i = 0 ; i <= l ; i++)
    {
      char n = str[i];
      int ni = (int)n;
      if(ni >= 97 && ni <= 122)
	{
	  temp[i] = (char)(ni-32);
	}
      else temp[i] = n;
    }
  return temp;
}

ECString
auxify(ECString wM, ECString trmM)
{
  char temp[128];
  ECString w = toUpper(wM.c_str(),temp);
  ECString trm = toUpper(trmM.c_str(),temp);
  if( isVerb( trm ) )
    {
      //cout << "saw verb " << trm << " " << wM << endl;
      if( isAux( w ) || hasAuxSuf( w ) )
	{
	  //cout << "was aux " << w << endl;
	  return "AUX";
	}
      else if( isAuxg( w ) )
	{
	  //cout << "was auxg " << w << endl;
	  return "AUXG";
	}
    }
  if(trm == "BES" || trm == "HVS")  //??? strange tags in switchboard
    {
      assert(w == "'S" || w == "-S");
      return "AUX";
    }
  return trmM;
}

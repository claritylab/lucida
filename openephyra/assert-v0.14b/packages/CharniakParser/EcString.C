
#include "EcString.h"

char*
toLower(char* str, char* temp)
{
  int l = strlen(str);
  assert(l < 128);
  for(int i = 0 ; i <= l ; i++)
    {
      char n = str[i];
      int ni = (int)n;
      if(ni >= 65 && ni <= 90)
	{
	  temp[i] = (char)(ni+32);
	}
      else temp[i] = n;
    }
  return temp;
}


#include "CntxArray.h"

//int  CntxArray::sz = 6;
int  CntxArray::sz = 5;

CntxArray::
CntxArray(int* data)
{
  int i;
  for(i = 0 ; i < sz ; i++)
    d[i] = data[i+1];
}

CntxArray::
CntxArray()
{
  int i;
  for(i = 0 ; i < sz ; i++)
    d[i] = -1;
}
    
int
operator<(CntxArray a1, CntxArray a2)
{
  int i;
  for(i = 0 ; i < CntxArray::sz ; i++)
    {
      if(a1.d[i] > a2.d[i]) return 0;
      else if(a1.d[i] < a2.d[i]) return 1;
      else if(a1.d[i] < 0) return 0;
    }
  return 0;
}

ostream&
operator<< ( ostream& os, const CntxArray& ca )
{
  int i;
  int sz = ca.sz;
  for(i = 0 ; i < sz ; i++)
    {
      int val = ca.d[i];
      if(val == -1) os << ".";
      else os << val;
      if(i != sz-1) os << "/";
    }
  return os;
}


#ifndef CNTXARRAY_H
#define CNTXARRAY_H

#include <iostream>

class CntxArray
{
 public:
  CntxArray();
  CntxArray(int* data);
  friend int operator< (CntxArray a1, CntxArray a2);
  friend ostream& operator<< ( ostream& os, const CntxArray& ca );
  int d[7];
  static int sz;
};


#endif /* ! CNTXARRAY_H */

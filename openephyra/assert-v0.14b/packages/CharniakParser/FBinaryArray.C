#include "FBinaryArray.h"
#include "Feat.h"
#include "FeatureTree.h"
void
FBinaryArray::
set(int sz) { size_ = sz; array_ = new Feat[sz]; };

Feat*
FBinaryArray::
index(int i) { return &array_[i]; }

Feat*
FBinaryArray::
find(const int id) const
{
  int top = size_;
  int bot = -1;
  int  midInd;
  for( ; ; )
    {
      if( top <= bot+1 )
	{
	  return NULL;
	}
      int mid = (top+bot)/2;
      Feat* midH = &array_[mid];
      midInd = midH->ind();
      if( id == midInd) return midH;
      else if( id < midInd) top = mid;
      else bot = mid;
    }
}

void
FTreeBinaryArray::
set(int sz) { size_ = sz; array_ = new FeatureTree[sz]; };

FeatureTree*
FTreeBinaryArray::
index(int i) { return &array_[i]; }

FeatureTree*
FTreeBinaryArray::
find(const int id) const
{
  int top = size_;
  int bot = -1;
  int  midInd;
  for( ; ; )
    {
      if( top <= bot+1 )
	{
	  return NULL;
	}
      int mid = (top+bot)/2;
      FeatureTree* midH = &array_[mid];
      midInd = midH->ind();
      if( id == midInd) return midH;
      else if( id < midInd) top = mid;
      else bot = mid;
    }
}

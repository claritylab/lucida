
#ifndef FBINARYARRAY_H
#define FBINARYARRAY_H

#include <iostream.h>
#include <fstream.h>

class Feat;
class FeatureTree;

class FBinaryArray
{
 public:
  FBinaryArray() : size_(0) {}
  void set(int sz);
  Feat*   find(const int id) const;
  int     size() const { return size_; }
  Feat*   index(int i);
  int size_;
  Feat* array_;
};

class FTreeBinaryArray
{
 public:
  FTreeBinaryArray() : size_(0) {}
  void set(int sz);
  FeatureTree*   find(const int id) const;
  int     size() const { return size_; }
  FeatureTree*   index(int i);
  int size_;
  FeatureTree* array_;
};

#endif /* ! FBINARYARRAY_H */

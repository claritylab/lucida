
#ifndef EDGEHEAP_H 
#define EDGEHEAP_H

#define HeapSize 370000
class Edge;

class           EdgeHeap
{
public:
  EdgeHeap();
  ~EdgeHeap();
  void    insert(Edge* edge);
  Edge*   pop();
  void    del(Edge* edge);
  Edge**  ar() { return array; }
  int     size() { return unusedPos_; }
  //void    check();
  bool print;
private:
  void  del_(int pos);
  void  downHeap(int pos);
  bool  upheap(int pos);
  int   left_child(int par) const { return (par*2) + 1; }
  int   right_child(int par) const { return ((par*2) + 2); }
  int   parent(int child) const { return ((child-1)/2); }
  int   unusedPos_;
  Edge* array[HeapSize];
};



#endif /* !EDGEHEAP_H */

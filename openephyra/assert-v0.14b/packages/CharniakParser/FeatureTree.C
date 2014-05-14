#include "FeatureTree.h"
#include "ECString.h"
#include <set.h>

FeatureTree* FeatureTree::roots_[20];

extern int MinCount;

FeatureTree* 
FeatureTree::
follow(int val, int auxCnt)
{
  if(!auxCnt)
    {
      return subtree.find(val);
    }
  if(!auxNd)
    {
      return NULL;
      //cerr << "No auxnd " << *this << ", " << val << ", " << auxCnt << endl;
      //assert(auxNd);
    }
  return auxNd->follow(val, auxCnt-1);
}

/* basic format
   assumedNum //e.g., 55 (np)
        rule# count
        ...
	--
	48 //this would be np under adjp
             rule# count
	     ...
        --  //end  of rules
        -- //end of 48
	49
	...
	--  //end of 49
	...
	--  // end of g(rtl)s
   Pringint is brown into 2 procedures.  The first takes a position
   in the tree and determines if the data there supports it (i.e.,
   there is enough for any one conditioned value).  if so, it
   prints out the link value, and the conditioned values. It then
   calls f2 on itself.
*/

FeatureTree::
FeatureTree(istream& is)
  : auxNd(NULL), back(NULL), ind_(ROOTIND)
{
  int done = 0;
  int c = 0;
  subtree.set(99);
  while(is && !done)
    {
      int val = readOneLevel0(is,c);
      if(val == -1) done = 1;
      c++;
    }
  roots_[Feature::whichInt] = this;
}
 
int
FeatureTree::
readOneLevel0(istream& is, int c)
{
  int nextInd;
  ECString nextIndStr;
  is >> nextIndStr;
  if(!is) return -1;
  if(nextIndStr == "Selected") return -1;
  nextInd = atoi(nextIndStr.c_str());
  FeatureTree& nft = subtree.array_[c];
  nft.ind_ = nextInd;
  nft.read(is,Feature::ftTree[Feature::whichInt].left);
  nft.back = this; 
  return nextInd;
}

void
FeatureTree::
read(istream& is, FTypeTree* ftt)
{
  ECString indStr;
  int indI;
  is >> count;
  int cfeats, ctrees;
  is >> cfeats;
  is >> ctrees;
  //cerr << "R " << ftt->n << " " << ind() << " " << count << endl;
  int cf;
  if(cfeats > 0) feats.set(cfeats);
  for(cf = 0 ; cf < cfeats ; cf++)
    {
      is >> indI;
      Feat& nf = feats.array_[cf];
      nf.ind_ = indI;
      float v;
      is >> v;
      nf.g() = v;
      //cerr << indI << "\t" << v << endl;
    }
  if(ctrees > 0) subtree.set(ctrees);
  othReadFeatureTree(is, ftt, ctrees);
}

void
FeatureTree::
othReadFeatureTree(istream& is, FTypeTree* ftt, int ctrees)
{
  //cerr << "F " << ftt->n << " " << ind() << " " << count
    //   << " " << ctrees << endl;
  ECString indStr;
  int indI;
  int c;
  for(c = 0 ; c < ctrees ; c++)
    {
      is >> indI;
      FeatureTree& ntr = subtree.array_[c];
      assert(ftt->left);
      ntr.ind_ = indI;
      ntr.read(is, ftt->left);
    }
  if(!ftt->right)
    {
      return;
    }
  assert(!auxNd);
  is >> indStr;
  if(indStr != "A")
    {
      cerr << "fi = " << ftt->n << " " << ctrees << " " << indStr
	   << " " << ind() << " " << count << endl;
      for(int i = 0 ; i < 5 ; i++)
	{
	  ECString tmp;
	  is >> tmp;
	  cerr << tmp << " ";
	}
      cerr << endl;
      cerr << ftt->right->n << endl;
      assert(indStr == "A");
    }
  int ac;
  is >> ac;  
  /* auxNds point back not to the node the are auxes of, but to its pred */
  auxNd = new FeatureTree(AUXIND,this->back);
  if(ac > 0)   auxNd->subtree.set(ac);
  auxNd->othReadFeatureTree(is,ftt->right, ac);
}


/* basic format
   assumedNum //e.g., 55 (np)
        rule# count
        ...
	--
	48 //this would be np under adjp
             rule# count
	     ...
	     --
	49
	...
	--  //end of 49
	...
	--  // end of g(rtl)s
	A  //indicates that there is another group of features here.
	2  //np's with head's pos->toInt() == 2;
   Pringint is brown into 2 procedures.  The first takes a position
   in the tree and determines if the data there supports it (i.e.,
   there is enough for any one conditioned value).  if so, it
   prints out the link value, and the conditioned values. It then
   calls f2 on itself.
*/

ostream&
operator<<(ostream& os, const FeatureTree& ft)
{
  os << "| " << ft.ind() << " " << ft.count << " |";
  return os;
}

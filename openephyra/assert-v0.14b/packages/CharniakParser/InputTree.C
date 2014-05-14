#include <fstream.h>
#include <iostream.h>
#include "InputTree.h"
#include "headFinder.h"
#include "utils.h"
#include <assert.h>
  
int              InputTree::pageWidth = 75;  //used for prettyPrinting
ECString         InputTree::tempword[128];          
int              InputTree::tempwordnum= 0;
int              InputTree::equivPos[101];

bool
scorePunctuation( const ECString trmString )
{
  if( trmString == ",") return true;
  if( trmString == ":") return true;
  if( trmString == ";") return true;
  if( trmString == "--") return true;
  if( trmString == "...") return true;
  if( trmString == "-") return true;
  if( trmString == "''") return true;
  if( trmString == "'") return true;
  if( trmString == "`") return true;
  if( trmString == "``") return true;
  if( trmString == ".") return true;
  if( trmString == "?") return true;
  if( trmString == "!") return true;
  return false;
}

int
InputTree::
equivInt(int x)
{
  int equivpos = equivPos[x];
  if(equivpos < 0) return x;
  else return equivInt(equivpos);
}

int
InputTree::
puncEquiv(int i, SentRep& sr)
{
  if(i == 0) return i;
  if(scorePunctuation(sr[i-1].lexeme())) return equivInt(i-1);
  return i;
}
  

void
InputTree::
setEquivInts(SentRep& sr)
{
  int i;
  int len = sr.length();
  equivPos[0] = -1;
  for(i = 1 ; i <= len ; i++)
    {
      equivPos[i] = -1;
      int puncequiv = puncEquiv(i,sr);
      if(puncequiv < i) equivPos[i] = puncequiv;
      /*
      int editequiv = editEquiv(i);
      if(editequiv < i)
	{
	  equivPos[i] = editequiv;
	  equivPos[puncequiv] = editequiv;
	}
	*/
    }
}
     
void
InputTree::
init()
{
  for(int i = 0 ; i < 128 ; i++)
    {
      tempword[i] = "";
    }
  tempwordnum = 0;
}

InputTree::
~InputTree()
{
  InputTree  *subTree;
  InputTreesIter  subTreeIter = subTrees_.begin();
  for( ; subTreeIter != subTrees_.end() ; subTreeIter++ )
    {
      subTree = *subTreeIter;
      delete subTree;
    }
}

InputTree::
InputTree(istream& is)
{
  readParse(is);
}

istream&
operator >>( istream& is, InputTree& parse )
{
  if(parse.word() != "" || parse.term() != ""
     || parse.subTrees().size() > 0)
    error( "Reading into non-empty parse." );
  parse.readParse(is);
  return is;
}


void
InputTree::
readParse(istream& is)
{
  int pos = 0;
  start_ = pos;
  finish_ = pos;

  ECString temp = readNext(is);
  if(!is) return;
  if(temp != "(")
    {
      cerr << "Saw " << temp << endl;
      error("Should have seen an open paren here.");
    }
  /* get annotated symbols like NP-OBJ.  term_ = NP ntInfo_ = OBJ */
  temp = readNext(is);
  term_ = "S1";
  if(temp != "(")
    {
      if(temp == "S1" || temp == "TOP")
	{
	  temp = readNext(is);
	}
      else error("did not see legal topmost type");
    }
  if(temp != "(") error("Should have seen second open paren here.");
  InputTree* nextTree = newParse(is, pos, this);
  subTrees_.push_back(nextTree);
  finish_ = pos;
  headTree_ = nextTree->headTree_;
  temp = readNext(is);
  if(temp != ")") error("Shoul have closed paren here.");
}

InputTree*
InputTree::
newParse(istream& is, int& strt, InputTree* par)
{
  int strt1 = strt;
  ECString wrd;
  ECString trm;
  ECString ntInf;
  InputTrees subTrs;

  parseTerm(is, trm, ntInf);
  for( ; ; )
    {
      ECString temp = readNext(is);
      if(temp == "(")
	{
	  InputTree* nextTree = newParse(is, strt, NULL);
	  if(nextTree) subTrs.push_back(nextTree);
	}
      else if(temp == ")") break;
      else
	{
	  if(trm != "-NONE-")
	    {
	      wrd = temp;
	      strt++;
	    }
	}
    }
  InputTree* ans = new InputTree(strt1, strt, wrd, trm, ntInf, subTrs,
				 par, NULL);
  InputTreesIter iti = subTrs.begin();
  for(; iti != subTrs.end() ; iti++)
    {
      InputTree* st = *iti;
      st->parentSet() = ans;
    }
  
  if(wrd == "" && subTrs.size() == 0) return NULL;
  if(wrd != "")
    {
      ans->headTree() = ans;
    }
  else
    {
      int hpos = headPosFromTree(ans);
      ans->headTree() = ithInputTree(hpos, subTrs)->headTree();
    }
  
  return ans;
}

ECString&
InputTree::
readNext( istream& is ) 
{
  // if we already have a word, use it, and increment pointer to next word;
  //cerr << "RN1 " << tempwordnum << " " << tempword[tempwordnum] << endl;
  if( tempword[tempwordnum] != "" )
    {
      return tempword[tempwordnum++];
    }
  //cerr << "RN2" << endl;
  // else zero out point and stuff in 
  int tempnum;
  for(tempnum = 0 ; tempword[tempnum] != "" ; tempnum++ )
    tempword[tempnum] = "";
  tempwordnum = 0;
  // then go though next input, separating "()[]";
  int    wordnum  = 0 ;
  ECString  temp;
  is >> temp;
  //cerr << "In readnext " << temp << endl;
  for( tempnum = 0 ; tempnum < temp.length() ; tempnum++ )
    {
      char tempC = temp[tempnum];
      if(tempC == '(' || tempC == ')' ||
	 tempC == '[' || tempC == ']' )
	{
	  if( tempword[wordnum] != "" )
	    wordnum++;
	  tempword[wordnum++] += tempC;
	}
      else tempword[wordnum] += temp[tempnum];
    }
  return tempword[tempwordnum++];
}

/* if we see NP-OBJ make NP a, and -OBJ b */
void
InputTree::
parseTerm(istream& is, ECString& a, ECString& b)
{
  ECString temp = readNext(is);
  if(temp == "(" || temp == ")") error("Saw paren rather than term");
  int len = temp.length();
  size_t pos = temp.find("-");
  /* things like -RCB- will have a - at position 0 */
  if(pos < len && pos > 0)
    {
      ECString na(temp, 0, pos);
      ECString nb(temp, pos, len-pos);
      a = na;
      len = pos;
      b = nb;
    }
  else
    {
      a = temp;
      b = "";
    }
  pos = a.find("=");
  if(pos < len && pos > 0)
    {
      ECString na(temp, 0, pos);
      ECString nb(temp, pos, len-pos);
      a = na;
      len = pos;
      b += nb;
    }
  pos = a.find("|");
  if(pos < len && pos > 0)
    {
      ECString na(temp, 0, pos);
      ECString nb(temp, pos, len-pos);
      a = na;
      b += nb;
    }
}	   
	   

ostream&
operator <<( ostream& os, const InputTree& parse )
{
  parse.prettyPrint( os, 0, false );
  return os;
}

void 
InputTree::
printproper( ostream& os ) const
{
  if( word_.length() != 0 )
    {
      os << "(" << term_ << " " << word_ << ")";
    }
  else
    {
      os << "(";
      os <<  term_ << ntInfo_;
      ConstInputTreesIter  subTreeIter= subTrees_.begin();
      InputTree  *subTree;
      for( ; subTreeIter != subTrees_.end() ; subTreeIter++ )
	{
	  subTree = *subTreeIter;
	  os << " ";
	  subTree->printproper( os );
	}
      os << ")";
    }
}

void 
InputTree::
prettyPrint(ostream& os, int start, bool startingLine) const              
{
  if(start >= pageWidth) //if we indent to much, just give up and print it.
    {
      printproper(os);
      return;
    }
  if(startingLine)
    {
      //os << "\n";
      int numtabs = start/8;
      int numspace = start%8;
      int i;
      //for( i = 0 ; i < numtabs ; i++ ) os << "\t"; //indent;
      //for( i = 0 ; i < numspace ; i++ ) os << " "; //indent;
	  if(numtabs <= 1) os << " ";
	  if(numspace <= 1) os << " ";
    }
  /* if there is enough space to print the rest of the tree, do so */
  if(spaceNeeded() <= pageWidth-start || word_ != "")
    {
      printproper(os);
    }
  else
    {
      os << "(";
      os << term_ << ntInfo_;
      os << " ";
      /* we need 2 extra spaces, for "(", " "  */
      int newStart = start + 2 + term_.length() + ntInfo_.length();
      //move start to right of space after term_ for next thing to print
      start++; //but for rest just move one space in.
      ConstInputTreesIter  subTreeIter = subTrees_.begin();
      InputTree  *subTree;
      int whichSubTree = 0;
      for( ; subTreeIter != subTrees_.end() ; subTreeIter++ )
	{
	  subTree = *subTreeIter;
	  if(whichSubTree++ == 0)
	    {
	      subTree->prettyPrint(os, newStart, false);
	    }
	  else
	    {
	      subTree->prettyPrint(os, start, true);
	    }
	}
      os << ")";
    }
}

/* estimates how much space we need to print the rest of the currently
   print tree */
int
InputTree::
spaceNeeded() const
{
  int needed = 1; // 1 for blank;    
  int wordLen = word_.length();
  needed += wordLen;
  needed += 3; //3 for () and " ";
  needed += term_.length();
  needed += ntInfo_.length();
  if(word_ != "") return needed;
  ConstInputTreesIter  subTreeIter = subTrees_.begin();
  InputTree  *subTree;
  for( ; subTreeIter != subTrees_.end() ; subTreeIter++ )
    {
      subTree = *subTreeIter;
      needed += subTree->spaceNeeded();
    }
  return needed;
}

void
InputTree::
make(list<ECString>& words)
{
  if(word_ != "")
    {
      words.push_back(word_);
    }
  else
    {
      ConstInputTreesIter subTreeIter = subTrees().begin();
      InputTree  *subTree;
      for(; subTreeIter != subTrees().end() ; subTreeIter++)
	{
	  subTree = *subTreeIter;
	  subTree->make(words);
	}
    }
}


void
InputTree::
mapCompat( InputTree& correct, ParseStats& parseStats,
	   SentRep& sr )
{
  //cerr << "mC " << *this << endl;
  if( subTrees().empty()) return;
  parseStats.totalMap++;
  int    len = finish()-start();
  if( term_ == "S1" )
    {
      parseStats.obviousMap++;
      parseStats.correctMap++;
    }
  else if ( correct.compat( start(), finish(), sr ) )
      {
	parseStats.correctMap++;
      }
  InputTree                  *subTree;

  InputTreesIter  subTreeIter = subTrees_.begin();
  for( ; subTreeIter != subTrees_.end() ; subTreeIter++ )
    {
      subTree = *subTreeIter;
      subTree->mapCompat( correct, parseStats, sr );
    }
}
	
bool
InputTree::
compat( int strt, int fnsh, SentRep& sr ) 
{
  //cerr << "compat " << start() << " " << finish()
	 //<< " : " << strt << " " << fnsh << endl;
  int  position = start();
  if( finish() < strt ) return false;
  if( start() > fnsh ) return false;

  bool spc = punctuationClose(start(), strt, sr);
  if( start() > strt && !spc) return false;
  bool fpc = punctuationClose(finish(), fnsh, sr);
  if( finish() < fnsh && !fpc) return false;

  bool      foundStart = false;
  
  InputTreesIter  subTreeIter = subTrees_.begin();
  InputTree    *subTree;    
    
  for( ; ; )
    {
      bool pspc = punctuationClose(position, strt, sr );
      bool pfpc = punctuationClose(position, fnsh, sr );
      if(pspc) foundStart = true;
      if( pfpc )
	if( foundStart ) return true;
      if( position > strt && !foundStart && !pspc ) return false;
      if( position > fnsh && !pfpc ) return false;
      if(subTreeIter == subTrees_.end()) break;
      subTree = *subTreeIter;
      ++subTreeIter;
      if( subTree->compat( strt, fnsh, sr ) )
	return true;
      position = subTree->finish();
    }

  return false;
}
	
bool
InputTree::
exact( int strt, int fnsh )
{
  if( start() > strt ) return false;
  if( finish() < fnsh ) return false;
  /* this next line prevents counting matches against terminals.  Note
     that if we allow traces, then this needs to be made more complex. */
  if(subTrees().empty()) return false;  // needed.
  if( start() == strt && finish() == fnsh ) return true;
  InputTree    *subTree;    
  InputTreesIter  subTreeIter = subTrees_.begin();
  for( ; subTreeIter != subTrees_.end() ; subTreeIter++ )
    {
      subTree = *subTreeIter;
      if( subTree->exact( strt, fnsh ) )
	return true;
    }
  return false;
}
	
bool
InputTree::
lexact( int strt, int fnsh, ECString trm )
{
  if( start() > strt ) return false;
  if( finish() < fnsh ) return false;
  /* this next line prevents counting matches against terminals.  Note
     that if we allow traces, then this needs to be made more complex. */
  if(subTrees().empty()) return false;  // needed.
  if( start() == strt && finish() == fnsh && term() == trm ) return true;
  InputTree    *subTree;    
    
  InputTreesIter  subTreeIter = subTrees_.begin();
  for( ; subTreeIter != subTrees_.end() ; subTreeIter++ )
    {
      subTree = *subTreeIter;
      if( subTree->lexact( strt, fnsh, trm ) )
	return true;
    }
  return false;
}

void
InputTree::
mapExact( InputTree& computed, ParseStats& parseStats )
{
  if( subTrees().empty() )
    {
      parseStats.ignoredExact++;
      return;
    }
  if(term_ != "" && term_ != "S1")
    {
      /* now this simply records how many applicable constits there are */
      parseStats.totalExact++;
    }
  else parseStats.ignoredExact++;
  
  InputTree                      *subTree;
  InputTreesIter  subTreeIter = subTrees_.begin();
  for( ; subTreeIter != subTrees_.end() ; subTreeIter++ )
    {
      subTree = *subTreeIter;
      subTree->mapExact( computed, parseStats );
    }
}

void
InputTree::
precision( InputTree& correct, ParseStats& parseStats,
	   SentRep& sent)
{
  bool lexact1 = false;
  if( subTrees().empty() ) // if no subtrees this is a termina symbol;
    {
      parseStats.obviousPrecision++;
      return;
    }
  else if( term_ == "S1" ) parseStats.obviousPrecision++;
  else if( correct.lexact( start(), finish(), term_ ) )
    {
      lexact1 = true;
      parseStats.correctPrecision++;
      parseStats.otherWrong++;
      parseStats.totalPrecision++;
      parseStats.exactPrecision++;
    }
  else if( correct.lexact2( start(), finish(), term_, sent ) )
    {
      parseStats.otherWrong++;
      parseStats.totalPrecision++;
    }
  else parseStats.totalPrecision++;
  if( !lexact1 && term_ != "S1" && correct.exact( start(), finish() ) )
    {
      parseStats.correctPrecision++;
    }
  InputTree                      *subTree;
  InputTreesIter  subTreeIter = subTrees_.begin();
  for( ; subTreeIter != subTrees_.end() ; subTreeIter++ )
    {
      subTree = *subTreeIter;
      subTree->precision( correct, parseStats, sent );
    }
}

bool
InputTree::
punctuationClose(int pos1, int pos2, SentRep& sent)
{
  if(equivInt(pos1) == equivInt(pos2))
    return true;
  return false;
}
	  
	
bool
InputTree::
lexact2( int strt, int fnsh, ECString trm, SentRep& sent )
{
  if( finish() < strt ) return false;
  if( start() > fnsh ) return false;
  /* this next line prevents counting matches against terminals.  Note
     that if we allow traces, then this needs to be made more complex. */
  if(subTrees().empty()) return false;  // needed;
  bool termOk = false;
  if(term() == trm) termOk = true;
  else if((term() == "ADVP" || term() == "PRT")
	  && (trm == "ADVP" || trm == "PRT"))
    termOk = true;
  bool startClose = punctuationClose(start(), strt, sent);
  if(!startClose && start() > strt) return false;
  bool finishClose = punctuationClose(finish(), fnsh, sent);
  if(!finishClose && finish() < fnsh) return false;
  if( startClose && finishClose && termOk ) return true;
  InputTree    *subTree;    
  InputTreesIter  subTreeIter = subTrees_.begin();
  for( ; subTreeIter != subTrees_.end() ; subTreeIter++ )
    {
      subTree = *subTreeIter;
      if( subTree->lexact2( strt, fnsh, trm, sent ) )
	return true;
    }
  return false;
}

InputTree*
ithInputTree(int i, const list<InputTree*> l)
{
  list<InputTree*>::const_iterator li = l.begin();
  for(int j = 0 ; j < i ; j++)
    {
      assert(li != l.end());
      li++;
    }
  return *li;
}

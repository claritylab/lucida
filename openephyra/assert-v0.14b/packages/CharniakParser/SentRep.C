/*
 * Copyright 1999, Brown University, Providence, RI.
 * 
 *                         All Rights Reserved
 * 
 * Permission to use, copy, modify, and distribute this software and its
 * documentation for any purpose other than its incorporation into a
 * commercial product is hereby granted without fee, provided that the
 * above copyright notice appear in all copies and that both that
 * copyright notice and this permission notice appear in supporting
 * documentation, and that the name of Brown University not be used in
 * advertising or publicity pertaining to distribution of the software
 * without specific, written prior permission.
 * 
 * BROWN UNIVERSITY DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE,
 * INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR ANY
 * PARTICULAR PURPOSE.  IN NO EVENT SHALL BROWN UNIVERSITY BE LIABLE FOR
 * ANY SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

#include "SentRep.h"
#include <iostream.h>
#include "utils.h"
#include <assert.h>

const ECString	SentRep::sentence_closer_ = ".";

SentRep::
SentRep()
: length_( 0 )
{
}

SentRep::
SentRep( istream& istr )
: length_( 0 )
{
  sentRepCreator(istr, SentRep::STD);
}

SentRep::
SentRep(istream& istr, SentRep::SentLayout layout)
: length_( 0 )
{
  sentRepCreator(istr, layout);
}

SentRep::
SentRep(list<ECString> wtList) 
       : length_( 0 )
{
  int len =  wtList.size();
  length_ = len;
  if(length_ >= 1000) return;
  list<ECString>::iterator wi = wtList.begin();
  for(int i = 0 ; i < len ; i++)
    {
      words_[i].lexeme_ = *wi;
      words_[i].loc_ = i;
      wi++;
    }
}

void
SentRep::
sentRepCreator( istream& istr, SentRep::SentLayout layout)
{
  //cout << "ready" << endl;
  // SGML layout introduces sentence with <s> and ends it with </s>.
  if( layout == SGML )
    {
	  for(; istr;)
		{
		  Wrd	temp;
		  istr >> temp;
		  if( temp.lexeme() == "<s>" ) break;
		  else if( temp.lexeme() == "</s>" )
			warn( "found sentence end before intro; " );
		}
    }
  for( ; ; )
    {
      if(length_ >= 1000) break;
	  
	  istr >> words_[ length_ ];
	  words_[length_].loc_ = length_;
	  // EOF or error.  Note that because we are looking ahead, istr
	  // can be finished while we still have words left to output;
	  if( !istr && words_[length_].lexeme() == "" )
		{
		  // For asci text be somewhat looser;
		  if(layout != ASCI) length_ = 0;
		  return;
		}
	  // end of sentence
	  if(layout == STD && isSentCloser(words_[length_++]))
	    break;
	  else if( layout == SGML && words_[ length_++ ].lexeme() == "</s>" )
		{
		  length_--;
		  break;
		}
	}
}

SentRep::
SentRep( ewDciTokStrm& istr, SentRep::SentLayout layout )
: length_( 0 )
{
    // SGML layout introduces sentence with <s> and ends it with </s>.
    if( layout == SGML || layout == ASCI )
    {
	for(; !(!istr);)
	{
	    istr >>words_[length_];
	    //cerr << length_ << " " << words_[length_] << endl;
	    ECString temp = words_[length_].lexeme();
	    if(temp == "</DOC>")
	      {
		length_++;
		return;
	      }
	    if( temp == "<s>" )
		break;
	    else if( temp == "</s>" ) 
		warn( "found sentence end before s intro; " );
	}
    }
    for( ; ; )
    {
        if(length_ >= 1000) break;

	istr >> words_[ length_ ];
	words_[length_].loc_ = length_;
	if(words_[length_].lexeme() == "</DOC>")
	   {
	     length_++;
	     break;
	   }
	//cerr<<"Word "<< length_ << " = " << words_[length_].lexeme() << endl;
	// EOF or error.  Note that because we are looking ahead, istr
	// can be finished while we still have words left to output;
	if( !istr && words_[length_].lexeme().length() == 0 )
	{
	    // For asci text be somewhat looser;
	    if(layout != ASCI) length_ = 0;
	    return;
	}
	// end of sentence
	if( layout == STD && isSentCloser(words_[ length_++ ] ) )
	    break;
	else if( layout != STD && words_[ length_++ ].lexeme() == "</s>" )
	{
	    length_--;
	    break;
	}
      }
}



SentRep::
SentRep( const SentRep& sr )
: length_( sr.length_ )
{
    for( int i = 0; i < length_; i++ )
	words_[ i ] = sr.words_[ i ];
}


int
SentRep::
read( istream& istr )
{
    length_ = 0;
    for( ; ; )
    {
        assert(length_ < 1000);

	istr >> words_[ length_ ];
	// EOF or error 
	if( !istr )
	{
	    length_ = 0;
	    return 1;
	}
	// end of sentence
	if( isSentCloser(words_[ length_++ ] ) )
	    break;
    }
    return 0;
}

int
SentRep::length() const
{
    return length_;
}

SentRep&
SentRep::
operator= (const SentRep& sr)
{
    length_ = sr.length_;
    for( int i = 0; i < length_; i++ )
	words_[ i ] = sr.words_[ i ];
    return *this;
}

int
SentRep::
operator== (const SentRep& sr) const
{
#if 0
    if( this == &sr )
	return true;
    if( length_ != sr.length_ )
	return false;
    for( int i = 0; i < length_; i++ )
    {
	if( !( words_[ i ] == sr.words_[ i ] ) )
	    return false;
    }
    return true;
#endif
    return false;
}

ostream&
operator<< (ostream& os, const SentRep& sr)
{
    for( int i = 0; i < sr.length_; i++ )
	os << sr.words_[ i ] << " ";
    return os;
}

bool
SentRep::
isSentCloser(const Wrd& wrd)
{
  const ECString& s = wrd.lexeme();
  if(s == "." || s == "?" || s == "!" ) return true;
  else return false;
}
					  

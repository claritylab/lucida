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

#ifndef SENTREP_H
#define SENTREP_H

#include "Wrd.h"
#include <list.h>
#include "ewDciTokStrm.h"

class SentRep
{
public:
    enum	SentLayout { STD, SGML, ASCI };

    SentRep();
    SentRep( istream& istr );
    SentRep( istream& istr, SentRep::SentLayout layout );
    SentRep( ewDciTokStrm& istr, SentRep::SentLayout layout = STD );
    SentRep( const SentRep& sr );
    int	    	read( istream& istr );
    int	    	length() const;
    SentRep(list<ECString> wtList);
    SentRep&	operator= (const SentRep& sr );
    int	    	operator== ( const SentRep& sr ) const;
    const Wrd& operator[] ( int index ) const
	    	{   return words_[ index ];   }
    Wrd&       operator[] ( int index ) 
	    	{   return words_[ index ];   }

    bool       isSentCloser(const Wrd& wrd);

friend ostream& operator<< (ostream& os, const SentRep& sr);
friend class SentRepIter;

  protected:
    void        sentRepCreator( istream& istr, SentRep::SentLayout layout);
    int		length_;
    Wrd	words_[1000];

    static const ECString sentence_closer_;
};

class SentRepIter
{
public:
    SentRepIter( const SentRep& sr )
    	: srhandle_( sr ), index_( 0 )
    	{}
    void reset()
	{
	    index_ = 0;
	}
    const Wrd * nextWrd()
	{
	    if( index_ < srhandle_.length_ )
	        return &( srhandle_.words_[ index_++ ] );
	    else
		return NULL;
	}
private:
    const SentRep& srhandle_;
    int	index_;
};
#endif /* ! SENTREP_H */

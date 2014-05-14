/*
 * Copyright 1997, Brown University, Providence, RI.
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
/***       This is file  /pro/dpg/usl/ew/dcitokstrm/ewDciTokStrm.C        ****
****                                                                      ****
****    Terminology: a raw string, as read into this module from       ****
****    TokStrm::istr_ , is called a Wrd; a string that gets passed    ****
****    out of this module to a client program is called a Word.  When    ****
****    they differ, it is almost always because the Wrd has been split   ****
****    at punctuation marks into two or more Words.                      ***/


#include <ctype.h>
#include "Wrd.h"

#include "ewDciTokStrm.h"




inline int Cap( ECString );

inline int has_alnum( ECString, int );

inline int has_one( char, ECString, int );





ewDciTokStrm::
ewDciTokStrm( const ECString& name )
: istr_(name.c_str()),
  savedWrd_( "" ),             // holds not-yet-processed parts of current Wrd
  nextWrd_( "" ),              // holds "on-deck" Wrd
  ellipFlag( 0 ),                   // counts how many dots are in an ellipsis
  parenFlag( 0 )                        // ParenFlag = 0 except while words
{                                       //  within parens are being processed.
    //???nextWrd_ = flush_to_sentence();     //  Then its value tells whether there
}                                       //  was a finalPunc before the parens.



int docEnd = 0;


ECString                                    // Discard Wrds from input stream
ewDciTokStrm::                               //  until reaching the next <s> ;
flush_to_sentence()                          //  return the <s> . Return empty
{                                            //  string if stream runs dry.
    ECString	str;
    while( istr_ )
    {
	istr_ >> str;
	if( str == "</DOC>") docEnd = 1;
	if( str == "<s>" || str == "<S>" )
	   return str;
    }
    str = "";
    return str;
}


 
ECString
ewDciTokStrm::
nextWrd2()
{
  ECString str;
  if(!istr_) return "";
  istr_ >> str;
  //cerr << str << endl;
  return str;
}
                                                                   // page TWO

ECString          // Update savedWrd_ and nextWrd_; return next Wrd by call-
ewDciTokStrm::     // ing splitAtPunc on savedWrd_ .  Upon entry to this func-
read()             // tion, savedWrd_  may or may not be empty, and nextWrd_
                   // will be empty only if the input stream has run dry.
{
    if( !savedWrd_.length() )
    {
	savedWrd_ = nextWrd_;
	//if( !istr_ )
	  //  nextWrd_ = "";
        //else if( savedWrd_ == "</s>" )
	  //  nextWrd_ = flush_to_sentence();
	//else
	  //{
	  nextWrd_ = nextWrd2();
	    //istr_ >> nextWrd_;
	  //}
/*** Execution reaches this point iff. a new Wrd has just "come on deck." ***/

	if( parenFlag > 0 ) parenFlag++;
    }

    if( savedWrd_ == "<COMMENT>" )              // Comments occur only as the
    {                                           //  last Wrds in a <s>...</s>
	nextWrd_ = flush_to_sentence();         //  line.  Discard the comment
	savedWrd_ = "";                         //  itself and the "Paragraph-
	return "</s>";                          //  ing Error" (unbracketed
    }                                           //  text) that follows.

    while( savedWrd_ == "<s>"               // Starting in 1989, lines brack-
           &&  nextWrd_ == "@" )            //  eted as sentences but with an
    {                                       //   @  following the  <s>  hold
        //cerr << "before @ flush" << endl;				      
	savedWrd_ = flush_to_sentence();    //  data arranged as charts or
        //cerr << "aft @ flush" << endl;				      
	if( !istr_ )                        //  tables.  It is non-text-like
	    nextWrd_ = "";                  //  and should be discarded.
	else
	    istr_ >> nextWrd_;
    }
    if(docEnd)
      {
	docEnd = 0;
	return "</DOC>";
      }
                                        // remember that global savedWrd_ gets
                                                  // reset (side effect) while;
    ECString retWrd = splitAtPunc( savedWrd_ );   // <-- this line executes!

    if( ellipFlag && retWrd != "." ) { ellipFlag = 0; }             // update
    if( retWrd == "." ) { ellipFlag++; }                            // flags
    if( nextWrd_.length() && nextWrd_[0] == '('                      // before
	&& ( retWrd == "!" || retWrd == "?"                        // return
	     || retWrd == "." && ellipFlag != 3 ))                  //    .
    { parenFlag = 1; }                                               //    .
                                                                     //    .
    if( retWrd == "(" )                                             //    .
	if( !parenFlag ) { parenFlag = -1; }                         //    .
	else if( !( Cap(savedWrd_)                                   //    .
		    || !savedWrd_.length() && Cap(nextWrd_) ))       //    .
	{ parenFlag = 0; }                                           //    .
                                                                     //    .
    if( retWrd == ")" || retWrd == "</s>" || retWrd == "</S>" ) { parenFlag = 0; }     //.....

    if(retWrd == "(") retWrd = "-LRB-";
    if(retWrd == ")") retWrd = "-RRB-";
    if(retWrd == "{") retWrd = "-LCB-";
    if(retWrd == "}") retWrd = "-RCB-";
    if(retWrd == "[") retWrd = "-LSB-";
    if(retWrd == "]") retWrd = "-RSB-";
    return retWrd;
}




								// page THREE

ECString                     // Extract and return the first Wrd that can be
ewDciTokStrm::                // split off the front end of savedWrd_; reset
splitAtPunc( ECString seq )      // savedWrd_ to hold everything that's left.
{  
    static ECString	puncChars( ".?!,:;'(){}`\"#$%*@]" );

/***  These are punctuation characters that may be splitters.  The default
   action is to return (as separate Wrds) the substring that precedes it,
   the splitter itself, and the substring that follows it.  The code handles
   numerous special cases.  General observations:

    .  is a splitter only if it is a sentence terminator.
    ,  is usually a splitter, but not if flanked by digits.
    '  is a splitter if it's a single-quote, but not if it's an apostrophe.
    "  gets converted to a 2-char string: `` (open-quote) or '' (close-quote).
    * @  (splitters) are used as footnote or annotation flags.
    ]  began appearing in 1989; meaning obscure; always unmatched.
    - / &  are not splitters because they often occur in names and other
           unitary tokens, and we haven't found any good algorithms that
           separate those cases from the ones we'd like to split.
    Substrings of identical puncChars are treated as single Wrds.
                                                                          ***/

    int length = seq.length();
    int puncIndex = seq.find_first_of(puncChars );
    if(puncIndex >= length)
      puncIndex = -1;

/***  Move the puncIndex forward to skip over puncChars
      in contexts where they should not be treated as splitters.          ***/

    while( puncIndex >0 && puncIndex < length         // SKIP over non-splitter
           && ( seq[puncIndex] == ','                //  puncChars in numbers:
		|| seq[puncIndex] == '.'                //eg   5,000,000  
		|| seq[puncIndex] == ':' )              //eg   9,999.99
	   && puncIndex < length - 1                    //eg   2,6  (British)
	   && isdigit( seq[ puncIndex + 1 ] ) )         //eg   2.5:1  (odds)
    {                                                   //eg   11:45  (time)
	ECString sseq(seq, puncIndex+1, length- puncIndex -1 );
	int len2 = sseq.length();
	int nextp = sseq.find_first_of(puncChars);
	if(nextp >= len2) nextp = -1;
	if( nextp == -1)  puncIndex = -1;
        else  puncIndex += ( nextp + 1 );
    }

    while( puncIndex != -1 
	&& seq[ puncIndex ] == '.'
	&& !( puncIndex < length-1            // but NOT over a string of them
	      && seq[puncIndex+1] == '.' )                  //eg   ..  ...
	&& !( ( puncIndex == length-1         // and NOT over a dot that might
		|| !has_alnum( seq, puncIndex+1 ) )     // be a sentence-ender
	      &&
	      ( nextWrd_ == "</s>" ||
		nextWrd_ == "</S>"
		|| nextWrd_.length() > 0 && nextWrd_[0] == '(' ) ) )
    {
	ECString sseq(seq, puncIndex+1, length - puncIndex -1 );
        int len3 = sseq.length();
	int nextp = sseq.find_first_of(puncChars);
        if(nextp >= len3) nextp = -1;
	if( nextp == -1)  puncIndex = -1;
        else  puncIndex += ++nextp;
    }
                                                                  // page FOUR

    if( puncIndex != -1                       // Apostrophes within hyphenated
	&& seq[ puncIndex ] == '\''           // words are not splitters:
	&& has_one( '-', seq, puncIndex )  //eg  don't-touch  will-o'-the-wisp

	&& ( puncIndex > 0             // Exclude leading apostrophe (probable
	     || ( length > 2           // quote) unless year-digits follow:
		  && isdigit( seq[1] )
		  && isdigit( seq[2] ) )))       //eg   '60s-style   '55-'56
    {
	while( puncIndex < length            
	       && ( seq[puncIndex] == '\''
		    || seq[puncIndex] == '-'
		    || isalnum( seq[puncIndex] ) ))
	{   puncIndex++;  }
        if( seq[ puncIndex-1 ] == '\'' )        // exclude trailing apostrophe
	   puncIndex--;                         // (probably a close-quote):
	else if( puncIndex == length )       //eg ...'scarce as hen's-teeth')"
           puncIndex = -1;
    }

    if( length > 2 && puncIndex == 0                    // handle two-digit
	&& seq[0] == '\''                               // date abbreviations:
	&& isdigit( seq[1] ) && isdigit( seq[2] ) )
    {
	int index = 3;                                         //eg   '76
	if( index < length && seq[index] == 's' ) index++;     //eg   '60s
	if( index == length )
	    puncIndex = -1;
	else if( seq[index] == '"'     // if there are trailing chars, treat
		|| seq[index] == '.'   // the leading apostrophe as open-quote
		|| seq[index] == '?'   // unless you see a plausible puncChar:
		|| seq[index] == '!'                          //eg  (in '29!!)
		|| seq[index] == ',' || seq[index] == '\''
	      || seq[index] == ')' || seq[index] == ':' || seq[index] == ';' )
	    puncIndex = index;
    }              // otherwise it's probably open-quote so leave puncIndex==0
                        //eg    "I replied, '1985 was tough too.'" </s>


/** In the raw text, the closing dot of a "stateLike" abbreviation may serve
    double duty as sentence-ending punctuation.  In such cases we will insert
    a second dot to embody the sentence-ending function in a separate lexeme.
    If the given Wrd has additional characters beyond the closing dot of the
    stateLike Wrd: if it's a quotemark, insert the dot before it; if it's a
    close-paren, insert the dot before it if the parens enclose a complete 
    sentence (indicated by parenFlag >= 3) and after it otherwise; for any 
    other character, something is probably wrong, so give up (don't add a 
    second dot after all).                                                 **/

    if( puncIndex > 0 && seq[puncIndex] == '.'
	&& is_stateLike( seq.substr( 0,puncIndex+1 ) ) )
    {
	puncIndex++;            // The closing dot of a stateLike Wrd should
                                // be left attached to that Wrd in all cases.
	if( (nextWrd_ == "</s>" ||  nextWrd_ == "</S>")
	    && !has_one( '.', seq, puncIndex )       // Don't add a dot if it
	    && !has_one( '?', seq, puncIndex )       // has finalPunc already:
	    && !has_one( '!', seq, puncIndex )      //eg     Lee, Ky.). </s>
	    && !has_one( ':', seq, puncIndex )      //eg      why L.A.? </s>
	    && !has_one( ';', seq, puncIndex ) )    //eg     Lee, Ky.): </s>



								// page FIVE
	{
	    if( parenFlag == -1 )                       // The outer context
	    	if( has_one( ')', seq, puncIndex ) )    // had no finalPunc
		{                                       //   ...
		    int index = puncIndex;
		    while( seq[index] != ')' && index < length )    //  ...
		    {  index++;  }                                  // so add
		    savedWrd_
                     = seq.substr( puncIndex, 1+index-puncIndex ); // dot in-
		    savedWrd_ += "." ;
		    savedWrd_ += seq.substr(index+1,length-index-1);//side the
		    return seq.substr(0,puncIndex);             // close-paren
		} else {                                        // ...  unless
		    savedWrd_ = seq.substr(puncIndex,length-puncIndex);// close-paren
		    return seq.substr( 0,puncIndex );           // is missing!
		}

	    if( puncIndex < length
		&& ( parenFlag == 0 || parenFlag > 3 )
		&& ( seq[puncIndex] == '"' || seq[puncIndex] == '\''
					   || seq[puncIndex] == ')' ))
	    {
	        savedWrd_ = ".";
		savedWrd_ += seq.substr(puncIndex,length-puncIndex);// add dot before
		return seq.substr( 0,puncIndex );             //  quote or
	    }                                                //  close-paren

	    if( puncIndex == length )
	    {				     // If != length, there were unex-
		savedWrd_ = ".";	     //  pected trailing characters,
		return seq;		     //  and we won't add a dot.
	    }
	}

	savedWrd_ = seq.substr(puncIndex,length-puncIndex); // default: don't add dot
	return seq.substr( 0,puncIndex );
    }                                  // end code for stateLike abbreviations
	    
    if( puncIndex == -1 )                           // This means seq contains
    {                                               // no splitting puncChars.
	savedWrd_ = "";
        return seq;
    }

    if( puncIndex > 0 )
    {                                               // begin puncIndex>0 cases
	if( seq[puncIndex] == '\'' )
	{                             // trailing-or-internal-apostrophe cases
	    if( seq[ puncIndex-1 ] == 'n'
		&& puncIndex < length-1
		&& seq[ puncIndex+1 ] == 't' )
	    {                                         // begin ...n't... cases
		if( length > 4 )
		{
		    if( seq.substr( 0,5 ) == "can't" || seq.substr( 0,5 ) == "Can't" )
		    {   savedWrd_ = seq.substr( 2,length-2 );
		        ECString retv =  seq.substr( 0,2 );
			retv += "n";
                	return retv;
		    }
		    if( seq.substr( 0,5 ) == "won't" || seq.substr( 0,5 ) == "Won't" )
		    {	savedWrd_ = seq.substr( 2,length-2 );
		        ECString retv =  seq.substr( 0,1 );
			retv += "ill";
                	return retv;
		    }
		    if( seq.substr( 0,5 ) == "ain't" || seq.substr( 0,5 ) == "Ain't" )
		    {	savedWrd_ = seq.substr( 2,length-2 );
                	return "IS";       // caps as warning: "am"|"are"|"is"
		    }
		}
                                                                 // page SIX

		if( puncIndex > 1 )          // split  ...n't...  BEFORE the n
		{   savedWrd_ = seq.substr( puncIndex-1,length-puncIndex+1 );
		    return seq.substr( 0, puncIndex-1 );
		}
                                        // now puncIndex must == 1 and
		savedWrd_ = seq.substr( 3,length-3 );   // length must be >= 3, so we
		return seq.substr( 0,3 );      // split  n't...  AFTER the t
	    }                                           // end ...n't... cases

                                          // special trailing-apostrophe cases
	    if( length > 4 &&
		  ( seq.substr( 0,5 ) == "goin'" || seq.substr( 0,5 ) == "doin'" )
	       || length > 6 &&
		  ( seq.substr( 0,7 ) == "lookin'" || seq.substr( 0,7 ) == "fishin'" ) )
	    {
		savedWrd_ = seq.substr( puncIndex+1,length-puncIndex-1 );
		return seq.substr( 0, puncIndex+1 );
	    }

	    if( length > 2 &&
		(  seq.substr(0,3) == "A's"   //eg  Oakland A's    Type A's die young
		|| seq.substr(0,3) == "O's"    // the Orioles are a minor league team
		|| seq.substr(0,3) == "R's" ))   //eg    the three R's
	    {
		savedWrd_ = seq.substr( 3,length-3 );
		return seq.substr( 0,3 );
	    }
                                     // one-letter prefixed contraction cases:

	    if( puncIndex == 1                //eg  O'Brien  Tam o'Shanter
		&& ( seq[0] == 'O'            //eg      Land o' Lakes
		     || seq[0] == 'o'         //eg   d'Amboise   D'Annunzio
		     || seq[0] == 'D'         //eg  coup d'etat  Louis L'Amour
		     || seq[0] == 'd'         //eg  enfin, l'etat?  c'est moi!
		     || seq[0] == 'C'         //eg       c'mon, y'all!
		     || seq[0] == 'c'          
		     || seq[0] == 'L'     // but try not to conflate with one-
		     || seq[0] == 'l'     // letter wd + suffixed contraction:
		     || seq[0] == 'N'     //eg    I'm    I'll   p's and q's
		     || seq[0] == 'n'
		     || seq[0] == 'Y'
		     || seq[0] == 'y' ))

	    {   savedWrd_ = seq.substr( 2,length-2 );
		return seq.substr( 0,2 );
	    }

            // multi-letter prefixed-contraction or trailing-apostrophe cases:

	    if( puncIndex == 2 && seq[1] == 'a'      // "house of" (Catalan?):
                && ( seq[0] == 'C' || seq[0] == 'c'))       //eg   Ca'Dario
	    {                                               //eg   ca' Hans
		savedWrd_ = seq.substr( 3,length-3 );
		return seq.substr( 0,3 );
	    }

	    if( puncIndex == 4 && seq[3] == 'l'         // "of the" (Italian): 
                && ( seq[0] == 'D' || seq[0] == 'd')    //eg   dell'   Dell'
                && seq[1] == 'e' && seq[2] == 'l' )
	    {   savedWrd_ = seq.substr( 5,length-5 );
		return seq.substr( 0,5 );
	    }
	}                         // end trailing-or-internal-apostrophe cases
                                                                 // page SEVEN

	savedWrd_ =
          seq.substr(puncIndex,length-puncIndex);    // default: split before puncChar
	return seq.substr( 0,puncIndex );
    }                                                 // end puncIndex>0 cases


/*** Almost always, double-quote means open-quote iff it is prepended to a  **
**   Wrd that contains alphanumeric chars.  Spacing errors defeat this rule **
**    --  he said, " I do!"  -- but in the wsj text most unattached "-s do  **
**   happen to mean close-quote, and that's how this rule treats them.    ***/

    if( seq[0] == '\"' )
    {
	savedWrd_ = seq.substr( 1,length-1 );
	if( has_alnum( seq,0 )
	    || seq == "\"." && nextWrd_ == "." )   //eg  he said, ". . . Yes!"
	{   return "``";  }
	else { return "''"; }
    }

    if( length == 1 )                                     // isolated puncChar
    {   savedWrd_ = "";
	return seq;
    }
                                   // now puncIndex = 0 and length must be > 1

    if( seq[0] == '\'' )  // leading-apostrophe and suffixed-contraction cases
    {
	if( (  seq[1] == 's' || seq[1] == 'S'           //eg   's  'S  'm  'd
            || seq[1] == 'm' || seq[1] == 'd'
	    || seq[1] == 't' || seq[1] == 'N' )         //eg  't Hooge [Dutch]
           && (length == 2 || !isalpha( seq[2] )) )     //eg    Sweet 'N Low
	{
	    savedWrd_ = seq.substr( 2, length-2 );
	    return seq.substr( 0,2 );
	}

	if( length > 2
	    && (   (seq[1] == 'r' && seq[2] == 'e')                //eg   're
		|| (seq[1] == 'v' && seq[2] == 'e')                //eg   've
		|| (seq[1] == 'l' && seq[2] == 'l')                //eg   'll
		|| (seq[1] == 'e' && seq[2] == 'm')                //eg   'em
		|| (seq[1] == 'n' && seq[2] == '\'') )     //eg  rock 'n' roll
	    && (length == 3 || !isalpha( seq[3] )) )
	{
	    savedWrd_ = seq.substr( 3,length-3 );
	    return seq.substr( 0,3 );
	}

	if( length > 3
	    && ( seq[1] == 't' || seq[1] == 'T' )          //eg   'tis   'til
	    && seq[2] == 'i'
	    && ( seq[3] == 's' || seq[3] == 'l' ) )
	{
	    savedWrd_ = seq.substr( 4,length-4 );
	    return seq.substr( 0,4 );
	}

	if( seq == "'cause" )               //eg   ". . . just 'cause I care!"
	{
	    savedWrd_ = "";
	    return seq;
	}
    }                 // end leading-apostrophe and suffixed-contraction cases
								// page EIGHT

    if( seq[puncIndex] == seq[ puncIndex+1 ] )    // treat string of identical
    {                                             //  puncChars as a Wrd
	int lastSame = 1;
	while( lastSame < length  &&  seq[0] == seq[lastSame]  )
	    lastSame++;
	if( lastSame == length )
	    lastSame--;
	savedWrd_ = seq.substr( lastSame,length-lastSame );
	return seq.substr( 0, lastSame );
    }

    savedWrd_ = seq.substr( 1,length-1 );            // default treatment: return the
    return seq.substr( 0,1 );                        //  splitter puncChar as a Wrd
}





int
ewDciTokStrm::                         // A stateLike Wrd has a final period
is_stateLike( const ECString &str )   // that shouldn't be split off from it.
{
    int E = str.length() - 1;
    return(                              
    ( E > 2                             // strings that end    <alph>.<alph>.
      && str[E] == '.'                  // are all assumed to be stateLike:
      && isalpha( str[E-1] )
      && str[E-2] == '.'                //eg     N.Y.  U.S.A.   a.m.  G.m.b.H.
      && isalpha( str[E-3] )
      && str[E-1] != 's' )    // unless last <alph> is  s  (plural+finalPunc?)
                                       //eg    I hate those S.O.B.s. </s>
    ||	str == "Ala."  
    ||	str == "Ca."	|| str == "Calif."	|| str == "Conn." 
    ||	str == "Fla."	|| str == "Ga."		|| str == "Ill."
    ||	str == "Ky."	|| str == "La."		
    || str == "Mass."	|| str == "Me."
    ||	str == "Md."	|| str == "Mich."	|| str == "Mo."
    ||	str == "Pa."	|| str == "Va."		|| str == "Vt."
    ||	str == "Wash."	|| str == "Wyo."	|| str == "W.Va."
    ||	str == "Co."	|| str == "co."		|| str == "Ltd."
    ||	str == "Cos."	|| str == "cos."	|| str == "Corp."
    ||	str == "Inc."	|| str == "INC."	|| str == "CORP."
    ||	str == "Jr."	|| str == "Sr." 	|| str == "Blvd."
    ||	str == "St."	|| str == "Ave."
    );
}





int                                         // Tells whether S has occurrences
has_one( char C, ECString S, int P )                  // of C to the right of
{                                                      // (or at) position P
    for( int i=P; i<S.length(); i++ )
    {	if( S[i] == C ) return 1;  }
    return 0;
}




								// page NINE
int                                        // Tells whether S has alphanumeric
has_alnum( ECString S, int P )            // chars to the right of (or at) P
{
    for( int i=P; i<S.length(); i++ )
    {	if( isalnum(S[i]) ) return 1;  }
    return 0;
}



int
Cap( ECString S )				     // Tells whether S begins
{                                                    //  with a capital letter
    if( !S.length() ) return 0;
    if( S[0] >= 'A' && S[0] <= 'Z' ) return 1;
    if( S[0] == '"' || S[0] == '\'' )
      {
	ECString s1;
	s1 += S[1];
	return Cap( s1 );
      }
    return 0;
}

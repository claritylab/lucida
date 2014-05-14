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

#ifndef EWDCI_H
#define EWDCI_H

#include <fstream.h>
#include "ECString.h"

/***      This is file  /pro/dpg/usl/ew/dcitokstrm/ewDciTokStrm.h         ****
****                                                                      ****
****   The code in this module is optimized to fit the peculiarities of   ****
****    wsj/text/198* .  Run any "improved" version side by side with     ****
****   this one and inspect the actual outputs, before changing this.     ***/


class ewDciTokStrm
{
  public:
    ewDciTokStrm( const ECString& );
    ECString	read();
    int		operator!()
      {
        return (savedWrd_.length()==0 && nextWrd_.length()==0 && !istr_ );
      }

 protected:
    ifstream	istr_;
  private:
 
    virtual ECString   nextWrd2();
    ECString	savedWrd_;
    ECString	nextWrd_;
    int         parenFlag;
    int		ellipFlag;
    ECString	flush_to_sentence();
    ECString	splitAtPunc( ECString );
    int         is_stateLike( const ECString& );
};
  

#endif /* ! EWDCI_H */

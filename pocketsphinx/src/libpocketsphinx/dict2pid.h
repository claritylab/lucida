/* -*- c-basic-offset: 4; indent-tabs-mode: nil -*- */
/* ====================================================================
 * Copyright (c) 1999-2004 Carnegie Mellon University.  All rights
 * reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer. 
 *
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 *
 * This work was supported in part by funding from the Defense Advanced 
 * Research Projects Agency and the National Science Foundation of the 
 * United States of America, and the CMU Sphinx Speech Consortium.
 *
 * THIS SOFTWARE IS PROVIDED BY CARNEGIE MELLON UNIVERSITY ``AS IS'' AND 
 * ANY EXPRESSED OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, 
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL CARNEGIE MELLON UNIVERSITY
 * NOR ITS EMPLOYEES BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT 
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, 
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY 
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT 
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE 
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 * ====================================================================
 *
 */
/*
 * dict2pid.h -- Triphones for dictionary
 * 
 * **********************************************
 * CMU ARPA Speech Project
 *
 * Copyright (c) 1999 Carnegie Mellon University.
 * ALL RIGHTS RESERVED.
 * **********************************************
 * 
 * HISTORY
 * $Log$
 * Revision 1.1  2006/04/05  20:27:30  dhdfu
 * A Great Reorganzation of header files and executables
 * 
 * Revision 1.9  2006/02/22 21:05:16  arthchan2003
 * Merged from branch SPHINX3_5_2_RCI_IRII_BRANCH:
 *
 * 1, Added logic to handle bothe composite and non composite left
 * triphone.  Composite left triphone's logic (the original one) is
 * tested thoroughly. The non-composite triphone (or full triphone) is
 * found to have bugs.  The latter is fended off from the users in the
 * library level.
 *
 * 2, Fixed dox-doc.
 *
 * Revision 1.8.4.5  2005/11/17 06:13:49  arthchan2003
 * Use compressed right context in expansion in triphones.
 *
 * Revision 1.8.4.4  2005/10/17 04:48:45  arthchan2003
 * Free resource correctly in dict2pid.
 *
 * Revision 1.8.4.3  2005/10/07 19:03:38  arthchan2003
 * Added xwdssid_t structure.  Also added compression routines.
 *
 * Revision 1.8.4.2  2005/09/25 19:13:31  arthchan2003
 * Added optional full triphone expansion support when building context phone mapping.
 *
 * Revision 1.8.4.1  2005/07/17 05:20:30  arthchan2003
 * Fixed dox-doc.
 *
 * Revision 1.8  2005/06/21 21:03:49  arthchan2003
 * 1, Introduced a reporting routine. 2, Fixed doyxgen documentation, 3, Added  keyword.
 *
 * Revision 1.5  2005/06/13 04:02:57  archan
 * Fixed most doxygen-style documentation under libs3decoder.
 *
 * Revision 1.4  2005/04/21 23:50:26  archan
 * Some more refactoring on the how reporting of structures inside kbcore_t is done, it is now 50% nice. Also added class-based LM test case into test-decode.sh.in.  At this moment, everything in search mode 5 is already done.  It is time to test the idea whether the search can really be used.
 *
 * Revision 1.3  2005/03/30 01:22:46  archan
 * Fixed mistakes in last updates. Add
 *
 * 
 * 14-Sep-1999	M K Ravishankar (rkm@cs.cmu.edu) at Carnegie Mellon University
 * 		Added dict2pid_comsseq2sen_active().
 * 
 * 04-May-1999	M K Ravishankar (rkm@cs.cmu.edu) at Carnegie Mellon University
 * 		Started.
 */


#ifndef _S3_DICT2PID_H_
#define _S3_DICT2PID_H_

/* System headers. */
#include <stdio.h>

/* SphinxBase headers. */
#include <sphinxbase/logmath.h>
#include <sphinxbase/bitvec.h>

/* Local headers. */
#include "s3types.h"
#include "bin_mdef.h"
#include "dict.h"

/** \file dict2pid.h
 * \brief Building triphones for a dictionary. 
 *
 * This is one of the more complicated parts of a cross-word
 * triphone model decoder.  The first and last phones of each word
 * get their left and right contexts, respectively, from other
 * words.  For single-phone words, both its contexts are from other
 * words, simultaneously.  As these words are not known beforehand,
 * life gets complicated.
 */

#ifdef __cplusplus
extern "C" {
#endif
#if 0
} /* Fool Emacs into not indenting things. */
#endif

/**
 * \struct xwdssid_t
 * \brief cross word triphone model structure 
 */

typedef struct {
    s3ssid_t  *ssid;	/**< Senone Sequence ID list for all context ciphones */
    s3cipid_t *cimap;	/**< Index into ssid[] above for each ci phone */
    int32     n_ssid;	/**< #Unique ssid in above, compressed ssid list */
} xwdssid_t;

/**
   \struct dict2pid_t
   \brief Building composite triphone (as well as word internal triphones) with the dictionary. 
*/

typedef struct {
    int refcount;

    bin_mdef_t *mdef;           /**< Model definition, used to generate
                                   internal ssids on the fly. */
    dict_t *dict;               /**< Dictionary this table refers to. */

    /*Notice the order of the arguments */
    /* FIXME: This is crying out for compression - in Mandarin we have
     * 180 context independent phones, which makes this an 11MB
     * array. */
    s3ssid_t ***ldiph_lc;	/**< For multi-phone words, [base][rc][lc] -> ssid; filled out for
				   word-initial base x rc combinations in current vocabulary */


    xwdssid_t **rssid;          /**< Right context state sequence id table 
                                   First dimension: base phone,
                                   Second dimension: left context. 
                                */


    s3ssid_t ***lrdiph_rc;      /**< For single-phone words, [base][lc][rc] -> ssid; filled out for
                                   single-phone base x lc combinations in current vocabulary */

    xwdssid_t **lrssid;          /**< Left-Right context state sequence id table 
                                    First dimension: base phone,
                                    Second dimension: left context. 
                                 */
} dict2pid_t;

/** Access macros; not designed for arbitrary use */
#define dict2pid_rssid(d,ci,lc)  (&(d)->rssid[ci][lc])
#define dict2pid_ldiph_lc(d,b,r,l) ((d)->ldiph_lc[b][r][l])
#define dict2pid_lrdiph_rc(d,b,l,r) ((d)->lrdiph_rc[b][l][r])

/**
 * Build the dict2pid structure for the given model/dictionary
 */
dict2pid_t *dict2pid_build(bin_mdef_t *mdef,   /**< A  model definition*/
                           dict_t *dict        /**< An initialized dictionary */
    );

/**
 * Retain a pointer to dict2pid
 */
dict2pid_t *dict2pid_retain(dict2pid_t *d2p);  

/**
 * Free the memory dict2pid structure
 */
int dict2pid_free(dict2pid_t *d2p /**< In: the d2p */
    );

/**
 * Return the senone sequence ID for the given word position.
 */
s3ssid_t dict2pid_internal(dict2pid_t *d2p,
                           int32 wid,
                           int pos);

/**
 * Add a word to the dict2pid structure (after adding it to dict).
 */
int dict2pid_add_word(dict2pid_t *d2p,
                      int32 wid);

/**
 * For debugging
 */
void dict2pid_dump(FILE *fp,        /**< In: a file pointer */
                   dict2pid_t *d2p /**< In: a dict2pid_t structure */
    );

/** Report a dict2pid data structure */
void dict2pid_report(dict2pid_t *d2p /**< In: a dict2pid_t structure */
    );

/**
 * Get number of rc 
 */
int32 get_rc_nssid(dict2pid_t *d2p,  /**< In: a dict2pid */
		   s3wid_t w         /**< In: a wid */
    );

/**
 * Get RC map 
 */
s3cipid_t* dict2pid_get_rcmap(dict2pid_t *d2p,  /**< In: a dict2pid */
			      s3wid_t w        /**< In: a wid */
    );

#if 0
{ /* Stop indent from complaining */
#endif
#ifdef __cplusplus
}
#endif


#endif

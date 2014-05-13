/* -*- c-basic-offset: 4; indent-tabs-mode: nil -*- */
/* ====================================================================
 * Copyright (c) 1999-2001 Carnegie Mellon University.  All rights
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
 * ad.c -- Wraps a "sphinx-II standard" audio interface around the basic audio
 * 		utilities.
 *
 * HISTORY
 * 
 * 11-Jun-96	M K Ravishankar (rkm@cs.cmu.edu) at Carnegie Mellon University.
 * 		Modified to conform to new A/D API.
 * 
 * 12-May-96	M K Ravishankar (rkm@cs.cmu.edu) at Carnegie Mellon University.
 * 		Dummy template created.
 */

#include <stdio.h>
#include <string.h>
#include <config.h>

#include "prim_type.h"
#include "ad.h"

#define QUIT(x)		{fprintf x; exit(-1);}


ad_rec_t *
ad_open_dev(const char *dev, int32 samples_per_sec)
{
    fprintf(stderr, "A/D library not implemented\n");
    return NULL;
}


ad_rec_t *
ad_open_sps(int32 samples_per_sec)
{
    fprintf(stderr, "A/D library not implemented\n");
    return NULL;
}

ad_rec_t *
ad_open(void)
{
    return ad_open_sps(DEFAULT_SAMPLES_PER_SEC);
}


int32
ad_start_rec(ad_rec_t * r)
{
    return -1;
}


int32
ad_stop_rec(ad_rec_t * r)
{
    return -1;
}


int32
ad_read(ad_rec_t * r, int16 * buf, int32 max)
{
    return -1;
}


int32
ad_close(ad_rec_t * r)
{
    return 0;
}

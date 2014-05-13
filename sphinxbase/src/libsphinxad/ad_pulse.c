/* -*- c-basic-offset: 4; indent-tabs-mode: nil -*- */
/* ====================================================================
 * Copyright (c) 1999-2011 Carnegie Mellon University.  All rights
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

/* Input for Pulseaudio */

#include <stdio.h>
#include <string.h>
#include <config.h>

#include <pulse/pulseaudio.h>
#include <pulse/simple.h>

#include "prim_type.h"
#include "ad.h"

ad_rec_t *
ad_open_dev(const char *dev, int32 samples_per_sec)
{
    ad_rec_t *handle;
    pa_simple *pa;
    pa_sample_spec ss;
    int error;

    ss.format = PA_SAMPLE_S16LE;
    ss.channels = 1;
    ss.rate = 16000;
    
    pa = pa_simple_new(NULL, "ASR", PA_STREAM_RECORD, dev, "Speech", &ss, NULL, NULL, &error);
    if (pa == NULL) {
        fprintf(stderr, "Error opening audio device %s for capture: %s\n", dev, pa_strerror(error));
        return NULL;
    }

    if ((handle = (ad_rec_t *) calloc(1, sizeof(ad_rec_t))) == NULL) {
        fprintf(stderr, "Failed to allocate memory for ad device\n");
	return NULL;
    }

    handle->pa = pa;
    handle->recording = 0;
    handle->sps = samples_per_sec;
    handle->bps = sizeof(int16);

    return handle;
}


ad_rec_t *
ad_open_sps(int32 samples_per_sec)
{
    return ad_open_dev(DEFAULT_DEVICE, samples_per_sec);
}

ad_rec_t *
ad_open(void)
{
    return ad_open_sps(DEFAULT_SAMPLES_PER_SEC);
}


int32
ad_start_rec(ad_rec_t * r)
{
    if (r->recording)
        return AD_ERR_GEN;

    r->recording = 1;

    return 0;
}


int32
ad_stop_rec(ad_rec_t * r)
{
    if (!r->recording)
        return AD_ERR_GEN;

    r->recording = 0;

    return 0;
}


int32
ad_read(ad_rec_t * r, int16 * buf, int32 max)
{
    int error;

    if (!r->recording)
	return AD_EOF;
	
	if (max > 2048) {
	    max = 2048;
    }
	
    if (pa_simple_read(r->pa, (void*)buf, max * 2, &error) < 0) {
	fprintf(stderr, "Failed to read speech: %s\n", pa_strerror(error));
    }
    
    return max;
}


int32
ad_close(ad_rec_t * r)
{
    if (r->pa == NULL)
        return AD_ERR_NOT_OPEN;

    if (r->recording) {
        if (ad_stop_rec(r) < 0)
            return AD_ERR_GEN;
    }
    pa_simple_free(r->pa);
    free(r);

    return 0;
}

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
/* -*- mode:c; indent-tabs-mode:t; c-basic-offset:4; comment-column:40 -*-
 *
 * Sphinx II libad (Linux)
 * ^^^^^^^^^^^^^^^^^^^^^^^
 * $Id: ad_alsa.c,v 1.6 2001/12/11 00:24:48 lenzo Exp $
 *
 * John G. Dorsey (jd5q+@andrew.cmu.edu)
 * Engineering Design Research Center
 * Carnegie Mellon University
 * ***************************************************************************
 * 
 * REVISION HISTORY
 *
 * 18-Mar-2006  David Huggins-Daines <dhuggins@cs.cmu.edu>
 *		Update this to the ALSA 1.0 API.
 *
 * 12-Dec-2000  David Huggins-Daines <dhd@cepstral.com> at Cepstral LLC
 *		Make this at least compile with the new ALSA API.
 *
 * 05-Nov-1999	Sean Levy (snl@stalphonsos.com) at St. Alphonsos, LLC.
 *		Ported to ALSA so I can actually get working full-duplex.
 *
 * 09-Aug-1999  Kevin Lenzo (lenzo@cs.cmu.edu) at Cernegie Mellon University.
 *              Incorporated nickr@cs.cmu.edu's changes (marked below) and
 *              SPS_EPSILON to allow for sample rates that are "close enough".
 * 
 * 15-Jun-1999	M. K. Ravishankar (rkm@cs.cmu.edu) at Carnegie Mellon Univ.
 *		Consolidated all ad functions into
 *		this one file.  Added ad_open_sps().
 * 		Other cosmetic changes for consistency (e.g., use of err.h).
 * 
 * 18-May-1999	Kevin Lenzo (lenzo@cs.cmu.edu) added <errno.h>.
 */


#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <alsa/asoundlib.h>
#include <errno.h>
#include <config.h>
#include <unistd.h>

#include "prim_type.h"
#include "ad.h"


#define AUDIO_FORMAT SND_PCM_SFMT_S16_LE        /* 16-bit signed, little endian */
#define INPUT_GAIN   (85)
#define SPS_EPSILON   200

static int
setparams(int32 sps, snd_pcm_t * handle)
{
    snd_pcm_hw_params_t *hwparams;
    unsigned int out_sps, buffer_time, period_time;
    int err;

    snd_pcm_hw_params_alloca(&hwparams);
    err = snd_pcm_hw_params_any(handle, hwparams);
    if (err < 0) {
        fprintf(stderr, "Can not configure this PCM device: %s\n",
                snd_strerror(err));
        return -1;
    }

    err =
        snd_pcm_hw_params_set_access(handle, hwparams,
                                     SND_PCM_ACCESS_RW_INTERLEAVED);
    if (err < 0) {
        fprintf(stderr,
                "Failed to set PCM device to interleaved: %s\n",
                snd_strerror(err));
        return -1;
    }

    err =
        snd_pcm_hw_params_set_format(handle, hwparams, SND_PCM_FORMAT_S16);
    if (err < 0) {
        fprintf(stderr,
                "Failed to set PCM device to 16-bit signed PCM: %s\n",
                snd_strerror(err));
        return -1;
    }

    err = snd_pcm_hw_params_set_channels(handle, hwparams, 1);
    if (err < 0) {
        fprintf(stderr, "Failed to set PCM device to mono: %s\n",
                snd_strerror(err));
        return -1;
    }

    out_sps = sps;
    err =
        snd_pcm_hw_params_set_rate_near(handle, hwparams, &out_sps, NULL);
    if (err < 0) {
        fprintf(stderr, "Failed to set sampling rate: %s\n",
                snd_strerror(err));
        return -1;
    }
    if (abs(out_sps - sps) > SPS_EPSILON) {
        fprintf(stderr,
                "Available samping rate %d is too far from requested %d\n",
                out_sps, sps);
        return -1;
    }

    /* Set buffer time to the maximum. */
    err = snd_pcm_hw_params_get_buffer_time_max(hwparams, &buffer_time, 0);
    period_time = buffer_time / 4;
    err = snd_pcm_hw_params_set_period_time_near(handle, hwparams,
                                                 &period_time, 0);
    if (err < 0) {
        fprintf(stderr, "Failed to set period time to %u: %s\n",
                period_time, snd_strerror(err));
        return -1;
    }
    err = snd_pcm_hw_params_set_buffer_time_near(handle, hwparams,
                                                 &buffer_time, 0);
    if (err < 0) {
        fprintf(stderr, "Failed to set buffer time to %u: %s\n",
                buffer_time, snd_strerror(err));
        return -1;
    }

    err = snd_pcm_hw_params(handle, hwparams);
    if (err < 0) {
        fprintf(stderr, "Failed to set hwparams: %s\n", snd_strerror(err));
        return -1;
    }

    err = snd_pcm_nonblock(handle, 1);
    if (err < 0) {
        fprintf(stderr, "Failed to set non-blocking mode: %s\n",
                snd_strerror(err));
        return -1;
    }
    return 0;
}

ad_rec_t *
ad_open_dev(const char *dev, int32 sps)
{
    ad_rec_t *handle;
    snd_pcm_t *dspH;

    int err;

    if (dev == NULL)
        dev = DEFAULT_DEVICE;

    err = snd_pcm_open(&dspH, dev, SND_PCM_STREAM_CAPTURE, 0);
    if (err < 0) {
        fprintf(stderr,
                "Error opening audio device %s for capture: %s\n",
                dev, snd_strerror(err));
        return NULL;
    }

    if (setparams(sps, dspH) < 0) {
        return NULL;
    }
    if ((handle = (ad_rec_t *) calloc(1, sizeof(ad_rec_t))) == NULL) {
        fprintf(stderr, "calloc(%d) failed\n", (int)sizeof(ad_rec_t));
        abort();
    }

    handle->dspH = dspH;
    handle->recording = 0;
    handle->sps = sps;
    handle->bps = sizeof(int16);

    return (handle);
}

ad_rec_t *
ad_open_sps(int32 sps)
{
    return ad_open_dev(DEFAULT_DEVICE, sps);
}

ad_rec_t *
ad_open(void)
{
    return ad_open_sps(DEFAULT_SAMPLES_PER_SEC);
}


int32
ad_close(ad_rec_t * handle)
{
    if (handle->dspH == NULL)
        return AD_ERR_NOT_OPEN;

    if (handle->recording) {
        if (ad_stop_rec(handle) < 0)
            return AD_ERR_GEN;
    }
    snd_pcm_close(handle->dspH);
    free(handle);

    return (0);
}


int32
ad_start_rec(ad_rec_t * handle)
{
    int err;

    if (handle->dspH == NULL)
        return AD_ERR_NOT_OPEN;

    if (handle->recording)
        return AD_ERR_GEN;

    err = snd_pcm_prepare(handle->dspH);
    if (err < 0) {
        fprintf(stderr, "snd_pcm_prepare failed: %s\n", snd_strerror(err));
        return AD_ERR_GEN;
    }
    err = snd_pcm_start(handle->dspH);
    if (err < 0) {
        fprintf(stderr, "snd_pcm_start failed: %s\n", snd_strerror(err));
        return AD_ERR_GEN;
    }
    handle->recording = 1;

    return (0);
}


int32
ad_stop_rec(ad_rec_t * handle)
{
    int err;

    if (handle->dspH == NULL)
        return AD_ERR_NOT_OPEN;

    if (!handle->recording)
        return AD_ERR_GEN;

    err = snd_pcm_drop(handle->dspH);
    if (err < 0) {
        fprintf(stderr, "snd_pcm_drop failed: %s\n", snd_strerror(err));
        return AD_ERR_GEN;
    }
    handle->recording = 0;

    return (0);
}


int32
ad_read(ad_rec_t * handle, int16 * buf, int32 max)
{
    int32 length, err;

    if (!handle->recording) {
	fprintf(stderr, "Recording is stopped, start recording with ad_start_rec\n");
	return AD_EOF;
    }

    length = snd_pcm_readi(handle->dspH, buf, max);
    if (length == -EAGAIN) {
        length = 0;
    }
    else if (length == -EPIPE) {
        fprintf(stderr, "Input overrun, read calls are too rare (non-fatal)\n");
        err = snd_pcm_prepare(handle->dspH);
	if (err < 0) {
		fprintf(stderr, "Can't recover from underrun: %s\n",
			snd_strerror(err));
		return AD_ERR_GEN;
	}
        length = 0;
    }
    else if (length == -ESTRPIPE) {
        fprintf(stderr, "Resuming sound driver (non-fatal)\n");
	while ((err = snd_pcm_resume(handle->dspH)) == -EAGAIN)
		usleep(10000); /* Wait for the driver to wake up */
	if (err < 0) {
		err = snd_pcm_prepare(handle->dspH);
		if (err < 0) {
			fprintf(stderr, "Can't recover from underrun: %s\n",
				snd_strerror(err));
			return AD_ERR_GEN;
		}
	}
	length = 0;
    }
    else if (length < 0) {
	fprintf(stderr, "Audio read error: %s\n",
		snd_strerror(length));
	return AD_ERR_GEN;
    }
    return length;
}

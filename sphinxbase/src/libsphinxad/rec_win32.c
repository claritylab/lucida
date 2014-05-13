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
 * rec.c -- low level audio recording for Windows NT/95.
 *
 * HISTORY
 * 
 * 19-Jan-1999	M K Ravishankar (rkm@cs.cmu.edu) at Carnegie Mellon University
 * 		Added AD_ return codes.  Added ad_open_sps_bufsize(), and
 * 		ad_rec_t.n_buf.
 * 
 * 07-Mar-98	M K Ravishankar (rkm@cs.cmu.edu) at Carnegie Mellon University
 * 		Added ad_open_sps(), and made ad_open() call it.
 * 
 * 10-Jun-96	M K Ravishankar (rkm@cs.cmu.edu) at Carnegie Mellon University
 * 		Added ad_rec_t type to all calls.
 * 
 * 03-Jun-96	M K Ravishankar (rkm@cs.cmu.edu) at Carnegie Mellon University
 * 		Created.
 */


#include <windows.h>
#include <mmsystem.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "sphinxbase/prim_type.h"
#include "sphinxbase/ad.h"


#define DEFAULT_N_WI_BUF	32      /* #Recording bufs */
#define WI_BUFSIZE		2500    /* Samples/buf (Why this specific value??
                                           So that at reasonable sampling rates
                                           data is returned frequently enough.) */

/* Silvio Moioli: using OutputDebugStringW instead of OutputDebugString */
#ifdef _WIN32_WCE
#include "ckd_alloc.h"
static void
wavein_error(char *src, int32 ret)
{
    TCHAR errbuf[512];
    wchar_t* werrbuf;
    size_t len;

    waveOutGetErrorText(ret, errbuf, sizeof(errbuf));
    len = mbstowcs(NULL, errbuf, 0) + 1;
    werrbuf = ckd_calloc(len, sizeof(*werrbuf));
    mbstowcs(werrbuf, errbuf, len);

    OutputDebugStringW(werrbuf);
}

#else
static void
wavein_error(char *src, int32 ret)
{
    char errbuf[1024];

    waveInGetErrorText(ret, errbuf, sizeof(errbuf));
    fprintf(stderr, "%s error %d: %s\n", src, ret, errbuf);
}
#endif


static void
wavein_free_buf(ad_wbuf_t * b)
{
    GlobalUnlock(b->h_whdr);
    GlobalFree(b->h_whdr);
    GlobalUnlock(b->h_buf);
    GlobalFree(b->h_buf);
}


static int32
wavein_alloc_buf(ad_wbuf_t * b, int32 samples_per_buf)
{
    HGLOBAL h_buf;              /* handle to data buffer */
    LPSTR p_buf;                /* pointer to data buffer */
    HGLOBAL h_whdr;             /* handle to header */
    LPWAVEHDR p_whdr;           /* pointer to header */

    /* Allocate data buffer */
    h_buf =
        GlobalAlloc(GMEM_MOVEABLE | GMEM_SHARE,
                    samples_per_buf * sizeof(int16));
    if (!h_buf) {
        fprintf(stderr, "GlobalAlloc failed\n");
        return -1;
    }
    if ((p_buf = GlobalLock(h_buf)) == NULL) {
        GlobalFree(h_buf);
        fprintf(stderr, "GlobalLock failed\n");
        return -1;
    }

    /* Allocate WAVEHDR structure */
    h_whdr = GlobalAlloc(GMEM_MOVEABLE | GMEM_SHARE, sizeof(WAVEHDR));
    if (h_whdr == NULL) {
        GlobalUnlock(h_buf);
        GlobalFree(h_buf);

        fprintf(stderr, "GlobalAlloc failed\n");
        return -1;
    }
    if ((p_whdr = GlobalLock(h_whdr)) == NULL) {
        GlobalUnlock(h_buf);
        GlobalFree(h_buf);
        GlobalFree(h_whdr);

        fprintf(stderr, "GlobalLock failed\n");
        return -1;
    }

    b->h_buf = h_buf;
    b->p_buf = p_buf;
    b->h_whdr = h_whdr;
    b->p_whdr = p_whdr;

    p_whdr->lpData = p_buf;
    p_whdr->dwBufferLength = samples_per_buf * sizeof(int16);
    p_whdr->dwUser = 0L;
    p_whdr->dwFlags = 0L;
    p_whdr->dwLoops = 0L;

    return 0;
}


static int32
wavein_enqueue_buf(HWAVEIN h, LPWAVEHDR whdr)
{
    int32 st;

    if ((st = waveInPrepareHeader(h, whdr, sizeof(WAVEHDR))) != 0) {
        wavein_error("waveInPrepareHeader", st);
        return -1;
    }
    if ((st = waveInAddBuffer(h, whdr, sizeof(WAVEHDR))) != 0) {
        wavein_error("waveInAddBuffer", st);
        return -1;
    }

    return 0;
}


static HWAVEIN
wavein_open(int32 samples_per_sec, int32 bytes_per_sample)
{
    WAVEFORMATEX wfmt;
    int32 st;
    HWAVEIN h;

    if (bytes_per_sample != sizeof(int16)) {
        fprintf(stderr, "bytes/sample != %d\n", sizeof(int16));
        return NULL;
    }

    wfmt.wFormatTag = WAVE_FORMAT_PCM;
    wfmt.nChannels = 1;
    wfmt.nSamplesPerSec = samples_per_sec;
    wfmt.nAvgBytesPerSec = samples_per_sec * bytes_per_sample;
    wfmt.nBlockAlign = bytes_per_sample;
    wfmt.wBitsPerSample = 8 * bytes_per_sample;

    /* There should be a check here for a device of the desired type; later... */

    st = waveInOpen((LPHWAVEIN) & h, WAVE_MAPPER,
                    (LPWAVEFORMATEX) & wfmt, (DWORD) 0L, 0L,
                    (DWORD) CALLBACK_NULL);
    if (st != 0) {
        wavein_error("waveInOpen", st);
        return NULL;
    }

    return h;
}


static int32
wavein_close(ad_rec_t * r)
{
    int32 i, st;

    /* Unprepare all buffers; multiple unprepares of the same buffer are benign */
    for (i = 0; i < r->n_buf; i++) {
        /* Unpreparing an unprepared buffer, on the other hand, fails
           on Win98/WinME, though this is not documented - dhuggins@cs,
           2004-07-14 */
        if (!(r->wi_buf[i].p_whdr->dwFlags & WHDR_PREPARED))
            continue;
        st = waveInUnprepareHeader(r->h_wavein,
                                   r->wi_buf[i].p_whdr, sizeof(WAVEHDR));
        if (st != 0) {
            wavein_error("waveInUnprepareHeader", st);
            return -1;
        }
    }

    /* Free buffers */
    for (i = 0; i < r->n_buf; i++)
        wavein_free_buf(&(r->wi_buf[i]));
    free(r->wi_buf);

    if ((st = waveInClose(r->h_wavein)) != 0) {
        wavein_error("waveInClose", st);
        return -1;
    }

    free(r);

    return 0;
}


ad_rec_t *
ad_open_sps_bufsize(int32 sps, int32 bufsize_msec)
{
    ad_rec_t *r;
    int32 i, j;
    HWAVEIN h;

    if ((h = wavein_open(sps, sizeof(int16))) == NULL)
        return NULL;

    if ((r = (ad_rec_t *) malloc(sizeof(ad_rec_t))) == NULL) {
        fprintf(stderr, "malloc(%d) failed\n", sizeof(ad_rec_t));
        waveInClose(h);
        return NULL;
    }

    r->n_buf = ((sps * bufsize_msec) / 1000) / WI_BUFSIZE;
    if (r->n_buf < DEFAULT_N_WI_BUF)
        r->n_buf = DEFAULT_N_WI_BUF;
    printf("Allocating %d buffers of %d samples each\n", r->n_buf,
           WI_BUFSIZE);

    if ((r->wi_buf =
         (ad_wbuf_t *) calloc(r->n_buf, sizeof(ad_wbuf_t))) == NULL) {
        fprintf(stderr, "calloc(%d,%d) failed\n", r->n_buf,
                sizeof(ad_wbuf_t));
        free(r);
        waveInClose(h);

        return NULL;
    }
    for (i = 0; i < r->n_buf; i++) {
        if (wavein_alloc_buf(&(r->wi_buf[i]), WI_BUFSIZE) < 0) {
            for (j = 0; j < i; j++)
                wavein_free_buf(&(r->wi_buf[j]));
            free(r->wi_buf);
            free(r);
            waveInClose(h);

            return NULL;
        }
    }

    r->h_wavein = h;
    r->opened = 1;
    r->recording = 0;
    r->curbuf = r->n_buf - 1;   /* current buffer with data for application */
    r->curlen = 0;              /* #samples in curbuf remaining to be consumed */
    r->lastbuf = r->curbuf;
    r->sps = sps;
    r->bps = sizeof(int16);     /* HACK!! Hardwired value for bytes/sec */

    return r;
}

/* FIXME: Dummy function, doesn't actually use dev. */
ad_rec_t *
ad_open_dev(const char *dev, int32 sps)
{
    return (ad_open_sps_bufsize
            (sps, WI_BUFSIZE * DEFAULT_N_WI_BUF * 1000 / sps));
}


ad_rec_t *
ad_open_sps(int32 sps)
{
    return (ad_open_sps_bufsize
            (sps, WI_BUFSIZE * DEFAULT_N_WI_BUF * 1000 / sps));
}


ad_rec_t *
ad_open(void)
{
    return (ad_open_sps(DEFAULT_SAMPLES_PER_SEC));      /* HACK!! Rename this constant */
}


int32
ad_close(ad_rec_t * r)
{
    if (!r->opened)
        return AD_ERR_NOT_OPEN;

    if (r->recording)
        if (ad_stop_rec(r) < 0)
            return AD_ERR_WAVE;

    if (wavein_close(r) < 0)
        return AD_ERR_WAVE;

    return 0;
}


int32
ad_start_rec(ad_rec_t * r)
{
    int32 i;

    if ((!r->opened) || r->recording)
        return -1;

    for (i = 0; i < r->n_buf; i++)
        if (wavein_enqueue_buf(r->h_wavein, r->wi_buf[i].p_whdr) < 0)
            return AD_ERR_WAVE;
    r->curbuf = r->n_buf - 1;   /* current buffer with data for application */
    r->curlen = 0;              /* #samples in curbuf remaining to be consumed */

    if (waveInStart(r->h_wavein) != 0)
        return AD_ERR_WAVE;

    r->recording = 1;

    return 0;
}


int32
ad_stop_rec(ad_rec_t * r)
{
    int32 i, st;

    if ((!r->opened) || (!r->recording))
        return -1;

    if (waveInStop(r->h_wavein) != 0)
        return AD_ERR_WAVE;

    if ((st = waveInReset(r->h_wavein)) != 0) {
        wavein_error("waveInReset", st);
        return AD_ERR_WAVE;
    }

    /* Wait until all buffers marked done */
    for (i = 0; i < r->n_buf; i++)
        while (!(r->wi_buf[i].p_whdr->dwFlags & WHDR_DONE));

    if ((r->lastbuf = r->curbuf - 1) < 0)
        r->lastbuf = r->n_buf - 1;

    r->recording = 0;

    return 0;
}


int32
ad_read(ad_rec_t * r, int16 * buf, int32 max)
{
    int32 t, st, len;
    LPWAVEHDR whdr;
    int16 *sysbufp;

    if (!r->opened)
        return AD_ERR_NOT_OPEN;

    /* Check if all recorded data exhausted */
    if ((!r->recording) && (r->curbuf == r->lastbuf)
        && (r->curlen == 0))
        return AD_EOF;

    len = 0;
    while (max > 0) {
        /* Look for next buffer with recording data */
        if (r->curlen == 0) {
            /* No current buffer with data; get next buffer in sequence if available */
            t = r->curbuf + 1;
            if (t >= r->n_buf)
                t = 0;

            if (!(r->wi_buf[t].p_whdr->dwFlags & WHDR_DONE))
                return len;

            r->curbuf = t;
            r->curlen = r->wi_buf[t].p_whdr->dwBytesRecorded >> 1;
            r->curoff = 0;
        }

        /* Copy data from curbuf to buf */
        whdr = r->wi_buf[r->curbuf].p_whdr;
        t = (max < r->curlen) ? max : r->curlen;        /* #Samples to copy */

        if (t > 0) {
            sysbufp = (int16 *) (whdr->lpData);
            memcpy(buf, sysbufp + r->curoff, t * sizeof(int16));

            buf += t;
            max -= t;
            r->curoff += t;
            r->curlen -= t;
            len += t;
        }

        /* If curbuf empty recycle it to system if still recording */
        if (r->curlen == 0) {
            if (r->recording) {
                /* Return empty buffer to system */
                st = waveInUnprepareHeader(r->h_wavein,
                                           whdr, sizeof(WAVEHDR));
                if (st != 0) {
                    wavein_error("waveInUnprepareHeader", st);
                    return AD_ERR_WAVE;
                }

                if (wavein_enqueue_buf(r->h_wavein, whdr) < 0)
                    return AD_ERR_WAVE;

            }
            else if (r->curbuf == r->lastbuf) {
                return len;
            }
        }
    }

    return len;
}

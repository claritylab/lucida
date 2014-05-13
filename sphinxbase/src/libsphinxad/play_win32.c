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
 * HISTORY
 * 
 * 17-Apr-98	M K Ravishankar (rkm@cs.cmu.edu) at Carnegie Mellon University
 * 		Added ad_open_play_sps(), and made ad_open_play() call it.
 * 
 * 10-Jun-96	M K Ravishankar (rkm@cs.cmu.edu) at Carnegie Mellon University
 * 		Added ad_play_t type to all calls.
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


#define WO_BUFSIZE	3200    /* Samples/buf */
#define N_WO_BUF	2       /* #Playback bufs */

/* Silvio Moioli: using OutputDebugStringW instead of OutputDebugString */
#ifdef _WIN32_WCE
#include "ckd_alloc.h"
static void
waveout_error(char *src, int32 ret)
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
waveout_error(char *src, int32 ret)
{
    char errbuf[1024];

    waveOutGetErrorText(ret, errbuf, sizeof(errbuf));
    fprintf(stderr, "%s error %d: %s\n", src, ret, errbuf);
}
#endif


static void
waveout_free_buf(ad_wbuf_t * b)
{
    GlobalUnlock(b->h_whdr);
    GlobalFree(b->h_whdr);
    GlobalUnlock(b->h_buf);
    GlobalFree(b->h_buf);
}


static int32
waveout_alloc_buf(ad_wbuf_t * b, int32 samples_per_buf)
{
    HGLOBAL h_buf;
    LPSTR p_buf;
    HGLOBAL h_whdr;
    LPWAVEHDR p_whdr;

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
waveout_enqueue_buf(HWAVEOUT h, LPWAVEHDR whdr)
{
    int32 st;

    if ((st = waveOutPrepareHeader(h, whdr, sizeof(WAVEHDR))) != 0) {
        waveout_error("waveOutPrepareHeader", st);
        return -1;
    }

    if ((st = waveOutWrite(h, whdr, sizeof(WAVEHDR))) != 0) {
        waveout_error("waveOutWrite", st);
        return -1;
    }

    return 0;
}


static HWAVEOUT
waveout_open(int32 samples_per_sec, int32 bytes_per_sample)
{
    WAVEFORMATEX wfmt;
    int32 st;
    HWAVEOUT h;

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
    wfmt.cbSize = 0;

    /* There should be a check here for a device of the desired type; later... */

    st = waveOutOpen((LPHWAVEOUT) & h, WAVE_MAPPER,
                     (LPWAVEFORMATEX) & wfmt, (DWORD) 0L, 0L,
                     (DWORD) CALLBACK_NULL);
    if (st != 0) {
        waveout_error("waveOutOpen", st);
        return NULL;
    }

    return h;
}


static void
waveout_mem_cleanup(ad_play_t * p, int32 n_buf)
{
    int32 i;

    for (i = 0; i < n_buf; i++)
        waveout_free_buf(&(p->wo_buf[i]));
    if (p->wo_buf)
        free(p->wo_buf);
    if (p->busy)
        free(p->busy);
}


static int32
waveout_close(ad_play_t * p)
{
    int32 st;

    waveout_mem_cleanup(p, N_WO_BUF);

    if ((st = waveOutClose(p->h_waveout)) != 0) {
        waveout_error("waveOutClose", st);
        return -1;
    }

    free(p);

    return 0;
}


ad_play_t *
ad_open_play_sps(int32 sps)
{
    ad_play_t *p;
    int32 i;
    HWAVEOUT h;

    if ((h = waveout_open(sps, sizeof(int16))) == NULL)
        return NULL;

    if ((p = (ad_play_t *) calloc(1, sizeof(ad_play_t))) == NULL) {
        fprintf(stderr, "calloc(1,%d) failed\n", sizeof(ad_play_t));
        waveOutClose(h);
        return NULL;
    }
    if ((p->wo_buf =
         (ad_wbuf_t *) calloc(N_WO_BUF, sizeof(ad_wbuf_t))) == NULL) {
        fprintf(stderr, "calloc(%d,%d) failed\n", N_WO_BUF,
                sizeof(ad_wbuf_t));
        free(p);
        waveOutClose(h);

        return NULL;
    }
    if ((p->busy = (char *) calloc(N_WO_BUF, sizeof(char))) == NULL) {
        fprintf(stderr, "calloc(%d,%d) failed\n", N_WO_BUF, sizeof(char));
        waveout_mem_cleanup(p, 0);
        free(p);
        waveOutClose(h);

        return NULL;
    }
    for (i = 0; i < N_WO_BUF; i++) {
        if (waveout_alloc_buf(&(p->wo_buf[i]), WO_BUFSIZE) < 0) {
            waveout_mem_cleanup(p, i);
            free(p);
            waveOutClose(h);

            return NULL;
        }
    }

    p->h_waveout = h;
    p->playing = 0;
    p->opened = 1;
    p->nxtbuf = 0;
    p->sps = sps;
    p->bps = sizeof(int16);     /* HACK!! Hardwired value for bytes/sec */

    return p;
}


ad_play_t *
ad_open_play(void)
{
    return (ad_open_play_sps(DEFAULT_SAMPLES_PER_SEC));
}


int32
ad_close_play(ad_play_t * p)
{
    if (!p->opened)
        return 0;

    if (p->playing)
        if (ad_stop_play(p) < 0)
            return -1;

    if (waveout_close(p) < 0)
        return -1;

    return 0;
}


int32
ad_start_play(ad_play_t * p)
{
    int32 i;

    if ((!p->opened) || p->playing)
        return -1;

    for (i = 0; i < N_WO_BUF; i++)
        p->busy[i] = 0;
    p->nxtbuf = 0;
    p->playing = 1;

    return 0;
}


int32
ad_stop_play(ad_play_t * p)
{
    int32 i, st;
    LPWAVEHDR whdr;

    if ((!p->opened) || (!p->playing))
        return -1;

#if 0
    whdr->dwUser = (plen <= 0) ? 1 : 0;
#endif

    /* Wait for all buffers to be emptied and unprepare them */
    for (i = 0; i < N_WO_BUF; i++) {
        whdr = p->wo_buf[i].p_whdr;

        while (p->busy[i] && (!(whdr->dwFlags & WHDR_DONE)))
            Sleep(100);

        st = waveOutUnprepareHeader(p->h_waveout, whdr, sizeof(WAVEHDR));
        if (st != 0) {
            waveout_error("waveOutUnprepareHeader", st);
            return -1;
        }

        p->busy[i] = 0;
    }

    return 0;
}


int32
ad_write(ad_play_t * p, int16 * buf, int32 size)
{
    int32 i, k, len, st;
    LPWAVEHDR whdr;

    if ((!p->opened) || (!p->playing))
        return -1;

    len = 0;

    for (i = 0; (i < N_WO_BUF) && (size > 0); i++) {
        whdr = p->wo_buf[p->nxtbuf].p_whdr;

        if (p->busy[p->nxtbuf]) {
            if (!(whdr->dwFlags & WHDR_DONE))
                return len;

            st = waveOutUnprepareHeader(p->h_waveout, whdr,
                                        sizeof(WAVEHDR));
            if (st != 0) {
                waveout_error("waveOutUnprepareHeader", st);
                return -1;
            }

            p->busy[p->nxtbuf] = 0;
        }

        k = (size > WO_BUFSIZE) ? WO_BUFSIZE : size;

        whdr->dwBufferLength = k * sizeof(int16);
        memcpy(whdr->lpData, (LPSTR) buf, k * sizeof(int16));

        if (waveout_enqueue_buf(p->h_waveout, whdr) < 0)
            return -1;

        buf += k;
        size -= k;
        len += k;

        p->busy[(p->nxtbuf)++] = 1;
        if (p->nxtbuf >= N_WO_BUF)
            p->nxtbuf = 0;
    }

    return len;
}

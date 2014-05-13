/* -*- c-basic-offset: 4; indent-tabs-mode: nil -*- */
/* ====================================================================
 * Copyright (c) 1999-2010 Carnegie Mellon University.  All rights
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
 * continuous.c - Simple pocketsphinx command-line application to test
 *                both continuous listening/silence filtering from microphone
 *                and continuous file transcription.
 */

/*
 * This is a simple example of pocketsphinx application that uses continuous listening
 * with silence filtering to automatically segment a continuous stream of audio input
 * into utterances that are then decoded.
 * 
 * Remarks:
 *   - Each utterance is ended when a silence segment of at least 1 sec is recognized.
 *   - Single-threaded implementation for portability.
 *   - Uses audio library; can be replaced with an equivalent custom library.
 */

#include <stdio.h>
#include <string.h>
#include <assert.h>

#if !defined(_WIN32_WCE)
#include <signal.h>
#include <setjmp.h>
#endif
#if defined(WIN32) && !defined(GNUWINCE)
#include <time.h>
#else
#include <sys/types.h>
#include <sys/time.h>
#endif

#include <sphinxbase/err.h>
#include <sphinxbase/ad.h>

#include "pocketsphinx.h"

static const arg_t cont_args_def[] = {
    POCKETSPHINX_OPTIONS,
    /* Argument file. */
    {"-argfile",
     ARG_STRING,
     NULL,
     "Argument file giving extra arguments."},
    {"-adcdev",
     ARG_STRING,
     NULL,
     "Name of audio device to use for input."},
    {"-infile",
     ARG_STRING,
     NULL,
     "Audio file to transcribe."},
    {"-time",
     ARG_BOOLEAN,
     "no",
     "Print word times in file transcription."},
    CMDLN_EMPTY_OPTION
};

static ps_decoder_t *ps;
static cmd_ln_t *config;
static FILE *rawfd;

static void
print_word_times(int32 start)
{
    ps_seg_t *iter = ps_seg_iter(ps, NULL);
    while (iter != NULL) {
        int32 sf, ef, pprob;
        float conf;

        ps_seg_frames(iter, &sf, &ef);
        pprob = ps_seg_prob(iter, NULL, NULL, NULL);
        conf = logmath_exp(ps_get_logmath(ps), pprob);
        printf("%s %f %f %f\n", ps_seg_word(iter), (sf + start) / 100.0,
               (ef + start) / 100.0, conf);
        iter = ps_seg_next(iter);
    }
}

/*
 * Continuous recognition from a file
 */
static void
recognize_from_file()
{

    int16 adbuf[4096];

    const char *hyp;
    const char *uttid;

    int32 k;
    uint8 cur_vad_state, vad_state;

    char waveheader[44];
    if ((rawfd = fopen(cmd_ln_str_r(config, "-infile"), "rb")) == NULL) {
        E_FATAL_SYSTEM("Failed to open file '%s' for reading",
                       cmd_ln_str_r(config, "-infile"));
    }

    //skip wav header
    fread(waveheader, 1, 44, rawfd);
    cur_vad_state = 0;
    ps_start_utt(ps, NULL);
    while ((k = fread(adbuf, sizeof(int16), 4096, rawfd)) > 0) {
        ps_process_raw(ps, adbuf, k, FALSE, FALSE);
        vad_state = ps_get_vad_state(ps);
        if (cur_vad_state && !vad_state) {
            //speech->silence transition,
            //time to end utterance and start new one
            ps_end_utt(ps);
            hyp = ps_get_hyp(ps, NULL, &uttid);
            printf("%s: %s\n", uttid, hyp);
            fflush(stdout);
            ps_start_utt(ps, NULL);
        }
        cur_vad_state = vad_state;
    }
    ps_end_utt(ps);
    hyp = ps_get_hyp(ps, NULL, &uttid);
    printf("%s: %s\n", uttid, hyp);
    fflush(stdout);

    fclose(rawfd);
}

/* Sleep for specified msec */
static void
sleep_msec(int32 ms)
{
#if (defined(WIN32) && !defined(GNUWINCE)) || defined(_WIN32_WCE)
    Sleep(ms);
#else
    /* ------------------- Unix ------------------ */
    struct timeval tmo;

    tmo.tv_sec = 0;
    tmo.tv_usec = ms * 1000;

    select(0, NULL, NULL, NULL, &tmo);
#endif
}

/*
 * Main utterance processing loop:
 *     for (;;) {
 * 	   start utterance and wait for speech to process
 *     decoding till end-of-utterance silence will be detected
 * 	   print utterance result;
 *     }
 */
static void
recognize_from_microphone()
{
    ad_rec_t *ad;
    int16 adbuf[4096];
    uint8 cur_vad_state, vad_state;
    int32 k;
    char const *hyp;
    char const *uttid;

    if ((ad = ad_open_dev(cmd_ln_str_r(config, "-adcdev"),
                          (int) cmd_ln_float32_r(config,
                                                 "-samprate"))) == NULL)
        E_FATAL("Failed to open audio device\n");
    if (ad_start_rec(ad) < 0)
        E_FATAL("Failed to start recording\n");

    if (ps_start_utt(ps, NULL) < 0)
        E_FATAL("Failed to start utterance\n");
    cur_vad_state = 0;
    /* Indicate listening for next utterance */
    printf("READY....\n");
    fflush(stdout);
    fflush(stderr);
    for (;;) {
        if ((k = ad_read(ad, adbuf, 4096)) < 0)
            E_FATAL("Failed to read audio\n");
        sleep_msec(100);
        ps_process_raw(ps, adbuf, k, FALSE, FALSE);
        vad_state = ps_get_vad_state(ps);
        if (vad_state && !cur_vad_state) {
            //silence -> speech transition,
            // let user know that he is heard
            printf("Listening...\n");
            fflush(stdout);
        }
        if (!vad_state && cur_vad_state) {
            //speech -> silence transition, 
            //time to start new utterance
            ps_end_utt(ps);
            hyp = ps_get_hyp(ps, NULL, &uttid);
            printf("%s: %s\n", uttid, hyp);
            fflush(stdout);
            //Exit if the first word spoken was GOODBYE
            if (hyp && (strcmp(hyp, "good bye") == 0))
                break;
            if (ps_start_utt(ps, NULL) < 0)
                E_FATAL("Failed to start utterance\n");
            /* Indicate listening for next utterance */
            printf("READY....\n");
            fflush(stdout);
            fflush(stderr);
        }
        cur_vad_state = vad_state;
    }
    ad_close(ad);
}

static jmp_buf jbuf;
static void
sighandler(int signo)
{
    longjmp(jbuf, 1);
}

int
main(int argc, char *argv[])
{
    char const *cfg;

    config = cmd_ln_parse_r(NULL, cont_args_def, argc, argv, TRUE);

    /* Handle argument file as -argfile. */
    if (config && (cfg = cmd_ln_str_r(config, "-argfile")) != NULL) {
        config = cmd_ln_parse_file_r(config, cont_args_def, cfg, FALSE);
    }
    if (config == NULL)
        return 1;

    ps_default_search_args(config);
    ps = ps_init(config);
    if (ps == NULL)
        return 1;

    E_INFO("%s COMPILED ON: %s, AT: %s\n\n", argv[0], __DATE__, __TIME__);

    if (cmd_ln_str_r(config, "-infile") != NULL) {
        recognize_from_file();
    }
    else {

        /* Make sure we exit cleanly (needed for profiling among other things) */
        /* Signals seem to be broken in arm-wince-pe. */
#if !defined(GNUWINCE) && !defined(_WIN32_WCE) && !defined(__SYMBIAN32__)
        signal(SIGINT, &sighandler);
#endif

        if (setjmp(jbuf) == 0) {
            recognize_from_microphone();
        }
    }

    ps_free(ps);
    return 0;
}

/** Silvio Moioli: Windows CE/Mobile entry point added. */
#if defined(_WIN32_WCE)
#pragma comment(linker,"/entry:mainWCRTStartup")
#include <windows.h>

//Windows Mobile has the Unicode main only
int
wmain(int32 argc, wchar_t * wargv[])
{
    char **argv;
    size_t wlen;
    size_t len;
    int i;

    argv = malloc(argc * sizeof(char *));
    for (i = 0; i < argc; i++) {
        wlen = lstrlenW(wargv[i]);
        len = wcstombs(NULL, wargv[i], wlen);
        argv[i] = malloc(len + 1);
        wcstombs(argv[i], wargv[i], wlen);
    }

    //assuming ASCII parameters
    return main(argc, argv);
}
#endif

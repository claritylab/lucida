/* -*- c-basic-offset: 4; indent-tabs-mode: nil -*- */
/* ====================================================================
 * Copyright (c) 2013 Carnegie Mellon University.  All rights
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
 * cont_seg.c -- Continuously listen and segment input speech into utterances.
 * 
 * HISTORY
 *
 * 05-Nov-13    Created from adseg and fileseg
 * 
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <math.h>

#include <sphinxbase/prim_type.h>
#include <sphinxbase/ad.h>
#include <sphinxbase/fe.h>
#include <sphinxbase/cmd_ln.h>
#include <sphinxbase/ckd_alloc.h>
#include <sphinxbase/err.h>

#define BLOCKSIZE 1024

static const arg_t cont_args_def[] = {
    waveform_to_cepstral_command_line_macro(),
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
     "Name of audio file to use for input."},
    {NULL, 0, NULL, NULL}
};

static fe_t *fe;
static cmd_ln_t *config;
static int (*read_audio) (int16 * buf, int len);
static ad_rec_t *ad;
static const char *infile_path;
static FILE *infile;

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

static
char *
replace_str(char *str, char *orig, char *rep)
{
    static char buffer[4096];
    char *p;

    if (!(p = strstr(str, orig)))
        return str;

    strncpy(buffer, str, p - str);
    buffer[p - str] = '\0';

    sprintf(buffer + (p - str), "%s%s", rep, p + strlen(orig));

    return buffer;
}

static int
read_audio_file(int16 * buf, int len)
{
    if (!infile) {
        E_FATAL("Failed to read audio from file\n");
        return -1;
    }
    return fread(buf, sizeof(int16), len, infile);
}

static int
read_audio_adev(int16 * buf, int len)
{
    int k;

    if (!ad) {
        E_FATAL("Failed to read audio from mic\n");
        return -1;
    }
    while ((k = ad_read(ad, buf, len)) == 0)
        /* wait until something is read */
        sleep_msec(50);
}

void
segment_audio()
{
    FILE *file;
    int16 pcm_buf[BLOCKSIZE];
    mfcc_t **cep_buf;
    int16 *voiced_buf;
    int32 voiced_nsamps;
    char file_name[1024];
    uint8 cur_vad_state, vad_state, writing;
    int uttno, uttlen, sample_rate;
    int32 nframes, nframes_tmp;
    int16 frame_size, frame_shift;
    size_t k;

    sample_rate = (int) cmd_ln_float32_r(config, "-samprate");
    frame_size =
        (int32) (cmd_ln_float32_r(config, "-wlen") * sample_rate + 0.5);
    frame_shift =
        (int32) (sample_rate / cmd_ln_int32_r(config, "-frate") + 0.5);
    nframes = (BLOCKSIZE - frame_size) / frame_shift;
    cep_buf =
        (mfcc_t **) ckd_calloc_2d(nframes, fe_get_output_size(fe),
                                  sizeof(mfcc_t));

    uttno = 0;
    uttlen = 0;
    cur_vad_state = 0;
    voiced_nsamps = 0;
    writing = 0;
    file = NULL;
    voiced_buf = NULL;
    fe_start_utt(fe);
    while ((k = read_audio(pcm_buf, BLOCKSIZE)) > 0) {
        int16 const *pcm_buf_tmp;
        pcm_buf_tmp = &pcm_buf[0];
        while (k) {
            nframes_tmp = nframes;
            fe_process_frames_ext(fe, &pcm_buf_tmp, &k, cep_buf,
                                  &nframes_tmp, &voiced_buf,
                                  &voiced_nsamps);
            vad_state = fe_get_vad_state(fe);
            if (!cur_vad_state && vad_state) {
                /* silence->speech transition, time to start new file */
                uttno++;
                sprintf(file_name, "%s%04d.raw", infile_path, uttno);
                if ((file = fopen(file_name, "wb")) == NULL)
                    E_FATAL_SYSTEM("Failed to open '%s' for reading",
                                   file_name);
                printf("Utterance %04d, logging to %s\n", uttno,
                       file_name);
                writing = 1;
            }

            if (writing && file && voiced_nsamps > 0) {
                fwrite(voiced_buf, sizeof(int16), voiced_nsamps, file);
                uttlen += voiced_nsamps;
            }

            if (cur_vad_state && !vad_state) {
                /* speech -> silence transition, time to finish file */
                fclose(file);
                printf("\tUtterance %04d = %d samples (%.1fsec)\n\n",
                       uttno, uttlen,
                       (double) uttlen / (double) sample_rate);
                fflush(stdout);
                fe_end_utt(fe, cep_buf[0], &nframes_tmp);
                writing = 0;
                uttlen = 0;
                voiced_nsamps = 0;
                fe_start_utt(fe);
            }
            cur_vad_state = vad_state;
        }
    }

    if (writing) {
        fclose(file);
        printf("\tUtterance %04d = %d samples (%.1fsec)\n\n",
               uttno, uttlen, (double) uttlen / (double) sample_rate);
        fflush(stdout);
    }
    fe_end_utt(fe, cep_buf[0], &nframes);
    ckd_free_2d(cep_buf);
}

int
main(int argc, char *argv[])
{
    char const *cfg;
    int i;
    int16 buf[2048];

    if (argc == 2) {
        config = cmd_ln_parse_file_r(NULL, cont_args_def, argv[1], TRUE);
    }
    else {
        config = cmd_ln_parse_r(NULL, cont_args_def, argc, argv, FALSE);
    }
    /* Handle argument file as -argfile. */
    if (config && (cfg = cmd_ln_str_r(config, "-argfile")) != NULL) {
        config = cmd_ln_parse_file_r(config, cont_args_def, cfg, FALSE);
    }
    if (config == NULL)
        return 1;

    if ((infile_path = cmd_ln_str_r(config, "-infile")) != NULL) {
        if ((infile = fopen(infile_path, "rb")) == NULL) {
            E_FATAL("Unable to read audio from [%s]\n", infile_path);
            return 1;
        }
        read_audio = &read_audio_file;
        /* skip wav header */
        read_audio(buf, 44);
    }
    else {
        if ((ad = ad_open_dev(cmd_ln_str_r(config, "-adcdev"),
                              (int) cmd_ln_float32_r(config,
                                                     "-samprate"))) ==
            NULL) {
            E_FATAL("Failed to open audio device\n");
            return 1;
        }
        read_audio = &read_audio_adev;
        printf("Start recording ...\n");
        fflush(stdout);
        if (ad_start_rec(ad) < 0)
            E_FATAL("Failed to start recording\n");

        /* TODO remove this thing */
        for (i = 0; i < 5; i++) {
            sleep_msec(200);
            read_audio(buf, 2048);
        }
        printf("You may speak now\n");
        fflush(stdout);
    }

    fe = fe_init_auto_r(config);
    if (fe == NULL)
        return 1;

    segment_audio();

    if (ad)
        ad_close(ad);
    if (infile)
        fclose(infile);

    fe_free(fe);
    return 0;
}

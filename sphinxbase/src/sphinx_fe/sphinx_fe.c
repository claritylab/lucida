/* -*- c-basic-offset: 4; indent-tabs-mode: nil -*- */
/* ====================================================================
 * Copyright (c) 1996-2004 Carnegie Mellon University.  All rights 
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
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <assert.h>

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#ifdef HAVE_SNDFILE_H
#include <sndfile.h>
#endif

#include <sphinxbase/fe.h>
#include <sphinxbase/strfuncs.h>
#include <sphinxbase/pio.h>
#include <sphinxbase/filename.h>
#include <sphinxbase/cmd_ln.h>
#include <sphinxbase/err.h>
#include <sphinxbase/ckd_alloc.h>
#include <sphinxbase/byteorder.h>
#include <sphinxbase/hash_table.h>

#include "sphinx_wave2feat.h"
#include "cmd_ln_defn.h"

typedef struct audio_type_s {
    char const *name;
    int (*detect)(sphinx_wave2feat_t *wtf);
    int (*decode)(sphinx_wave2feat_t *wtf);
} audio_type_t;

typedef struct output_type_s {
    char const *name;
    int (*output_header)(sphinx_wave2feat_t *wtf, int nfloat);
    int (*output_frames)(sphinx_wave2feat_t *wtf, mfcc_t **frames, int nfr);
} output_type_t;

struct sphinx_wave2feat_s {
    int refcount;     /**< Reference count. */
    cmd_ln_t *config; /**< Configuration parameters. */
    fe_t *fe;         /**< Front end object. */
    char *infile;     /**< Path to input file. */
    char *outfile;    /**< Path to output file. */
    FILE *infh;       /**< Input file handle. */
    FILE *outfh;      /**< Output file handle. */
    short *audio;     /**< Audio buffer. */
    mfcc_t **feat;    /**< Feature buffer. */
    int blocksize;    /**< Size of audio buffer. */
    int featsize;     /**< Size of feature buffer. */
    int veclen;       /**< Length of each output vector. */
    int in_veclen;    /**< Length of each input vector (for cep<->spec). */
    int byteswap;     /**< Whether byteswapping is necessary. */
#ifdef HAVE_SNDFILE_H
    SNDFILE *insfh;   /**< Input sndfile handle. */
#endif
    output_type_t const *ot;/**< Output type object. */
};

/** RIFF 44-byte header structure for MS wav files. */
typedef struct RIFFHeader{
    char rifftag[4];      /* "RIFF" string */
    int32 TotalLength;      /* Total length */
    char wavefmttag[8];   /* "WAVEfmt " string (note space after 't') */
    int32 RemainingLength;  /* Remaining length */
    int16 data_format;    /* data format tag, 1 = PCM */
    int16 numchannels;    /* Number of channels in file */
    int32 SamplingFreq;     /* Sampling frequency */
    int32 BytesPerSec;      /* Average bytes/sec */
    int16 BlockAlign;     /* Block align */
    int16 BitsPerSample;  /* 8 or 16 bit */
    char datatag[4];      /* "data" string */
    int32 datalength;       /* Raw data length */
} MSWAV_hdr;

/**
 * Detect RIFF file and parse its header if detected.
 *
 * @return TRUE if it's a RIFF file, FALSE if not, -1 if an error occurred.
 */
static int
detect_riff(sphinx_wave2feat_t *wtf)
{
    FILE *fh;
    MSWAV_hdr hdr;

    if ((fh = fopen(wtf->infile, "rb")) == NULL) {
        E_ERROR_SYSTEM("Failed to open %s", wtf->infile);
        return -1;
    }
    if (fread(&hdr, sizeof(hdr), 1, fh) != 1) {
        E_ERROR_SYSTEM("Failed to read RIFF header");
        fclose(fh);
        return -1;
    }
    /* Make sure it is actually a RIFF file. */
    if (0 != memcmp(hdr.rifftag, "RIFF", 4)) {
        fclose(fh);
        return FALSE;
    }

    /* Get relevant information. */
    cmd_ln_set_int32_r(wtf->config, "-nchans", hdr.numchannels);
    cmd_ln_set_float32_r(wtf->config, "-samprate", hdr.SamplingFreq);
    wtf->infh = fh;

    return TRUE;
}

static int
open_nist_file(sphinx_wave2feat_t *wtf, char const *infile, FILE **out_fh, int detect_endian)
{
    char nist[7];
    lineiter_t *li;
    FILE *fh;

    if ((fh = fopen(infile, "rb")) == NULL) {
        E_ERROR_SYSTEM("Failed to open %s", infile);
        return -1;
    }
    if (fread(&nist, 1, 7, fh) != 7) {
        E_ERROR_SYSTEM("Failed to read NIST header");
        fclose(fh);
        return -1;
    }
    /* Is this actually a NIST file? */
    if (0 != strncmp(nist, "NIST_1A", 7)) {
        fclose(fh);
        return FALSE;
    }
    /* Rewind, parse lines. */
    fseek(fh, 0, SEEK_SET);
    for (li = lineiter_start(fh); li; li = lineiter_next(li)) {
        char **words;
        int nword;

        string_trim(li->buf, STRING_BOTH);
        if (strlen(li->buf) == 0) {
            lineiter_free(li);
            break;
        }
        nword = str2words(li->buf, NULL, 0);
        if (nword != 3)
            continue;
        words = ckd_calloc(nword, sizeof(*words));
        str2words(li->buf, words, nword);
        if (0 == strcmp(words[0], "sample_rate")) {
            cmd_ln_set_float32_r(wtf->config, "-samprate", atof_c(words[2]));
        }
        if (0 == strcmp(words[0], "channel_count")) {
            cmd_ln_set_int32_r(wtf->config, "-nchans", atoi(words[2]));
        }
        if (detect_endian && 0 == strcmp(words[0], "sample_byte_format")) {
            cmd_ln_set_str_r(wtf->config, "-input_endian",
                             (0 == strcmp(words[2], "10")) ? "big" : "little");
        }
        ckd_free(words);
    }

    fseek(fh, 1024, SEEK_SET);
    if (out_fh)
        *out_fh = fh;
    else
        fclose(fh);
    return TRUE;
}

#ifdef HAVE_POPEN
static int
detect_sph2pipe(sphinx_wave2feat_t *wtf)
{
    FILE *fh;
    char *cmdline;
    int rv;

    /* Determine if it's NIST file and get parameters. */
    if ((rv = open_nist_file(wtf, wtf->infile, NULL, FALSE)) != TRUE)
        return rv;

    /* Now popen it with sph2pipe. */
    cmdline = string_join("sph2pipe -f raw '", wtf->infile, "'", NULL);
    if ((fh = popen(cmdline, "r")) == NULL) {
        E_ERROR_SYSTEM("Failed to popen(\"sph2pipe -f raw '%s'\")", wtf->infile);
        ckd_free(cmdline);
        return -1;
    }

    wtf->infh = fh;
    return TRUE;
}
#else /* !HAVE_POPEN */
static int
detect_sph2pipe(sphinx_wave2feat_t *wtf)
{
    E_ERROR("popen() not available, cannot run sph2pipe\n");
    return -1;
}
#endif /* !HAVE_POPEN */

/**
 * Detect NIST file and parse its header if detected.
 *
 * @return TRUE if it's a NIST file, FALSE if not, -1 if an error occurred.
 */
static int
detect_nist(sphinx_wave2feat_t *wtf)
{
    FILE *fh;
    int rv;

    if ((rv = open_nist_file(wtf, wtf->infile, &fh, TRUE)) != TRUE)
        return rv;
    wtf->infh = fh;

    return TRUE;
}


/**
 * Default "detection" function, just opens the file and keeps the
 * default configuration parameters.
 *
 * @return TRUE, or -1 on error.
 */
static int
detect_raw(sphinx_wave2feat_t *wtf)
{
    FILE *fh;

    if ((fh = fopen(wtf->infile, "rb")) == NULL) {
        E_ERROR_SYSTEM("Failed to open %s", wtf->infile);
        return -1;
    }
    wtf->infh = fh;
    return TRUE;
}

/**
 * "Detect" Sphinx MFCC files, meaning verify their lousy headers, and
 * set up some parameters from the config object.
 *
 * @return TRUE, or -1 on error.
 */
static int
detect_sphinx_mfc(sphinx_wave2feat_t *wtf)
{
    FILE *fh;
    int32 len;
    long flen;

    if ((fh = fopen(wtf->infile, "rb")) == NULL) {
        E_ERROR_SYSTEM("Failed to open %s", wtf->infile);
        return -1;
    }
    if (fread(&len, 4, 1, fh) != 1) {
        E_ERROR_SYSTEM("Failed to read header from %s\n", wtf->infile);
        fclose(fh);
        return -1;
    }
    fseek(fh, 0, SEEK_END);
    flen = ftell(fh);

    /* figure out whether to byteswap */
    flen = (flen / 4) - 1;
    if (flen != len) {
        /* First make sure this is an endianness problem, otherwise fail. */
        SWAP_INT32(&len);
        if (flen != len) {
            SWAP_INT32(&len);
            E_ERROR("Mismatch in header/file lengths: 0x%08x vs 0x%08x\n",
                    len, flen);
            return -1;
        }
        /* Set the input endianness to the opposite of the machine endianness... */
        cmd_ln_set_str_r(wtf->config, "-input_endian",
                         (0 == strcmp("big", cmd_ln_str_r(wtf->config, "-mach_endian"))
                          ? "little" : "big"));
    }
    
    fseek(fh, 4, SEEK_SET);
    wtf->infh = fh;
    if (cmd_ln_boolean_r(wtf->config, "-spec2cep")) {
        wtf->in_veclen = cmd_ln_int32_r(wtf->config, "-nfilt");
    }
    else if (cmd_ln_boolean_r(wtf->config, "-cep2spec")) {
        wtf->in_veclen = cmd_ln_int32_r(wtf->config, "-ncep");
        wtf->veclen = cmd_ln_int32_r(wtf->config, "-nfilt");
    }
    else {
        /* Should not happen. */
        E_ERROR("Sphinx MFCC file reading requested but -spec2cep/-cep2spec not given\n");
        assert(FALSE);
    }
            
    return TRUE;
}

int
mixnpick_channels(int16 *buf, int32 nsamp, int32 nchans, int32 whichchan)
{
    int i, j;

    if (whichchan > 0) {
        for (i = whichchan - 1; i < nsamp; i += nchans)
            buf[i/nchans] = buf[i];
    }
    else {
        for (i = 0; i < nsamp; i += nchans) {
            float64 tmp = 0.0;
            for (j = 0; j < nchans && i + j < nsamp; ++j) {
                tmp += buf[i + j];
            }
            buf[i/nchans] = (int16)(tmp / nchans);
        }
    }
    return i/nchans;
}

#ifdef HAVE_SNDFILE_H
/**
 * Detect a file supported by libsndfile and parse its header if detected.
 *
 * @return TRUE if it's a supported file, FALSE if not, -1 if an error occurred.
 */
static int
detect_sndfile(sphinx_wave2feat_t *wtf)
{
    SNDFILE *sf;
    SF_INFO sfinfo;

    memset(&sfinfo, 0, sizeof(sfinfo));
    /* We let other detectors catch I/O errors, since there is
       no way to tell them from format errors when opening :( */
    if ((sf = sf_open(wtf->infile, SFM_READ, &sfinfo)) == NULL) {
        return FALSE;
    }
    /* Get relevant information. */
    cmd_ln_set_int32_r(wtf->config, "-nchans", sfinfo.channels);
    cmd_ln_set_float32_r(wtf->config, "-samprate", sfinfo.samplerate);
    wtf->insfh = sf;
    wtf->infh = NULL;

    return TRUE;
}

/**
 * Process PCM audio from a libsndfile file.  FIXME: looks a lot like
 * decode_pcm!  Also needs stereo support (as does decode_pcm).
 */
static int
decode_sndfile(sphinx_wave2feat_t *wtf)
{
    size_t nsamp;
    int32 nfr, nchans, whichchan;
    int nfloat, n;

    nchans = cmd_ln_int32_r(wtf->config, "-nchans");
    whichchan = cmd_ln_int32_r(wtf->config, "-whichchan");
    fe_start_utt(wtf->fe);
    nfloat = 0;
    while ((nsamp = sf_read_short(wtf->insfh,
                                  wtf->audio,
                                  wtf->blocksize)) != 0) {
        int16 const *inspeech;
        size_t nvec;

        /* Mix or pick channels. */
        if (nchans > 1)
            nsamp = mixnpick_channels(wtf->audio, nsamp, nchans, whichchan);

        inspeech = wtf->audio;
        nvec = wtf->featsize;
        /* Consume all samples. */
        while (nsamp) {
            nfr = nvec;
            fe_process_frames(wtf->fe, &inspeech, &nsamp, wtf->feat, &nfr);
            if (nfr) {
                if ((n = (*wtf->ot->output_frames)(wtf, wtf->feat, nfr)) < 0)
                    return -1;
                nfloat += n;
            }
        }
        inspeech = wtf->audio;
    }
    /* Now process any leftover audio frames. */
    fe_end_utt(wtf->fe, wtf->feat[0], &nfr);
    if (nfr) {
        if ((n = (*wtf->ot->output_frames)(wtf, wtf->feat, nfr)) < 0)
            return -1;
        nfloat += n;
    }

    sf_close(wtf->insfh);
    wtf->insfh = NULL;
    return nfloat;
}
#endif /* HAVE_SNDFILE_H */

/**
 * Process PCM audio from a filehandle.  Assume that wtf->infh is
 * positioned just after the file header.
 */
static int
decode_pcm(sphinx_wave2feat_t *wtf)
{
    size_t nsamp;
    int32 nfr, nchans, whichchan;
    int nfloat, n;

    nchans = cmd_ln_int32_r(wtf->config, "-nchans");
    whichchan = cmd_ln_int32_r(wtf->config, "-whichchan");
    fe_start_utt(wtf->fe);
    nfloat = 0;
    while ((nsamp = fread(wtf->audio, 2, wtf->blocksize, wtf->infh)) != 0) {
        size_t nvec;
        int16 const *inspeech;

        /* Byteswap stuff here if necessary. */
        if (wtf->byteswap) {
            for (n = 0; n < nsamp; ++n)
                SWAP_INT16(wtf->audio + n);
        }

        /* Mix or pick channels. */
        if (nchans > 1)
            nsamp = mixnpick_channels(wtf->audio, nsamp, nchans, whichchan);
            
        inspeech = wtf->audio;
        nvec = wtf->featsize;
        /* Consume all samples. */
        while (nsamp) {
            nfr = nvec;
            fe_process_frames(wtf->fe, &inspeech, &nsamp, wtf->feat, &nfr);
            if (nfr) {
                if ((n = (*wtf->ot->output_frames)(wtf, wtf->feat, nfr)) < 0)
                    return -1;
                nfloat += n;
            }
        }
        inspeech = wtf->audio;
    }
    /* Now process any leftover audio frames. */
    fe_end_utt(wtf->fe, wtf->feat[0], &nfr);
    if (nfr) {
        if ((n = (*wtf->ot->output_frames)(wtf, wtf->feat, nfr)) < 0)
            return -1;
        nfloat += n;
    }

    if (fclose(wtf->infh) == EOF)
        E_ERROR_SYSTEM("Failed to close input file");
    wtf->infh = NULL;
    return nfloat;
}

/**
 * Process Sphinx MFCCs/logspectra from a filehandle.  Assume that
 * wtf->infh is positioned just after the file header.
 */
static int
decode_sphinx_mfc(sphinx_wave2feat_t *wtf)
{
    int nfloat = 0, n;
    int featsize = wtf->featsize;

    /* If the input vector length is less than the output length, we
     * need to do this one frame at a time, because there's empty
     * space at the end of each vector in wtf->feat. */
    if (wtf->in_veclen < wtf->veclen)
        featsize = 1;
    while ((n = fread(wtf->feat[0], sizeof(**wtf->feat),
                      featsize * wtf->in_veclen, wtf->infh)) != 0) {
        int i, nfr = n / wtf->in_veclen;
        if (n % wtf->in_veclen) {
            E_ERROR("Size of file %d not a multiple of veclen %d\n",
                    n, wtf->in_veclen);
            return -1;
        }
        /* Byteswap stuff here if necessary. */
        if (wtf->byteswap) {
            for (i = 0; i < n; ++i)
                SWAP_FLOAT32(wtf->feat[0] + i);
        }
        fe_float_to_mfcc(wtf->fe, (float32 **)wtf->feat, wtf->feat, nfr);
        for (i = 0; i < nfr; ++i) {
            if (cmd_ln_boolean_r(wtf->config, "-spec2cep")) {
                if (0 == strcmp(cmd_ln_str_r(wtf->config, "-transform"), "legacy"))
                    fe_logspec_to_mfcc(wtf->fe, wtf->feat[i], wtf->feat[i]);
                else
                    fe_logspec_dct2(wtf->fe, wtf->feat[i], wtf->feat[i]);
            }
            else if (cmd_ln_boolean_r(wtf->config, "-cep2spec")) {
                fe_mfcc_dct3(wtf->fe, wtf->feat[i], wtf->feat[i]);
            }
        }
        if ((n = (*wtf->ot->output_frames)(wtf, wtf->feat, nfr)) < 0)
            return -1;
        nfloat += n;
    }

    if (fclose(wtf->infh) == EOF)
        E_ERROR_SYSTEM("Failed to close input file");
    wtf->infh = NULL;
    return nfloat;
}

static const audio_type_t types[] = {
#ifdef HAVE_SNDFILE_H
    { "-sndfile", &detect_sndfile, &decode_sndfile },
#endif
    { "-mswav", &detect_riff, &decode_pcm },
    { "-nist", &detect_nist, &decode_pcm },
    { "-raw", &detect_raw, &decode_pcm },
    { "-sph2pipe", &detect_sph2pipe, &decode_pcm }
};
static const int ntypes = sizeof(types)/sizeof(types[0]);
static const audio_type_t mfcc_type = {
    "sphinx_mfc", &detect_sphinx_mfc, &decode_sphinx_mfc
};

/**
 * Output sphinx format "header"
 *
 * @return 0 for success, <0 for error.
 */
static int
output_header_sphinx(sphinx_wave2feat_t *wtf, int32 nfloat)
{
    if (fwrite(&nfloat, 4, 1, wtf->outfh) != 1) {
        E_ERROR_SYSTEM("Failed to write to %s", wtf->outfile);
        return -1;
    }
    return 0;
}

/**
 * Output frames in sphinx format.
 *
 * @return 0 for success, <0 for error.
 */
static int
output_frames_sphinx(sphinx_wave2feat_t *wtf, mfcc_t **frames, int nfr)
{
    int i, nfloat = 0;

    fe_mfcc_to_float(wtf->fe, frames, (float32 **)frames, nfr);
    for (i = 0; i < nfr; ++i) {
        if (fwrite(frames[i], sizeof(float32), wtf->veclen, wtf->outfh) != wtf->veclen) {
            E_ERROR_SYSTEM("Writing %d values to %s failed",
                           wtf->veclen, wtf->outfile);
            return -1;
        }
        nfloat += wtf->veclen;
    }
    return nfloat;
}

typedef enum htk_feature_kind_e {
    WAVEFORM = 0,   /* PCM audio (rarely used) */
    LPC = 1,        /* LPC filter coefficients */
    LPCREFC = 2,    /* LPC reflection coefficients */
    LPCEPSTRA = 3,  /* LPC-based cepstral coefficients */
    LPCDELCEP = 4,  /* LPCC plus deltas */
    IREFC = 5,      /* 16-bit integer LPC reflection coefficients */
    MFCC = 6,       /* MFCCs */
    FBANK = 7,      /* Log mel spectrum */
    MELSPEC = 8,    /* Linear mel spectrum */
    USER = 9,       /* User defined */
    DISCRETE = 10,  /* Vector quantized data */
    PLP = 11        /* PLP coefficients */
} htk_feature_kind_t;

typedef enum htk_feature_flag_e {
    _E = 0000100, /* has energy */
    _N = 0000200, /* absolute energy supressed */
    _D = 0000400, /* has delta coefficients */
    _A = 0001000, /* has acceleration (delta-delta) coefficients */
    _C = 0002000, /* is compressed */
    _Z = 0004000, /* has zero mean static coefficients (i.e. CMN) */
    _K = 0010000, /* has CRC checksum */
    _O = 0020000, /* has 0th cepstral coefficient */
    _V = 0040000, /* has VQ data */
    _T = 0100000  /* has third differential coefficients */
} htk_feature_flag_t;

/**
 * Output HTK format header.
 */
static int
output_header_htk(sphinx_wave2feat_t *wtf, int32 nfloat)
{
    int32 samp_period;
    int16 samp_size;
    int16 param_kind;
    int swap = FALSE;

    /* HTK files are big-endian. */
    if (0 == strcmp("little", cmd_ln_str_r(wtf->config, "-mach_endian")))
        swap = TRUE;
    /* Same file size thing as in Sphinx files (I think) */
    if (swap) SWAP_INT32(&nfloat);
    if (fwrite(&nfloat, 4, 1, wtf->outfh) != 1)
        return -1;
    /* Sample period in 100ns units. */
    samp_period = (int32)(1e+7 / cmd_ln_float32_r(wtf->config, "-frate"));
    if (swap) SWAP_INT32(&samp_period);
    if (fwrite(&samp_period, 4, 1, wtf->outfh) != 1)
        return -1;
    /* Sample size - veclen * sizeof each sample. */
    samp_size = wtf->veclen * 4;
    if (swap) SWAP_INT16(&samp_size);
    if (fwrite(&samp_size, 2, 1, wtf->outfh) != 1)
        return -1;
    /* Format and flags. */
    if (cmd_ln_boolean_r(wtf->config, "-logspec")
        || cmd_ln_boolean_r(wtf->config, "-cep2spec"))
        param_kind = FBANK; /* log mel-filter bank outputs */
    else
        param_kind = MFCC | _O; /* MFCC + CEP0 (note reordering...) */
    if (swap) SWAP_INT16(&param_kind);
    if (fwrite(&param_kind, 2, 1, wtf->outfh) != 1)
        return -1;

    return 0;
}

/**
 * Output frames in HTK format.
 */
static int
output_frames_htk(sphinx_wave2feat_t *wtf, mfcc_t **frames, int nfr)
{
    int i, j, swap, htk_reorder, nfloat = 0;

    fe_mfcc_to_float(wtf->fe, frames, (float32 **)frames, nfr);
    /* This is possibly inefficient, but probably not a big deal. */
    swap = (0 == strcmp("little", cmd_ln_str_r(wtf->config, "-mach_endian")));
    htk_reorder = (0 == strcmp("htk", wtf->ot->name)
                   && !(cmd_ln_boolean_r(wtf->config, "-logspec")
                        || cmd_ln_boolean_r(wtf->config, "-cep2spec")));
    for (i = 0; i < nfr; ++i) {
        if (htk_reorder) {
            mfcc_t c0 = frames[i][0];
            memmove(frames[i] + 1, frames[i], (wtf->veclen - 1) * 4);
            frames[i][wtf->veclen - 1] = c0;
        }
        if (swap)
            for (j = 0; j < wtf->veclen; ++j)
                SWAP_FLOAT32(frames[i] + j);
        if (fwrite(frames[i], sizeof(float32), wtf->veclen, wtf->outfh) != wtf->veclen) {
            E_ERROR_SYSTEM("Writing %d values to %s failed",
                           wtf->veclen, wtf->outfile);
            return -1;
        }
        nfloat += wtf->veclen;
    }
    return nfloat;
}

/**
 * Output frames in text format.
 */
static int
output_frames_text(sphinx_wave2feat_t *wtf, mfcc_t **frames, int nfr)
{
    int i, j, nfloat = 0;

    fe_mfcc_to_float(wtf->fe, frames, (float32 **)frames, nfr);
    for (i = 0; i < nfr; ++i) {
        for (j = 0; j < wtf->veclen; ++j) {
            fprintf(wtf->outfh, "%.5g", frames[i][j]);
            if (j == wtf->veclen - 1)
                fprintf(wtf->outfh, "\n");
            else
                fprintf(wtf->outfh, " ");
        }
        nfloat += wtf->veclen;
    }
    return nfloat;
}

static const output_type_t outtypes[] = {
    { "sphinx", &output_header_sphinx, &output_frames_sphinx },
    { "htk", &output_header_htk, &output_frames_htk },
    { "text", NULL, &output_frames_text }
};
static const int nouttypes = sizeof(outtypes)/sizeof(outtypes[0]);

sphinx_wave2feat_t *
sphinx_wave2feat_init(cmd_ln_t *config)
{
    sphinx_wave2feat_t *wtf;
    int i;

    wtf = ckd_calloc(1, sizeof(*wtf));
    wtf->refcount = 1;
    wtf->config = cmd_ln_retain(config);
    wtf->fe = fe_init_auto_r(wtf->config);
    wtf->ot = outtypes; /* Default (sphinx) type. */
    for (i = 0; i < nouttypes; ++i) {
        output_type_t const *otype = &outtypes[i];
        if (0 == strcmp(cmd_ln_str_r(config, "-ofmt"), otype->name)) {
            wtf->ot = otype;
            break;
        }
    }
    if (i == nouttypes) {
        E_ERROR("Unknown output type: '%s'\n",
                cmd_ln_str_r(config, "-ofmt"));
        sphinx_wave2feat_free(wtf);
        return NULL;
    }

    return wtf;
}

int
sphinx_wave2feat_free(sphinx_wave2feat_t *wtf)
{
    if (wtf == NULL)
        return 0;
    if (--wtf->refcount > 0)
        return wtf->refcount;

    if (wtf->audio)
	ckd_free(wtf->audio);
    if (wtf->feat)
	ckd_free_2d(wtf->feat);
    if (wtf->infile)
        ckd_free(wtf->infile);
    if (wtf->outfile)
	ckd_free(wtf->outfile);
    if (wtf->infh) {
        if (fclose(wtf->infh) == EOF)
            E_ERROR_SYSTEM("Failed to close input file");
    }
    if (wtf->outfh) {
        if (fclose(wtf->outfh) == EOF)
            E_ERROR_SYSTEM("Failed to close output file");
    }
    cmd_ln_free_r(wtf->config);
    fe_free(wtf->fe);
    ckd_free(wtf);

    return 0;
}

sphinx_wave2feat_t *
sphinx_wave2feat_retain(sphinx_wave2feat_t *wtf)
{
    ++wtf->refcount;
    return wtf;
}

static audio_type_t const *
detect_audio_type(sphinx_wave2feat_t *wtf)
{
    audio_type_t const *atype;
    int i;

    /* Special case audio type for Sphinx MFCC inputs. */
    if (cmd_ln_boolean_r(wtf->config, "-spec2cep")
        || cmd_ln_boolean_r(wtf->config, "-cep2spec")) {
        int rv = mfcc_type.detect(wtf);
        if (rv == -1)
            goto error_out;
        return &mfcc_type;
    }

    /* Try to use the type of infile given on the command line. */
    for (i = 0; i < ntypes; ++i) {
        int rv;
        atype = &types[i];
        if (cmd_ln_boolean_r(wtf->config, atype->name)) {
            rv = (*atype->detect)(wtf);
            if (rv == -1)
                goto error_out;
            else if (rv == TRUE)
                break;
        }
    }
    if (i == ntypes) {
        /* Detect file type of infile and get parameters. */
        for (i = 0; i < ntypes; ++i) {
            int rv;
            atype = &types[i];
            rv = (*atype->detect)(wtf);
            if (rv == -1)
                goto error_out;
            else if (rv == TRUE)
                break;
        }
        if (i == ntypes)
            goto error_out;
    }
    return atype;
 error_out:
    if (wtf->infh)
        fclose(wtf->infh);
    wtf->infh = NULL;
    return NULL;
}

int
sphinx_wave2feat_convert_file(sphinx_wave2feat_t *wtf,
                              char const *infile, char const *outfile)
{
    int nchans, minfft, nfft, nfloat, veclen;
    audio_type_t const *atype;
    int fshift, fsize;

    if (cmd_ln_boolean_r(wtf->config, "-verbose"))
        E_INFO("Converting %s to %s\n", infile, outfile);

    wtf->infile = ckd_salloc(infile);

    /* Detect input file type. */
    if ((atype = detect_audio_type(wtf)) == NULL)
        return -1;

    /* Determine whether to byteswap input. */
    wtf->byteswap = strcmp(cmd_ln_str_r(wtf->config, "-mach_endian"),
                           cmd_ln_str_r(wtf->config, "-input_endian"));

    /* Make sure the FFT size is sufficiently large. */
    minfft = (int)(cmd_ln_float32_r(wtf->config, "-samprate")
                   * cmd_ln_float32_r(wtf->config, "-wlen") + 0.5);
    for (nfft = 1; nfft < minfft; nfft <<= 1)
        ;
    if (nfft > cmd_ln_int32_r(wtf->config, "-nfft")) {
        E_WARN("Value of -nfft = %d is too small, increasing to %d\n",
               cmd_ln_int32_r(wtf->config, "-nfft"), nfft);
        cmd_ln_set_int32_r(wtf->config, "-nfft", nfft);
        fe_free(wtf->fe);
        wtf->fe = fe_init_auto_r(wtf->config);
    }

    /* Get the output frame size (if not already set). */
    if (wtf->veclen == 0)
        wtf->veclen = fe_get_output_size(wtf->fe);

    /* Set up the input and output buffers. */
    fe_get_input_size(wtf->fe, &fshift, &fsize);
    /* Want to get at least a whole frame plus shift in here.  Also we
       will either pick or mix multiple channels so we need to read
       them all at once. */
    nchans = cmd_ln_int32_r(wtf->config, "-nchans");
    wtf->blocksize = cmd_ln_int32_r(wtf->config, "-blocksize") * nchans;
    if (wtf->blocksize < (fsize + fshift) * nchans) {
        E_INFO("Block size of %d too small, increasing to %d\n",
               wtf->blocksize,
               (fsize + fshift) * nchans);
        wtf->blocksize = (fsize + fshift) * nchans;
    }
    wtf->audio = ckd_calloc(wtf->blocksize, sizeof(*wtf->audio));
    wtf->featsize = (wtf->blocksize / nchans - fsize) / fshift;

    /* Use the maximum of the input and output frame sizes to allocate this. */
    veclen = wtf->veclen;
    if (wtf->in_veclen > veclen) veclen = wtf->in_veclen;
    
    wtf->feat = ckd_calloc_2d(wtf->featsize, veclen, sizeof(**wtf->feat));

    /* Let's go! */
    if ((wtf->outfh = fopen(outfile, "wb")) == NULL) {
        E_ERROR_SYSTEM("Failed to open %s for writing", outfile);
        return -1;
    }
    /* Write an empty header, which we'll fill in later. */
    if (wtf->ot->output_header &&
        (*wtf->ot->output_header)(wtf, 0) < 0) {
        E_ERROR_SYSTEM("Failed to write empty header to %s\n", outfile);
        goto error_out;
    }
    wtf->outfile = ckd_salloc(outfile);

    if ((nfloat = (*atype->decode)(wtf)) < 0) {
    	E_ERROR("Failed to convert");
    	goto error_out;
    }

    if (wtf->ot->output_header) {
        if (fseek(wtf->outfh, 0, SEEK_SET) < 0) {
            E_ERROR_SYSTEM("Failed to seek to beginning of %s\n", outfile);
            goto error_out;
        }
        if ((*wtf->ot->output_header)(wtf, nfloat) < 0) {
            E_ERROR_SYSTEM("Failed to write header to %s\n", outfile);
            goto error_out;
        }
    }
    

    if (wtf->audio)
	ckd_free(wtf->audio);
    if (wtf->feat)
	ckd_free_2d(wtf->feat);
    if (wtf->infile)
        ckd_free(wtf->infile);
    if (wtf->outfile)
	ckd_free(wtf->outfile);

    wtf->audio = NULL;
    wtf->infile = NULL;
    wtf->feat = NULL;
    wtf->outfile = NULL;

    if (wtf->outfh)
	if (fclose(wtf->outfh) == EOF)
    	    E_ERROR_SYSTEM("Failed to close output file");
    wtf->outfh = NULL;

    return 0;

error_out:

    if (wtf->audio)
	ckd_free(wtf->audio);
    if (wtf->feat)
	ckd_free_2d(wtf->feat);
    if (wtf->infile)
        ckd_free(wtf->infile);
    if (wtf->outfile)
	ckd_free(wtf->outfile);

    wtf->audio = NULL;
    wtf->infile = NULL;
    wtf->feat = NULL;
    wtf->outfile = NULL;

    if (wtf->outfh)
	if (fclose(wtf->outfh) == EOF)
    	    E_ERROR_SYSTEM("Failed to close output file");
    wtf->outfh = NULL;

    return -1;
}

void
build_filenames(cmd_ln_t *config, char const *basename,
                char **out_infile, char **out_outfile)
{
    char const *di, *do_, *ei, *eo;

    di = cmd_ln_str_r(config, "-di");
    do_ = cmd_ln_str_r(config, "-do");
    ei = cmd_ln_str_r(config, "-ei");
    eo = cmd_ln_str_r(config, "-eo");

    *out_infile = string_join(di ? di : "",
                              di ? "/" : "",
                              basename,
                              ei ? "." : "",
                              ei ? ei : "",
                              NULL);
    *out_outfile = string_join(do_ ? do_ : "",
                               do_ ? "/" : "",
                               basename,
                               eo ? "." : "",
                               eo ? eo : "",
                              NULL);
    /* Build output directory structure if possible/requested (it is
     * by default). */
    if (cmd_ln_boolean_r(config, "-build_outdirs")) {
        char *dirname = ckd_salloc(*out_outfile);
        path2dirname(*out_outfile, dirname);
        build_directory(dirname);
        ckd_free(dirname);
    }
}

static int
run_control_file(sphinx_wave2feat_t *wtf, char const *ctlfile)
{
    hash_table_t *files;
    hash_iter_t *itor;
    lineiter_t *li;
    FILE *ctlfh;
    int nskip, runlen, npart, rv = 0;

    if ((ctlfh = fopen(ctlfile, "r")) == NULL) {
        E_ERROR_SYSTEM("Failed to open control file %s", ctlfile);
        return -1;
    }
    nskip = cmd_ln_int32_r(wtf->config, "-nskip");
    runlen = cmd_ln_int32_r(wtf->config, "-runlen");
    if ((npart = cmd_ln_int32_r(wtf->config, "-npart"))) {
        /* Count lines in the file. */
        int partlen, part, nlines = 0;
        part = cmd_ln_int32_r(wtf->config, "-part");
        for (li = lineiter_start(ctlfh); li; li = lineiter_next(li))
            ++nlines;
        fseek(ctlfh, 0, SEEK_SET);
        partlen = nlines / npart;
        nskip = partlen * (part - 1);
        if (part == npart)
            runlen = -1;
        else
            runlen = partlen;
    }
    if (runlen != -1){
        E_INFO("Processing %d utterances at position %d\n", runlen, nskip);
        files = hash_table_new(runlen, HASH_CASE_YES);
    }
    else {
        E_INFO("Processing all remaining utterances at position %d\n", nskip);
        files = hash_table_new(1000, HASH_CASE_YES);
    }
    for (li = lineiter_start(ctlfh); li; li = lineiter_next(li)) {
        char *c, *infile, *outfile;

        if (nskip-- > 0)
            continue;
        if (runlen == 0) {
            lineiter_free(li);
            break;
        }
        --runlen;

        string_trim(li->buf, STRING_BOTH);
        /* Extract the file ID from the control line. */
        if ((c = strchr(li->buf, ' ')) != NULL)
            *c = '\0';
        if (strlen(li->buf) == 0) {
    	    E_WARN("Empty line %d in control file, skipping\n", li->lineno);
    	    continue;
        }
        build_filenames(wtf->config, li->buf, &infile, &outfile);
        if (hash_table_lookup(files, infile, NULL) == 0)
            continue;
        rv = sphinx_wave2feat_convert_file(wtf, infile, outfile);
        hash_table_enter(files, infile, outfile);
        if (rv != 0) {
            lineiter_free(li);
            break;
        }
    }
    for (itor = hash_table_iter(files); itor;
         itor = hash_table_iter_next(itor)) {
        ckd_free((void *)hash_entry_key(itor->ent));
        ckd_free(hash_entry_val(itor->ent));
    }
    hash_table_free(files);

    if (fclose(ctlfh) == EOF)
        E_ERROR_SYSTEM("Failed to close control file");
    return rv;
}

int
main(int argc, char *argv[])
{
    sphinx_wave2feat_t *wtf;
    cmd_ln_t *config;
    int rv;

    config = cmd_ln_parse_r(NULL, defn, argc, argv, TRUE);

    if (config && cmd_ln_str_r(config, "-argfile"))
        config = cmd_ln_parse_file_r(config, defn,
                                     cmd_ln_str_r(config, "-argfile"), FALSE);
    if (config == NULL) {
        E_ERROR("Command line parsing failed\n");
        return 1;
    }

    if ((wtf = sphinx_wave2feat_init(config)) == NULL) {
        E_ERROR("Failed to initialize wave2feat object\n");
        return 1;
    }

    /* If there's a control file run through it, otherwise we will do
     * a single file (which is what run_control_file will do
     * internally too) */
    if (cmd_ln_str_r(config, "-c"))
        rv = run_control_file(wtf, cmd_ln_str_r(config, "-c"));
    else
        rv = sphinx_wave2feat_convert_file(wtf, cmd_ln_str_r(config, "-i"),
                                           cmd_ln_str_r(config, "-o"));

    sphinx_wave2feat_free(wtf);
    cmd_ln_free_r(config);
    return rv;
}

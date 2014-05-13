/* -*- c-basic-offset: 4; indent-tabs-mode: nil -*- */
/* ====================================================================
 * Copyright (c) 2008 Carnegie Mellon University.  All rights 
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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdio.h>
#include <string.h>

#include <sphinxbase/cmd_ln.h>
#include <sphinxbase/yin.h>
#include <sphinxbase/ckd_alloc.h>
#include <sphinxbase/byteorder.h>
#include <sphinxbase/strfuncs.h>
#include <sphinxbase/err.h>
#include <sphinxbase/pio.h>

static arg_t defn[] = {
  { "-i",
    ARG_STRING,
    NULL,
    "Single audio input file" },

  { "-o",
    ARG_STRING,
    NULL,
    "Single text output file (standard output will be used if not given)" },
  
  { "-c",
    ARG_STRING,
    NULL,
    "Control file for batch processing" },
  
  { "-nskip",
    ARG_INT32,
    "0",
    "If a control file was specified, the number of utterances to skip at the head of the file" },
  
  { "-runlen",
    ARG_INT32,
    "-1",
    "If a control file was specified, the number of utterances to process (see -nskip too)" },
  
  { "-di",
    ARG_STRING,
    NULL,
    "Input directory, input file names are relative to this, if defined" },
  
  { "-ei",
    ARG_STRING,
    NULL,
    "Input extension to be applied to all input files" },
  
  { "-do",
    ARG_STRING,
    NULL,
    "Output directory, output files are relative to this" },
  
  { "-eo",
    ARG_STRING,
    NULL,
    "Output extension to be applied to all output files" },
  
  { "-nist",
    ARG_BOOLEAN,
    "no",
    "Defines input format as NIST sphere" },
  
  { "-raw",
    ARG_BOOLEAN,
    "no",
    "Defines input format as raw binary data" },
  
  { "-mswav",
    ARG_BOOLEAN,
    "no",
    "Defines input format as Microsoft Wav (RIFF)" },

  { "-samprate",
    ARG_INT32,
    "0",
    "Sampling rate of audio data (will be determined automatically if 0)" },

  { "-input_endian",
    ARG_STRING,
    NULL,
    "Endianness of audio data (will be determined automatically if not given)" },

  { "-fshift",
    ARG_FLOAT32,
    "0.01",
    "Frame shift: number of seconds between each analysis frame." },

  { "-flen",
    ARG_FLOAT32,
    "0.025",
    "Number of seconds in each analysis frame (needs to be greater than twice the longest period you wish to detect - to detect down to 80Hz you need a frame length of 2.0/80 = 0.025)." },

  { "-smooth_window",
    ARG_INT32,
    "2",
    "Number of frames on either side of the current frame to use for smoothing." },

  { "-voice_thresh",
    ARG_FLOAT32,
    "0.1",
    "Threshold of normalized difference under which to search for the fundamental period." },

  { "-search_range",
    ARG_FLOAT32,
    "0.2",
    "Fraction of the best local estimate to use as a search range for smoothing." },

  { NULL, 0, NULL, NULL }
};

static int extract_pitch(const char *in, const char *out);
static int run_control_file(const char *ctl);

int
main(int argc, char *argv[])
{
    cmd_ln_parse(defn, argc, argv, TRUE);

    /* Run a control file if requested. */
    if (cmd_ln_str("-c")) {
        if (run_control_file(cmd_ln_str("-c")) < 0)
            return 1;
    }
    else {
        if (extract_pitch(cmd_ln_str("-i"), cmd_ln_str("-o")) < 0)
            return 1;
    }

    cmd_ln_free();
    return 0;
}

static int
guess_file_type(char const *file, FILE *infh)
{
    char header[4];

    fseek(infh, 0, SEEK_SET);
    if (fread(header, 1, 4, infh) != 4) {
        E_ERROR_SYSTEM("Failed to read 4 byte header");
        return -1;
    }
    if (0 == memcmp(header, "RIFF", 4)) {
        E_INFO("%s appears to be a WAV file\n", file);
        cmd_ln_set_boolean("-mswav", TRUE);
        cmd_ln_set_boolean("-nist", FALSE);
        cmd_ln_set_boolean("-raw", FALSE);
    }
    else if (0 == memcmp(header, "NIST", 4)) {
        E_INFO("%s appears to be a NIST SPHERE file\n", file);
        cmd_ln_set_boolean("-mswav", FALSE);
        cmd_ln_set_boolean("-nist", TRUE);
        cmd_ln_set_boolean("-raw", FALSE);
    }
    else {
        E_INFO("%s appears to be raw data\n", file);
        cmd_ln_set_boolean("-mswav", FALSE);
        cmd_ln_set_boolean("-nist", FALSE);
        cmd_ln_set_boolean("-raw", TRUE);
    }
    fseek(infh, 0, SEEK_SET);
    return 0;
}

#define TRY_FREAD(ptr, size, nmemb, stream)                             \
    if (fread(ptr, size, nmemb, stream) != (nmemb)) {                   \
        E_ERROR_SYSTEM("Failed to read %d bytes", size * nmemb);       \
        goto error_out;                                                 \
    }

static int
read_riff_header(FILE *infh)
{
    char id[4];
    int32 intval, header_len;
    int16 shortval;

    /* RIFF files are little-endian by definition. */
    cmd_ln_set_str("-input_endian", "little");

    /* Read in all the header chunks and etcetera. */
    TRY_FREAD(id, 1, 4, infh);
    /* Total file length (we don't care) */
    TRY_FREAD(&intval, 4, 1, infh);
    /* 'WAVE' */
    TRY_FREAD(id, 1, 4, infh);
    if (0 != memcmp(id, "WAVE", 4)) {
        E_ERROR("This is not a WAVE file\n");
        goto error_out;
    }
    /* 'fmt ' */
    TRY_FREAD(id, 1, 4, infh);
    if (0 != memcmp(id, "fmt ", 4)) {
        E_ERROR("Format chunk missing\n");
        goto error_out;
    }
    /* Length of 'fmt ' chunk */
    TRY_FREAD(&intval, 4, 1, infh);
    SWAP_LE_32(&intval);
    header_len = intval;

    /* Data format. */
    TRY_FREAD(&shortval, 2, 1, infh);
    SWAP_LE_16(&shortval);
    if (shortval != 1) { /* PCM */
        E_ERROR("WAVE file is not in PCM format\n");
        goto error_out;
    }

    /* Number of channels. */
    TRY_FREAD(&shortval, 2, 1, infh);
    SWAP_LE_16(&shortval);
    if (shortval != 1) { /* PCM */
        E_ERROR("WAVE file is not single channel\n");
        goto error_out;
    }

    /* Sampling rate (finally!) */
    TRY_FREAD(&intval, 4, 1, infh);
    SWAP_LE_32(&intval);
    if (cmd_ln_int32("-samprate") == 0)
        cmd_ln_set_int32("-samprate", intval);
    else if (cmd_ln_int32("-samprate") != intval) {
        E_WARN("WAVE file sampling rate %d != -samprate %d\n",
               intval, cmd_ln_int32("-samprate"));
    }

    /* Average bytes per second (we don't care) */
    TRY_FREAD(&intval, 4, 1, infh);

    /* Block alignment (we don't care) */
    TRY_FREAD(&shortval, 2, 1, infh);

    /* Bits per sample (must be 16) */
    TRY_FREAD(&shortval, 2, 1, infh);
    SWAP_LE_16(&shortval);
    if (shortval != 16) {
        E_ERROR("WAVE file is not 16-bit\n");
        goto error_out;
    }

    /* Any extra parameters. */
    if (header_len > 16)
        fseek(infh, header_len - 16, SEEK_CUR);

    /* Now skip to the 'data' chunk. */
    while (1) {
        TRY_FREAD(id, 1, 4, infh);
        if (0 == memcmp(id, "data", 4)) {
            /* Total number of bytes of data (we don't care). */
            TRY_FREAD(&intval, 4, 1, infh);
            break;
        }
        else {
            /* Some other stuff... */
            /* Number of bytes of ... whatever */
            TRY_FREAD(&intval, 4, 1, infh);
            SWAP_LE_32(&intval);
            fseek(infh, intval, SEEK_CUR);
        }
    }

    /* We are ready to rumble. */
    return 0;
error_out:
    return -1;
}

static int
read_nist_header(FILE *infh)
{
    char hdr[1024];
    char *line, *c;

    TRY_FREAD(hdr, 1, 1024, infh);
    hdr[1023] = '\0';

    /* Roughly parse it to find the sampling rate and byte order
     * (don't bother with other stuff) */
    if ((line = strstr(hdr, "sample_rate")) == NULL) {
        E_ERROR("No sampling rate in NIST header!\n");
        goto error_out;
    }
    c = strchr(line, '\n');
    if (c) *c = '\0';
    c = strrchr(line, ' ');
    if (c == NULL) {
        E_ERROR("Could not find sampling rate!\n");
        goto error_out;
    }
    ++c;
    if (cmd_ln_int32("-samprate") == 0)
        cmd_ln_set_int32("-samprate", atoi(c));
    else if (cmd_ln_int32("-samprate") != atoi(c)) {
        E_WARN("NIST file sampling rate %d != -samprate %d\n",
               atoi(c), cmd_ln_int32("-samprate"));
    }

    if (line + strlen(line) < hdr + 1023)
        line[strlen(line)] = ' ';
    if ((line = strstr(hdr, "sample_byte_format")) == NULL) {
        E_ERROR("No sample byte format in NIST header!\n");
        goto error_out;
    }
    c = strchr(line, '\n');
    if (c) *c = '\0';
    c = strrchr(line, ' ');
    if (c == NULL) {
        E_ERROR("Could not find sample byte order!\n");
        goto error_out;
    }
    ++c;
    if (0 == memcmp(c, "01", 2)) {
        cmd_ln_set_str("-input_endian", "little");
    }
    else if (0 == memcmp(c, "10", 2)) {
        cmd_ln_set_str("-input_endian", "big");
    }
    else {
        E_ERROR("Unknown byte order %s\n", c);
        goto error_out;
    }

    /* We are ready to rumble. */
    return 0;
error_out:
    return -1;
}

static int
extract_pitch(const char *in, const char *out)
{
    FILE *infh = NULL, *outfh = NULL;
    size_t flen, fshift, nsamps;
    int16 *buf = NULL;
    yin_t *yin = NULL;
    uint16 period, bestdiff;
    int32 sps;

    if (out) {
        if ((outfh = fopen(out, "w")) == NULL) {
            E_ERROR_SYSTEM("Failed to open %s for writing", out);
            goto error_out;
        }
    }
    else {
        outfh = stdout;
    }
    if ((infh = fopen(in, "rb")) == NULL) {
        E_ERROR_SYSTEM("Failed to open %s for reading", in);
        goto error_out;
    }

    /* If we weren't told what the file type is, weakly try to
     * determine it (actually it's pretty obvious) */
    if (!(cmd_ln_boolean("-raw")
          || cmd_ln_boolean("-mswav")
          || cmd_ln_boolean("-nist"))) {
        if (guess_file_type(in, infh) < 0)
            goto error_out;
    }
    
    /* Grab the sampling rate and byte order from the header and also
     * make sure this is 16-bit linear PCM. */
    if (cmd_ln_boolean("-mswav")) {
        if (read_riff_header(infh) < 0)
            goto error_out;
    }
    else if (cmd_ln_boolean("-nist")) {
        if (read_nist_header(infh) < 0)
            goto error_out;
    }
    else if (cmd_ln_boolean("-raw")) {
        /* Just use some defaults for sampling rate and endian. */
        if (cmd_ln_str("-input_endian") == NULL) {
            cmd_ln_set_str("-input_endian", "little");
        }
        if (cmd_ln_int32("-samprate") == 0)
            cmd_ln_set_int32("-samprate", 16000);
    }

    /* Now read frames and write pitch estimates. */
    sps = cmd_ln_int32("-samprate");
    flen = (size_t)(0.5 + sps * cmd_ln_float32("-flen"));
    fshift = (size_t)(0.5 + sps * cmd_ln_float32("-fshift"));
    yin = yin_init(flen, cmd_ln_float32("-voice_thresh"),
                   cmd_ln_float32("-search_range"),
                   cmd_ln_int32("-smooth_window"));
    if (yin == NULL) {
        E_ERROR("Failed to initialize YIN\n");
        goto error_out;
    }
    buf = ckd_calloc(flen, sizeof(*buf));
    /* Read the first full frame of data. */
    if (fread(buf, sizeof(*buf), flen, infh) != flen) {
        /* Fail silently, which is probably okay. */
    }
    yin_start(yin);
    nsamps = 0;
    while (!feof(infh)) {
        /* Process a frame of data. */
        yin_write(yin, buf);
        if (yin_read(yin, &period, &bestdiff)) {
            fprintf(outfh, "%.3f %.2f %.2f\n",
                    /* Time point. */
                    (double)nsamps/sps,
                    /* "Probability" of voicing. */
                    bestdiff > 32768 ? 0.0 : 1.0 - (double)bestdiff / 32768,
                    /* Pitch (possibly bogus) */
                    period == 0 ? sps : (double)sps / period);
            nsamps += fshift;
        }
        /* Shift it back and get the next frame's overlap. */
        memmove(buf, buf + fshift, (flen - fshift) * sizeof(*buf));
        if (fread(buf + flen - fshift, sizeof(*buf), fshift, infh) != fshift) {
            /* Fail silently (FIXME: really?) */
        }
    }
    yin_end(yin);
    /* Process trailing frames of data. */
    while (yin_read(yin, &period, &bestdiff)) {
            fprintf(outfh, "%.3f %.2f %.2f\n",
                    /* Time point. */
                    (double)nsamps/sps,
                    /* "Probability" of voicing. */
                    bestdiff > 32768 ? 0.0 : 1.0 - (double)bestdiff / 32768,
                    /* Pitch (possibly bogus) */
                    period == 0 ? sps : (double)sps / period);
    }

    if (yin)
        yin_free(yin);
    ckd_free(buf);
    fclose(infh);
    if (outfh && outfh != stdout)
        fclose(outfh);
    return 0;

error_out:
    if (yin)
        yin_free(yin);
    ckd_free(buf);
    if (infh) fclose(infh);
    if (outfh && outfh != stdout) 
        fclose(outfh);
    return -1;
}

static int
run_control_file(const char *ctl)
{
    FILE *ctlfh;
    char *line;
    char *di, *dout, *ei, *eio;
    size_t len;
    int rv, guess_type, guess_sps, guess_endian;
    int32 skip, runlen;

    skip = cmd_ln_int32("-nskip");
    runlen = cmd_ln_int32("-runlen");

    /* Whether to guess file types */
    guess_type = !(cmd_ln_boolean("-raw")
                   || cmd_ln_boolean("-mswav")
                   || cmd_ln_boolean("-nist"));
    /* Whether to guess sampling rate */
    guess_sps = (cmd_ln_int32("-samprate") == 0);
    /* Whether to guess endian */
    guess_endian = (cmd_ln_str("-input_endian") == NULL);

    if ((ctlfh = fopen(ctl, "r")) == NULL) {
        E_ERROR_SYSTEM("Failed to open control file %s", ctl);
        return -1;
    }
    if (cmd_ln_str("-di"))
        di = string_join(cmd_ln_str("-di"), "/", NULL);
    else
        di = ckd_salloc("");
    if (cmd_ln_str("-do"))
        dout = string_join(cmd_ln_str("-do"), "/", NULL);
    else
        dout = ckd_salloc("");
    if (cmd_ln_str("-ei"))
        ei = string_join(".", cmd_ln_str("-ei"), NULL);
    else
        ei = ckd_salloc("");
    if (cmd_ln_str("-eo"))
        eio = string_join(".", cmd_ln_str("-eo"), NULL);
    else
        eio = ckd_salloc("");
    rv = 0;
    while ((line = fread_line(ctlfh, &len)) != NULL) {
        char *infile, *outfile;

        if (skip-- > 0) {
            ckd_free(line);
            continue;
        }
        if (runlen == 0) {
            ckd_free(line);
            break;
        }
        --runlen;

        if (line[len-1] == '\n')
            line[len-1] = '\0';

        infile = string_join(di, line, ei, NULL);
        outfile = string_join(dout, line, eio, NULL);

        /* Reset various guessed information */
        if (guess_type) {
            cmd_ln_set_boolean("-nist", FALSE);
            cmd_ln_set_boolean("-mswav", FALSE);
            cmd_ln_set_boolean("-raw", FALSE);
        }
        if (guess_sps)
            cmd_ln_set_int32("-samprate", 0);
        if (guess_endian)
            cmd_ln_set_str("-input_endian", NULL);

        rv = extract_pitch(infile, outfile);

        ckd_free(infile);
        ckd_free(outfile);
        ckd_free(line);

        if (rv != 0)
            break;
    }
    ckd_free(di);
    ckd_free(dout);
    ckd_free(ei);
    ckd_free(eio);
    fclose(ctlfh);
    return rv;
}

/* -*- c-basic-offset: 4; indent-tabs-mode: nil -*- */
/* ====================================================================
 * Copyright (c) 1994-2001 Carnegie Mellon University.  All rights
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
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef _WIN32
#pragma warning (disable: 4996) 
#endif

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <sphinxbase/strfuncs.h>
#include <sphinxbase/prim_type.h>
#include <sphinxbase/cmd_ln.h>
#include <sphinxbase/ckd_alloc.h>
#include <sphinxbase/err.h>
#include <sphinxbase/bio.h>
#include <sphinxbase/pio.h>

/** \file main_cepview.c
    \brief Main driver of cepview
 */
#define IO_ERR  (-1)
#define IO_SUCCESS  (0)

#define SHOW_ALL "-1"

/* Default cepstral vector size */
#define NUM_COEFF  "13"

/* Default display size, i.e., number of coefficients displayed, less
 * than the vector size so we display one frame per line.
 */
#define DISPLAY_SIZE "10"
#define STR_MAX_INT "2147483647"

static arg_t arg[] = {

    {"-logfn",
     ARG_STRING,
     NULL,
     "Log file (default stdout/stderr)"},
    {"-i",
     ARG_INT32,
     NUM_COEFF,
     "Number of coefficients in the feature vector."},
    {"-d",
     ARG_INT32,
     DISPLAY_SIZE,
     "Number of displayed coefficients."},
    {"-header",
     ARG_INT32,
     "0",
     "Whether header is shown."},
    {"-describe",
     ARG_INT32,
     "0",
     "Whether description will be shown."},
    {"-b",
     ARG_INT32,
     "0",
     "The beginning frame 0-based."},
    {"-e",
     ARG_INT32,
     "2147483647",
     "The ending frame."},
    {"-f",
     ARG_STRING,
     NULL,
     "Input feature file."},
    {NULL, ARG_INT32, NULL, NULL}
};

int read_cep(char const *file, float ***cep, int *nframes, int numcep);

int
main(int argc, char *argv[])
{
    int i, j, offset;
    int32 noframe, vsize, dsize, column;
    int32 frm_begin, frm_end;
    int is_header, is_describe;
    float *z, **cep;
    char const *cepfile;

    cmd_ln_appl_enter(argc, argv, "default.arg", arg);

    vsize = cmd_ln_int32("-i");
    dsize = cmd_ln_int32("-d");
    frm_begin = cmd_ln_int32("-b");
    frm_end = cmd_ln_int32("-e");
    is_header = cmd_ln_int32("-header");
    is_describe = cmd_ln_int32("-describe");

    if (vsize < 0)
        E_FATAL("-i : Input vector size should be larger than 0.\n");
    if (dsize < 0)
        E_FATAL("-d : Column size should be larger than 0\n");
    if (frm_begin < 0)
        E_FATAL("-b : Beginning frame should be larger than 0\n");
    /* The following condition is redundant
     * if (frm_end < 0) E_FATAL("-e : Ending frame should be larger than 0\n");
     */
    if (frm_begin >= frm_end)
        E_FATAL
            ("Ending frame (-e) should be larger than beginning frame (-b).\n");

    if ((cepfile = cmd_ln_str("-f")) == NULL) {
        E_FATAL("Input file was not specified with (-f)\n");
    }
    if (read_cep(cepfile, &cep, &noframe, vsize) == IO_ERR)
        E_FATAL_SYSTEM("Failed to open '%s' for reading", cepfile);

    z = cep[0];

    offset = 0;
    column = (vsize > dsize) ? dsize : vsize;
    frm_end = (frm_end > noframe) ? noframe : frm_end;

    E_INFO("Displaying %d out of %d columns per frame\n", column, vsize);
    E_INFO("Total %d frames\n\n", noframe);

    /* This part should be moved to a special library if this file is
       longer than 300 lines. */

    if (is_header) {
        if (is_describe) {
            printf("\n%6s", "frame#:");
        }

        for (j = 0; j < column; ++j) {
            printf("%3s%3d%s ", "c[", j, "]");
        }
        printf("\n");
    }

    offset += frm_begin * vsize;
    for (i = frm_begin; i < frm_end; ++i) {
        if (is_describe) {
            printf("%6d:", i);
        }
        for (j = 0; j < column; ++j)
            printf("%7.3f ", z[offset + j]);
        printf("\n");

        offset += vsize;
    }
    fflush(stdout);
    cmd_ln_appl_exit();
    ckd_free_2d(cep);

    return (IO_SUCCESS);

}

int
read_cep(char const *file, float ***cep, int *numframes, int cepsize)
{
    FILE *fp;
    int n_float;
    struct stat statbuf;
    int i, n, byterev;
    float32 **mfcbuf;

    if (stat_retry(file, &statbuf) < 0) {
        E_ERROR_SYSTEM("Failed to get file size '%s'", file);
        return IO_ERR;
    }

    if ((fp = fopen(file, "rb")) == NULL) {
        E_ERROR_SYSTEM("Failed to open '%s' for reading", file);
        return IO_ERR;
    }

    /* Read #floats in header */
    if (fread(&n_float, sizeof(int), 1, fp) != 1) {
        fclose(fp);
        return IO_ERR;
    }

    /* Check if n_float matches file size */
    byterev = FALSE;
    if ((int) (n_float * sizeof(float) + 4) != statbuf.st_size) {
        n = n_float;
        SWAP_INT32(&n);

        if ((int) (n * sizeof(float) + 4) != statbuf.st_size) {
    	    E_ERROR("Header size field: %d(%08x); filesize: %d(%08x)\n",
                 n_float, n_float, (int) statbuf.st_size,
                 (int) statbuf.st_size);
            fclose(fp);
            return IO_ERR;
        }

        n_float = n;
        byterev = TRUE;
    }
    if (n_float <= 0) {
        E_ERROR("Header size field: %d\n", n_float);
        fclose(fp);
        return IO_ERR;
    }

    /* n = #frames of input */
    n = n_float / cepsize;
    if (n * cepsize != n_float) {
        E_ERROR("Header size field: %d; not multiple of %d\n",
               n_float, cepsize);
        fclose(fp);
        return IO_ERR;
    }

    mfcbuf = (float **) ckd_calloc_2d(n, cepsize, sizeof(float32));

    /* Read mfc data and byteswap if necessary */
    n_float = n * cepsize;
    if ((int) fread(mfcbuf[0], sizeof(float), n_float, fp) != n_float) {
        E_ERROR("Error reading mfc data from the file '%s'", file);
        fclose(fp);
        return IO_ERR;
    }
    if (byterev) {
        for (i = 0; i < n_float; i++)
            SWAP_FLOAT32(&(mfcbuf[0][i]));
    }
    fclose(fp);

    *numframes = n;
    *cep = mfcbuf;
    return IO_SUCCESS;
}

/** Silvio Moioli: Windows CE/Mobile entry point added. */
#if defined(_WIN32_WCE)
#pragma comment(linker,"/entry:mainWCRTStartup")

/* Windows Mobile has the Unicode main only */
int wmain(int32 argc, wchar_t *wargv[]) {
    char** argv;
    size_t wlen;
    size_t len;
    int i;

    argv = malloc(argc*sizeof(char*));
    for (i = 0; i < argc; i++){
        wlen = lstrlenW(wargv[i]);
        len = wcstombs(NULL, wargv[i], wlen);
        argv[i] = malloc(len+1);
        wcstombs(argv[i], wargv[i], wlen);
    }

    /* assuming ASCII parameters */
    return main(argc, argv);
}
#endif

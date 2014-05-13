/* -*- c-basic-offset: 4; indent-tabs-mode: nil -*- */
/* ====================================================================
 * Copyright (c) 2006 Carnegie Mellon University.  All rights
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
/**
 * mdef_convert.c - convert text to binary model definition files (and vice versa)
 *
 * Author: David Huggins-Daines <dhuggins@cs.cmu.edu>
 **/

#include <stdio.h>
#include <string.h>

#include <pocketsphinx.h>

#include "bin_mdef.h"

int
main(int argc, char *argv[])
{
    const char *infile, *outfile;
    bin_mdef_t *bin;
    int tobin = 1;

    if (argc < 3 || argc > 4) {
        fprintf(stderr, "Usage: %s [-text | -bin] INPUT OUTPUT\n",
                argv[0]);
        return 1;
    }
    if (argv[1][0] == '-') {
        if (strcmp(argv[1], "-text") == 0) {
            tobin = 0;
            ++argv;
        }
        else if (strcmp(argv[1], "-bin") == 0) {
            tobin = 1;
            ++argv;
        }
        else {
            fprintf(stderr, "Unknown argument %s\n", argv[1]);
            fprintf(stderr, "Usage: %s [-text | -bin] INPUT OUTPUT\n",
                    argv[0]);
            return 1;
        }
    }
    infile = argv[1];
    outfile = argv[2];

    if (tobin) {
        if ((bin = bin_mdef_read_text(NULL, infile)) == NULL) {
            fprintf(stderr, "Failed to read text mdef from %s\n", infile);
            return 1;
        }
        if (bin_mdef_write(bin, outfile) < 0) {
            fprintf(stderr, "Failed to write binary mdef to %s\n",
                    outfile);
            return 1;
        }
    }
    else {
        if ((bin = bin_mdef_read(NULL, infile)) == NULL) {
            fprintf(stderr, "Failed to read binary mdef from %s\n",
                    infile);
            return 1;
        }
        if (bin_mdef_write_text(bin, outfile) < 0) {
            fprintf(stderr, "Failed to write text mdef to %s\n", outfile);
            return 1;
        }
    }

    return 0;
}

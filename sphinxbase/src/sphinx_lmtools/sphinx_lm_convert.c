/* -*- c-basic-offset: 4; indent-tabs-mode: nil -*- */
/* ====================================================================
 * Copyright (c) 2009 Carnegie Mellon University.  All rights 
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
 * \file sphinx_lm_convert.c
 * Language model conversion tool.
 */
#include <sphinxbase/logmath.h>
#include <sphinxbase/ngram_model.h>
#include <sphinxbase/cmd_ln.h>
#include <sphinxbase/ckd_alloc.h>
#include <sphinxbase/err.h>
#include <sphinxbase/pio.h>
#include <sphinxbase/strfuncs.h>

#include <stdio.h>
#include <string.h>
#include <math.h>

static const arg_t defn[] = {
  { "-help",
    ARG_BOOLEAN,
    "no",
    "Shows the usage of the tool"},

  { "-logbase",
    ARG_FLOAT64,
    "1.0001",
    "Base in which all log-likelihoods calculated" },

  { "-i",
    REQARG_STRING,
    NULL,
    "Input language model file (required)"},

  { "-o",
    REQARG_STRING,
    NULL,
    "Output language model file (required)"},

  { "-ifmt",
    ARG_STRING,
    NULL,
    "Input language model format (will guess if not specified)"},

  { "-ofmt",
    ARG_STRING,
    NULL,
    "Output language model file (will guess if not specified)"},

  { "-ienc",
    ARG_STRING,
    NULL,
    "Input language model text encoding (no conversion done if not specified)"},

  { "-oenc",
    ARG_STRING,
    "utf8",
    "Output language model text encoding"},

  { "-case",
    ARG_STRING,
    NULL,
    "Ether 'lower' or 'upper' - case fold to lower/upper case (NOT UNICODE AWARE)" },

  { "-mmap",
    ARG_BOOLEAN,
    "no",
    "Use memory-mapped I/O for reading binary LM files"},

  { "-debug",
    ARG_INT32,
    NULL,
    "Verbosity level for debugging messages"
  },

  { NULL, 0, NULL, NULL }
};

static void
usagemsg(char *pgm)
{
    E_INFO("Usage: %s -i <input.lm> \\\n", pgm);
    E_INFOCONT("\t[-ifmt txt] [-ofmt dmp]\n");
    E_INFOCONT("\t-o <output.lm.DMP>\n");

    exit(0);
}


int
main(int argc, char *argv[])
{
	cmd_ln_t *config;
	ngram_model_t *lm = NULL;
	logmath_t *lmath;
        int itype, otype;
        char const *kase;

	if ((config = cmd_ln_parse_r(NULL, defn, argc, argv, TRUE)) == NULL)
		return 1;
		
	if (cmd_ln_boolean_r(config, "-help")) {
	    usagemsg(argv[0]);
	}

        err_set_debug_level(cmd_ln_int32_r(config, "-debug"));

	/* Create log math object. */
	if ((lmath = logmath_init
	     (cmd_ln_float64_r(config, "-logbase"), 0, 0)) == NULL) {
		E_FATAL("Failed to initialize log math\n");
	}
	
	if (cmd_ln_str_r(config, "-i") == NULL || cmd_ln_str_r(config, "-i") == NULL) {
            E_ERROR("Please specify both input and output models\n");
            goto error_out;
        }
	    
	
	/* Load the input language model. */
        if (cmd_ln_str_r(config, "-ifmt")) {
            if ((itype = ngram_str_to_type(cmd_ln_str_r(config, "-ifmt")))
                == NGRAM_INVALID) {
                E_ERROR("Invalid input type %s\n", cmd_ln_str_r(config, "-ifmt"));
                goto error_out;
            }
            lm = ngram_model_read(config, cmd_ln_str_r(config, "-i"),
                                  itype, lmath);
        }
        else {
            lm = ngram_model_read(config, cmd_ln_str_r(config, "-i"),
                                  NGRAM_AUTO, lmath);
	}

	if (lm == NULL) {
	    E_FATAL("Failed to read the model from the file '%s'", cmd_ln_str_r(config, "-i"));
	}

        /* Guess or set the output language model type. */
        if (cmd_ln_str_r(config, "-ofmt")) {
            if ((otype = ngram_str_to_type(cmd_ln_str_r(config, "-ofmt")))
                == NGRAM_INVALID) {
                E_ERROR("Invalid output type %s\n", cmd_ln_str_r(config, "-ofmt"));
                goto error_out;
            }
        }
        else {
            otype = ngram_file_name_to_type(cmd_ln_str_r(config, "-o"));
        }

        /* Recode the language model if desired. */
        if (cmd_ln_str_r(config, "-ienc")) {
            if (ngram_model_recode(lm, cmd_ln_str_r(config, "-ienc"),
                                   cmd_ln_str_r(config, "-oenc")) != 0) {
                E_ERROR("Failed to recode language model from %s to %s\n",
                        cmd_ln_str_r(config, "-ienc"),
                        cmd_ln_str_r(config, "-oenc"));
                goto error_out;
            }
        }

        /* Case fold if requested. */
        if ((kase = cmd_ln_str_r(config, "-case"))) {
            if (0 == strcmp(kase, "lower")) {
                ngram_model_casefold(lm, NGRAM_LOWER);
            }
            else if (0 == strcmp(kase, "upper")) {
                ngram_model_casefold(lm, NGRAM_UPPER);
            }
            else {
                E_ERROR("Unknown value for -case: %s\n", kase);
                goto error_out;
            }
        }

        /* Write the output language model. */
        if (ngram_model_write(lm, cmd_ln_str_r(config, "-o"), otype) != 0) {
            E_ERROR("Failed to write language model in format %s to %s\n",
                    ngram_type_to_str(otype), cmd_ln_str_r(config, "-o"));
            goto error_out;
        }

        /* That's all folks! */
        ngram_model_free(lm);
	return 0;

error_out:
        ngram_model_free(lm);
	return 1;
}

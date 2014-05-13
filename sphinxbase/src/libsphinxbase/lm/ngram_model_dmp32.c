/* -*- c-basic-offset: 4; indent-tabs-mode: nil -*- */
/* ====================================================================
 * Copyright (c) 1999-2007 Carnegie Mellon University.  All rights
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
 * \file ngram_model_dmp32.c DMP32 format language models
 *
 * Author: David Huggins-Daines <dhuggins@cs.cmu.edu>
 */

#include "sphinxbase/ckd_alloc.h"

#include "ngram_model_internal.h"

static ngram_funcs_t ngram_model_dmp32_funcs;

ngram_model_t *
ngram_model_dmp32_read(cmd_ln_t *config,
                       const char *file_name,
                       logmath_t *lmath)
{
    return NULL;
}

int
ngram_model_dmp32_write(ngram_model_t *model,
                        const char *file_name)
{
    return -1;
}

static int
ngram_model_dmp32_apply_weights(ngram_model_t *model, float32 lw,
                              float32 wip, float32 uw)
{
    return 0;
}

static int32
ngram_model_dmp32_score(ngram_model_t *model, int32 wid,
                        int32 *history, int32 n_hist,
                        int32 *n_used)
{
    return model->log_zero;
}

static int32
ngram_model_dmp32_raw_score(ngram_model_t *model, int32 wid,
                            int32 *history, int32 n_hist,
                            int32 *n_used)
{
    return model->log_zero;
}

static void
ngram_model_dmp32_free(ngram_model_t *base)
{
    ckd_free(base);
}

static ngram_funcs_t ngram_model_dmp32_funcs = {
    ngram_model_dmp32_free,          /* free */
    ngram_model_dmp32_apply_weights, /* apply_weights */
    ngram_model_dmp32_score,         /* score */
    ngram_model_dmp32_raw_score,     /* raw_score */
    NULL                             /* add_ug */
};

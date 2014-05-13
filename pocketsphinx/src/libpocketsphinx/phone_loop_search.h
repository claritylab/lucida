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

/**
 * @file phone_loop_search.h Fast and rough context-independent
 * phoneme loop search.
 *
 * This exists for the purposes of phoneme lookahead, and thus it
 * actually does not do phoneme recognition (it wouldn't be very
 * accurate anyway).
 */

#ifndef __PHONE_LOOP_SEARCH_H__
#define __PHONE_LOOP_SEARCH_H__

/* SphinxBase headers. */
#include <sphinxbase/cmd_ln.h>
#include <sphinxbase/logmath.h>
#include <sphinxbase/ngram_model.h>
#include <sphinxbase/listelem_alloc.h>

/* Local headers. */
#include "pocketsphinx_internal.h"
#include "hmm.h"

/**
 * Phone loop structure.
 */
struct phone_loop_s {
    hmm_t hmm;       /**< Basic HMM structure. */
    int16 ciphone;   /**< Context-independent phone ID. */
    int16 frame;     /**< Last frame this phone was active. */
};
typedef struct phone_loop_s phone_loop_t;

/**
 * Renormalization event.
 */
struct phone_loop_renorm_s {
    int frame_idx;  /**< Frame of renormalization. */
    int32 norm;     /**< Normalization constant. */
};
typedef struct phone_loop_renorm_s phone_loop_renorm_t;

/**
 * Phone loop search structure.
 */
struct phone_loop_search_s {
    ps_search_t base;       /**< Base search structure. */
    hmm_context_t *hmmctx;  /**< HMM context structure. */
    int16 frame;            /**< Current frame being searched. */
    int16 n_phones;         /**< Size of phone array. */
    phone_loop_t *phones;   /**< Array of phone arcs. */

    int32 best_score;       /**< Best Viterbi score in current frame. */
    int32 beam;             /**< HMM pruning beam width. */
    int32 pbeam;            /**< Phone exit pruning beam width. */
    int32 pip;              /**< Phone insertion penalty ("language score"). */
    glist_t renorm;         /**< List of renormalizations. */
};
typedef struct phone_loop_search_s phone_loop_search_t;

ps_search_t *phone_loop_search_init(cmd_ln_t *config,
                                    acmod_t *acmod,
                                    dict_t *dict);

/**
 * Return lookahead heuristic score for a specific phone.
 */
#define phone_loop_search_score(pls,ci) \
    ((pls == NULL) ? 0                                          \
     : (hmm_bestscore(&pls->phones[ci].hmm) - (pls)->best_score))

#endif /* __PHONE_LOOP_SEARCH_H__ */

/*
 *      CRF1d tagger (implementation of crfsuite_model_t and crfsuite_tagger_t).
 *
 * Copyright (c) 2007-2010, Naoaki Okazaki
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in the
 *       documentation and/or other materials provided with the distribution.
 *     * Neither the names of the authors nor the names of its contributors
 *       may be used to endorse or promote products derived from this
 *       software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER
 * OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
 * LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/* $Id$ */

#ifdef    HAVE_CONFIG_H
#include <config.h>
#endif/*HAVE_CONFIG_H*/

#include <os.h>

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <crfsuite.h>

#include "crf1d.h"

enum {
    LEVEL_NONE = 0,
    LEVEL_SET,
    LEVEL_ALPHABETA,
};

typedef struct {
    crf1dm_t *model;        /**< CRF model. */
    crf1d_context_t *ctx;   /**< CRF context. */
    int num_labels;         /**< Number of distinct output labels (L). */
    int num_attributes;     /**< Number of distinct attributes (A). */
    int level;
} crf1dt_t;

static void crf1dt_state_score(crf1dt_t *crf1dt, const crfsuite_instance_t *inst)
{
    int a, i, l, t, r, fid;
    crf1dm_feature_t f;
    feature_refs_t attr;
    floatval_t value, *state = NULL;
    crf1dm_t* model = crf1dt->model;
    crf1d_context_t* ctx = crf1dt->ctx;
    const crfsuite_item_t* item = NULL;
    const int T = inst->num_items;
    const int L = crf1dt->num_labels;

    /* Loop over the items in the sequence. */
    for (t = 0;t < T;++t) {
        item = &inst->items[t];
        state = STATE_SCORE(ctx, t);

        /* Loop over the contents (attributes) attached to the item. */
        for (i = 0;i < item->num_contents;++i) {
            /* Access the list of state features associated with the attribute. */
            a = item->contents[i].aid;
            crf1dm_get_attrref(model, a, &attr);
            /* A scale usually represents the atrribute frequency in the item. */
            value = item->contents[i].value;

            /* Loop over the state features associated with the attribute. */
            for (r = 0;r < attr.num_features;++r) {
                /* The state feature #(attr->fids[r]), which is represented by
                   the attribute #a, outputs the label #(f->dst). */
                fid = crf1dm_get_featureid(&attr, r);
                crf1dm_get_feature(model, fid, &f);
                l = f.dst;
                state[l] += f.weight * value;
            }
        }
    }
}

static void crf1dt_transition_score(crf1dt_t* crf1dt)
{
    int i, r, fid;
    crf1dm_feature_t f;
    feature_refs_t edge;
    floatval_t *trans = NULL;
    crf1dm_t* model = crf1dt->model;
    crf1d_context_t* ctx = crf1dt->ctx;
    const int L = crf1dt->num_labels;

    /* Compute transition scores between two labels. */
    for (i = 0;i < L;++i) {
        trans = TRANS_SCORE(ctx, i);
        crf1dm_get_labelref(model, i, &edge);
        for (r = 0;r < edge.num_features;++r) {
            /* Transition feature from #i to #(f->dst). */
            fid = crf1dm_get_featureid(&edge, r);
            crf1dm_get_feature(model, fid, &f);
            trans[f.dst] = f.weight;
        }        
    }
}

static void crf1dt_set_level(crf1dt_t *crf1dt, int level)
{
    int prev = crf1dt->level;
    crf1d_context_t* ctx = crf1dt->ctx;

    if (level <= LEVEL_ALPHABETA && prev < LEVEL_ALPHABETA) {
        crf1dc_exp_state(ctx);
        crf1dc_alpha_score(ctx);
        crf1dc_beta_score(ctx);
    }

    crf1dt->level = level;
}

static void crf1dt_delete(crf1dt_t* crf1dt)
{
    /* Note: we don't own the model object (crf1t->model). */
    if (crf1dt->ctx != NULL) {
        crf1dc_delete(crf1dt->ctx);
        crf1dt->ctx = NULL;
    }
    free(crf1dt);
}

static crf1dt_t *crf1dt_new(crf1dm_t* crf1dm)
{
    crf1dt_t* crf1dt = NULL;

    crf1dt = (crf1dt_t*)calloc(1, sizeof(crf1dt_t));
    if (crf1dt != NULL) {
        crf1dt->num_labels = crf1dm_get_num_labels(crf1dm);
        crf1dt->num_attributes = crf1dm_get_num_attrs(crf1dm);
        crf1dt->model = crf1dm;
        crf1dt->ctx = crf1dc_new(CTXF_VITERBI | CTXF_MARGINALS, crf1dt->num_labels, 0);
        if (crf1dt->ctx != NULL) {
            crf1dc_reset(crf1dt->ctx, RF_TRANS);
            crf1dt_transition_score(crf1dt);
            crf1dc_exp_transition(crf1dt->ctx);
        } else {
            crf1dt_delete(crf1dt);
            crf1dt = NULL;
        }
        crf1dt->level = LEVEL_NONE;
    }

    return crf1dt;
}



/*
 *    Implementation of crfsuite_tagger_t object.
 *    This object is instantiated only by a crfsuite_model_t object.
 */

static int tagger_addref(crfsuite_tagger_t* tagger)
{
    /* This object is owned only by a crfsuite_model_t object. */
    return tagger->nref;
}

static int tagger_release(crfsuite_tagger_t* tagger)
{
    /* This object is owned only by a crfsuite_model_t object. */
    return tagger->nref;
}

static int tagger_set(crfsuite_tagger_t* tagger, crfsuite_instance_t *inst)
{
    crf1dt_t* crf1dt = (crf1dt_t*)tagger->internal;
    crf1d_context_t* ctx = crf1dt->ctx;
    crf1dc_set_num_items(ctx, inst->num_items);
    crf1dc_reset(crf1dt->ctx, RF_STATE);
    crf1dt_state_score(crf1dt, inst);
    crf1dt->level = LEVEL_SET;
    return 0;
}

static int tagger_length(crfsuite_tagger_t* tagger)
{
    crf1dt_t* crf1dt = (crf1dt_t*)tagger->internal;
    crf1d_context_t* ctx = crf1dt->ctx;
    return ctx->num_items;
}

static int tagger_viterbi(crfsuite_tagger_t* tagger, int *labels, floatval_t *ptr_score)
{
    floatval_t score;
    crf1dt_t* crf1dt = (crf1dt_t*)tagger->internal;
    crf1d_context_t* ctx = crf1dt->ctx;

    score = crf1dc_viterbi(ctx, labels);
    if (ptr_score != NULL) {
        *ptr_score = score;
    }

    return 0;
}

static int tagger_score(crfsuite_tagger_t* tagger, int *path, floatval_t *ptr_score)
{
    floatval_t score;
    crf1dt_t* crf1dt = (crf1dt_t*)tagger->internal;
    crf1d_context_t* ctx = crf1dt->ctx;
    score = crf1dc_score(ctx, path);
    if (ptr_score != NULL) {
        *ptr_score = score;
    }
    return 0;
}

static int tagger_lognorm(crfsuite_tagger_t* tagger, floatval_t *ptr_norm)
{
    crf1dt_t* crf1dt = (crf1dt_t*)tagger->internal;
    crf1dt_set_level(crf1dt, LEVEL_ALPHABETA);
    *ptr_norm = crf1dc_lognorm(crf1dt->ctx);
    return 0;
}

static int tagger_marginal_point(crfsuite_tagger_t *tagger, int l, int t, floatval_t *ptr_prob)
{
    crf1dt_t* crf1dt = (crf1dt_t*)tagger->internal;
    crf1dt_set_level(crf1dt, LEVEL_ALPHABETA);
    *ptr_prob = crf1dc_marginal_point(crf1dt->ctx, l, t);
    return 0;
}

static int tagger_marginal_path(crfsuite_tagger_t *tagger, const int *path, int begin, int end, floatval_t *ptr_prob)
{
    crf1dt_t* crf1dt = (crf1dt_t*)tagger->internal;
    crf1dt_set_level(crf1dt, LEVEL_ALPHABETA);
    *ptr_prob = crf1dc_marginal_path(crf1dt->ctx, path, begin, end);
    return 0;
}



/*
 *    Implementation of crfsuite_dictionary_t object for attributes.
 *    This object is instantiated only by a crfsuite_model_t object.
 */

static int model_attrs_addref(crfsuite_dictionary_t* dic)
{
    /* This object is owned only by a crfsuite_model_t object. */
    return dic->nref;
}

static int model_attrs_release(crfsuite_dictionary_t* dic)
{
    /* This object is owned and freed only by a crfsuite_model_t object. */
    return dic->nref;
}

static int model_attrs_get(crfsuite_dictionary_t* dic, const char *str)
{
    /* This object is ready only. */
    return CRFSUITEERR_NOTSUPPORTED;
}

static int model_attrs_to_id(crfsuite_dictionary_t* dic, const char *str)
{
    crf1dm_t *crf1dm = (crf1dm_t*)dic->internal;
    return crf1dm_to_aid(crf1dm, str);
}

static int model_attrs_to_string(crfsuite_dictionary_t* dic, int id, char const **pstr)
{
    crf1dm_t *crf1dm = (crf1dm_t*)dic->internal;
    *pstr = crf1dm_to_attr(crf1dm, id);
    return 0;
}

static int model_attrs_num(crfsuite_dictionary_t* dic)
{
    crf1dm_t *crf1dm = (crf1dm_t*)dic->internal;
    return crf1dm_get_num_attrs(crf1dm);
}

static void model_attrs_free(crfsuite_dictionary_t* dic, const char *str)
{
    /* all strings are freed on the release of the dictionary object. */
}




/*
 *    Implementation of crfsuite_dictionary_t object for labels.
 *    This object is instantiated only by a crfsuite_model_t object.
 */

static int model_labels_addref(crfsuite_dictionary_t* dic)
{
    /* This object is owned only by a crfsuite_model_t object. */
    return dic->nref;
}

static int model_labels_release(crfsuite_dictionary_t* dic)
{
    /* This object is owned and freed only by a crfsuite_model_t object. */
    return dic->nref;
}

static int model_labels_get(crfsuite_dictionary_t* dic, const char *str)
{
    /* This object is ready only. */
    return CRFSUITEERR_NOTSUPPORTED;
}

static int model_labels_to_id(crfsuite_dictionary_t* dic, const char *str)
{
    crf1dm_t *crf1dm = (crf1dm_t*)dic->internal;
    return crf1dm_to_lid(crf1dm, str);
}

static int model_labels_to_string(crfsuite_dictionary_t* dic, int id, char const **pstr)
{
    crf1dm_t *crf1dm = (crf1dm_t*)dic->internal;
    *pstr = crf1dm_to_label(crf1dm, id);
    return 0;
}

static int model_labels_num(crfsuite_dictionary_t* dic)
{
    crf1dm_t *crf1dm = (crf1dm_t*)dic->internal;
    return crf1dm_get_num_labels(crf1dm);
}

static void model_labels_free(crfsuite_dictionary_t* dic, const char *str)
{
    /* all strings are freed on the release of the dictionary object. */
}



/*
 *    Implementation of crfsuite_model_t object.
 *    This object is instantiated by crf1m_model_create() function.
 */

typedef struct {
    crf1dm_t*    crf1dm;

    crfsuite_dictionary_t*    attrs;
    crfsuite_dictionary_t*    labels;
    crfsuite_tagger_t*        tagger;
} model_internal_t;

static int model_addref(crfsuite_model_t* model)
{
    return crfsuite_interlocked_increment(&model->nref);
}

static int model_release(crfsuite_model_t* model)
{
    int count = crfsuite_interlocked_decrement(&model->nref);
    if (count == 0) {
        /* This instance is being destroyed. */
        model_internal_t* internal = (model_internal_t*)model->internal;
        crf1dt_delete((crf1dt_t*)internal->tagger->internal);
        free(internal->tagger);
        free(internal->labels);
        free(internal->attrs);
        crf1dm_close(internal->crf1dm);
        free(internal);
        free(model);
    }
    return count;
}

static int model_get_tagger(crfsuite_model_t* model, crfsuite_tagger_t** ptr_tagger)
{
    model_internal_t* internal = (model_internal_t*)model->internal;
    /* We don't increment the reference counter. */
    *ptr_tagger = internal->tagger;
    return 0;
}

static int model_get_labels(crfsuite_model_t* model, crfsuite_dictionary_t** ptr_labels)
{
    model_internal_t* internal = (model_internal_t*)model->internal;
    /* We don't increment the reference counter. */
    *ptr_labels = internal->labels;
    return 0;
}

static int model_get_attrs(crfsuite_model_t* model, crfsuite_dictionary_t** ptr_attrs)
{
    model_internal_t* internal = (model_internal_t*)model->internal;
    /* We don't increment the reference counter. */
    *ptr_attrs = internal->attrs;
    return 0;
}

static int model_dump(crfsuite_model_t* model, FILE *fpo)
{
    model_internal_t* internal = (model_internal_t*)model->internal;
    crf1dm_dump(internal->crf1dm, fpo);
    return 0;
}

static int crf1m_model_create(const char *filename, crfsuite_model_t** ptr_model)
{
    int ret = 0;
    crf1dm_t *crf1dm = NULL;
    crf1dt_t *crf1dt = NULL;
    crfsuite_model_t *model = NULL;
    model_internal_t *internal = NULL;
    crfsuite_tagger_t *tagger = NULL;
    crfsuite_dictionary_t *attrs = NULL, *labels = NULL;

    *ptr_model = NULL;

    /* Open the model file. */
    crf1dm = crf1dm_new(filename);
    if (crf1dm == NULL) {
        ret = CRFSUITEERR_INCOMPATIBLE;
        goto error_exit;
    }

    /* Construct a tagger based on the model. */
    crf1dt = crf1dt_new(crf1dm);
    if (crf1dt == NULL) {
        ret = CRFSUITEERR_OUTOFMEMORY;
        goto error_exit;
    }

    /* Create an instance of internal data attached to the model. */
    internal = (model_internal_t*)calloc(1, sizeof(model_internal_t));
    if (internal == NULL) {
        ret = CRFSUITEERR_OUTOFMEMORY;
        goto error_exit;
    }

    /* Create an instance of dictionary object for attributes. */
    attrs = (crfsuite_dictionary_t*)calloc(1, sizeof(crfsuite_dictionary_t));
    if (attrs == NULL) {
        ret = CRFSUITEERR_OUTOFMEMORY;
        goto error_exit;
    }
    attrs->internal = crf1dm;
    attrs->nref = 1;
    attrs->addref = model_attrs_addref;
    attrs->release = model_attrs_release;
    attrs->get = model_attrs_get;
    attrs->to_id = model_attrs_to_id;
    attrs->to_string = model_attrs_to_string;
    attrs->num = model_attrs_num;
    attrs->free = model_attrs_free;

    /* Create an instance of dictionary object for labels. */
    labels = (crfsuite_dictionary_t*)calloc(1, sizeof(crfsuite_dictionary_t));
    if (labels == NULL) {
        ret = CRFSUITEERR_OUTOFMEMORY;
        goto error_exit;
    }
    labels->internal = crf1dm;
    labels->nref = 1;
    labels->addref = model_labels_addref;
    labels->release = model_labels_release;
    labels->get = model_labels_get;
    labels->to_id = model_labels_to_id;
    labels->to_string = model_labels_to_string;
    labels->num = model_labels_num;
    labels->free = model_labels_free;

    /* Create an instance of tagger object. */
    tagger = (crfsuite_tagger_t*)calloc(1, sizeof(crfsuite_tagger_t));
    if (tagger == NULL) {
        ret = CRFSUITEERR_OUTOFMEMORY;
        goto error_exit;
    }
    tagger->internal = crf1dt;
    tagger->nref = 1;
    tagger->addref = tagger_addref;
    tagger->release = tagger_release;
    tagger->set = tagger_set;
    tagger->length = tagger_length;
    tagger->viterbi = tagger_viterbi;
    tagger->score = tagger_score;
    tagger->lognorm = tagger_lognorm;
    tagger->marginal_point = tagger_marginal_point;
    tagger->marginal_path = tagger_marginal_path;

    /* Set the internal data for the model object. */
    internal->crf1dm = crf1dm;
    internal->attrs = attrs;
    internal->labels = labels;
    internal->tagger = tagger;

    /* Create an instance of model object. */
    model = (crfsuite_model_t*)calloc(1, sizeof(crfsuite_model_t));
    if (model == NULL) {
        ret = CRFSUITEERR_OUTOFMEMORY;
        goto error_exit;
    }
    model->internal = internal;
    model->nref = 1;
    model->addref = model_addref;
    model->release = model_release;
    model->get_attrs = model_get_attrs;
    model->get_labels = model_get_labels;
    model->get_tagger = model_get_tagger;
    model->dump = model_dump;

    *ptr_model = model;
    return 0;

error_exit:
    free(tagger);
    free(labels);
    free(attrs);
    if (crf1dt != NULL) {
        crf1dt_delete(crf1dt);
    }
    if (crf1dm != NULL) {
        crf1dm_close(crf1dm);
    }
    free(internal);
    free(model);
    return ret;
}

int crf1m_create_instance_from_file(const char *filename, void **ptr)
{
    return crf1m_model_create(filename, (crfsuite_model_t**)ptr);
}

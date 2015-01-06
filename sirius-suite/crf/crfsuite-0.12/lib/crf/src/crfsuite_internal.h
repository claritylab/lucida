/*
 *      CRFsuite internal interface.
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

#ifndef __CRFSUITE_INTERNAL_H__
#define __CRFSUITE_INTERNAL_H__

#include <crfsuite.h>
#include "logging.h"

enum {
    FTYPE_NONE = 0,             /**< Unselected. */
    FTYPE_CRF1D,                /**< 1st-order tyad features. */
    FTYPE_CRF1T,                /**< 1st-order triad features. */
};

enum {
    TRAIN_NONE = 0,             /**< Unselected. */
    TRAIN_LBFGS,                /**< L-BFGS batch training. */
    TRAIN_L2SGD,                /**< Pegasos online training. */
    TRAIN_AVERAGED_PERCEPTRON,  /**< Averaged perceptron. */
    TRAIN_PASSIVE_AGGRESSIVE,
    TRAIN_AROW,
};

struct tag_crfsuite_train_internal;
typedef struct tag_crfsuite_train_internal crfsuite_train_internal_t;

struct tag_encoder;
typedef struct tag_encoder encoder_t;

typedef struct {
    crfsuite_data_t *data;
    int *perm;
    int num_instances;
} dataset_t;

void dataset_init_trainset(dataset_t *ds, crfsuite_data_t *data, int holdout);
void dataset_init_testset(dataset_t *ds, crfsuite_data_t *data, int holdout);
void dataset_finish(dataset_t *ds);
void dataset_shuffle(dataset_t *ds);
crfsuite_instance_t *dataset_get(dataset_t *ds, int i);

typedef void (*crfsuite_encoder_features_on_path_callback)(void *instance, int fid, floatval_t value);

/**
 * Internal data structure for 
 */
struct tag_crfsuite_train_internal {
    encoder_t *gm;      /** Interface to the graphical model. */
    crfsuite_params_t *params;       /**< Parameter interface. */
    logging_t* lg;              /**< Logging interface. */
    int feature_type;           /**< Feature type. */
    int algorithm;              /**< Training algorithm. */
};

/**
 * Interface for a graphical model.
 */
struct tag_encoder
{
    void *internal;

    const floatval_t *w;
    floatval_t scale;

    dataset_t *ds;
    const crfsuite_instance_t *inst;
    int level;

    int num_features;
    int cap_items;

    /**
     * Exchanges options.
     *  @param  self        The encoder instance.
     *  @param  params      The parameter interface.
     *  @param  mode        The direction of parameter exchange.
     *  @return             A status code.
     */
    int (*exchange_options)(encoder_t *self, crfsuite_params_t* params, int mode);

    /**
     * Initializes the encoder with a training data set.
     *  @param  self        The encoder instance.
     *  @param  ds          The data set for training.
     *  @param  lg          The logging interface.
     *  @return             A status code.
     */
    int (*initialize)(encoder_t *self, dataset_t *ds, logging_t *lg);

    /**
     * Compute the objective value and gradients for the whole data set.
     *  @param  self        The encoder instance.
     *  @param  ds          The data set.
     *  @param  w           The feature weights.
     *  @param  f           The pointer to a floatval_t variable to which the
     *                      objective value is stored by this function.
     *  @param  g           The pointer to the array that receives gradients.
     *  @return             A status code.
     */
    int (*objective_and_gradients_batch)(encoder_t *self, dataset_t *ds, const floatval_t *w, floatval_t *f, floatval_t *g);

    int (*features_on_path)(encoder_t *self, const crfsuite_instance_t *inst, const int *path, crfsuite_encoder_features_on_path_callback func, void *instance);

    /**
     * Sets the feature weights (and their scale factor).
     *  @param  self        The encoder instance.
     *  @param  w           The array of feature weights.
     *  @param  scale       The scale factor that should be applied to the
     *                      feature weights.
     *  @return             A status code.
     */
    int (*set_weights)(encoder_t *self, const floatval_t *w, floatval_t scale);

    /* Instance-wise operations. */
    int (*set_instance)(encoder_t *self, const crfsuite_instance_t *inst);

    /* Level 0. */

    /* Level 1 (feature weights). */
    int (*score)(encoder_t *self, const int *path, floatval_t *ptr_score);
    int (*viterbi)(encoder_t *self, int *path, floatval_t *ptr_score);

    /* Level 2 (forward-backward). */
    int (*partition_factor)(encoder_t *self, floatval_t *ptr_pf);

    /* Level 3 (marginals). */
    int (*objective_and_gradients)(encoder_t *self, floatval_t *f, floatval_t *g, floatval_t gain);

    int (*save_model)(encoder_t *self, const char *filename, const floatval_t *w, logging_t *lg);

};

/**
 * \defgroup crf1d_encode.c
 */
/** @{ */

encoder_t *crf1d_create_encoder();

/** @} */


void holdout_evaluation(
    encoder_t *gm,
    dataset_t *testset,
    const floatval_t *w,
    logging_t *lg
    );
    
int crfsuite_train_lbfgs(
    encoder_t *gm,
    dataset_t *trainset,
    dataset_t *testset,
    crfsuite_params_t *params,
    logging_t *lg,
    floatval_t **ptr_w
    );

void crfsuite_train_lbfgs_init(crfsuite_params_t* params);

void crfsuite_train_averaged_perceptron_init(crfsuite_params_t* params);

int crfsuite_train_averaged_perceptron(
    encoder_t *gm,
    dataset_t *trainset,
    dataset_t *testset,
    crfsuite_params_t *params,
    logging_t *lg,
    floatval_t **ptr_w
    );

void crfsuite_train_l2sgd_init(crfsuite_params_t* params);

int crfsuite_train_l2sgd(
    encoder_t *gm,
    dataset_t *trainset,
    dataset_t *testset,
    crfsuite_params_t *params,
    logging_t *lg,
    floatval_t **ptr_w
    );

void crfsuite_train_passive_aggressive_init(crfsuite_params_t* params);

int crfsuite_train_passive_aggressive(
    encoder_t *gm,
    dataset_t *trainset,
    dataset_t *testset,
    crfsuite_params_t *params,
    logging_t *lg,
    floatval_t **ptr_w
    );

void crfsuite_train_arow_init(crfsuite_params_t* params);

int crfsuite_train_arow(
    encoder_t *gm,
    dataset_t *trainset,
    dataset_t *testset,
    crfsuite_params_t *params,
    logging_t *lg,
    floatval_t **ptr_w
    );


#endif/*__CRFSUITE_INTERNAL_H__*/

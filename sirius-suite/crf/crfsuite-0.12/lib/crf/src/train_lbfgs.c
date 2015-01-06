/*
 *      Batch training with L-BFGS.
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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <time.h>

#include <crfsuite.h>
#include "crfsuite_internal.h"

#include "logging.h"
#include "params.h"
#include "vecmath.h"
#include <lbfgs.h>

/**
 * Training parameters (configurable with crfsuite_params_t interface).
 */
typedef struct {
    floatval_t  c1;
    floatval_t  c2;
    int         memory;
    floatval_t  epsilon;
    int         stop;
    floatval_t  delta;
    int         max_iterations;
    char*       linesearch;
    int         linesearch_max_iterations;
} training_option_t;

/**
 * Internal data structure for the callback function of lbfgs().
 */
typedef struct {
    encoder_t *gm;
    dataset_t *trainset;
    dataset_t *testset;
    logging_t *lg;
    floatval_t c2;
    floatval_t* best_w;
    clock_t begin;
} lbfgs_internal_t;

static lbfgsfloatval_t lbfgs_evaluate(
    void *instance,
    const lbfgsfloatval_t *x,
    lbfgsfloatval_t *g,
    const int n,
    const lbfgsfloatval_t step
    )
{
    int i;
    floatval_t f, norm = 0.;
    lbfgs_internal_t *lbfgsi = (lbfgs_internal_t*)instance;
    encoder_t *gm = lbfgsi->gm;
    dataset_t *trainset = lbfgsi->trainset;

    /* Compute the objective value and gradients. */
    gm->objective_and_gradients_batch(gm, trainset, x, &f, g);
    
    /* L2 regularization. */
    if (0 < lbfgsi->c2) {
        const floatval_t c22 = lbfgsi->c2 * 2.;
        for (i = 0;i < n;++i) {
            g[i] += (c22 * x[i]);
            norm += x[i] * x[i];
        }
        f += (lbfgsi->c2 * norm);
    }

    return f;
}

static int lbfgs_progress(
    void *instance,
    const lbfgsfloatval_t *x,
    const lbfgsfloatval_t *g,
    const lbfgsfloatval_t fx,
    const lbfgsfloatval_t xnorm,
    const lbfgsfloatval_t gnorm,
    const lbfgsfloatval_t step,
    int n,
    int k,
    int ls)
{
    int i, num_active_features = 0;
    clock_t duration, clk = clock();
    lbfgs_internal_t *lbfgsi = (lbfgs_internal_t*)instance;
    dataset_t *testset = lbfgsi->testset;
    encoder_t *gm = lbfgsi->gm;
    logging_t *lg = lbfgsi->lg;

    /* Compute the duration required for this iteration. */
    duration = clk - lbfgsi->begin;
    lbfgsi->begin = clk;

	/* Store the feature weight in case L-BFGS terminates with an error. */
    for (i = 0;i < n;++i) {
        lbfgsi->best_w[i] = x[i];
        if (x[i] != 0.) ++num_active_features;
    }

    /* Report the progress. */
    logging(lg, "***** Iteration #%d *****\n", k);
    logging(lg, "Loss: %f\n", fx);
    logging(lg, "Feature norm: %f\n", xnorm);
    logging(lg, "Error norm: %f\n", gnorm);
    logging(lg, "Active features: %d\n", num_active_features);
    logging(lg, "Line search trials: %d\n", ls);
    logging(lg, "Line search step: %f\n", step);
    logging(lg, "Seconds required for this iteration: %.3f\n", duration / (double)CLOCKS_PER_SEC);

    /* Send the tagger with the current parameters. */
    if (testset != NULL) {
        holdout_evaluation(gm, testset, x, lg);
    }

    logging(lg, "\n");

    /* Continue. */
    return 0;
}

static int exchange_options(crfsuite_params_t* params, training_option_t* opt, int mode)
{
    BEGIN_PARAM_MAP(params, mode)
        DDX_PARAM_FLOAT(
            "c1", opt->c1, 0,
            "Coefficient for L1 regularization."
            )
        DDX_PARAM_FLOAT(
            "c2", opt->c2, 1.0,
            "Coefficient for L2 regularization."
            )
        DDX_PARAM_INT(
            "max_iterations", opt->max_iterations, INT_MAX,
            "The maximum number of iterations for L-BFGS optimization."
            )
        DDX_PARAM_INT(
            "num_memories", opt->memory, 6,
            "The number of limited memories for approximating the inverse hessian matrix."
            )
        DDX_PARAM_FLOAT(
            "epsilon", opt->epsilon, 1e-5,
            "Epsilon for testing the convergence of the objective."
            )
        DDX_PARAM_INT(
            "period", opt->stop, 10,
            "The duration of iterations to test the stopping criterion."
            )
        DDX_PARAM_FLOAT(
            "delta", opt->delta, 1e-5,
            "The threshold for the stopping criterion; an L-BFGS iteration stops when the\n"
            "improvement of the log likelihood over the last ${period} iterations is no\n"
            "greater than this threshold."
            )
        DDX_PARAM_STRING(
            "linesearch", opt->linesearch, "MoreThuente",
            "The line search algorithm used in L-BFGS updates:\n"
            "{   'MoreThuente': More and Thuente's method,\n"
            "    'Backtracking': Backtracking method with regular Wolfe condition,\n"
            "    'StrongBacktracking': Backtracking method with strong Wolfe condition\n"
            "}\n"
            )
        DDX_PARAM_INT(
            "max_linesearch", opt->linesearch_max_iterations, 20,
            "The maximum number of trials for the line search algorithm."
            )
    END_PARAM_MAP()

    return 0;
}


void crfsuite_train_lbfgs_init(crfsuite_params_t* params)
{
    exchange_options(params, NULL, 0);
}

int crfsuite_train_lbfgs(
    encoder_t *gm,
    dataset_t *trainset,
    dataset_t *testset,
    crfsuite_params_t *params,
    logging_t *lg,
    floatval_t **ptr_w
    )
{
    int ret = 0, lbret;
    floatval_t *w = NULL;
    clock_t begin = clock();
    const int N = trainset->num_instances;
    const int L = trainset->data->labels->num(trainset->data->labels);
    const int A =  trainset->data->attrs->num(trainset->data->attrs);
    const int K = gm->num_features;
    lbfgs_internal_t lbfgsi;
    lbfgs_parameter_t lbfgsparam;
    training_option_t opt;

	/* Initialize the variables. */
	memset(&lbfgsi, 0, sizeof(lbfgsi));
	memset(&opt, 0, sizeof(opt));
    lbfgs_parameter_init(&lbfgsparam);

    /* Allocate an array that stores the current weights. */ 
    w = (floatval_t*)calloc(sizeof(floatval_t), K);
    if (w == NULL) {
		ret = CRFSUITEERR_OUTOFMEMORY;
		goto error_exit;
    }
 
    /* Allocate an array that stores the best weights. */ 
    lbfgsi.best_w = (floatval_t*)calloc(sizeof(floatval_t), K);
    if (lbfgsi.best_w == NULL) {
		ret = CRFSUITEERR_OUTOFMEMORY;
		goto error_exit;
    }

    /* Read the L-BFGS parameters. */
    exchange_options(params, &opt, -1);
    logging(lg, "L-BFGS optimization\n");
    logging(lg, "c1: %f\n", opt.c1);
    logging(lg, "c2: %f\n", opt.c2);
    logging(lg, "num_memories: %d\n", opt.memory);
    logging(lg, "max_iterations: %d\n", opt.max_iterations);
    logging(lg, "epsilon: %f\n", opt.epsilon);
    logging(lg, "stop: %d\n", opt.stop);
    logging(lg, "delta: %f\n", opt.delta);
    logging(lg, "linesearch: %s\n", opt.linesearch);
    logging(lg, "linesearch.max_iterations: %d\n", opt.linesearch_max_iterations);
    logging(lg, "\n");

    /* Set parameters for L-BFGS. */
    lbfgsparam.m = opt.memory;
    lbfgsparam.epsilon = opt.epsilon;
    lbfgsparam.past = opt.stop;
    lbfgsparam.delta = opt.delta;
    lbfgsparam.max_iterations = opt.max_iterations;
    if (strcmp(opt.linesearch, "Backtracking") == 0) {
        lbfgsparam.linesearch = LBFGS_LINESEARCH_BACKTRACKING;
    } else if (strcmp(opt.linesearch, "StrongBacktracking") == 0) {
        lbfgsparam.linesearch = LBFGS_LINESEARCH_BACKTRACKING_STRONG_WOLFE;
    } else {
        lbfgsparam.linesearch = LBFGS_LINESEARCH_MORETHUENTE;
    }
    lbfgsparam.max_linesearch = opt.linesearch_max_iterations;

    /* Set regularization parameters. */
    if (0 < opt.c1) {
        lbfgsparam.orthantwise_c = opt.c1;
        lbfgsparam.linesearch = LBFGS_LINESEARCH_BACKTRACKING;
    } else {
        lbfgsparam.orthantwise_c = 0;
    }

    /* Set other callback data. */
    lbfgsi.gm = gm;
    lbfgsi.trainset = trainset;
    lbfgsi.testset = testset;
    lbfgsi.c2 = opt.c2;
    lbfgsi.lg = lg;

    /* Call the L-BFGS solver. */
    lbfgsi.begin = clock();
    lbret = lbfgs(
        K,
        w,
        NULL,
        lbfgs_evaluate,
        lbfgs_progress,
        &lbfgsi,
        &lbfgsparam
        );
    if (lbret == LBFGS_CONVERGENCE) {
        logging(lg, "L-BFGS resulted in convergence\n");
    } else if (lbret == LBFGS_STOP) {
        logging(lg, "L-BFGS terminated with the stopping criteria\n");
    } else if (lbret == LBFGSERR_MAXIMUMITERATION) {
        logging(lg, "L-BFGS terminated with the maximum number of iterations\n");
    } else {
        logging(lg, "L-BFGS terminated with error code (%d)\n", lbret);
    }

	/* Restore the feature weights of the last call of lbfgs_progress(). */
	veccopy(w, lbfgsi.best_w, K);

	/* Report the run-time for the training. */
    logging(lg, "Total seconds required for training: %.3f\n", (clock() - begin) / (double)CLOCKS_PER_SEC);
    logging(lg, "\n");

	/* Exit with success. */
	free(lbfgsi.best_w);
    *ptr_w = w;
    return 0;

error_exit:
	free(lbfgsi.best_w);
	free(w);
	*ptr_w = NULL;
	return ret;
}

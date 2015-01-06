/*
 *      Online training with averaged perceptron.
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
#include <time.h>

#include <crfsuite.h>
#include "crfsuite_internal.h"
#include "logging.h"
#include "params.h"
#include "vecmath.h"

/**
 * Training parameters (configurable with crfsuite_params_t interface).
 */
typedef struct {
    int max_iterations;
    floatval_t epsilon;
} training_option_t;

/**
 * Internal data structure for updating (averaging) feature weights.
 */
typedef struct {
    floatval_t *w;
    floatval_t *ws;
    floatval_t c;
    floatval_t cs;
} update_data;

static void update_weights(void *instance, int fid, floatval_t value)
{
    update_data *ud = (update_data*)instance;
    ud->w[fid] += ud->c * value;
    ud->ws[fid] += ud->cs * value;
}

static int diff(int *x, int *y, int n)
{
    int i, d = 0;
    for (i = 0;i < n;++i) {
        if (x[i] != y[i]) {
            ++d;
        }
    }
    return d;
}

static int exchange_options(crfsuite_params_t* params, training_option_t* opt, int mode)
{
    BEGIN_PARAM_MAP(params, mode)
        DDX_PARAM_INT(
            "max_iterations", opt->max_iterations, 100,
            "The maximum number of iterations."
            )
        DDX_PARAM_FLOAT(
            "epsilon", opt->epsilon, 0.,
            "The stopping criterion (the ratio of incorrect label predictions)."
            )
    END_PARAM_MAP()

    return 0;
}

void crfsuite_train_averaged_perceptron_init(crfsuite_params_t* params)
{
    exchange_options(params, NULL, 0);
}

int crfsuite_train_averaged_perceptron(
    encoder_t *gm,
    dataset_t *trainset,
    dataset_t *testset,
    crfsuite_params_t *params,
    logging_t *lg,
    floatval_t **ptr_w
    )
{
    int n, i, c, ret = 0;
    int *viterbi = NULL;
    floatval_t *w = NULL;
    floatval_t *ws = NULL;
    floatval_t *wa = NULL;
    const int N = trainset->num_instances;
    const int K = gm->num_features;
    const int T = gm->cap_items;
    training_option_t opt;
    update_data ud;
    clock_t begin = clock();

	/* Initialize the variable. */
	memset(&ud, 0, sizeof(ud));

    /* Obtain parameter values. */
    exchange_options(params, &opt, -1);

    /* Allocate arrays. */
    w = (floatval_t*)calloc(sizeof(floatval_t), K);
    ws = (floatval_t*)calloc(sizeof(floatval_t), K);
    wa = (floatval_t*)calloc(sizeof(floatval_t), K);
    viterbi = (int*)calloc(sizeof(int), T);
    if (w == NULL || ws == NULL || wa == NULL || viterbi == NULL) {
        ret = CRFSUITEERR_OUTOFMEMORY;
        goto error_exit;
    }

    /* Show the parameters. */
    logging(lg, "Averaged perceptron\n");
    logging(lg, "max_iterations: %d\n", opt.max_iterations);
    logging(lg, "epsilon: %f\n", opt.epsilon);
    logging(lg, "\n");

    c = 1;
    ud.w = w;
    ud.ws = ws;

	/* Loop for epoch. */
    for (i = 0;i < opt.max_iterations;++i) {
        floatval_t norm = 0., loss = 0.;
        clock_t iteration_begin = clock();

        /* Shuffle the instances. */
        dataset_shuffle(trainset);

		/* Loop for each instance. */
        for (n = 0;n < N;++n) {
            int d = 0;
            floatval_t score;
            const crfsuite_instance_t *inst = dataset_get(trainset, n);

            /* Set the feature weights to the encoder. */
            gm->set_weights(gm, w, 1.);
            gm->set_instance(gm, inst);

            /* Tag the sequence with the current model. */
            gm->viterbi(gm, viterbi, &score);

            /* Compute the number of different labels. */
            d = diff(inst->labels, viterbi, inst->num_items);
            if (0 < d) {
                /*
                    For every feature k on the correct path:
                        w[k] += 1; ws[k] += c;
                 */
                ud.c = 1;
                ud.cs = c;
                gm->features_on_path(gm, inst, inst->labels, update_weights, &ud);

                /*
                    For every feature k on the Viterbi path:
                        w[k] -= 1; ws[k] -= c;
                 */
                ud.c = -1;
                ud.cs = -c;
                gm->features_on_path(gm, inst, viterbi, update_weights, &ud);

                /* We define the loss as the ratio of wrongly predicted labels. */
                loss += d / (floatval_t)inst->num_items;
            }

            ++c;
        }

        /* Perform averaging to wa. */
        veccopy(wa, w, K);
        vecasub(wa, 1./c, ws, K);

        /* Output the progress. */
        logging(lg, "***** Iteration #%d *****\n", i+1);
        logging(lg, "Loss: %f\n", loss);
        logging(lg, "Feature norm: %f\n", sqrt(vecdot(wa, wa, K)));
        logging(lg, "Seconds required for this iteration: %.3f\n", (clock() - iteration_begin) / (double)CLOCKS_PER_SEC);

        /* Holdout evaluation if necessary. */
        if (testset != NULL) {
            holdout_evaluation(gm, testset, wa, lg);
        }

        logging(lg, "\n");

        /* Convergence test. */
        if (loss / N < opt.epsilon) {
            logging(lg, "Terminated with the stopping criterion\n");
            logging(lg, "\n");
            break;
        }
    }

    logging(lg, "Total seconds required for training: %.3f\n", (clock() - begin) / (double)CLOCKS_PER_SEC);
    logging(lg, "\n");

    free(viterbi);
    free(ws);
    free(w);
    *ptr_w = wa;
    return ret;

error_exit:
    free(viterbi);
    free(wa);
    free(ws);
    free(w);
    *ptr_w = NULL;

    return ret;
}

/*
 *      Online training with L2-regularized Stochastic Gradient Descent (SGD).
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

/*
    SGD for L2-regularized MAP estimation.

    The iterative algorithm is inspired by Pegasos:

    Shai Shalev-Shwartz, Yoram Singer, and Nathan Srebro.
    Pegasos: Primal Estimated sub-GrAdient SOlver for SVM.
    In Proc. of ICML 2007, pp 807-814, 2007.

    The calibration strategy is inspired by the implementation of sgd:
    http://leon.bottou.org/projects/sgd
    written by Léon Bottou.

    The objective function to minimize is:
        
        f(w) = (lambda/2) * ||w||^2 + (1/N) * \sum_i^N log P^i(y|x)
        lambda = 2 * C / N

    The original version of the Pegasos algorithm.

    0) Initialization
        t = t0
        k = [the batch size]
    1) Computing the learning rate (eta).
        eta = 1 / (lambda * t)
    2) Updating feature weights.
        w = (1 - eta * lambda) w - (eta / k) \sum_i (oexp - mexp)
    3) Projecting feature weights within an L2-ball.
        w = min{1, (1/sqrt(lambda))/||w||} * w
    4) Goto 1 until convergence.

    This implementation omit the step 3) because it makes the source code
    tricky (in order to maintain L2-norm of feature weights at any time) and
    because the project step does not have a strong impact to the quality of
    solution.

    A naive implementation requires O(K) computations for steps 2,
    where K is the total number of features. This code implements the procedure
    in an efficient way:

    0) Initialization
        decay = 1
    1) Computing various factors
        eta = 1 / (lambda * t)
        decay *= (1 - eta * lambda)
        gain = (eta / k) / decay
    2) Updating feature weights
        Updating feature weights from observation expectation:
            delta = gain * (1.0) * f(x,y)
            w += delta
        Updating feature weights from model expectation:
            delta = gain * (-P(y|x)) * f(x,y)
            w += delta
    4) Goto 1 until convergence.
*/


#ifdef    HAVE_CONFIG_H
#include <config.h>
#endif/*HAVE_CONFIG_H*/

#include <os.h>

#include <float.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <math.h>

#include <crfsuite.h>
#include "crfsuite_internal.h"

#include "logging.h"
#include "params.h"
#include "crf1d.h"
#include "vecmath.h"

#define MIN(a, b)   ((a) < (b) ? (a) : (b))

typedef struct {
    floatval_t  c2;
    floatval_t  lambda;
    floatval_t  t0;
    int         max_iterations;
    int         period;
    floatval_t  delta;
    floatval_t  calibration_eta;
    floatval_t  calibration_rate;
    int         calibration_samples;
    int         calibration_candidates;
    int         calibration_max_trials;
} training_option_t;

static int l2sgd(
    encoder_t *gm,
    dataset_t *trainset,
    dataset_t *testset,
    floatval_t *w,
    logging_t *lg,
    const int N,
    const floatval_t t0,
    const floatval_t lambda,
    const int num_epochs,
    int calibration,
    int period,
    const floatval_t epsilon,
    floatval_t *ptr_loss
    )
{
    int i, epoch, ret = 0;
    floatval_t t = 0;
    floatval_t loss = 0, sum_loss = 0;
    floatval_t best_sum_loss = DBL_MAX;
    floatval_t eta, gain, decay = 1.;
    floatval_t improvement = 0.;
    floatval_t norm2 = 0.;
    floatval_t *pf = NULL;
    floatval_t *best_w = NULL;
    clock_t clk_prev, clk_begin = clock();
    const int K = gm->num_features;

    if (!calibration) {
        pf = (floatval_t*)malloc(sizeof(floatval_t) * period);
        best_w = (floatval_t*)calloc(K, sizeof(floatval_t));
        if (pf == NULL || best_w == NULL) {
            ret = CRFSUITEERR_OUTOFMEMORY;
            goto error_exit;
        }
    }

    /* Initialize the feature weights. */
    vecset(w, 0, K);

    /* Loop for epochs. */
    for (epoch = 1;epoch <= num_epochs;++epoch) {
        clk_prev = clock();

        if (!calibration) {
            logging(lg, "***** Epoch #%d *****\n", epoch);
            /* Shuffle the training instances. */
            dataset_shuffle(trainset);
        }

        /* Loop for instances. */
        sum_loss = 0.;
        for (i = 0;i < N;++i) {
            const crfsuite_instance_t *inst = dataset_get(trainset, i);

            /* Update various factors. */
            eta = 1 / (lambda * (t0 + t));
            decay *= (1.0 - eta * lambda);
            gain = eta / decay;

            /* Compute the loss and gradients for the instance. */
            gm->set_weights(gm, w, decay);
            gm->set_instance(gm, inst);
            gm->objective_and_gradients(gm, &loss, w, gain);

            sum_loss += loss;
            ++t;
        }

        /* Terminate when the loss is abnormal (NaN, -Inf, +Inf). */
        if (!isfinite(loss)) {
            logging(lg, "ERROR: overflow loss\n");
            ret = CRFSUITEERR_OVERFLOW;
            sum_loss = loss;
            goto error_exit;
        }

        /* Scale the feature weights. */
        vecscale(w, decay, K);
        decay = 1.;

        /* Include the L2 norm of feature weights to the objective. */
        /* The factor N is necessary because lambda = 2 * C / N. */
        norm2 = vecdot(w, w, K);
        sum_loss += 0.5 * lambda * norm2 * N;

        /* One epoch finished. */
        if (!calibration) {
            /* Check if the current epoch is the best. */
            if (sum_loss < best_sum_loss) {
                /* Store the feature weights to best_w. */
                best_sum_loss = sum_loss;
                veccopy(best_w, w, K);
            }

            /* We don't test the stopping criterion while period < epoch. */
            if (period < epoch) {
                improvement = (pf[(epoch-1) % period] - sum_loss) / sum_loss;
            } else {
                improvement = epsilon;
            }

            /* Store the current value of the objective function. */
            pf[(epoch-1) % period] = sum_loss;

            logging(lg, "Loss: %f\n", sum_loss);
            if (period < epoch) {
                logging(lg, "Improvement ratio: %f\n", improvement);
            }
            logging(lg, "Feature L2-norm: %f\n", sqrt(norm2));
            logging(lg, "Learning rate (eta): %f\n", eta);
            logging(lg, "Total number of feature updates: %.0f\n", t);
            logging(lg, "Seconds required for this iteration: %.3f\n", (clock() - clk_prev) / (double)CLOCKS_PER_SEC);

            /* Holdout evaluation if necessary. */
            if (testset != NULL) {
                holdout_evaluation(gm, testset, w, lg);
            }
            logging(lg, "\n");

            /* Check for the stopping criterion. */
            if (improvement < epsilon) {
                ret = 0;
                break;
            }
        }
    }

    /* Output the optimization result. */
    if (!calibration) {
        if (ret == 0) {
            if (epoch < num_epochs) {
                logging(lg, "SGD terminated with the stopping criteria\n");
            } else {
                logging(lg, "SGD terminated with the maximum number of iterations\n");
            }
        } else {
            logging(lg, "SGD terminated with error code (%d)\n", ret);
        }
    }

    /* Restore the best weights. */
    if (best_w != NULL) {
        sum_loss = best_sum_loss;
        veccopy(w, best_w, K);
    }

error_exit:
    free(best_w);
    free(pf);
    if (ptr_loss != NULL) {
        *ptr_loss = sum_loss;
    }
    return ret;
}

static floatval_t
l2sgd_calibration(
    encoder_t *gm,
    dataset_t *ds,
    floatval_t *w,
    logging_t *lg,
    const training_option_t* opt
    )
{
    int i, s;
    int dec = 0, ok, trials = 1;
    int num = opt->calibration_candidates;
    clock_t clk_begin = clock();
    floatval_t loss = 0.;
    floatval_t init_loss = 0.;
    floatval_t best_loss = DBL_MAX;
    floatval_t eta = opt->calibration_eta;
    floatval_t best_eta = opt->calibration_eta;
    const int N = ds->num_instances;
    const int S = MIN(N, opt->calibration_samples);
    const int K = gm->num_features;
    const floatval_t init_eta = opt->calibration_eta;
    const floatval_t rate = opt->calibration_rate;
    const floatval_t lambda = opt->lambda;

    logging(lg, "Calibrating the learning rate (eta)\n");
    logging(lg, "calibration.eta: %f\n", eta);
    logging(lg, "calibration.rate: %f\n", rate);
    logging(lg, "calibration.samples: %d\n", S);
    logging(lg, "calibration.candidates: %d\n", num);
    logging(lg, "calibration.max_trials: %d\n", opt->calibration_max_trials);

    /* Initialize a permutation that shuffles the instances. */
    dataset_shuffle(ds);

    /* Initialize feature weights as zero. */
    vecset(w, 0, K);

    /* Compute the initial loss. */
    gm->set_weights(gm, w, 1.);
    init_loss = 0;
    for (i = 0;i < S;++i) {
        floatval_t score;
        const crfsuite_instance_t *inst = dataset_get(ds, i);
        gm->set_instance(gm, inst);
        gm->score(gm, inst->labels, &score);
        init_loss -= score;
        gm->partition_factor(gm, &score);
        init_loss += score;
    }
    init_loss += 0.5 * lambda * vecdot(w, w, K) * N;
    logging(lg, "Initial loss: %f\n", init_loss);

    while (num > 0 || !dec) {
        logging(lg, "Trial #%d (eta = %f): ", trials, eta);

        /* Perform SGD for one epoch. */
        l2sgd(
            gm,
            ds,
            NULL,
            w,
            lg,
            S, 1.0 / (lambda * eta), lambda, 1, 1, 1, 0., &loss);

        /* Make sure that the learning rate decreases the log-likelihood. */
        ok = isfinite(loss) && (loss < init_loss);
        if (ok) {
            logging(lg, "%f\n", loss);
            --num;
        } else {
            logging(lg, "%f (worse)\n", loss);
        }

        if (isfinite(loss) && loss < best_loss) {
            best_loss = loss;
            best_eta = eta;
        }

        if (!dec) {
            if (ok && 0 < num) {
                eta *= rate;
            } else {
                dec = 1;
                num = opt->calibration_candidates;
                eta = init_eta / rate;
            }
        } else {
            eta /= rate;
        }

        ++trials;
        if (opt->calibration_max_trials <= trials) {
            break;
        }
    }

    eta = best_eta;
    logging(lg, "Best learning rate (eta): %f\n", eta);
    logging(lg, "Seconds required: %.3f\n", (clock() - clk_begin) / (double)CLOCKS_PER_SEC);
    logging(lg, "\n");

    return 1.0 / (lambda * eta);
}

int exchange_options(crfsuite_params_t* params, training_option_t* opt, int mode)
{
    BEGIN_PARAM_MAP(params, mode)
        DDX_PARAM_FLOAT(
            "c2", opt->c2, 1.,
            "Coefficient for L2 regularization."
            )
        DDX_PARAM_INT(
            "max_iterations", opt->max_iterations, 1000,
            "The maximum number of iterations (epochs) for SGD optimization."
            )
        DDX_PARAM_INT(
            "period", opt->period, 10,
            "The duration of iterations to test the stopping criterion."
            )
        DDX_PARAM_FLOAT(
            "delta", opt->delta, 1e-6,
            "The threshold for the stopping criterion; an optimization process stops when\n"
            "the improvement of the log likelihood over the last ${period} iterations is no\n"
            "greater than this threshold."
            )
        DDX_PARAM_FLOAT(
            "calibration.eta", opt->calibration_eta, 0.1,
            "The initial value of learning rate (eta) used for calibration."
            )
        DDX_PARAM_FLOAT(
            "calibration.rate", opt->calibration_rate, 2.,
            "The rate of increase/decrease of learning rate for calibration."
            )
        DDX_PARAM_INT(
            "calibration.samples", opt->calibration_samples, 1000,
            "The number of instances used for calibration."
            )
        DDX_PARAM_INT(
            "calibration.candidates", opt->calibration_candidates, 10,
            "The number of candidates of learning rate."
            )
        DDX_PARAM_INT(
            "calibration.max_trials", opt->calibration_max_trials, 20,
            "The maximum number of trials of learning rates for calibration."
            )
    END_PARAM_MAP()

    return 0;
}

void crfsuite_train_l2sgd_init(crfsuite_params_t* params)
{
    exchange_options(params, NULL, 0);
}

int crfsuite_train_l2sgd(
    encoder_t *gm,
    dataset_t *trainset,
    dataset_t *testset,
    crfsuite_params_t *params,
    logging_t *lg,
    floatval_t **ptr_w
    )
{
    int ret = 0;
    floatval_t *w = NULL;
    clock_t clk_begin;
    floatval_t loss = 0;
    const int N = trainset->num_instances;
    const int K = gm->num_features;
    const int T = gm->cap_items;
    training_option_t opt;

    /* Obtain parameter values. */
    exchange_options(params, &opt, -1);

    /* Allocate arrays. */
    w = (floatval_t*)calloc(sizeof(floatval_t), K);
    if (w == NULL) {
        ret = CRFSUITEERR_OUTOFMEMORY;
        goto error_exit;
    }

    opt.lambda = 2. * opt.c2 / N;

    logging(lg, "Stochastic Gradient Descent (SGD)\n");
    logging(lg, "c2: %f\n", opt.c2);
    logging(lg, "max_iterations: %d\n", opt.max_iterations);
    logging(lg, "period: %d\n", opt.period);
    logging(lg, "delta: %f\n", opt.delta);
    logging(lg, "\n");
    clk_begin = clock();

    /* Calibrate the training rate (eta). */
    opt.t0 = l2sgd_calibration(gm, trainset, w, lg, &opt);

    /* Perform stochastic gradient descent. */
    ret = l2sgd(
        gm,
        trainset,
        testset,
        w,
        lg,
        N,
        opt.t0,
        opt.lambda,
        opt.max_iterations,
        0,
        opt.period,
        opt.delta,
        &loss
        );

    logging(lg, "Loss: %f\n", loss);
    logging(lg, "Total seconds required for training: %.3f\n", (clock() - clk_begin) / (double)CLOCKS_PER_SEC);
    logging(lg, "\n");

    *ptr_w = w;
    return ret;

error_exit:
    free(w);
    return ret;
}

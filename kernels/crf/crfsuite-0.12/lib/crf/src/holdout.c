/*
 *      Holdout evaluation.
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

#include <stdlib.h>
#include <crfsuite.h>
#include "crfsuite_internal.h"
#include "logging.h"

void holdout_evaluation(
    encoder_t *gm,
    dataset_t *ds,
    const floatval_t *w,
    logging_t *lg
    )
{
    int i;
    crfsuite_evaluation_t eval;
    const int N = ds->num_instances;
    int *viterbi = NULL;
    int max_length = 0;

    /* Initialize the evaluation table. */
    crfsuite_evaluation_init(&eval, ds->data->labels->num(ds->data->labels));

    gm->set_weights(gm, w, 1.);

    for (i = 0;i < N;++i) {
        floatval_t score;
        const crfsuite_instance_t *inst = dataset_get(ds, i);

        if (max_length < inst->num_items) {
            free(viterbi);
            viterbi = (int*)malloc(sizeof(int) * inst->num_items);
        }

        gm->set_instance(gm, inst);
        gm->viterbi(gm, viterbi, &score);

        crfsuite_evaluation_accmulate(&eval, inst->labels, viterbi, inst->num_items);
    }

    /* Report the performance. */
    crfsuite_evaluation_finalize(&eval);
    crfsuite_evaluation_output(&eval, ds->data->labels, lg->func, lg->instance);
}

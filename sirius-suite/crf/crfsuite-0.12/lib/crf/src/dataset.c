/*
 *      Implementation for data sets (dataset_t).
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

void dataset_init_trainset(dataset_t *ds, crfsuite_data_t *data, int holdout)
{
    int i, n = 0;

    for (i = 0;i < data->num_instances;++i) {
        if (data->instances[i].group != holdout) {
            ++n;
        }
    }

    ds->data = data;
    ds->num_instances = n;
    ds->perm = (int*)malloc(sizeof(int) * n);

    n = 0;
    for (i = 0;i < data->num_instances;++i) {
        if (data->instances[i].group != holdout) {
            ds->perm[n++] = i;
        }
    }    
}

void dataset_init_testset(dataset_t *ds, crfsuite_data_t *data, int holdout)
{
    int i, n = 0;

    for (i = 0;i < data->num_instances;++i) {
        if (data->instances[i].group == holdout) {
            ++n;
        }
    }

    ds->data = data;
    ds->num_instances = n;
    ds->perm = (int*)malloc(sizeof(int) * n);

    n = 0;
    for (i = 0;i < data->num_instances;++i) {
        if (data->instances[i].group == holdout) {
            ds->perm[n++] = i;
        }
    }
}

void dataset_finish(dataset_t *ds)
{
    free(ds->perm);
}

void dataset_shuffle(dataset_t *ds)
{
    int i;
    for (i = 0;i < ds->num_instances;++i) {
        int j = rand() % ds->num_instances;
        int tmp = ds->perm[j];
        ds->perm[j] = ds->perm[i];
        ds->perm[i] = tmp;
    }
}

crfsuite_instance_t *dataset_get(dataset_t *ds, int i)
{
    return &ds->data->instances[ds->perm[i]];
}

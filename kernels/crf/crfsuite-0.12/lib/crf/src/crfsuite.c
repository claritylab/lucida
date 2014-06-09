/*
 *      CRFsuite library.
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

#include <os.h>

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <crfsuite.h>
#include "logging.h"

int crf1de_create_instance(const char *iid, void **ptr);
int crfsuite_dictionary_create_instance(const char *interface, void **ptr);
int crf1m_create_instance_from_file(const char *filename, void **ptr);

int crfsuite_create_instance(const char *iid, void **ptr)
{
    int ret = 
        crf1de_create_instance(iid, ptr) == 0 ||
        crfsuite_dictionary_create_instance(iid, ptr) == 0;

    return ret;
}

int crfsuite_create_instance_from_file(const char *filename, void **ptr)
{
    int ret = crf1m_create_instance_from_file(filename, ptr);
    return ret;
}



void crfsuite_attribute_init(crfsuite_attribute_t* cont)
{
    memset(cont, 0, sizeof(*cont));
    cont->value = 1;
}

void crfsuite_attribute_set(crfsuite_attribute_t* cont, int aid, floatval_t value)
{
    crfsuite_attribute_init(cont);
    cont->aid = aid;
    cont->value = value;
}

void crfsuite_attribute_copy(crfsuite_attribute_t* dst, const crfsuite_attribute_t* src)
{
    dst->aid = src->aid;
    dst->value = src->value;
}

void crfsuite_attribute_swap(crfsuite_attribute_t* x, crfsuite_attribute_t* y)
{
    crfsuite_attribute_t tmp = *x;
    x->aid = y->aid;
    x->value = y->value;
    y->aid = tmp.aid;
    y->value = tmp.value;
}



void crfsuite_item_init(crfsuite_item_t* item)
{
    memset(item, 0, sizeof(*item));
}

void crfsuite_item_init_n(crfsuite_item_t* item, int num_contents)
{
    crfsuite_item_init(item);
    item->num_contents = num_contents;
    item->cap_contents = num_contents;
    item->contents = (crfsuite_attribute_t*)calloc(num_contents, sizeof(crfsuite_attribute_t));
}

void crfsuite_item_finish(crfsuite_item_t* item)
{
    free(item->contents);
    crfsuite_item_init(item);
}

void crfsuite_item_copy(crfsuite_item_t* dst, const crfsuite_item_t* src)
{
    int i;

    dst->num_contents = src->num_contents;
    dst->cap_contents = src->cap_contents;
    dst->contents = (crfsuite_attribute_t*)calloc(dst->num_contents, sizeof(crfsuite_attribute_t));
    for (i = 0;i < dst->num_contents;++i) {
        crfsuite_attribute_copy(&dst->contents[i], &src->contents[i]);
    }
}

void crfsuite_item_swap(crfsuite_item_t* x, crfsuite_item_t* y)
{
    crfsuite_item_t tmp = *x;
    x->num_contents = y->num_contents;
    x->cap_contents = y->cap_contents;
    x->contents = y->contents;
    y->num_contents = tmp.num_contents;
    y->cap_contents = tmp.cap_contents;
    y->contents = tmp.contents;
}

int crfsuite_item_append_attribute(crfsuite_item_t* item, const crfsuite_attribute_t* cont)
{
    if (item->cap_contents <= item->num_contents) {
        item->cap_contents = (item->cap_contents + 1) * 2;
        item->contents = (crfsuite_attribute_t*)realloc(
            item->contents, sizeof(crfsuite_attribute_t) * item->cap_contents);
    }
    crfsuite_attribute_copy(&item->contents[item->num_contents++], cont);
    return 0;
}

int  crfsuite_item_empty(crfsuite_item_t* item)
{
    return (item->num_contents == 0);
}




void crfsuite_instance_init(crfsuite_instance_t* inst)
{
    memset(inst, 0, sizeof(*inst));
}

void crfsuite_instance_init_n(crfsuite_instance_t* inst, int num_items)
{
    crfsuite_instance_init(inst);
    inst->num_items = num_items;
    inst->cap_items = num_items;
    inst->items = (crfsuite_item_t*)calloc(num_items, sizeof(crfsuite_item_t));
    inst->labels = (int*)calloc(num_items, sizeof(int));
}

void crfsuite_instance_finish(crfsuite_instance_t* inst)
{
    int i;

    for (i = 0;i < inst->num_items;++i) {
        crfsuite_item_finish(&inst->items[i]);
    }
    free(inst->labels);
    free(inst->items);
    crfsuite_instance_init(inst);
}

void crfsuite_instance_copy(crfsuite_instance_t* dst, const crfsuite_instance_t* src)
{
    int i;

    dst->num_items = src->num_items;
    dst->cap_items = src->cap_items;
    dst->items = (crfsuite_item_t*)calloc(dst->num_items, sizeof(crfsuite_item_t));
    dst->labels = (int*)calloc(dst->num_items, sizeof(int));
    dst->group = src->group;
    for (i = 0;i < dst->num_items;++i) {
        crfsuite_item_copy(&dst->items[i], &src->items[i]);
        dst->labels[i] = src->labels[i];
    }
}

void crfsuite_instance_swap(crfsuite_instance_t* x, crfsuite_instance_t* y)
{
    crfsuite_instance_t tmp = *x;
    x->num_items = y->num_items;
    x->cap_items = y->cap_items;
    x->items = y->items;
    x->labels = y->labels;
    x->group = y->group;
    y->num_items = tmp.num_items;
    y->cap_items = tmp.cap_items;
    y->items = tmp.items;
    y->labels = tmp.labels;
    y->group = tmp.group;
}

int crfsuite_instance_append(crfsuite_instance_t* inst, const crfsuite_item_t* item, int label)
{
    if (inst->cap_items <= inst->num_items) {
        inst->cap_items = (inst->cap_items + 1) * 2;
        inst->items = (crfsuite_item_t*)realloc(inst->items, sizeof(crfsuite_item_t) * inst->cap_items);
        inst->labels = (int*)realloc(inst->labels, sizeof(int) * inst->cap_items);
    }
    crfsuite_item_copy(&inst->items[inst->num_items], item);
    inst->labels[inst->num_items] = label;
    ++inst->num_items;
    return 0;
}

int  crfsuite_instance_empty(crfsuite_instance_t* inst)
{
    return (inst->num_items == 0);
}




void crfsuite_data_init(crfsuite_data_t* data)
{
    memset(data, 0, sizeof(*data));
}

void crfsuite_data_init_n(crfsuite_data_t* data, int n)
{
    crfsuite_data_init(data);
    data->num_instances = n;
    data->cap_instances = n;
    data->instances = (crfsuite_instance_t*)calloc(n, sizeof(crfsuite_instance_t));
}

void crfsuite_data_finish(crfsuite_data_t* data)
{
    int i;

    for (i = 0;i < data->num_instances;++i) {
        crfsuite_instance_finish(&data->instances[i]);
    }
    free(data->instances);
    crfsuite_data_init(data);
}

void crfsuite_data_copy(crfsuite_data_t* dst, const crfsuite_data_t* src)
{
    int i;

    dst->num_instances = src->num_instances;
    dst->cap_instances = src->cap_instances;
    dst->instances = (crfsuite_instance_t*)calloc(dst->num_instances, sizeof(crfsuite_instance_t));
    for (i = 0;i < dst->num_instances;++i) {
        crfsuite_instance_copy(&dst->instances[i], &src->instances[i]);
    }
}

void crfsuite_data_swap(crfsuite_data_t* x, crfsuite_data_t* y)
{
    crfsuite_data_t tmp = *x;
    x->num_instances = y->num_instances;
    x->cap_instances = y->cap_instances;
    x->instances = y->instances;
    y->num_instances = tmp.num_instances;
    y->cap_instances = tmp.cap_instances;
    y->instances = tmp.instances;
}

int  crfsuite_data_append(crfsuite_data_t* data, const crfsuite_instance_t* inst)
{
    if (0 < inst->num_items) {
        if (data->cap_instances <= data->num_instances) {
            data->cap_instances = (data->cap_instances + 1) * 2;
            data->instances = (crfsuite_instance_t*)realloc(
                data->instances, sizeof(crfsuite_instance_t) * data->cap_instances);
        }
        crfsuite_instance_copy(&data->instances[data->num_instances++], inst);
    }
    return 0;
}

int crfsuite_data_maxlength(crfsuite_data_t* data)
{
    int i, T = 0;
    for (i = 0;i < data->num_instances;++i) {
        if (T < data->instances[i].num_items) {
            T = data->instances[i].num_items;
        }
    }
    return T;
}

int  crfsuite_data_totalitems(crfsuite_data_t* data)
{
    int i, n = 0;
    for (i = 0;i < data->num_instances;++i) {
        n += data->instances[i].num_items;
    }
    return n;
}

static char *safe_strncpy(char *dst, const char *src, size_t n)
{
    strncpy(dst, src, n-1);
    dst[n-1] = 0;
    return dst;
}

void crfsuite_evaluation_init(crfsuite_evaluation_t* eval, int n)
{
    memset(eval, 0, sizeof(*eval));
    eval->tbl = (crfsuite_label_evaluation_t*)calloc(n+1, sizeof(crfsuite_label_evaluation_t));
    if (eval->tbl != NULL) {
        eval->num_labels = n;
    }
}

void crfsuite_evaluation_clear(crfsuite_evaluation_t* eval)
{
    int i;
    for (i = 0;i <= eval->num_labels;++i) {
        memset(&eval->tbl[i], 0, sizeof(eval->tbl[i]));
    }

    eval->item_total_correct = 0;
    eval->item_total_num = 0;
    eval->item_total_model = 0;
    eval->item_total_observation = 0;
    eval->item_accuracy = 0;

    eval->inst_total_correct = 0;
    eval->inst_total_num = 0;
    eval->inst_accuracy = 0;

    eval->macro_precision = 0;
    eval->macro_recall = 0;
    eval->macro_fmeasure = 0;
}

void crfsuite_evaluation_finish(crfsuite_evaluation_t* eval)
{
    free(eval->tbl);
    memset(eval, 0, sizeof(*eval));
}

int crfsuite_evaluation_accmulate(crfsuite_evaluation_t* eval, const int* reference, const int* prediction, int T)
{
    int t, nc = 0;

    for (t = 0;t < T;++t) {
        int lr = reference[t];
        int lt = prediction[t];

        if (eval->num_labels <= lr || eval->num_labels <= lt) {
            return 1;
        }

        ++eval->tbl[lr].num_observation;
        ++eval->tbl[lt].num_model;
        if (lr == lt) {
            ++eval->tbl[lr].num_correct;
            ++nc;
        }
        ++eval->item_total_num;
    }

    if (nc == T) {
        ++eval->inst_total_correct;
    }
    ++eval->inst_total_num;

    return 0;
}

void crfsuite_evaluation_finalize(crfsuite_evaluation_t* eval)
{
    int i;

    for (i = 0;i <= eval->num_labels;++i) {
        crfsuite_label_evaluation_t* lev = &eval->tbl[i];

        /* Do not evaluate labels that does not in the test data. */
        if (lev->num_observation == 0) {
            continue;
        }

        /* Sum the number of correct labels for accuracy calculation. */
        eval->item_total_correct += lev->num_correct;
        eval->item_total_model += lev->num_model;
        eval->item_total_observation += lev->num_observation;

        /* Initialize the precision, recall, and f1-measure values. */
        lev->precision = 0;
        lev->recall = 0;
        lev->fmeasure = 0;

        /* Compute the precision, recall, and f1-measure values. */
        if (lev->num_model > 0) {
            lev->precision = lev->num_correct / (double)lev->num_model;
        }
        if (lev->num_observation > 0) {
            lev->recall = lev->num_correct / (double)lev->num_observation;
        }
        if (lev->precision + lev->recall > 0) {
            lev->fmeasure = lev->precision * lev->recall * 2 / (lev->precision + lev->recall);
        }

        /* Exclude unknown labels from calculation of macro-average values. */
        if (i != eval->num_labels) {
            eval->macro_precision += lev->precision;
            eval->macro_recall += lev->recall;
            eval->macro_fmeasure += lev->fmeasure;
        }
    }

    /* Copute the macro precision, recall, and f1-measure values. */
    eval->macro_precision /= eval->num_labels;
    eval->macro_recall /= eval->num_labels;
    eval->macro_fmeasure /= eval->num_labels;

    /* Compute the item accuracy. */
    eval->item_accuracy = 0;
    if (0 < eval->item_total_num) {
        eval->item_accuracy = eval->item_total_correct / (double)eval->item_total_num;
    }

    /* Compute the instance accuracy. */
    eval->inst_accuracy = 0;
    if (0 < eval->inst_total_num) {
        eval->inst_accuracy = eval->inst_total_correct / (double)eval->inst_total_num;
    }
}

void crfsuite_evaluation_output(crfsuite_evaluation_t* eval, crfsuite_dictionary_t* labels, crfsuite_logging_callback cbm, void *instance)
{
    int i;
    const char *lstr = NULL;
    logging_t lg;

    lg.func = cbm;
    lg.instance = instance;

    logging(&lg, "Performance by label (#match, #model, #ref) (precision, recall, F1):\n");

    for (i = 0;i < eval->num_labels;++i) {
        const crfsuite_label_evaluation_t* lev = &eval->tbl[i];

        labels->to_string(labels, i, &lstr);
        if (lstr == NULL) lstr = "[UNKNOWN]";

        if (lev->num_observation == 0) {
            logging(&lg, "    %s: (%d, %d, %d) (******, ******, ******)\n",
                lstr, lev->num_correct, lev->num_model, lev->num_observation
                );
        } else {
            logging(&lg, "    %s: (%d, %d, %d) (%1.4f, %1.4f, %1.4f)\n",
                lstr, lev->num_correct, lev->num_model, lev->num_observation,
                lev->precision, lev->recall, lev->fmeasure
                );
        }
        labels->free(labels, lstr);
    }
    logging(&lg, "Macro-average precision, recall, F1: (%f, %f, %f)\n",
        eval->macro_precision, eval->macro_recall, eval->macro_fmeasure
        );
    logging(&lg, "Item accuracy: %d / %d (%1.4f)\n",
        eval->item_total_correct, eval->item_total_num, eval->item_accuracy
        );
    logging(&lg, "Instance accuracy: %d / %d (%1.4f)\n",
        eval->inst_total_correct, eval->inst_total_num, eval->inst_accuracy
        );
}

int crfsuite_interlocked_increment(int *count)
{
    return ++(*count);
}

int crfsuite_interlocked_decrement(int *count)
{
    return --(*count);
}

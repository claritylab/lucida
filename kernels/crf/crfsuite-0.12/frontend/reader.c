/*
 *        Data reader.
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

#include <stdio.h>
#include <stdlib.h>

#include <crfsuite.h>
#include "iwa.h"

static int progress(FILE *fpo, int prev, int current)
{
    while (prev < current) {
        ++prev;
        if (prev % 2 == 0) {
            if (prev % 10 == 0) {
                fprintf(fpo, "%d", prev / 10);
                fflush(fpo);
            } else {
                fprintf(fpo, ".", prev / 10);
                fflush(fpo);
            }
        }
    }
    return prev;
}

int read_data(FILE *fpi, FILE *fpo, crfsuite_data_t* data, int group)
{
    int n = 0;
    int lid = -1;
    crfsuite_instance_t inst;
    crfsuite_item_t item;
    crfsuite_attribute_t cont;
    iwa_t* iwa = NULL;
    crfsuite_dictionary_t *attrs = data->attrs;
    crfsuite_dictionary_t *labels = data->labels;
    const iwa_token_t *token = NULL;
    long filesize = 0, begin = 0, offset = 0;
    int prev = 0, current = 0;

    /* Initialize the instance.*/
    crfsuite_instance_init(&inst);
    inst.group = group;

    /* Obtain the file size. */
    begin = ftell(fpi);
    fseek(fpi, 0, SEEK_END);
    filesize = ftell(fpi) - begin;
    fseek(fpi, begin, SEEK_SET);

    /* */
    fprintf(fpo, "0");
    fflush(fpo);
    prev = 0;

    iwa = iwa_reader(fpi);
    while (token = iwa_read(iwa), token != NULL) {
        /* Progress report. */
        offset = ftell(fpi);
        current = (int)((offset - begin) * 100.0 / (double)filesize);
        prev = progress(fpo, prev, current);

        switch (token->type) {
        case IWA_BOI:
            /* Initialize an item. */
            lid = -1;
            crfsuite_item_init(&item);
            break;
        case IWA_EOI:
            /* Append the item to the instance. */
            if (0 <= lid) {
                crfsuite_instance_append(&inst, &item, lid);
            }
            crfsuite_item_finish(&item);
            break;
        case IWA_ITEM:
            if (lid == -1) {
                lid = labels->get(labels, token->attr);
            } else {
                crfsuite_attribute_init(&cont);
                cont.aid = attrs->get(attrs, token->attr);
                if (token->value && *token->value) {
                    cont.value = atof(token->value);
                } else {
                    cont.value = 1.0;
                }
                crfsuite_item_append_attribute(&item, &cont);
            }
            break;
        case IWA_NONE:
        case IWA_EOF:
            /* Put the training instance. */
            crfsuite_data_append(data, &inst);
            crfsuite_instance_finish(&inst);
            inst.group = group;
            ++n;
            break;
        }
    }

    progress(fpo, prev, 100);
    fprintf(fpo, "\n");

    return n;
}

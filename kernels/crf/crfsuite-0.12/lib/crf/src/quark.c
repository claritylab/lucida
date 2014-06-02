/*
 *      Quark object.
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

#include "os.h"
#include <stdlib.h>
#include <string.h>
#include "rumavl.h"
#include "quark.h"

typedef struct {
    char *str;
    int qid;
} record_t;

struct tag_quark {
    int num;
    int max;
    RUMAVL* string_to_id;
    char **id_to_string;
};

static int keycmp(const void *_x, const void *_y, size_t n, void *udata)
{
    const record_t* x = (const record_t*)_x;
    const record_t* y = (const record_t*)_y;
    return strcmp(x->str, y->str);
}

static int owcb(RUMAVL *tree, RUMAVL_NODE *n, void *_x, const void *_y, void *udata)
{
    record_t* x = (record_t*)_x;
    free(x->str);
    return 0;
}

static int delcb(RUMAVL *tree, RUMAVL_NODE *n, void *_record, void *udata)
{
    record_t* record = (record_t*)_record;
    free(record->str);
    return 0;
}

quark_t* quark_new()
{
    quark_t* qrk = (quark_t*)malloc(sizeof(quark_t));
    if (qrk != NULL) {
        qrk->num = 0;
        qrk->max = 0;
        qrk->string_to_id = rumavl_new(sizeof(record_t), keycmp, NULL, NULL);
        if (qrk->string_to_id != NULL) {
            *rumavl_delcb(qrk->string_to_id) = delcb;
            *rumavl_owcb(qrk->string_to_id) = owcb;
        }
        qrk->id_to_string = NULL;
    }
    return qrk;
}

void quark_delete(quark_t* qrk)
{
    if (qrk != NULL) {
        rumavl_destroy(qrk->string_to_id);
        free(qrk->id_to_string);
        free(qrk);
    }
}

int quark_get(quark_t* qrk, const char *str)
{
    record_t key, *record = NULL;

    key.str = (char *)str;
    record = (record_t*)rumavl_find(qrk->string_to_id, &key);
    if (record == NULL) {
        char *newstr = (char*)malloc(strlen(str)+1);
        if (newstr != NULL) {
            strcpy(newstr, str);
        }

        if (qrk->max <= qrk->num) {
            qrk->max = (qrk->max + 1) * 2;
            qrk->id_to_string = (char **)realloc(qrk->id_to_string, sizeof(char *) * qrk->max);
        }

        qrk->id_to_string[qrk->num] = newstr;
        key.str = newstr;
        key.qid = qrk->num;
        rumavl_insert(qrk->string_to_id, &key);

        ++qrk->num;
        return key.qid;
    } else {
        return record->qid;
    }    
}

int quark_to_id(quark_t* qrk, const char *str)
{
    record_t key, *record = NULL;

    key.str = (char *)str;
    record = (record_t*)rumavl_find(qrk->string_to_id, &key);
    return (record != NULL) ? record->qid : -1;
}

const char *quark_to_string(quark_t* qrk, int qid)
{
    return (qid < qrk->num) ? qrk->id_to_string[qid] : NULL;
}

int quark_num(quark_t* qrk)
{
    return qrk->num;
}



#if 0
int main(int argc, char *argv[])
{
    quark_t *qrk = quark_new();
    int qid = 0;

    qid = quark_get(qrk, "zero");
    qid = quark_get(qrk, "one");
    qid = quark_get(qrk, "zero");
    qid = quark_to_id(qrk, "three");
    qid = quark_get(qrk, "two");
    qid = quark_get(qrk, "three");
    qid = quark_to_id(qrk, "three");
    qid = quark_get(qrk, "zero");
    qid = quark_get(qrk, "one");

    printf("%s\n", quark_to_string(qrk, 0));
    printf("%s\n", quark_to_string(qrk, 1));
    printf("%s\n", quark_to_string(qrk, 2));
    printf("%s\n", quark_to_string(qrk, 3));

    quark_delete(qrk);
    
    return 0;
}
#endif

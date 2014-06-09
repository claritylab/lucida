/*
 *      Parameter exchange.
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

#include <stdlib.h>
#include <string.h>

#include <crfsuite.h>
#include "quark.h"

enum {
    PT_NONE = 0,
    PT_INT,
    PT_FLOAT,
    PT_STRING,
};

typedef struct {
    char*       name;
    int         type;
    int         val_i;
    floatval_t  val_f;
    char*       val_s;
    char*       help;
} param_t;

typedef struct {
    int num_params;
    param_t* params;
} params_t;

static char *mystrdup(const char *src)
{
    char *dst = (char*)malloc(strlen(src) + 1);
    if (dst != NULL) {
        strcpy(dst, src);
    }
    return dst;
}

static int params_addref(crfsuite_params_t* params)
{
    return crfsuite_interlocked_increment(&params->nref);
}

static int params_release(crfsuite_params_t* params)
{
    int count = crfsuite_interlocked_decrement(&params->nref);
    if (count == 0) {
        int i;
        params_t* pars = (params_t*)params->internal;
        for (i = 0;i < pars->num_params;++i) {
            free(pars->params[i].name);
            free(pars->params[i].val_s);
            free(pars->params[i].help);
        }
        free(pars);
    }
    return count;
}

static param_t* find_param(params_t* pars, const char *name)
{
    int i;

    for (i = 0;i < pars->num_params;++i) {
        if (strcmp(pars->params[i].name, name) == 0) {
            return &pars->params[i];
        }
    }

    return NULL;
}

static int params_num(crfsuite_params_t* params)
{
    params_t* pars = (params_t*)params->internal;
    return pars->num_params;
}

static int params_name(crfsuite_params_t* params, int i, char **ptr_name)
{
    params_t* pars = (params_t*)params->internal;
    *ptr_name = mystrdup(pars->params[i].name);
    return 0;
}

static int params_set(crfsuite_params_t* params, const char *name, const char *value)
{
    params_t* pars = (params_t*)params->internal;
    param_t* par = find_param(pars, name);
    if (par == NULL) return -1;
    switch (par->type) {
    case PT_INT:
        par->val_i = (value != NULL) ? atoi(value) : 0;
        break;
    case PT_FLOAT:
        par->val_f = (value != NULL) ? (floatval_t)atof(value) : 0;
        break;
    case PT_STRING:
        free(par->val_s);
        par->val_s = (value != NULL) ? mystrdup(value) : mystrdup("");
    }
    return 0;
}

static int params_get(crfsuite_params_t* params, const char *name, char **value)
{
    char buffer[1024];
    params_t* pars = (params_t*)params->internal;
    param_t* par = find_param(pars, name);
    if (par == NULL) return -1;
    switch (par->type) {
    case PT_INT:
        snprintf(buffer, sizeof(buffer)-1, "%d", par->val_i);
        *value = mystrdup(buffer);
        break;
    case PT_FLOAT:
        snprintf(buffer, sizeof(buffer)-1, "%f", par->val_f);
        *value = mystrdup(buffer);
        break;
    case PT_STRING:
        *value = mystrdup(par->val_s);
    }
    return 0;
}

static void params_free(crfsuite_params_t* params, const char *value)
{
    free((char*)value);
}

static int params_set_int(crfsuite_params_t* params, const char *name, int value)
{
    params_t* pars = (params_t*)params->internal;
    param_t* par = find_param(pars, name);
    if (par == NULL) return -1;
    if (par->type != PT_INT) return -1;
    par->val_i = value;
    return 0;
}

static int params_set_float(crfsuite_params_t* params, const char *name, floatval_t value)
{
    params_t* pars = (params_t*)params->internal;
    param_t* par = find_param(pars, name);
    if (par == NULL) return -1;
    if (par->type != PT_FLOAT) return -1;
    par->val_f = value;
    return 0;
}

static int params_set_string(crfsuite_params_t* params, const char *name, const char *value)
{
    params_t* pars = (params_t*)params->internal;
    param_t* par = find_param(pars, name);
    if (par == NULL) return -1;
    if (par->type != PT_STRING) return -1;
    free(par->val_s);
    par->val_s = mystrdup(value);
    return 0;
}

static int params_get_int(crfsuite_params_t* params, const char *name, int *value)
{
    params_t* pars = (params_t*)params->internal;
    param_t* par = find_param(pars, name);
    if (par == NULL) return -1;
    if (par->type != PT_INT) return -1;
    *value = par->val_i;
    return 0;
}

static int params_get_float(crfsuite_params_t* params, const char *name, floatval_t *value)
{
    params_t* pars = (params_t*)params->internal;
    param_t* par = find_param(pars, name);
    if (par == NULL) return -1;
    if (par->type != PT_FLOAT) return -1;
    *value = par->val_f;
    return 0;
}

static int params_get_string(crfsuite_params_t* params, const char *name, char **value)
{
    params_t* pars = (params_t*)params->internal;
    param_t* par = find_param(pars, name);
    if (par == NULL) return -1;
    if (par->type != PT_STRING) return -1;
    *value = par->val_s;
    return 0;
}

static int params_help(crfsuite_params_t* params, const char *name, char **ptr_type, char **ptr_help)
{
    params_t* pars = (params_t*)params->internal;
    param_t* par = find_param(pars, name);
    if (par == NULL) return -1;
    if (ptr_type != NULL) {
        switch (par->type) {
        case PT_INT:
            *ptr_type = mystrdup("int");
            break;
        case PT_FLOAT:
            *ptr_type = mystrdup("float");
            break;
        case PT_STRING:
            *ptr_type = mystrdup("string");
            break;
        default:
            *ptr_type = mystrdup("unknown");
        }
    }
    if (ptr_help != NULL) {
        *ptr_help = mystrdup(par->help);
    }
    return 0;
}

crfsuite_params_t* params_create_instance()
{
    crfsuite_params_t* params = (crfsuite_params_t*)calloc(1, sizeof(crfsuite_params_t));

    if (params != NULL) {
        /* Construct the internal data. */
        params->internal = (params_t*)calloc(1, sizeof(params_t));
        if (params->internal == NULL) {
            free(params);
            return NULL;
        }

        /* Set member functions. */
        params->nref = 1;
        params->addref = params_addref;
        params->release = params_release;
        params->num = params_num;
        params->name = params_name;
        params->set = params_set;
        params->get = params_get;
        params->free = params_free;
        params->set_int = params_set_int;
        params->set_float = params_set_float;
        params->set_string = params_set_string;
        params->get_int = params_get_int;
        params->get_float = params_get_float;
        params->get_string = params_get_string;
        params->help = params_help;
    }

    return params;
}

int params_add_int(crfsuite_params_t* params, const char *name, int value, const char *help)
{
    param_t* par = NULL;
    params_t* pars = (params_t*)params->internal;
    pars->params = (param_t*)realloc(pars->params, (pars->num_params+1) * sizeof(param_t));
    if (pars->params == NULL) {
        return -1;
    }

    par = &pars->params[pars->num_params++];
    memset(par, 0, sizeof(*par));
    par->name = mystrdup(name);
    par->type = PT_INT;
    par->val_i = value;
    par->help = mystrdup(help);
    return 0;
}

int params_add_float(crfsuite_params_t* params, const char *name, floatval_t value, const char *help)
{
    param_t* par = NULL;
    params_t* pars = (params_t*)params->internal;
    pars->params = (param_t*)realloc(pars->params, (pars->num_params+1) * sizeof(param_t));
    if (pars->params == NULL) {
        return -1;
    }

    par = &pars->params[pars->num_params++];
    memset(par, 0, sizeof(*par));
    par->name = mystrdup(name);
    par->type = PT_FLOAT;
    par->val_f = value;
    par->help = mystrdup(help);
    return 0;
}

int params_add_string(crfsuite_params_t* params, const char *name, const char *value, const char *help)
{
    param_t* par = NULL;
    params_t* pars = (params_t*)params->internal;
    pars->params = (param_t*)realloc(pars->params, (pars->num_params+1) * sizeof(param_t));
    if (pars->params == NULL) {
        return -1;
    }

    par = &pars->params[pars->num_params++];
    memset(par, 0, sizeof(*par));
    par->name = mystrdup(name);
    par->type = PT_STRING;
    par->val_s = mystrdup(value);
    par->help = mystrdup(help);
    return 0;
}

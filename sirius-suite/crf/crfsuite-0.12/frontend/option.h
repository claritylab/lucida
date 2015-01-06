/*
 *        A parser for command-line options.
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

#ifndef    __OPTION_H__
#define    __OPTION_H__

#ifdef    __cplusplus
extern "C" {
#endif/*__cplusplus*/

typedef int (*option_handler_t)(void *instance, char c, const char *longname, const char *arg);

int option_parse(char * const argv[], int num_argv, option_handler_t handler, void *instance);
int option_strcmp(const char *option, const char *longname);

/** The begin of inline option map. */
#define    BEGIN_OPTION_MAP(name, type) \
    int name(void *instance, char __c, const char *__longname, const char *arg) \
    { \
        int used_args = 0; \
        type *opt = (type *)instance; \
        if (0) { \

/** An entry of option map */
#define    ON_OPTION(test) \
            return used_args; \
        } else if (test) { \
            used_args = 0; \

#define    ON_OPTION_WITH_ARG(test) \
            return used_args; \
        } else if (test) { \
            used_args = 1; \

/** The end of option map implementation */
#define    END_OPTION_MAP() \
            return used_args; \
        } \
        if (__c != 0) { \
            fprintf(stderr, "Unrecognized option -%c\n", __c); \
        } else if (__longname != NULL) { \
            fprintf(stderr, "Unrecognized option --%s\n", __longname); \
        } \
        return -1; \
    } \

/** A predicator for short options */
#define    SHORTOPT(x)        (__c == x)
/** A predicator for long options */
#define    LONGOPT(x)        (!__c && option_strcmp(__longname, x) == 0)

#ifdef    __cplusplus
}
#endif/*__cplusplus*/

#endif/*__OPTION_H__*/


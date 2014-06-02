/*
 *        A parser for Item With Attributes (IWA) format.
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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "iwa.h"

typedef struct {
    size_t size;
    size_t offset;
    char *value;
} iwa_string_t;

struct tag_iwa {
    FILE *fp;

    iwa_token_t token;

    char *buffer;
    char *offset;
    char *end;

    iwa_string_t attr;
    iwa_string_t value;
};

#define    DEFAULT_SIZE    4096
#define    BUFFER_SIZE        (DEFAULT_SIZE * 16)

static void string_init(iwa_string_t* str)
{
    str->value = (char*)calloc(DEFAULT_SIZE, sizeof(char));
    str->size = DEFAULT_SIZE;
    str->offset = 0;
}

static void string_finish(iwa_string_t* str)
{
    free(str->value);
    memset(str, 0, sizeof(*str));
}

static void string_clear(iwa_string_t* str)
{
    memset(str->value, 0, str->size);
    str->offset = 0;
}

static void string_append(iwa_string_t* str, int c)
{
    if (str->size <= str->offset) {
        str->size *= 2;
        str->value = (char*)realloc(str->value, str->size);
        memset(&str->value[str->offset], 0, str->size - str->offset);
    }
    str->value[str->offset++] = c;
}

iwa_t* iwa_reader(FILE *fp)
{
    iwa_t* iwa = (iwa_t*)malloc(sizeof(iwa_t));

    if (iwa == NULL) {
        goto error_exit;
    }

    memset(iwa, 0, sizeof(iwa_t));

    iwa->fp = fp;

    iwa->buffer = (char*)malloc(sizeof(char) * BUFFER_SIZE);
    iwa->offset = iwa->buffer + BUFFER_SIZE;
    iwa->end = iwa->buffer + BUFFER_SIZE;
    if (iwa->buffer == NULL) {
        goto error_exit;
    }

    string_init(&iwa->attr);
    string_init(&iwa->value);

    return iwa;

error_exit:
    iwa_delete(iwa);
    return NULL;
}

void iwa_delete(iwa_t* iwa)
{
    if (iwa != NULL) {
        string_finish(&iwa->value);
        string_finish(&iwa->attr);
        free(iwa->buffer);
    }
    free(iwa);
}

static int peek_char(iwa_t* iwa)
{
    /* Refill the buffer if necessary. */
    if (iwa->end <= iwa->offset) {
        size_t count = fread(iwa->buffer, sizeof(char), BUFFER_SIZE, iwa->fp);
        iwa->offset = iwa->buffer;
        iwa->end = iwa->buffer + count;
        if (count == 0) {
            return EOF;
        }
    }

    /* Return the current character */
    return *iwa->offset;
}

static int get_char(iwa_t* iwa)
{
    int c = peek_char(iwa);
    if (c != EOF) {
        ++iwa->offset;
    }
    return c;
}

static void read_field_unquoted(iwa_t* iwa, iwa_string_t* str)
{
    int c;
    /* Read until a colon, space, tab, or break-line character. */
    while (c = peek_char(iwa), c != ':' && c != '\t' && c != '\n' && c != EOF) {
        get_char(iwa);
        string_append(str, c);
    }
    /* The input stream points to the character just after the field is terminated. */
}

static void read_field_unescaped(iwa_t* iwa, iwa_string_t* str)
{
    int c, d;
    /* Read until a colon, space, tab, or break-line character. */
    while (c = peek_char(iwa), c != ':' && c != '\t' && c != '\n' && c != EOF) {
        get_char(iwa);
        if (c == '\\') {
            /* Possibly a escape sequence. */
            d = peek_char(iwa);
            switch (d) {
            case ':':
            case '\\':
                c = d;
                get_char(iwa);
                break;
            }
        }

        string_append(str, c);
    }
    /* The input stream points to the character just after the field is terminated. */
}

static int read_item(iwa_t* iwa)
{
    int c;
    
    read_field_unescaped(iwa, &iwa->attr);

    /* Check the character just after the attribute field is terminated. */
    c = peek_char(iwa);
    if (c == ':') {
        /* Discard the colon. */
        get_char(iwa);

        read_field_unescaped(iwa, &iwa->value);

        c = peek_char(iwa);
        if (c == ':') {
            return 1;
        }
    }

    return 0;
}

const iwa_token_t* iwa_read(iwa_t* iwa)
{
    iwa_token_t* token = &iwa->token;

    /* Initialization. */
    token->attr = NULL;
    token->value = NULL;
    string_clear(&iwa->attr);
    string_clear(&iwa->value);

    /* Return NULL if the stream hits EOF. */
    if (peek_char(iwa) == EOF) {
        switch (token->type) {
        case IWA_EOF:
            return NULL;
        case IWA_BOI:
            token->type = IWA_EOI;
            return token;
        case IWA_NONE:
        case IWA_EOI:
            token->type = IWA_EOF;
            return token;
        }
    }

    /* Conditions based on the previous state. */
    switch (token->type) {
    case IWA_NONE:
    case IWA_EOI:
        if (peek_char(iwa) == '\n') {
            /* A empty line. */
            get_char(iwa);
            token->type = IWA_NONE;
        } else {
            /* A non-empty line. */
            token->type = IWA_BOI;
        }
        break;
    case IWA_BOI:
    case IWA_ITEM:
        for (;;) {
            int c = peek_char(iwa);

            if (c == '\t') {
                /* Skip white spaces. */
                get_char(iwa);
            } else if (c == '\n') {
                get_char(iwa);
                token->type = IWA_EOI;
                break;
            } else {
                read_item(iwa);
                token->type = IWA_ITEM;
                token->attr = iwa->attr.value;
                token->value = iwa->value.value;
                break;
            }
        }
        break;
    }

    return token;
}

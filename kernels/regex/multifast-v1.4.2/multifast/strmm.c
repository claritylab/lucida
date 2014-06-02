/*
 * strmm.c:
 * This file is part of multifast.
 *
    Copyright 2010-2013 Kamiar Kanani <kamiar.kanani@gmail.com>

    multifast is free software: you can redistribute it and/or modify
    it under the terms of the GNU Lesser General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    multifast is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public License
    along with multifast.  If not, see <http://www.gnu.org/licenses/>.
*/

#include <stdlib.h>
#include <string.h>

#include "strmm.h"

// Must be bigger than AC_PATTRN_MAX_LENGTH
#define STRING_TABLE_ALLOC_SIZE       4096
#define STRING_TABLE_MAX_CHUNK_NUMBER 40

#if (AC_PATTRN_MAX_LENGTH>=STRING_TABLE_ALLOC_SIZE)
#error "STRING_TABLE_ALLOC_SIZE must be bigger than AC_PATTRN_MAX_LENGTH"
#endif

//*****************************************************************************
// FUNCTION: strmm_init
//*****************************************************************************

void strmm_init (STRMM_t * st)
{
    st->last_chunk = 0;
    st->max_chunk = STRING_TABLE_MAX_CHUNK_NUMBER;
    st->space = (AC_ALPHABET_t **) malloc (st->max_chunk*sizeof(AC_ALPHABET_t *));
    st->space[st->last_chunk] = (AC_ALPHABET_t *) malloc (STRING_TABLE_ALLOC_SIZE*sizeof(AC_ALPHABET_t));
    st->last_pos = 0;
}

//*****************************************************************************
// FUNCTION: strmm_add
//*****************************************************************************

AC_ALPHABET_t * strmm_add (STRMM_t * st, AC_PATTERN_t * str)
{
    AC_ALPHABET_t * free_pos;

    if (st->last_pos + str->length + 1 > STRING_TABLE_ALLOC_SIZE)
    {
        if(str->length + 1 > STRING_TABLE_ALLOC_SIZE)
            return NULL; // Fatal Error

        st->last_chunk++;
        if (st->last_chunk >= st->max_chunk)
        {
            st->max_chunk += STRING_TABLE_MAX_CHUNK_NUMBER;
            st->space = (AC_ALPHABET_t **) realloc (st->space, st->max_chunk*sizeof(AC_ALPHABET_t *));
        }
        st->space[st->last_chunk] = (AC_ALPHABET_t *) malloc (STRING_TABLE_ALLOC_SIZE*sizeof(AC_ALPHABET_t));
        st->last_pos = 0;
    }

    free_pos = &((st->space[st->last_chunk])[st->last_pos]);
    memcpy (free_pos, str->astring, str->length*sizeof(AC_ALPHABET_t));
    st->last_pos += str->length;
    (st->space[st->last_chunk])[st->last_pos++] = (AC_ALPHABET_t)0;

    str->astring = free_pos;
    return free_pos;
}

//*****************************************************************************
// FUNCTION: strmm_addstrid
//*****************************************************************************

char * strmm_addstrid (STRMM_t * st, char * str)
{
    char * free_pos;
    int str_length = strlen(str);

    if (st->last_pos + str_length + 1 > STRING_TABLE_ALLOC_SIZE)
    {
        if (str_length + 1 > STRING_TABLE_ALLOC_SIZE)
            return NULL; // Fatal Error

        st->last_chunk++;
        if (st->last_chunk >= st->max_chunk)
        {
            st->max_chunk += STRING_TABLE_MAX_CHUNK_NUMBER;
            st->space = (char **) realloc (st->space, st->max_chunk*sizeof(char *));
        }
        st->space[st->last_chunk] = (char *) malloc (STRING_TABLE_ALLOC_SIZE*sizeof(char));
        st->last_pos = 0;
    }

    free_pos = &((st->space[st->last_chunk])[st->last_pos]);
    memcpy (free_pos, str, str_length*sizeof(char));
    st->last_pos += str_length;
    (st->space[st->last_chunk])[st->last_pos++] = (char)0;

    return free_pos;
}

//*****************************************************************************
// FUNCTION: strmm_release
//*****************************************************************************

void strmm_release (STRMM_t * st)
{
    int i;
    for (i=0; i<=st->last_chunk; i++)
        free (st->space[i]);
}

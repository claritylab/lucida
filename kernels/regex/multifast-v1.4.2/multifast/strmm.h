/*
 * strmm.h:
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

#ifndef _STRINGMM_H_
#define _STRINGMM_H_

#include "actypes.h"

typedef struct
{
    AC_ALPHABET_t ** space;
    unsigned int  last_chunk;
    unsigned int  max_chunk;
    unsigned long last_pos;
} STRMM_t;

void strmm_init (STRMM_t * st);
AC_ALPHABET_t * strmm_add (STRMM_t * st, AC_PATTERN_t * str);
char * strmm_addstrid (STRMM_t * st, char * str);
void strmm_release (STRMM_t * st);

#endif /* _STRINGMM_H_ */

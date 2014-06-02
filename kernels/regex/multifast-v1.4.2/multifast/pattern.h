/*
 * pattern.h:
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

#ifndef _PATTERN_H_
#define _PATTERN_H_

#include "ahocorasick.h"

int  pattern_load    (const char * infile, AC_AUTOMATA_t ** ppaca);
void pattern_release (void);
void pattern_print   (AC_PATTERN_t * acs);

#endif /* _PATTERN_H_ */

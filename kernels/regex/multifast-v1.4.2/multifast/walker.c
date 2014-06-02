/*
 * walker.c:
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

#define _XOPEN_SOURCE 500
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <unistd.h>
#include <ftw.h>

#include "walker.h"
#include "ahocorasick.h"

static AC_AUTOMATA_t * pacautomata;

extern int search_file (const char * filename, AC_AUTOMATA_t * paca);

static int walker_ftw_callback
    (const char *fpath, const struct stat *sb, int tflag, struct FTW *ftwbuf);

//*****************************************************************************
// FUNCTION: search_file
//*****************************************************************************

int walker_find (char * rootdir, AC_AUTOMATA_t * paca)
{
    int flags = FTW_DEPTH|FTW_PHYS;
    pacautomata = paca;
    if (nftw(rootdir, walker_ftw_callback, 20, flags) == -1)
        return -1;
    return 0;
}

//*****************************************************************************
// FUNCTION: walker_ftw_callback
//*****************************************************************************

static int walker_ftw_callback
    (const char *fpath, const struct stat *sb, int tflag, struct FTW *ftwbuf)
{
    if (tflag != FTW_F)
        return 0;
    search_file (fpath, pacautomata);
    return 0;
}

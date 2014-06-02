/*
 * multifast.h:
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

#ifndef _MULTIFAST_H_
#define _MULTIFAST_H_

struct program_config
{
    char * pattern_file_name;
    char ** input_files;
    long input_files_num;
    short find_first;
    short verbosity;
    short insensitive;
    short output_show_item;     // Item number
    short output_show_dpos;     // Start position (decimal)
    short output_show_xpos;     // Start position (hex)
    short output_show_reprv;    // Representative
    short output_show_pattern;  // Pattern
};

void lower_case (char * s, unsigned int l);
void print_usage (char * progname);
int  search_file (const char * filename, AC_AUTOMATA_t * paca);
int  match_handler (AC_MATCH_t * m, void * param);

// Parameter to match_handler
struct match_param
{
    unsigned long total_match;
    unsigned long item;
    char * fname;
};

#endif /* _MULTIFAST_H_ */

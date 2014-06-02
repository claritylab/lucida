/*
 * example2.c: This program illustrates how to use ahocorasick library
 * it shows some techniques for using the library.
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

#include <stdio.h>
#include <string.h>

#include "ahocorasick.h"

char input_file [] =
    "ACAAGATGCCATTGTCCCCCGGCCTCCTGCTGCTGCTGCTCTCCGGGGCCACGGCCACCGCTGCCCTGCC"
    "CCTGGAGGGTGGCCCCACCGGCCGAGACAGCGAGCATATGCAGGAAGCGGCAGGAATAAGGAAAAGCAGC"
    "CTCCTGACTTTCCTCGCTTGGTGGTTTGAGTGGACCTCCCAGGCCAGTGCCGGGCCCCTCATAGGAGAGG"
    "AAGCTCGGGAGGTGGCCAGGCGGCAGGAAGGCGCACCCCCCCAGCAATCCGCGCGCCGGGACAGAATGCC"
    "CTGCAGGAACTTCTTCTGGAAGACCTTCTCCTCCTGCAAATAAAACCTCACCCATGAATGCTCACGCAAG"
    "TTTAATTACAGACCTGAA";

#define BUFFER_SIZE 64
char buffer[BUFFER_SIZE];

AC_PATTERN_t sample_patterns[] =
{
    {"TGGAGGGT", 0, {stringy: "one"}},
    {"GTGCCGGGCCC", 0, {stringy: "two"}},
    {"TTCT", 0, {stringy: "tree"}},
    {"GGGCCC", 0, {stringy: "four"}},
    {"AACTTCTT", 0, {stringy: "five"}},
    {"CTT", 0, {stringy: "six"}},
    {"TCCCCC", 0, {stringy: "seven"}},
};
#define PATTERN_COUNT (sizeof(sample_patterns)/sizeof(AC_PATTERN_t))

struct parameter
{
    int position;       // input: end position
    int match_count;    // output: total match count
};

int match_handler (AC_MATCH_t * matchp, void * param)
{
    unsigned int j;
    struct parameter * par = (struct parameter *)param;

    if (matchp->position > par->position)
        return 1;
    
    printf ("@ %2ld : ", matchp->position);
    
    for (j=0; j < matchp->match_num; j++)
        printf ("%s (%s), ", matchp->patterns[j].rep.stringy, matchp->patterns[j].astring);
    
    par->match_count += matchp->match_num;

    printf ("\n");
    
    return 0;
}


int main (int argc, char ** argv)
{
    unsigned int i;
    struct parameter my_param;
    // we use this struct to send/receive input/output parameters to/from automata
    my_param.position = 250;    // input: end position; change it to 1000 and see what happens
    my_param.match_count = 0;   // output:

    AC_TEXT_t input_text;
    AC_AUTOMATA_t * atm = ac_automata_init ();

    for (i=0; i<PATTERN_COUNT; i++)
    {
        AC_STATUS_t status;
        sample_patterns[i].length = strlen (sample_patterns[i].astring);
        status = ac_automata_add (atm, &sample_patterns[i]);
        switch (status)
        {
            case ACERR_DUPLICATE_PATTERN:
                printf ("Add pattern failed: ACERR_DUPLICATE_PATTERN: %s\n", sample_patterns[i].astring);
                break;
            case ACERR_LONG_PATTERN:
                printf ("Add pattern failed: ACERR_LONG_PATTERN: %s\n", sample_patterns[i].astring);
                break;
            case ACERR_ZERO_PATTERN:
                printf ("Add pattern failed: ACERR_ZERO_PATTERN: %s\n", sample_patterns[i].astring);
                break;
            case ACERR_AUTOMATA_CLOSED:
                printf ("Add pattern failed: ACERR_AUTOMATA_CLOSED: %s\n", sample_patterns[i].astring);
                break;
            case ACERR_SUCCESS:
                printf ("Pattern Added: %s\n", sample_patterns[i].astring);
                break;
        }
    }

    ac_automata_finalize (atm);
    
    // here we illustrates how to search a big text chunk by chunk.
    // in this example input buffer size is 64 and input file is pretty
    // bigger than that. we want to imitate reading from input file.
    // in such situations searching must be done inside a loop. the loop
    // continues until it consumed all input file.

    printf ("Automata finalized.\n\nSearching...\n");

    char * chunk_start = input_file;
    char * end_of_file = input_file + sizeof(input_file);
    input_text.astring = buffer;

    while (chunk_start<end_of_file)
    {
        input_text.length = (chunk_start<end_of_file)?sizeof(buffer):(sizeof(input_file)%sizeof(buffer));
        strncpy (buffer, chunk_start, input_text.length);

        if (ac_automata_search (atm, &input_text, 1, match_handler, (void *)(&my_param)))
            // if the search stopped in the middle (returned 1) we should break the loop
            break;

        chunk_start += sizeof(buffer);
    }
    
    printf ("found %d occurrence in the beginning %d bytes\n", my_param.match_count, my_param.position);

    // TODO: do the same search with settext/findnext interface
    
    ac_automata_release (atm);

    return 0;
}

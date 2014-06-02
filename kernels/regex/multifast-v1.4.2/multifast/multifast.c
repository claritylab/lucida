/*
 * multifast.c:
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
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <ctype.h>
#include <dirent.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include "pattern.h"
#include "walker.h"
#include "multifast.h"

// Program configuration options
struct program_config configuration = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};

//*****************************************************************************
// FUNCTION: main
//*****************************************************************************

int main (int argc, char ** argv)
{
    int clopt; // Command line option
    AC_AUTOMATA_t * paca; // Aho-Corasick automata pointer

    // Command line argument control
    if(argc < 4)
    {
        print_usage (argv[0]);
        exit(1);
    }

    // Read Command line options
    while ((clopt = getopt(argc, argv, "P:ndxrpfivh")) != -1)
    {
        switch (clopt)
        {
        case 'P':
            configuration.pattern_file_name = optarg;
            break;
        case 'n':
            configuration.output_show_item = 1;
            break;
        case 'd':
            configuration.output_show_dpos = 1;
            break;
        case 'x':
            configuration.output_show_xpos = 1;
            break;
        case 'r':
            configuration.output_show_reprv = 1;
            break;
        case 'p':
            configuration.output_show_pattern = 1;
            break;
        case 'f':
            configuration.find_first = 1;
            break;
        case 'i':
            configuration.insensitive = 1;
            break;
        case 'v':
            configuration.verbosity = 1;
            break;
        case '?':
        case 'h':
        default:
            print_usage (argv[0]);
            exit(1);
        }
    }
    configuration.input_files = argv + optind;
    configuration.input_files_num = argc - optind;

    if (configuration.pattern_file_name == NULL ||
            configuration.input_files[0] == NULL)
    {
        print_usage (argv[0]);
        exit(1);
    }
    if (!(configuration.output_show_item || configuration.output_show_dpos ||
            configuration.output_show_xpos || configuration.output_show_reprv
            || configuration.output_show_pattern))
    {
        configuration.output_show_xpos = 1;
        configuration.output_show_reprv = 1;
    }

    // Show program title
    if(configuration.verbosity)
    {
        printf("Loading Patterns From '%s'\n", configuration.pattern_file_name);
    }

    // Load patterns
    if (pattern_load (configuration.pattern_file_name, &paca))
        exit(1);
    
    if(configuration.verbosity)
        printf("Total Patterns: %lu\n", paca->total_patterns);

    if (paca->total_patterns == 0)
    {
        printf ("No pattern to search!\n");
        return 1;
    }

    // Search
    if (opendir(configuration.input_files[0]))
        // if it is a directory
    {
        if (configuration.verbosity)
            printf("Searching directory %s:\n", configuration.input_files[0]);
        walker_find (configuration.input_files[0], paca);
    } 
    else // if it is not a directory
    {
        int i;
        if (configuration.verbosity)
            printf("Searching %ld files\n", configuration.input_files_num);
        for (i=0; i<configuration.input_files_num; i++)
            search_file(configuration.input_files[i], paca);
    }

    // Release
    // pattern_release ();

    return 0;
}

//*****************************************************************************
// FUNCTION: search_file
//*****************************************************************************

int search_file (const char * filename, AC_AUTOMATA_t * paca)
{
    #define STREAM_BUFFER_SIZE 4096
    int fd_input; // Input file descriptor
    static AC_TEXT_t intext; // input text
    static AC_ALPHABET_t in_stream_buffer[STREAM_BUFFER_SIZE];
    static struct match_param mparm; // Match parameters
    long num_read; // Number of byes read from input file

    intext.astring = in_stream_buffer;

    // Open input file
    if (!strcmp(configuration.input_files[0], "-"))
    {
        fd_input = 0; // read from stdin
    }
    else if ((fd_input = open(filename, O_RDONLY|O_NONBLOCK))==-1)
    {
        fprintf(stderr, "Cannot read from input file '%s'\n", filename);
        return -1;
    }

    // Reset the parameter
    mparm.item = 0;
    mparm.total_match = 0;
    mparm.fname = fd_input?(char *)filename:NULL;

    int keep = 0;
    // loop to load and search the input file repeatedly, chunk by chunk
    do
    {
        // Read a chunk from input file
        num_read = read (fd_input, (void *)in_stream_buffer, STREAM_BUFFER_SIZE);
        
        intext.length = num_read;

        // Handle case sensitivity
        if (configuration.insensitive)
            lower_case(in_stream_buffer, intext.length);

        // Break loop if call-back function has done its work
        if (ac_automata_search (paca, &intext, keep, match_handler, &mparm))
            break;
        keep = 1;
    } while (num_read == STREAM_BUFFER_SIZE);

    close (fd_input);

    return 0;
}

//*****************************************************************************
// FUNCTION: lower_case
//*****************************************************************************

void lower_case (char * s, unsigned int l)
{
    unsigned int i;
    for(i=0; i<l; i++)
        s[i] = tolower(s[i]);
}

//*****************************************************************************
// FUNCTION: print_usage
//*****************************************************************************

void print_usage (char * progname)
{
    printf("Usage : %s -P pattern_file [-ndxrpfivh] file1 [file2 ...]\n", progname);
}

//*****************************************************************************
// FUNCTION: print_usage
//*****************************************************************************

int match_handler (AC_MATCH_t * m, void * param)
{
    unsigned int j;
    struct match_param * mparm = (struct match_param *)param;

    for (j=0; j < m->match_num; j++)
    {
        //if (mparm->item==0)
        if (mparm->fname)
            printf ("%s: ", mparm->fname);

        if (configuration.output_show_item)
            printf("#%ld ", ++mparm->item);

        if (configuration.output_show_dpos)
            printf("@%ld ", m->position - m->patterns[j].length + 1);

        if (configuration.output_show_xpos)
            printf("@%08X ", (unsigned int)(m->position - m->patterns[j].length + 1));

        if (configuration.output_show_reprv)
            printf("%s ", m->patterns[j].rep.stringy);

        if (configuration.output_show_pattern)
            pattern_print (&m->patterns[j]);

        printf("\n");
    }

    mparm->total_match += m->match_num;

    if (configuration.find_first)
        return 1; // Find First Match
    else
        return 0; // Find all matches
}

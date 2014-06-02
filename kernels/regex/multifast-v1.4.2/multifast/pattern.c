/*
 * pattern.c:
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
#include <ctype.h>
#include <stdlib.h>
#include <unistd.h>

#include "pattern.h"
#include "reader.h"
#include "strmm.h"
#include "multifast.h"

static STRMM_t strmem;              // Holds strings in memory for easy display
static AC_AUTOMATA_t * acautomata;  // Aho-Corasick automata

extern struct program_config configuration;

void pattern_print (AC_PATTERN_t * patt);
void pattern_getrep (const char ** id);
int  pattern_addtoac (AC_PATTERN_t * acs);

// This is the AC call-back handler function which is defined in multifast.c
extern int match_handler (AC_MATCH_t * m, void * param);

//*****************************************************************************
// FUNCTION: pattern_load
//*****************************************************************************

int pattern_load (const char * infile, AC_AUTOMATA_t ** ppaca)
{
    FILE * fd;
    char * buffer = reader_init();
    struct token_s * mytok;
    int readcount, loopguard=0;
    static enum token_type last_type=ENTOK_NONE;
    static AC_PATTERN_t mypattern;

    if ((fd = fopen(infile, "r"))==NULL)
    {
        printf ("Error in reading the pattern file %s\n", infile);
        return -1;
    }

    // Initialize string memory
    strmm_init (&strmem);

    // Initialize automata
    acautomata = ac_automata_init ();

    // Main loop to read patterns from pattern file
    while ((readcount=fread((void*)buffer, 1, READ_BUFFER_SIZE, fd))>0)
    {
        reader_reset_buffer(readcount);

        while ((mytok = reader_get_next_token()))
        {
            if (mytok->type==ENTOK_EOBUF)
                break;

            switch (mytok->type)
            {
            case ENTOK_AX:
                // Do nothing
                break;
            case ENTOK_ID:
                if (mytok->last==0)
                    mytok->type=ENTOK_AX; // Empty ID field
                else
                    mypattern.rep.stringy = strmm_addstrid (&strmem, mytok->value);
                    // mytok->value is null-terminated
                break;
            case ENTOK_STRING:
                if (last_type==ENTOK_AX)
                    pattern_getrep (&mypattern.rep.stringy);
                // Handle case sensitivity
                if (configuration.insensitive)
                    lower_case(mytok->value, mytok->last);
                mypattern.astring = mytok->value;
                mypattern.length = mytok->last;
                pattern_addtoac (&mypattern);
                break;
            case ENTOK_ERR:
                printf ("%s\n", mytok->value);
                loopguard=1;
                break;
            case ENTOK_EOF:
                loopguard=1;
                break;
            default:
                break;
            }
            last_type=mytok->type;
            if (loopguard)
                break;
        }

        if (loopguard)
            break;
    }

    // Check sanity of pattern file
    if (!(last_type==ENTOK_STRING || last_type==ENTOK_EOF))
    {
        printf ("Unexpected end of pattern file\n");
        return -1;
    }

    // Build Automata index
    ac_automata_finalize (acautomata);

    *ppaca=acautomata;

    close ((long)fd);
    reader_release();

    return 0;
}

//*****************************************************************************
// FUNCTION: pattern_addtoac
//*****************************************************************************

int pattern_addtoac (AC_PATTERN_t * acs)
{
    // Make a copy of pattern to the string memory
    if (!strmm_add (&strmem, acs))
    {
        printf("Fatal: Large Pattern\n");
        exit(1);
    }

    // Add pattern to automata
    switch (ac_automata_add (acautomata, acs))
    {
        case ACERR_DUPLICATE_PATTERN:
            printf("WARNINIG: Skip duplicate string: %s\n", acs->astring);
            break;
        case ACERR_LONG_PATTERN:
            printf("WARNINIG: Skip long string: %s\n", acs->astring);
            break;
        case ACERR_ZERO_PATTERN:
            printf("WARNINIG: Skip zero length string.\n");
            break;
        case ACERR_SUCCESS:
            if(configuration.verbosity)
            {
                printf ("Added successfully: %s - ", acs->rep.stringy);
                pattern_print (acs);
                printf ("\n");
            }
            break;
        default:
            printf("WARNINIG: Skip adding string.\n");
            break;
    }

    return 0;
}

//*****************************************************************************
// FUNCTION: pattern_release
//*****************************************************************************

void pattern_release ()
{
    // Release Automata
    ac_automata_release (acautomata);

    // Release string memory
    strmm_release (&strmem);
}

//*****************************************************************************
// FUNCTION: pattern_print
//*****************************************************************************

void pattern_print (AC_PATTERN_t * patt)
{
    #define DISPLAY_PATT_LEN 80
    int i, ishex=0;
    int maxdisplay = (patt->length<=DISPLAY_PATT_LEN)?patt->length:DISPLAY_PATT_LEN;

    for (i=0; i<maxdisplay; i++)
        if (!isprint(patt->astring[i]))
            ishex =1;
    printf ("{");
    if (ishex)
    {
        for (i=0; i<maxdisplay; i++)
            printf ("%s%02x", i?" ":"", (unsigned char)(patt->astring[i]));
    }
    else
    {
        for (i=0; i<maxdisplay; i++)
            printf ("%c", patt->astring[i]);
    }
    if (patt->length>DISPLAY_PATT_LEN)
        printf ("...");
    printf ("}");
}

//*****************************************************************************
// FUNCTION: pattern_getrep
// Get automatic representative for none-representative patterns.
//*****************************************************************************

void pattern_getrep (const char ** id)
{
    static char strid[64];
    static int item = 1;
    sprintf(strid, "p%06d", item++);
    *id = strmm_addstrid(&strmem, strid);
}

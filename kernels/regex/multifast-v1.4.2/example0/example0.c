/*
 * example0.c: This program illustrates how to use ahocorasick library
 * it shows how to use the settext/findnext interface to find patterns
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

AC_ALPHABET_t * sample_patterns[] = {
    "city",
    "clutter",
    "ever",
    "experience",
    "neo",
    "one",
    "simplicity",
    "utter",
    "whatever",
};
#define PATTERN_COUNT (sizeof(sample_patterns)/sizeof(AC_ALPHABET_t *))

AC_ALPHABET_t * input_text1 = {"experience the ease and simplicity of multifast"};
AC_ALPHABET_t * input_text2 = {"whatever you are be a good one"};
AC_ALPHABET_t * input_text3 = {"out of clutter, find simplicity"};


int main (int argc, char ** argv)
{
    unsigned int i;

    // 1. Define AC variables:
    
    AC_AUTOMATA_t   *atm;
    AC_PATTERN_t    tmp_pattern;
    AC_TEXT_t       tmp_text;

    // 2. Get a new automata
    
    atm = ac_automata_init ();

    // 3. Add patterns to automata
    
    for (i=0; i<PATTERN_COUNT; i++)
    {
        tmp_pattern.astring = sample_patterns[i];
        tmp_pattern.rep.number = i+1; // optional
        tmp_pattern.length = strlen(tmp_pattern.astring);
        ac_automata_add (atm, &tmp_pattern);
    }

    // 4. Finalize automata
    
    ac_automata_finalize (atm);
    // after you have finished with adding patterns you must finalize the automata
    // from now you can not add patterns anymore.

    // 4.1. Display automata (if you are interested)
    
    // ac_automata_display (atm, 'n');
    // the second argument determines the cast type of the pattern representative. 
    // 'n': as number 
    // 's': as string
    // because we use the integer part of union (tmp_patt.rep.number) so we used 'n'
    
    printf ("Searching: \"%s\"\n", input_text1);

    // 5. Set the input text
    
    tmp_text.astring = input_text1;
    tmp_text.length = strlen(tmp_text.astring);
    ac_automata_settext (atm, &tmp_text, 0);
    
    // 6. find
    
    AC_MATCH_t * matchp;
    
    while ((matchp = ac_automata_findnext(atm)))
    {
        unsigned int j;
        
        printf ("@%2ld: ", matchp->position);

        for (j=0; j < matchp->match_num; j++)
            printf("#%ld (%s), ", matchp->patterns[j].rep.number, matchp->patterns[j].astring);
            // CAUTION: be careful about using m->matched_patterns[j].astring
            // if 'astring' has permanent allocation inside your program's
            // memory area, you can use it. otherwise it will point to
            // an incorrect memory place. 
        
        printf ("\n");
    }

    printf ("Searching: \"%s\"\n", input_text2);
    // you can do more search 
    // just use function pair ac_automata_settext/ac_automata_findnext
    
    tmp_text.astring = input_text2;
    tmp_text.length = strlen(tmp_text.astring);
    ac_automata_settext (atm, &tmp_text, 0);
    
    while ((matchp = ac_automata_findnext(atm)))
    {
        unsigned int j;
        
        printf ("@%2ld: ", matchp->position);

        for (j=0; j < matchp->match_num; j++)
            printf("#%ld (%s), ", matchp->patterns[j].rep.number, matchp->patterns[j].astring);
        
        printf ("\n");
    }

    printf ("Searching: \"%s\" with \'keep\' enabled\n", input_text3);
    // and again
    
    tmp_text.astring = input_text3;
    tmp_text.length = strlen(tmp_text.astring);
    ac_automata_settext (atm, &tmp_text, 1);
    // when the keep option (3rd argument) in set, then the automata
    // considers that the given text is the next chunk of the previous text.
    // to understand the difference try it with 0 and 1 and compare the result
    
    while ((matchp = ac_automata_findnext(atm)))
    {
        unsigned int j;
        
        printf ("@ %2ld: ", matchp->position);

        for (j=0; j < matchp->match_num; j++)
            printf("#%ld (%s), ", matchp->patterns[j].rep.number, matchp->patterns[j].astring);
        
        printf ("\n");
    }

    // 7. Release the automata
    
    ac_automata_release (atm);
    // do not forget to release the automata after you have done with it

    return 0;
}

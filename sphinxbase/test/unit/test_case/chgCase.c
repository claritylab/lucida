/* -*- c-basic-offset: 4; indent-tabs-mode: nil -*- */
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <case.h>
#include <err.h>

#define MAX_STR_LEN 64
#define NUM_STRS 6

#define STR0 "this string should NEVER show up"
#define STR1 ""
#define STR2 "az3o%\tW@^#\\\n\r[]{}|\() '\""
#define STR3 "az3o%\tw@^#\\\n\r[]{}|\() '\""
#define STR4 "AZ3O%\tW@^#\\\n\r[]{}|\() '\""
#define STR5 "AZ3O%\tw@^#\\\n\r[]{}|\() '\""


int
main(int argc, char **argv)
{
    int cmp;
    char *n1 = NULL;
    char *n2 = NULL;

    char s1[MAX_STR_LEN];
    char s2[MAX_STR_LEN];

    char strs[NUM_STRS][MAX_STR_LEN] = { STR0,
        STR1,
        STR2,
        STR3,
        STR4,
        STR5
    };

    if (argc < 2 ||
        3 == argc ||
        argc > 4 ||
        (strcmp(argv[1], "lcase") &&
         strcmp(argv[1], "ucase") && strcmp(argv[1], "strcmp_nocase")
        )) {
        /*printf("INVALID PARAMETERS to chgCase\n"); */
        exit(1);
    }


    if (2 == argc) {
        if (0 == strcmp(argv[1], "ucase")) {
            ucase(n1);
        }
        else if (0 == strcmp(argv[1], "lcase")) {
            lcase(n1);
        }
        else {
            strcmp_nocase(n1, n2);
        }
        /*
           if we're still alive we obviously didn't segfault
         */
        exit(0);
    }

    if (4 == argc) {

        if (0 >= atoi(argv[2]) ||
            atoi(argv[2]) >= NUM_STRS ||
            0 >= atoi(argv[3]) || atoi(argv[3]) >= NUM_STRS) {
            E_INFO("INVALID PARAMS TO chkCase\n");
            exit(1);
        }

        strcpy(s1, strs[atoi(argv[2])]);
        strcpy(s2, strs[atoi(argv[3])]);

        if (0 == strcmp(argv[1], "ucase")) {
            ucase(s1);
            cmp = strcmp(s1, s2);
        }
        else if (0 == strcmp(argv[1], "lcase")) {
            lcase(s1);
            cmp = strcmp(s1, s2);
        }
        else {
            cmp = strcmp_nocase(s1, s2);
        }

        /*    E_INFO("Value of cmp %d\n", cmp); */
        if (0 != cmp) {
            E_FATAL("test failed\nstr1:|%s|\nstr2:|%s|\n", s1, s2);
        }

        return (cmp != 0);
    }

    /*somehow we got here and we shouldn't have */

    exit(1);
}

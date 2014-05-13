/**
 * @file test_fopen.c Test file opening
 * @author David Huggins-Daines <dhuggins@cs.cmu.edu>
 */

#include "pio.h"
#include "test_macros.h"

#include <stdio.h>
#include <stdlib.h>

int
main(int argc, char *argv[])
{
    FILE *fh;
    char line[256], *c;
    int ispipe;

    fh = fopen_comp(LMDIR "/100.arpa.gz", "r", &ispipe);
    TEST_ASSERT(fh != NULL);
    c = fgets(line, sizeof(line), fh);
    TEST_EQUAL('#', line[0]);
    fclose_comp(fh, ispipe);

    fh = fopen_compchk(LMDIR "/100.arpa.gz", &ispipe);
    TEST_ASSERT(fh != NULL);
    c = fgets(line, sizeof(line), fh);
    TEST_EQUAL('#', line[0]);
    fclose_comp(fh, ispipe);

    fh = fopen_compchk(LMDIR "/100.arpa.bz2", &ispipe);
    TEST_ASSERT(fh != NULL);
    c = fgets(line, sizeof(line), fh);
    TEST_EQUAL('#', line[0]);
    fclose_comp(fh, ispipe);

    fh = fopen_compchk(LMDIR "/100.arpa", &ispipe);
    TEST_ASSERT(fh != NULL);
    c = fgets(line, sizeof(line), fh);
    TEST_EQUAL('#', line[0]);
    fclose_comp(fh, ispipe);

    return 0;
}

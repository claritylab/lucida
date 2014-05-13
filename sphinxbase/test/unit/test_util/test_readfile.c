/* -*- c-basic-offset: 4; indent-tabs-mode: nil -*- */
/**
 * @file test_readfile.c: Test for the methods to read the file
 * @author David Huggins-Daines <dhuggins@cs.cmu.edu>
 */

#include "ckd_alloc.h"
#include "bio.h"
#include "test_macros.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

int
main(int argc, char *argv[])
{
    int nsamps;
    int16 *data;
    
    data = bio_read_wavfile(TESTDATADIR, "chan3", ".wav", 44, FALSE, &nsamps);
    TEST_EQUAL(230108, nsamps);
    
    ckd_free(data);

    return 0;
}

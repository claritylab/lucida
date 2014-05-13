/* -*- c-basic-offset: 4; indent-tabs-mode: nil -*- */
/**
 * @file test_build_directory.c Test recursive directory creation
 * @author David Huggins-Daines <dhuggins@cs.cmu.edu>
 */

#include "pio.h"
#include "test_macros.h"

#include <stdio.h>
#include <stdlib.h>

int
main(int argc, char *argv[])
{
    TEST_EQUAL(0, build_directory("foo/bar/baz"));
    TEST_ASSERT(stat_mtime("foo/bar/baz") != -1);
    TEST_EQUAL(0, build_directory("./quux/"));
    TEST_ASSERT(stat_mtime("quux") != -1);
    TEST_EQUAL(0, build_directory("./foo/bar/baz"));
    TEST_ASSERT(stat_mtime("foo/bar/baz") != -1);
    TEST_EQUAL(0, build_directory("/tmp/sphinxbase_foo_bar_baz"));
    TEST_ASSERT(stat_mtime("/tmp/sphinxbase_foo_bar_baz") != -1);

    return 0;
}

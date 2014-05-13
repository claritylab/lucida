#!/bin/sh

. ../testfuncs.sh

ulimit -c 0
if ./test_ckd_alloc_abort; then
    fail expected_failure
else
    pass expected_failure
fi


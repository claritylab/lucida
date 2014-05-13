#!/bin/sh

. ../testfuncs.sh

./test_ckd_alloc_fail
if [ $? = 255 ]; then
    pass expected_failure
else
    fail expected_failure
fi


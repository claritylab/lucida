#!/bin/sh
. ./testfuncs.sh

echo "CEPVIEW TEST"
tmpout="test-cepview.out"

run_program sphinx_cepview/sphinx_cepview \
-i 13 \
-d 13 \
-f $tests/regression/chan3.mfc \
> $tmpout 2>test-cepview.err
if diff -w $tmpout $tests/regression/chan3.cepview > /dev/null 2>&1; then
pass "CEPVIEW test"; else
fail "CEPVIEW test"; fi

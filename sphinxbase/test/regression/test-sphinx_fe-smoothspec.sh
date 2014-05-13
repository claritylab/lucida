#!/bin/sh
. ./testfuncs.sh

tmpout="test-sphinx_fe-smoothspec.out"

echo "WAVE2FEAT-SMOOTHSPEC TEST"
run_program sphinx_fe/sphinx_fe \
-smoothspec yes \
-samprate 11025 \
-frate 105 \
-wlen 0.024 \
-alpha 0.97 \
-nfft 512 \
-nfilt 36 \
-upperf 5400 \
-lowerf 130 \
-blocksize 262500 \
-i $tests/regression/chan3.raw \
-input_endian little \
-o test-sphinx_fe-smoothspec.logspec.out  \
-raw yes > $tmpout 2>&1 

run_program sphinx_cepview/sphinx_cepview \
-i 36 \
-d 36 \
-f test-sphinx_fe-smoothspec.logspec.out \
> test-sphinx_fe-smoothspec.cepview.out 2>>$tmpout

compare_table "WAVE2FEAT-SMOOTHSPEC test" test-sphinx_fe-smoothspec.cepview.out \
    $tests/regression/chan3-smoothspec.cepview 0.1

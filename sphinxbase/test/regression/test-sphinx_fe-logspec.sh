#!/bin/sh
. ./testfuncs.sh

tmpout="test-sphinx_fe-logspec.out"

echo "WAVE2FEAT-LOGSPEC TEST"
run_program sphinx_fe/sphinx_fe \
-logspec 1 \
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
-o test-sphinx_fe-logspec.mfc.out  \
-raw 1 > $tmpout 2>&1 

run_program sphinx_cepview/sphinx_cepview \
-i 36 \
-d 36 \
-f test-sphinx_fe-logspec.mfc.out \
> test-sphinx_fe-logspec.cepview.out 2>>$tmpout

compare_table "WAVE2FEAT-LOGSPEC test" test-sphinx_fe-logspec.cepview.out \
    $tests/regression/chan3-logspec.cepview 0.2

#!/bin/sh
. ./testfuncs.sh

tmpout="test-sphinx_fe.out"

echo "WAVE2FEAT TEST"
run_program sphinx_fe/sphinx_fe \
-samprate 11025 \
-frate 105 \
-wlen 0.024 \
-alpha 0.97 \
-ncep 13 \
-nfft 512 \
-nfilt 36 \
-upperf 5400 \
-lowerf 130 \
-blocksize 262500 \
-i $tests/regression/chan3.raw \
-input_endian little \
-o test-sphinx_fe.mfc  \
-raw 1 > $tmpout 2>&1 

run_program sphinx_cepview/sphinx_cepview \
-i 13 \
-d 13 \
-f test-sphinx_fe.mfc \
> test-sphinx_fe.cepview 2>>$tmpout

compare_table "WAVE2FEAT test" test-sphinx_fe.cepview $tests/regression/chan3.cepview 0.1

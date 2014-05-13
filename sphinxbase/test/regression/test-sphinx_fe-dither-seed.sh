#!/bin/sh
. ./testfuncs.sh

tmpout="test-sphinx_fe-dither-seed.out"

# Run sphinx_fe with dither and seed, so it is repeatable. There's
# nothing special about the seed. Just chose it because it's pretty
echo "WAVE2FEAT-DITHER-SEED TEST"
run_program sphinx_fe/sphinx_fe \
-dither 1 \
-seed 1234 \
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
-o test-sphinx_fe-dither-seed.mfc.out  \
-raw 1 > $tmpout 2>&1 

run_program sphinx_cepview/sphinx_cepview \
-i 13 \
-d 13 \
-f test-sphinx_fe-dither-seed.mfc.out \
> test-sphinx_fe-dither-seed.cepview.out 2>>$tmpout

compare_table "WAVE2FEAT-DITHER-SEED test" test-sphinx_fe-dither-seed.cepview.out \
    $tests/regression/chan3-dither.cepview 0.1

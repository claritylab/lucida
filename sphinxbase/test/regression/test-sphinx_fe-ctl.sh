#!/bin/sh
. ./testfuncs.sh

tmpout="test-sphinx_fe-ctl.out"

echo "WAVE2FEAT CTL/WAV/SPH TEST"
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
-verbose yes \
-c $tests/regression/chan3.ctl \
-di $tests/regression \
-do . \
-eo mfc \
-input_endian little \
> $tmpout 2>&1 

if ! cmp chan3.wav.mfc chan3.raw.mfc; then
    fail "WAV and RAW compare"
fi

if ! cmp chan3.2chan.wav.mfc chan3.wav.mfc; then
    fail "WAV2 and WAV compare"
fi

if ! cmp chan3.raw.mfc chan3.sph.mfc; then
    fail "SPH and RAW compare"
fi

run_program sphinx_cepview/sphinx_cepview \
-i 13 \
-d 13 \
-f chan3.wav.mfc \
> test-sphinx_fe.cepview 2>>$tmpout

compare_table "WAVE2FEAT test" test-sphinx_fe.cepview $tests/regression/chan3.cepview 0.1

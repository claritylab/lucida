#!/bin/sh
. ./testfuncs.sh

tmpout="test-sphinx_pitch.out"

echo "PITCH TEST"
run_program sphinx_adtools/sphinx_pitch \
-samprate 11025 \
-i $tests/regression/chan3.raw \
-input_endian little \
-o test-sphinx_pitch.f0  \
-raw yes > $tmpout 2>&1 

compare_table "PITCH test" test-sphinx_pitch.f0 $tests/regression/chan3.f0 0.1

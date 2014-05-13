#!/bin/sh
. ./testfuncs.sh

tmpout="test-sphinx_fe-dct.out"

echo "WAVE2FEAT-DCT INVERTIBLE TEST"
run_program sphinx_fe/sphinx_fe \
-transform dct \
-cep2spec yes \
-nfilt 36 \
-i $tests/regression/chan3.mfc \
-o test-sphinx_fe-dct.logspec.out  \
> $tmpout 2>&1 

run_program sphinx_fe/sphinx_fe \
-transform dct \
-spec2cep yes \
-nfilt 36 \
-i test-sphinx_fe-dct.logspec.out  \
-o test-sphinx_fe-dct.mfc.out  \
>> $tmpout 2>&1 

run_program sphinx_cepview/sphinx_cepview \
-i 13 \
-d 13 \
-f test-sphinx_fe-dct.mfc.out \
> test-sphinx_fe-dct.cepview.out 2>>$tmpout

compare_table "WAVE2FEAT-DCT INVERTIBLE test" test-sphinx_fe-dct.cepview.out \
    $tests/regression/chan3.cepview

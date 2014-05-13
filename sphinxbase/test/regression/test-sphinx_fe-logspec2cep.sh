#!/bin/sh
. ./testfuncs.sh

tmpout="test-sphinx_fe-logspec2cep.out"

echo "WAVE2FEAT-LOGSPEC2CEP TEST"
run_program sphinx_fe/sphinx_fe \
-spec2cep yes \
-nfilt 36 \
-i $tests/regression/chan3.logspec \
-o test-sphinx_fe-logspec2cep.mfc.out  \
> $tmpout 2>&1 

run_program sphinx_cepview/sphinx_cepview \
-i 13 \
-d 13 \
-f test-sphinx_fe-logspec2cep.mfc.out \
> test-sphinx_fe-logspec2cep.cepview.out 2>>$tmpout

compare_table "WAVE2FEAT-LOGSPEC2CEP test" test-sphinx_fe-logspec2cep.cepview.out \
    $tests/regression/chan3.cepview

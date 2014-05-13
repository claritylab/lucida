#!/bin/sh

. ../testfuncs.sh

bn=`basename $0 .sh`

echo "Test: $bn"
run_program pocketsphinx_batch \
    -hmm $model/hmm/en_US/hub4wsj_sc_8k \
    -lm $model/lm/en_US/wsj0vp.5000.DMP \
    -dict $model/lm/en_US/cmu07a.dic \
    -ctl $data/wsj/test5k.n800.ctl \
    -cepdir $data/wsj \
    -adcin yes -adchdr 44 -samprate 8000 -input_endian little \
    -cepext .wav \
    -hyp $bn.match \
    -fwdtree TRUE \
    -fwdflat FALSE \
    -bestpath FALSE \
    > $bn.log 2>&1

# Test whether it actually completed
if [ $? = 0 ]; then
    pass "run"
else
    fail "run"
fi

# Check the decoding results
grep AVERAGE $bn.log
$tests/word_align.pl -i $data/wsj/test5k.n800.lsn $bn.match \
    | grep 'TOTAL Percent'
compare_table "match" $data/wsj/$bn.match $bn.match 1000000

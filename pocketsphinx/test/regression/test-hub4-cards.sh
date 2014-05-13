#!/bin/sh

. ../testfuncs.sh

bn=`basename $0 .sh`

echo "Test: $bn"
run_program pocketsphinx_batch \
    -hmm $model/hmm/en_US/hub4wsj_sc_8k \
    -jsgf $data/cards/cards.gram \
    -dict $model/lm/en_US/cmu07a.dic \
    -ctl $data/cards/cards.fileids \
    -adcin yes \
    -cepdir $data/cards \
    -cepext .wav \
    -hyp $bn.match \
    -backtrace yes \
    > $bn.log 2>&1

# Test whether it actually completed
if [ $? = 0 ]; then
    pass "run"
else
    fail "run"
fi

# Check the decoding results
grep AVERAGE $bn.log
$tests/word_align.pl -i $data/cards/cards.transcription $bn.match | grep 'TOTAL Percent'
compare_table "match" $data/cards/cards.hyp $bn.match 1000000

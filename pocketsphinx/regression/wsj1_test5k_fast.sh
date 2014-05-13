#!/bin/sh

expt=$1
if [ x"$expt" = x ]; then
    >&2 echo "Usage: $0 EXPTID [DECODER]"
    exit 1
fi
decode=${2:-../src/programs/pocketsphinx_batch}

# `dirname $decode`/../../libtool --mode=execute \
#     valgrind --tool=massif \
#     --alloc-fn=__ckd_calloc__ --alloc-fn=__ckd_calloc_2d__ --alloc-fn=__ckd_calloc_3d__ --alloc-fn=__ckd_malloc__ --alloc-fn=__listelem_malloc__ --alloc-fn=listelem_add_block --alloc-fn=__ckd_salloc__ --alloc-fn=__ckd_realloc__ \
#     $decode -ctlcount 10 \

$decode \
    -hmm ../model/hmm/wsj1 \
    -dict bcb05cnp.dic \
    -lm bcb05cnp.z.DMP \
    -lw 9.5 -wip 0.5 \
    -beam 1e-50 -wbeam 1e-30 \
    -fwdflat no -bestpath no \
    -cepdir . -cepext .sph \
    -adcin yes -adchdr 1024 \
    -ctl wsj_test.fileids \
    -hyp $expt.hyp \
    > $expt.log 2>&1

cat wsj_test.transcription | ./word_align.pl -i - $expt.hyp > $expt.align

grep -h ': AVERAGE' $expt.log
tail -n3 $expt.align

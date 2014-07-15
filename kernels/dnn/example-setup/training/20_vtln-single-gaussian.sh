#!/bin/bash
# VTLN step 1/3: estimate a single Gaussian acoustic model from the best 
# alignment so far.

AMT=./executables/acoustic-model-trainer.*
CONFIG=config/triphone-lda-mixtures.config
INITALIGN=data/am.lda-2.alignment.7.cache
NAME=am.lda-2
LDA_ITER=2

function accumulate()
{
    local prevmix=$1
    local newmix=$2
    local alignment=$3
    $AMT --config=$CONFIG --LDA_ITER=$LDA_ITER \
        --action=accumulate-mixture-set-text-dependent \
        --*.mixture-set-trainer.split-first=false \
        --*.mixture-set-trainer.old-mixture-set-file=$prevmix\
        --*.mixture-set-trainer.mixture-set.file=$prevmix \
        --*.mixture-set-trainer.new-mixture-set-file=$newmix \
        --*.alignment-cache.path=$alignment \
        --*.alignment-cache.read-only=true
}

set -e
set -x

INITMIX=data/${NAME}.single-gaussian.mix
accumulate "" $INITMIX $INITALIGN

#!/bin/bash
# basic acoustic model training of triphone models

AMT=./executables/acoustic-model-trainer.*
CONFIG=config/triphone-mixtures.config
INITALIGN=data/monophone-alignment.cache
NAME=am
NSPLIT=7
NITER=3
if [ -z $1 ]; then
    STARTSPLIT=0
else
    STARTSPLIT=$1
fi

function split()
{
    local prevmix=$1
    local newmix=$2
    local alignment=$3
    $AMT --config=$CONFIG \
        --action=accumulate-mixture-set-text-dependent \
        --*.mixture-set-trainer.split-first=true \
        --*.mixture-set-trainer.old-mixture-set-file=$prevmix\
        --*.mixture-set-trainer.mixture-set.file=$prevmix \
        --*.mixture-set-trainer.new-mixture-set-file=$newmix \
        --*.alignment-cache.path=$alignment \
        --*.alignment-cache.read-only=true
}

function accumulate()
{
    local prevmix=$1
    local newmix=$2
    local alignment=$3
    $AMT --config=$CONFIG \
        --action=accumulate-mixture-set-text-dependent \
        --*.mixture-set-trainer.split-first=false \
        --*.mixture-set-trainer.old-mixture-set-file=$prevmix\
        --*.mixture-set-trainer.mixture-set.file=$prevmix \
        --*.mixture-set-trainer.new-mixture-set-file=$newmix \
        --*.alignment-cache.path=$alignment \
        --*.alignment-cache.read-only=true
}

function align()
{
    local mix=$1
    local alignment=$2
    $AMT --config=$CONFIG \
        --action=dry \
        --*.mixture-set.file=$mix\
        --*.alignment-cache.path=$alignment
}

set -e
set -x

INITMIX=data/${NAME}.init.mix
accumulate "" $INITMIX $INITALIGN

for ((s=$STARTSPLIT;s<=${NSPLIT};s++)); do
    if [ $s -eq 0 ]; then
        mix=$INITMIX
    else
        mix=data/${NAME}.$[${s}-1]-${NITER}.mix 
    fi
    alignment=data/${NAME}.alignment.${s}.cache
    align $mix $alignment

    newmix=data/${NAME}.${s}-0.mix 
    split $mix $newmix $alignment

    for ((i=1;i<=${NITER};i++)); do
        prevmix=data/${NAME}.${s}-$[${i}-1].mix 
        newmix=data/${NAME}.${s}-${i}.mix 
        accumulate $prevmix $newmix $alignment
    done
done


#!/bin/bash
# basic acoustic model training of monophone models

AMT=./executables/acoustic-model-trainer.*
CONFIG=config/monophone-mixtures.config
INITMIX=data/linear-segmentation.mix
NAME=am.mono
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

for ((s=$STARTSPLIT;s<=${NSPLIT};s++)); do
    if [ $s -eq 0 ]; then
        mix=$INITMIX
    elif [ $s -eq 1 ]; then
        mix=data/${NAME}.$[${s}-1]-0.mix
    else
        mix=data/${NAME}.$[${s}-1]-${NITER}.mix 
    fi
    alignment=data/${NAME}.alignment.${s}.cache
    align $mix $alignment


    newmix=data/${NAME}.${s}-0.mix 
    if [ $s -gt 0 ]; then
        split $mix $newmix $alignment
    else
        accumulate $mix $newmix $alignment
    fi
    if [ $s -gt 0 ]; then
        for ((i=1;i<=${NITER};i++)); do
            prevmix=data/${NAME}.${s}-$[${i}-1].mix 
            newmix=data/${NAME}.${s}-${i}.mix 
            accumulate $prevmix $newmix $alignment
        done
    fi
done


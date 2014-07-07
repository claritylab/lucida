#!/bin/bash
# VTLN step 3/3: train a classifier for Fast VTLN

AMT=./executables/acoustic-model-trainer.*
CONFIG=config/warping-factor-gmm.config
MAP=data/warping-factors-map.xml
ALPHAS=data/reduced-warping-factors.xml
NAME=am.warping-factor-gmm
NSPLIT=7
NITER=3
if [ -z $1 ]; then
    STARTSPLIT=0
else
    STARTSPLIT=$1
fi

function accumulate()
{
    local prevmix=$1
    local newmix=$2
    local split=$3
    $AMT --config=$CONFIG \
         --*.action=accumulate-mixture-set-text-independent \
         --*.labeling.labels=$ALPHAS \
         --*.warping-factor.labels=$ALPHAS \
         --*.warping-factor.map-file=$MAP \
         --*.mixture-set-trainer.split-first=$split \
         --*.mixture-set-trainer.new-mixture-set-file=$newmix \
         --*.mixture-set-trainer.old-mixture-set-file=$prevmix\
         --*.mixture-set-trainer.mixture-set.file=$prevmix
}

set -uxe

for ((s=$STARTSPLIT;s<=${NSPLIT};s++)); do
    if [ $s -eq 0 ]; then
        prevmix=""
    else
        prevmix=data/${NAME}.$[${s}-1]-${NITER}.mix 
    fi

    newmix=data/${NAME}.${s}-0.mix 
    accumulate "$prevmix" $newmix true

    for ((i=1;i<=${NITER};i++)); do
        prevmix=data/${NAME}.${s}-$[${i}-1].mix 
        newmix=data/${NAME}.${s}-${i}.mix 
        accumulate $prevmix $newmix false
    done
done

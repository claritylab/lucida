#!/bin/bash
# initialization steps for NN training
set -x

NNTRAINER=./executables/nn-trainer.*
CONFIG=config/nn-normalization.config
ALIGN=data/am.lda-2.alignment.7.cache
CACHE=data/mfcc.features.cache

# accumulate statistics for mean/std estimattion
$NNTRAINER \
    --config=$CONFIG \
    --*.base-feature-extraction-cache.path=$CACHE \
    --*.alignment-cache.path=$ALIGN \
    --*.statistics-filename=data/mv.stats \

# estimate global mean and standard deviation from statistics file
$NNTRAINER \
    --config=$CONFIG \
    --*.action=estimate-mean-and-standard-deviation \
    --*.statistics-file=data/mv.stats \
    --*.mean-file=data/mean \
    --*.standard-deviation-file=data/std \
    --*.DESCRIPTION=nn-normalization-est

# normalization performed with double precision for numerical stability, but
# the training will be done with single precision for performance
sed -i "s/f64/f32/g" data/mean-f64.xml data/std-f64.xml
mv data/mean-f64.xml data/mean-f32.xml
mv data/std-f64.xml  data/std-f32.xml

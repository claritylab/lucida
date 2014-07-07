#!/bin/bash
set -x

FEATEXTR=./executables/feature-extraction.*
FEATSTAT=./executables/feature-statistics.*

CONFIG_FORWARD=config/feature-extraction.nn.config
CONFIG_PCA=config/pca-estimation.config

CACHE=data/mfcc.features.cache
NNCACHE=data/nn.features.cache
NORM_MEAN=data/mean-f32.xml
NORM_STD=data/std-f32.xml
MODEL=data/weights-bn-10
#export OMP_NUM_THREADS=4

$FEATEXTR \
    --config=$CONFIG_FORWARD \
    --*.base-feature-extraction-cache.path=$CACHE \
    --*.nn-cache.path=$NNCACHE \
    --*.mean-file=$NORM_MEAN \
    --*.standard-deviation-file=$NORM_STD \
    \
    --*.parameters-old=bin:$MODEL \
    --*.channels.output-channel.file=log/nn-feature-extraction-tandem.log

$FEATSTAT \
    --config=$CONFIG_PCA \
    --*.feature-extraction.*.base-feature-extraction-cache.path=$NNCACHE \
    --*.DESCRIPTION=pca \
    --ITER=2

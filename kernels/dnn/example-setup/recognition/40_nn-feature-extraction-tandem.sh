#!/bin/bash
# Extraction of neural network based bottleneck features 
# for the Tandem system.
# Requires training step 40_nn-training-tandem.sh
set -x

FEATEXTR=./executables/feature-extraction.*

CONFIG=../training/config/feature-extraction.nn.config

CACHE=data/mfcc.features.recognition.cache
NNCACHE=data/nn.features.recognition.cache
NORM_MEAN=data/mean-f32.xml
NORM_STD=data/std-f32.xml
MODEL=bin:data/weights-bn-10
#export OMP_NUM_THREADS=4

$FEATEXTR \
    --config=$CONFIG \
    --*.corpus.file=../input/an4_test.20081021.corpus.gz \
    --*.base-feature-extraction-cache.path=$CACHE \
    --*.nn-cache.path=$NNCACHE \
    --*.mean-file=$NORM_MEAN \
    --*.standard-deviation-file=$NORM_STD \
    \
    --*.parameters-old=$MODEL \
    --*.channels.output-channel.file=log/nn-feature-extraction-tandem.log

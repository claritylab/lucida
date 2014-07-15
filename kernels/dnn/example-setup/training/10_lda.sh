#!/bin/sh

AMT=./executables/acoustic-model-trainer.* \

$AMT \
    --config=config/scatter-estimation.config \
    --ITER=2

$AMT \
    --config=config/lda-estimation.config \
    --ITER=2

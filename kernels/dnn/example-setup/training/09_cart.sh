#!/bin/sh

AMT=./executables/acoustic-model-trainer.*
BEST_ALIGNMENT=data/monophone-alignment.cache

$AMT \
   --config=config/cart-accumulation-lda.config \
   --*.alignment-cache.path=$BEST_ALIGNMENT \
   --ITER=2

$AMT \
    --config=config/cart-estimation.config \
    --ITER=2

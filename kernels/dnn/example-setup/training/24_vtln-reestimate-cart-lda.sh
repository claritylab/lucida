#!/bin/sh
# estimate a new CART and LDA given VTLN warped features

set -xe

BEST_ALIGNMENT=data/am.lda-2.alignment.7.cache

# choose either regular of fast VTLN warped cache
#FEATURE_CACHE=data/base.mfcc-fast-vtln.cache
FEATURE_CACHE=data/base.mfcc-vtln.cache

for ITER in 3 4; do

    ./executables/acoustic-model-trainer.* \
       --config=config/cart-accumulation-lda.config \
       --*.alignment-cache.path=$BEST_ALIGNMENT \
       --*.feature-extraction.*.base-feature-extraction-cache.path=$FEATURE_CACHE \
        --ITER=$ITER
    
    ./executables/acoustic-model-trainer.* \
        --config=config/cart-estimation.config \
        --ITER=$ITER
    
    ./executables/acoustic-model-trainer.* \
       --config=config/scatter-estimation.config \
       --*.alignment-cache.path=$BEST_ALIGNMENT \
       --*.feature-extraction.*.base-feature-extraction-cache.path=$FEATURE_CACHE \
       --ITER=$ITER
    
    ./executables/acoustic-model-trainer.* \
        --config=config/lda-estimation.config \
        --ITER=$ITER

done

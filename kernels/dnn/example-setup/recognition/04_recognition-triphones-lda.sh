#!/bin/sh
# Recognition using generalized triphone acoustic model
# and LDA transformed features.
# Requires training step 08_triphone-lda-training.sh

./executables/speech-recognizer.* \
    --config=config/recognition-triphones-lda.config \
    --*.mixture-set.file=data/am.lda.7-3.mix \
    --*.LDA_ITER=1

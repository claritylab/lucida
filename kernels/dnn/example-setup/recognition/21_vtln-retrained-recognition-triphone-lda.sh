#!/bin/bash
# Recognition using generalized triphone acoustic model and LDA transformed and
# Fast-VTLN warped features (second iteration), retrained on VTLN warped
# features.
# Requires training step 25_vtln-train-triphone.sh

set -ex

./executables/speech-recognizer.* \
    --config=config/recognition-triphones-lda.config \
    --*.base-feature-extraction-cache.path=data/base.mfcc-fast-vtln.recognition.cache \
    --*.mixture-set.file=data/am.lda-3.7-3.mix \
    --*.LDA_ITER=4 \
    --*.DESCRIPTION=recognition-triphones-lda-vtln-retrained

[[ "$?" == 0 ]] && analog log/recognition-triphones-lda-vtln-retrained.log.gz

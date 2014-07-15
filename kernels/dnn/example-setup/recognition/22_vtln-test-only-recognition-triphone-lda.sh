#!/bin/sh
# Recognition using generalized triphone acoustic model and LDA transformed
# and Fast-VTLN warped features (second iteration).
# Requires training step 22_vtln-train-warping-factor-classifier.sh

set -ex

./executables/speech-recognizer.* \
    --config=config/recognition-triphones-lda.config \
    --*.base-feature-extraction-cache.path=data/base.mfcc-fast-vtln.recognition.cache \
    --*.mixture-set.file=data/am.lda-2.7-3.mix \
    --*.LDA_ITER=2 \
    --*.DESCRIPTION=recognition-triphones-lda-vtln-test-only

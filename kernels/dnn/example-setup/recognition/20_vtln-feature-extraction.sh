#!/bin/sh
# Fast VTLN feature extraction
# Requires training step 22_vtln-train-warping-factor-classifier.sh

WARPING_FACTORS=data/reduced-warping-factors.xml
WARPING_FACTORS_GMM=data/am.warping-factor-gmm.7-3.mix

./executables/feature-extraction.* \
    --config=config/feature-extraction.mfcc.config \
    --*.warping-factor-recognizer.class-label-file=$WARPING_FACTORS \
    --*.warping-factor-recognizer.likelihood-function.file=$WARPING_FACTORS_GMM \
    --*.warping-factor-recognizer.likelihood-function.feature-scorer-type=diagonal-maximum \
    --*.linear-transform.file=data/total-scatter-normalization.matrix \
    --*.feature-extraction.*.base-feature-extraction.file=vtln/warped-mfcc.recognized-factors.postprocessing.flow \
    --*.base-feature-extraction-cache.path=data/base.mfcc-fast-vtln.recognition.cache

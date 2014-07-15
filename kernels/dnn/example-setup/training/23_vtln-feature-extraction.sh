#!/bin/bash
# extract VTLN features (either normal or fast) for the next training cycle

set -uxe

# for vtln
MAP=data/warping-factors-map.xml

# for fast vtln
WARPING_FACTORS=data/reduced-warping-factors.xml
WARPING_FACTORS_GMM=data/am.warping-factor-gmm.7-3.mix

feat=${1:-mfcc-vtln}

if [ "$feat" == "mfcc-vtln" ]; then
    OPTION="--*.warping-factor.map-file=$MAP"
    FLOW="vtln/warped-mfcc.factors-from-map.postprocessing.flow"

elif [ "$feat" == "mfcc-fast-vtln" ]; then
    OPTION="--*.warping-factor-recognizer.class-label-file=$WARPING_FACTORS \
            --*.warping-factor-recognizer.likelihood-function.file=$WARPING_FACTORS_GMM \
            --*.warping-factor-recognizer.likelihood-function.feature-scorer-type=diagonal-maximum \
            --*.linear-transform.file=data/total-scatter-normalization.matrix"
    FLOW="vtln/warped-mfcc.recognized-factors.postprocessing.flow"
fi

./executables/feature-extraction.* \
    --config=config/feature-extraction.mfcc.config \
    --*.feature-extraction.*.base-feature-extraction.file=$FLOW \
    --*.base-feature-extraction-cache.path=data/base.$feat.cache \
    $OPTION

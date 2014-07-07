#!/bin/bash
# Recognition using a Tandem model trained on
# LDA transformed MFCCs and PCA transformed bottleneck features.
# Requires training step 42_nn-gmm-training.sh and 
# features extracted in recognition step 41_nn-feature-extraction-tandem.sh
set -x

# this needs to point to src/Tools/Analog/analog
ANALOG=analog

./executables/speech-recognizer.* \
    --config=config/recognition-tandem.config \
    --*.mixture-set.file=data/am.tandem.7-3.mix \
    --*.LDA_ITER=2

[ "$?" -eq 0 ] && $ANALOG log/recognition-triphones-lda-2-tandem.log.gz

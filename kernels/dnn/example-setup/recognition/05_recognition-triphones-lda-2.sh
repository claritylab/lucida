#!/bin/bash
# Recognition using generalized triphone acoustic model
# and LDA transformed features (second iteration).
# Requires training step 11_triphone-lda-2-trainig.sh
set -x

# this needs to point to src/Tools/Analog/analog
ANALOG=analog

./executables/speech-recognizer.* \
    --config=config/recognition-triphones-lda.config \
    --*.mixture-set.file=data/am.lda-2.7-3.mix \
    --*.LDA_ITER=2

[ "$?" -eq 0 ] && $ANALOG log/recognition-triphones-lda-2.log.gz

#!/bin/sh
# Recognition using generalized triphone acoustic model
# Requires training step 06_triphone_training.sh

./executables/speech-recognizer.* \
    --config=config/recognition-triphones.config

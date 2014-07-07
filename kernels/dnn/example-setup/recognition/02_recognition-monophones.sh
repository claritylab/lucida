#!/bin/sh
# Recognition using monophone acoustic model
# Requires training step 04_monophone-training.sh
set -x
# this needs to point to src/Tools/Analog/analog
ANALOG=analog

./executables/speech-recognizer.* \
    --config=config/recognition-monophones.config

[ "$?" -eq 0 ] && $ANALOG log/recognition-monophones.log.gz

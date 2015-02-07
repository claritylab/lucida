#!/usr/bin/env bash

if [ ! -f crfsuite ]; then
  ./sirius-crf.sh 1>/dev/null
fi

./crfsuite tag -qt -m ../input/model.model ../input/test.crfsuite.txt

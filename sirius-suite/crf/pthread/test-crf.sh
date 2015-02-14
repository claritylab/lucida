#!/usr/bin/env bash

./rebuild.sh

./crfsuite tag -qt -m ../input/model.model ../input/test.crfsuite.txt

diff ../input/crf.pthread ../input/crf.baseline > /dev/null

if [ $$? -eq 1 ] 
then 
    echo "CRF-PTHREAD test failed"
fi

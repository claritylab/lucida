#!/usr/bin/env bash

dir=./speech-recognition/kaldi/

cd $dir/scripts

# Download models and change paths
sh ./prepare.sh

echo "Kaldi preparation done."

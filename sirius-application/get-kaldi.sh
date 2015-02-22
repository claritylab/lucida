#!/usr/bin/env bash

# Yiping Kang 2014
# ypkang@umich.edu

dir=./speech-recognition/kaldi/

cd $dir/scripts

# Download models and change paths
./prepare.sh

echo "Kaldi preparation done."

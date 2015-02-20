#!/bin/bash

# Yiping Kang 2014
# ypkang@umich.edu

dir=./speech-recognition/kaldi/

cd $dir/scripts

# Download models and change paths
./prepare.sh

# Untar the kaldi source files
#cd ..
#tar -xvzf src.tar.gz
#tar -xvzf tools.tar.gz

echo "Kaldi preparation done."

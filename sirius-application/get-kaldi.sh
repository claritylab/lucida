#!/bin.bash

# Download and install Kaldi
# Yiping Kang 2014
# ypkang@umich.edu

parallel_make=4

hash wget 2>/dev/null || {
  echo >&2 "$0: [ERROR] wget is not intalled. Aborting."
  exit 1
}

hash make 2>/dev/null || {
  echo >&2 "$0: [ERROR] make is not installed. Aborting."
  exit 1
}

dir=./speech-recognition/kaldi/

if [ -d "$dir" ]; then
  echo >&2 "$0: [ERROR] $dir already exists. Aborting."
fi

# Make directory
mkdir $dir

# Download kaldi source code
wget http://siris.clarity-lab.org/downloads/kaldi-src.tar.gz

# Unzip
tar xzf ./kaldi-src.tar.gz -C $dir

# Make kaldi
cd $dir/src
make -j $parallel_make 

# Clean up 
rm ./kaldi-src.tar.gz

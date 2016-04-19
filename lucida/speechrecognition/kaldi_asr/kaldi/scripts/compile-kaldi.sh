#!/usr/bin/env bash
# exit if any server fails
set -e

# thread for parallel build
[[ -n "$THREADS" ]] || THREADS=4

asr_kaldi=../
################
# Kaldi
################

cd $asr_kaldi
# Untar the src and tools tarball
if [ ! -d tools ]; then
  tar -xzf tools.tar.gz
fi

cd tools/
make -j $THREADS ;
cd ..

if [ ! -d src ]; then
    tar -xzf src.tar.gz --overwrite
fi

cd src/
./configure ;
make -j $THREADS ;

cd ./online2bin
make -j $THREADS ;
cd ../../../../
echo "Kaldi server done."


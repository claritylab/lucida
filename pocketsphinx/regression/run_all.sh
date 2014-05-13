#!/bin/sh

set -e

platform=${1:-i386-linux}

echo "SVN HEAD:"
./wsj1_test5k.sh $platform-simple ../$platform/src/programs/pocketsphinx_batch
./wsj1_test5k_fast.sh $platform-fast ../$platform/src/programs/pocketsphinx_batch

echo "0.4.1:"
./wsj1_test5k.sh $platform-simple-base ../../pocketsphinx-0.4/pocketsphinx/$platform/src/programs/pocketsphinx_batch
./wsj1_test5k_fast.sh $platform-fast-base ../../pocketsphinx-0.4/pocketsphinx/$platform/src/programs/pocketsphinx_batch

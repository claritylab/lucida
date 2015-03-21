#!/usr/bin/env bash
set -e

dir=./speech-recognition/kaldi/

cd $dir/scripts

if ( [ -z $1 ] || ! [ "${1}" == "-f" ] ); then
  param=""
else
  param="-f"
fi

# Download models and change paths
./prepare.sh "${param}"

echo "Kaldi preparation done."

#!/usr/bin/env bash

# Replaces required files in crf-suite wih Sirius pthread version
# of the viterbi component of the CRF
# Original dataset, training and test set included

hash make 2>/dev/null || {
  echo >&2 "$0: [ERROR] make is not installed. Aborting."
  exit 1
}

crfdir=crfsuite-0.12

if [ ! -d $crfsuite ]; then
  ./sirius-crf.sh 1>/dev/null
fi

# temp directory created by crfsuite
rm -rf .libs/

cp tag.c $crfdir/frontend

# Rebuild
cd $crfdir/frontend && make

cd ../../ ;

# copy crfsuite to base directory
cp $crfdir/frontend/crfsuite .

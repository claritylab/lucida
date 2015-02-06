#!/usr/bin/env bash

# Replaces required files in crf-suite wih Sirius pthread version
# of the viterbi component of the CRF
# Original dataset, training and test set included

hash make 2>/dev/null || {
  echo >&2 "$0: [ERROR] make is not installed. Aborting."
  exit 1
}

crfdir=crfsuite-0.12

cp tag.c $crfdir/frontend

# Rebuild
cd $crfdir && make
cp $crfdir/frontend/crfsuite .

# copy the binary to base directory
cp $crfdir/frontend/crfsuite .

#!/usr/bin/env bash

# Replaces required files in crf-suite wih Sirius pthread version
# of the viterbi component of the CRF
# Original dataset, training and test set included

hash make 2>/dev/null || {
  echo >&2 "$0: [ERROR] make is not installed. Aborting."
  exit 1
}

crfdir=crfsuite-0.12
sirius=`pwd`

# Makefile includes -lpthread
cp $sirius/tag.c \
  $sirius/Makefile \
  $sirius/input/test.crfsuite.txt \
  $sirius/input/model.model \
  $crfdir/frontend

# Rebuild
cd $crfdir
make

cd frontend/
# test using:
./crfsuite tag -qt -m model.model test.crfsuite.txt

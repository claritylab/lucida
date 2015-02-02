#!/usr/bin/env bash

# Downloads and replaces required files in crf-suite wih Sirius pthread version
# of the viterbi component of the CRF
# Original dataset, training and test set included

hash wget 2>/dev/null || {
  echo >&2 "$0: [ERROR] wget is not installed. Aborting."
  exit 1
}

hash tar 2>/dev/null || {
  echo >&2 "$0: [ERROR] tar is not installed. Aborting."
  exit 1
}

hash make 2>/dev/null || {
  echo >&2 "$0: [ERROR] make is not installed. Aborting."
  exit 1
}

crfdir=crfsuite-0.12
sirius=`pwd`

wget https://github.com/downloads/chokkan/crfsuite/crfsuite-0.12.tar.gz
tar xzf crfsuite-0.12.tar.gz
cd $crfdir;
./configure
make
cd $sirius

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

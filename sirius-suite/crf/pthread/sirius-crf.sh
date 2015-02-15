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

if [ ! -d $crfdir ]; then
  wget -q https://github.com/downloads/chokkan/crfsuite/crfsuite-0.12.tar.gz
  tar xzf crfsuite-0.12.tar.gz
  cd $crfdir;
  ./configure
  make
  cd ../
fi

cp tag.c \
  Makefile \
  ../input/test.crfsuite.txt \
  ../input/model.model \
  $crfdir/frontend

# Rebuild
cd $crfdir && make

cd ../;
rm -rf crfsuite-0.12.tar.gz

# copy the binary to base directory
cp $crfdir/frontend/crfsuite .

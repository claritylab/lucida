#!/usr/bin/env bash
set -e

ver=1.0

wget http://web.eecs.umich.edu/~jahausw/download/sirius-suite-$ver.tar.gz
tar xzf sirius-suite-$ver.tar.gz sirius-suite

cd sirius-suite;
make test;
cd ..;

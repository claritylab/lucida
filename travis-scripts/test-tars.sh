#!/usr/bin/env bash
set -e

MAKE="make --jobs=$NUM_THREADS"

ver=1.0

wget http://web.eecs.umich.edu/~jahausw/download/sirius-application-$ver.tar.gz
wget http://web.eecs.umich.edu/~jahausw/download/sirius-suite-$ver.tar.gz
wget http://web.eecs.umich.edu/~jahausw/download/sirius-caffe-$ver.tar.gz

tar xzf sirius-application-$ver.tar.gz sirius-application
tar xzf sirius-suite-$ver.tar.gz sirius-suite
tar xzf sirius-caffe-$ver.tar.gz sirius-caffe

cd sirius-caffe;
sudo ./make-and-install.sh
cd ..;

cd sirius-suite;
make test;
cd ..;

cd sirius-application;
sudo ./get-dependencies.sh
./get-kaldi.sh
./compile-sirius-servers.sh
cd ..;

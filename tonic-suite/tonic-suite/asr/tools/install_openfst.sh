#!/bin/bash

wget http://openfst.cs.nyu.edu/twiki/pub/FST/FstDownload/openfst-1.3.2.tar.gz

tar -xovf openfst-1.3.2.tar.gz

for dir in openfst-1.3.2/{src/,}include/fst; do
  ( [ -d $dir ] && cd $dir && patch -p0 -N <../../../../openfst.patch )
done

rm openfst 2>/dev/null # Remove any existing link
ln -s openfst-1.3.2 openfst

cd openfst-1.3.2

./configure --prefix=`pwd` 

make -j4
make install

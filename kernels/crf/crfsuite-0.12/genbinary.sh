#!/bin/bash

LIBLBFGS=$HOME/local
PKG=crfsuite-0.12
BINDIR=$HOME/build/$PKG
TARGET=`pwd`/$PKG-`/usr/bin/arch`.tar.gz

rm -rf $BINDIR
./configure --prefix=$BINDIR --with-liblbfgs=$LIBLBFGS
make clean
make LDFLAGS=-all-static
make install
cd $BINDIR/..
tar cvzf $TARGET $PKG


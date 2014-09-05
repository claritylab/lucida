#!/bin/bash
# Install sphinxbase + pocketsphinx
# Run as sudo to install correctly to default
# jahausw 2014

mkdir -p bits ;
cd bits ;

# set correct version
ver=0.8
# default
installdir=/usr/local/bin
sphinxdir=sphinxbase-
pocketdir=pocketsphinx-

wget http://sourceforge.net/projects/cmusphinx/files/sphinxbase/$ver/${sdir}${ver}.tar.gz
wget http://sourceforge.net/projects/cmusphinx/files/pocketsphinx/$ver/$pdir${ver}.tar.gz

tar xzf ${sdir}.tar.gz
cd $sdir ;
./autogen.sh
./configure --prefix=$installdir
make
make install
cd .. ;
rm -rf $sdir* ;

tar xzf ${pdir}.tar.gz
cd $pdir ;
./autogen.sh
./configure --prefix=$installdir
make
make install
cd .. ;
rm -rf $pdir* ;

cd ../ ; rm -rf bits ;

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
sdir=sphinxbase-
pdir=pocketsphinx-

wget http://sourceforge.net/projects/cmusphinx/files/sphinxbase/$ver/${sdir}${ver}.tar.gz
wget http://sourceforge.net/projects/cmusphinx/files/pocketsphinx/$ver/$pdir${ver}.tar.gz

sdir=$sdir$ver
pdir=$pdir$ver

# sphinxbase
tar xzf ${sdir}.tar.gz
cd $sdir ;
./autogen.sh
./configure --prefix=$installdir
make -j8
make install
cd .. ;

# pocketsphinx
tar xzf ${pdir}.tar.gz
cd $pdir ;
./autogen.sh
./configure --prefix=$installdir
make -j8
make install
cd .. ;

# clean up
rm -rf $pdir ;
rm -rf $sdir ;
cd ../ ; rm -rf bits ;

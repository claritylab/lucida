#!/bin/bash
# Install sphinxbase + pocketsphinx
# Run as sudo to install correctly
# jahausw 2014

mkdir -p bits ;
cd bits ;

sphinxdir=sphinxbase-0.8
pocketdir=pocketsphinx-0.8

wget http://sourceforge.net/projects/cmusphinx/files/sphinxbase/0.8/sphinxbase-0.8.tar.gz
wget http://sourceforge.net/projects/cmusphinx/files/pocketsphinx/0.8/pocketsphinx-0.8.tar.gz

tar xzf ${sphinxdir}.tar.gz
cd $sphinxdir ;
./autogen.sh
./configure
make
make install
cd .. ;
rm -rf $sphinxdir* ;

tar xzf ${pocketdir}.tar.gz
cd $pocketdir ;
./autogen.sh
./configure
make
make install
cd .. ;
rm -rf $pocketdir* ;

cd ../ ; rm -rf bits ;

#!/usr/bin/env bash

# Install sphinxbase + pocketsphinx
# Run as sudo to install correctly to default
# jahausw 2014

hash wget 2>/dev/null || {
  echo >&2 "$0: [ERROR] wget is not installed. Aborting."
  exit 1
}

hash make 2>/dev/null || {
  echo >&2 "$0: [ERROR] make is not installed. Aborting."
  exit 1
}

if [ "$EUID" -ne 0 ]
then
  echo >&2 "$0: [ERROR] Sudo access required. Aborting."
  exit 1
fi

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
make && make install
cd .. ;

# pocketsphinx
tar xzf ${pdir}.tar.gz
cd $pdir ;
./autogen.sh
./configure --prefix=$installdir
make && make install
cd .. ;

# (optionally) clean up
# rm -rf $pdir ;
# rm -rf $sdir ;
# cd ../ ; rm -rf bits ;

#!/usr/bin/env bash

hash unzip 2>/dev/null || {
  echo >&2 "$0: [ERROR] unzip is not installed. Aborting."
  exit 1
}

# set number of threads when building
# build and install sphinxbase
if [ ! -d sphinxbase-master ]; then
  unzip -q -o sphinxbase.zip
  cd sphinxbase-master;
  ./autogen.sh > /dev/null
  cd ..;
fi

cd sphinxbase-master;
make -j $THREADS > /dev/null
sudo make install > /dev/null
cd ..;

# build and install pocketsphinx
if [ ! -d pocketsphinx-master ]; then
  unzip -q -o pocketsphinx.zip
  cd pocketsphinx-master;
  ./autogen.sh > /dev/null
  cd ..;
fi

cd pocketsphinx-master;
make -j $THREADS > /dev/null
sudo make install > /dev/null
cd ..;
